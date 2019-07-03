/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "AutoFlushController.h"

#include "BackgroundTaskCollection.h"
#include "PartitionTimestampQueue.h"
#include "StreamLayerClientImpl.h"
#include "TimeUtils.h"

namespace olp {
namespace dataservice {
namespace write {

namespace {
/**
 Returns true if curr is within the specified percentage of req.
 */
template <typename _Rep, typename _Period>
constexpr bool IsCloseEnough(std::chrono::duration<_Rep, _Period> req,
                             std::chrono::duration<_Rep, _Period> curr,
                             int percentage) noexcept {
  return curr > (req - (req * percentage / 100));
}
}  // namespace

/**
 class DisabledAutoFlushControllerImpl
 To be used when auto-flush is disbled, prevents any automated flush events
 being triggered.
 */
class DisabledAutoFlushControllerImpl
    : public AutoFlushController::AutoFlushControllerImpl {
 public:
  std::future<void> Disable() override {
    std::promise<void> ret;
    ret.set_value();
    return ret.get_future();
  }

  void NotifyQueueEventStart() override {}

  void NotifyQueueEventComplete() override {}

  void NotifyFlushEvent() override {}
};

/**
 class EnabledAutoFlushControllerImpl
 To be used when auto-flush is enabled, implements the auto-flush mechanism.
 */
template <typename ClientImpl, typename FlushResponse>
class EnabledAutoFlushControllerImpl
    : public AutoFlushController::AutoFlushControllerImpl,
      public std::enable_shared_from_this<
          EnabledAutoFlushControllerImpl<ClientImpl, FlushResponse>> {
 public:
  EnabledAutoFlushControllerImpl(
      std::shared_ptr<ClientImpl> client_impl, FlushSettings flush_settings,
      std::shared_ptr<FlushEventListener<FlushResponse>> listener)
      : client_impl_(client_impl),
        flush_settings_(flush_settings),
        listener_(listener),
        background_task_col_(),
        cancel_mutex_(),
        cancel_token_map_(),
        is_cancelled_(false) {}

  ~EnabledAutoFlushControllerImpl() override {
    Cancel();
    const auto kDestructTimeout{std::chrono::milliseconds(180000)};
    WaitForBackgroundTaskCompletion(kDestructTimeout);
  }

  void Enable() override {
    InitialiseAutoFlushPeriodic();
    AutoFlushNumEvents();
  }

  std::future<void> Disable() override {
    Cancel();
    return WaitForBackgroundTaskCompletionAsync();
  }

  void NotifyQueueEventStart() override { HandleNotifyQueueEventStart(); }

  void NotifyQueueEventComplete() override { HandleNotifyQueueEventComplete(); }

  void NotifyFlushEvent() override { HandleNotifyFlushEvent(); }

  void AddCancelToken(size_t id,
                      const olp::client::CancellationToken& cancel_token) {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    cancel_token_map_.insert(
        std::pair<size_t, olp::client::CancellationToken>(id, cancel_token));
  }

  void RemoveCancelToken(size_t id) {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    cancel_token_map_.erase(id);
  }

  bool IsCancelled() {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    return is_cancelled_;
  }

 private:
  void AutoFlushNumEvents() {
    if (IsAutoFlushNumEventsRequired()) {
      AddBackgroundFlushTask();
    }
  }

  bool IsAutoFlushNumEventsRequired() {
    auto impl_pointer = client_impl_.lock();
    if (impl_pointer) {
      return impl_pointer->QueueSize() >= flush_settings_.auto_flush_num_events;
    }
    return false;
  }

  void InitialiseAutoFlushInterval() {
    if (flush_settings_.auto_flush_interval > 0) {
      TriggerAutoFlushInterval();
    }
  }

  void InitialiseAutoFlushOldPartitionInterval() {
    auto impl_pointer = client_impl_.lock();
    if (impl_pointer &&
        flush_settings_.auto_flush_old_events_force_flush_interval > 0) {
      PushPartitionTimestamps(partition_timestamp_queue_,
                              impl_pointer->QueueSize());
      TriggerAutoFlushOldPartitionInterval();
    }
  }

  void InitialiseAutoFlushPeriod() {
    if (flush_settings_.auto_flush_time_period.is_initialized()) {
      TriggerAutoFlushPeriod();
    }
  }

  void InitialiseAutoFlushPeriodic() {
    InitialiseAutoFlushInterval();
    InitialiseAutoFlushOldPartitionInterval();
    InitialiseAutoFlushPeriod();
  }

  void HandleNotifyFlushEvent() {
    if (flush_settings_.auto_flush_old_events_force_flush_interval > 0) {
      partition_timestamp_queue_.pop();
    }
  }

  void HandleNotifyQueueEventStart() {
    if (flush_settings_.auto_flush_old_events_force_flush_interval > 0) {
      PushPartitionTimestamps(partition_timestamp_queue_, 1);
    }
  }

  void HandleNotifyQueueEventComplete() { AutoFlushNumEvents(); }

  void NotifyFlushEventStart() const {
    if (listener_) listener_->NotifyFlushEventStarted();
  }

  void NotifyFlushEventResults(FlushResponse results) const {
    if (listener_) listener_->NotifyFlushEventResults(results);
  }

  void Cancel() {
    std::lock_guard<std::mutex> lock(cancel_mutex_);
    for (auto& pair : cancel_token_map_) {
      pair.second.cancel();
    }
    is_cancelled_ = true;
  }

  bool AddBackgroundFlushTask() {
    auto impl_pointer = client_impl_.lock();
    if (!impl_pointer) {
      return false;
    }

    auto self = this->shared_from_this();
    NotifyFlushEventStart();
    auto flush_thread = std::thread([self, impl_pointer]() {
      auto id = self->background_task_col_.AddTask();
      auto cancel_token =
          impl_pointer->Flush([self, id](FlushResponse results) {
            self->background_task_col_.ReleaseTask(id);
            self->RemoveCancelToken(id);
            self->NotifyFlushEventResults(results);
          });
      self->AddCancelToken(id, cancel_token);
    });
    flush_thread.detach();
    return true;
  }

  void TriggerAutoFlushPeriod() {
    auto self = this->shared_from_this();
    auto ms = getDelayTillPeriod(*flush_settings_.auto_flush_time_period,
                                 std::chrono::system_clock::now());
    auto auto_flush_period_thread = std::thread([self, ms]() {
      std::this_thread::sleep_for(ms);
      if (self->IsCancelled()) {
        return;
      }
      if (self->AddBackgroundFlushTask()) {
        self->TriggerAutoFlushPeriod();
      }
    });
    auto_flush_period_thread.detach();
  }

  void TriggerAutoFlushInterval() {
    auto self = this->shared_from_this();
    auto seconds = std::chrono::seconds(flush_settings_.auto_flush_interval);
    auto auto_flush_interval_thread = std::thread([self, seconds]() {
      std::this_thread::sleep_for(seconds);
      if (self->IsCancelled()) {
        return;
      }

      if (self->AddBackgroundFlushTask()) {
        self->TriggerAutoFlushInterval();
      }
    });
    auto_flush_interval_thread.detach();
  }

  void TriggerAutoFlushOldPartitionInterval() {
    TriggerAutoFlushOldPartitionInterval(
        std::chrono::seconds(
            flush_settings_.auto_flush_old_events_force_flush_interval) -
        CalculateTimeSinceOldestPartition(partition_timestamp_queue_));
  }

  void TriggerAutoFlushOldPartitionInterval(std::chrono::milliseconds ms) {
    auto self = this->shared_from_this();

    auto old_partition_thread = std::thread([self, ms]() {
      std::this_thread::sleep_for(ms);
      if (self->IsCancelled()) {
        return;
      }

      std::chrono::milliseconds time_since_oldest_partition_queued =
          CalculateTimeSinceOldestPartition(self->partition_timestamp_queue_);
      std::chrono::milliseconds partition_queue_flush_interval =
          std::chrono::seconds(
              self->flush_settings_.auto_flush_old_events_force_flush_interval);

      if (IsCloseEnough(partition_queue_flush_interval,
                        time_since_oldest_partition_queued, 1)) {
        if (!self->AddBackgroundFlushTask()) {
          // No need to keep this thread alive once the client_impl_ is dead
          return;
        }
      } else {
        // adjust next task execution time based on current queue state.
        partition_queue_flush_interval -= time_since_oldest_partition_queued;
      }
      self->TriggerAutoFlushOldPartitionInterval(
          partition_queue_flush_interval);
    });

    old_partition_thread.detach();
  }

  void WaitForBackgroundTaskCompletion() {
    background_task_col_.WaitForBackgroundTaskCompletion();
  }

  template <typename T>
  void WaitForBackgroundTaskCompletion(const T& timeout) {
    background_task_col_.WaitForBackgroundTaskCompletion(timeout);
  }

  std::future<void> WaitForBackgroundTaskCompletionAsync() {
    auto self = this->shared_from_this();
    auto ret = std::make_shared<std::promise<void>>();
    std::thread([self, ret]() {
      self->WaitForBackgroundTaskCompletion();
      ret->set_value();
    })
        .detach();
    return ret->get_future();
  }

 private:
  std::weak_ptr<ClientImpl> client_impl_;
  FlushSettings flush_settings_;
  std::shared_ptr<FlushEventListener<FlushResponse>> listener_;
  PartitionTimestampQueue partition_timestamp_queue_;
  BackgroundTaskCollection<size_t> background_task_col_;
  std::mutex cancel_mutex_;
  std::map<size_t, olp::client::CancellationToken> cancel_token_map_;
  bool is_cancelled_{false};
};

AutoFlushController::AutoFlushController(const FlushSettings& flush_settings)
    : flush_settings_(flush_settings),
      impl_(std::make_shared<DisabledAutoFlushControllerImpl>()) {}

template <typename ClientImpl, typename FlushResponse>
void AutoFlushController::Enable(
    std::shared_ptr<ClientImpl> client_impl,
    std::shared_ptr<FlushEventListener<FlushResponse>> listener) {
  if (std::dynamic_pointer_cast<
          EnabledAutoFlushControllerImpl<ClientImpl, FlushResponse>>(impl_)) {
    return;
  }

  std::atomic_store(
      &impl_,
      std::static_pointer_cast<AutoFlushControllerImpl>(
          std::make_shared<
              EnabledAutoFlushControllerImpl<ClientImpl, FlushResponse>>(
              client_impl, flush_settings_, listener)));
  impl_->Enable();
}

template void AutoFlushController::Enable<
    StreamLayerClientImpl, const StreamLayerClient::FlushResponse&>(
    std::shared_ptr<StreamLayerClientImpl> apiIngestPublish,
    std::shared_ptr<StreamLayerClient::FlushListener> listener);

std::future<void> AutoFlushController::Disable() {
  auto sp = std::atomic_exchange(
      &impl_, std::static_pointer_cast<AutoFlushControllerImpl>(
                  std::make_shared<DisabledAutoFlushControllerImpl>()));
  return sp->Disable();
}

void AutoFlushController::NotifyQueueEventStart() {
  impl_->NotifyQueueEventStart();
}
void AutoFlushController::NotifyQueueEventComplete() {
  impl_->NotifyQueueEventComplete();
}
void AutoFlushController::NotifyFlushEvent() { impl_->NotifyFlushEvent(); }

}  // namespace write
}  // namespace dataservice
}  // namespace olp
