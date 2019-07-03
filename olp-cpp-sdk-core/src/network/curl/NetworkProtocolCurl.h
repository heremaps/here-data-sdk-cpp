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

#pragma once

#include "../Memory.h"

#include <olp/core/network/NetworkProtocol.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <curl/curl.h>
#ifdef NETWORK_HAS_OPENSSL
#include <openssl/crypto.h>
#endif

namespace olp {
namespace network {
/**
 * @brief The implementation of NetworkProtocol for cURL
 */
class NetworkProtocolCurl
    : public NetworkProtocol,
      public std::enable_shared_from_this<NetworkProtocolCurl> {
 public:
  /**
   * @brief NetworkProtocolCurl constructor
   */
  NetworkProtocolCurl();

  /**
   * @brief ~NetworkProtocolCurl destructor
   */
  ~NetworkProtocolCurl() override;

  NetworkProtocolCurl(const NetworkProtocolCurl& other) = delete;

  NetworkProtocolCurl(NetworkProtocolCurl&& other) = delete;

  NetworkProtocolCurl& operator=(const NetworkProtocolCurl& other) = delete;

  NetworkProtocolCurl& operator=(NetworkProtocolCurl&& other) = delete;

  bool Initialize() override;

  void Deinitialize() override;

  bool Initialized() const override;

  bool Ready() override;

  ErrorCode Send(const NetworkRequest& request, int id,
                 const std::shared_ptr<std::ostream>& payload,
                 std::shared_ptr<NetworkConfig> config,
                 Network::HeaderCallback headerCallback,
                 Network::DataCallback dataCallback,
                 Network::Callback callback) override;

  bool Cancel(int id) override;

  size_t AmountPending() override;

  void protocolReady();

 private:
  static constexpr int kStaticHandleCount = 8;
  static constexpr int kTotalHandleCount = 32;

  struct RequestHandle {
    mem::MemoryScopeTracker tracker{mem::MemoryScopeTracker(false)};
    std::string etag{};
    std::string content_type{};
    std::string date{};
    std::chrono::steady_clock::time_point send_time{};
    std::shared_ptr<std::ostream> payload{};
    std::weak_ptr<NetworkProtocolCurl> self{};
    Network::Callback callback{};
    Network::HeaderCallback header_callback{};
    Network::DataCallback data_callback{};
    std::uint64_t count{};
    std::uint64_t offset{};
    CURL* handle{nullptr};
    struct curl_slist* chunk{nullptr};
    std::uint32_t transfer_timeout{};
    size_t retry_count{};
    size_t max_retries{};
    int index{};
    int max_Age{};
    time_t expires{};
    int id{};
    bool ignore_offset{};
    bool in_use{};
    bool range_out{};
    bool cancelled{};
    bool get_statistics{};
    bool skip_content{};
    char error_text[CURL_ERROR_SIZE]{};
  };

  struct EventInfo {
    enum class Type : char { SendEvent, CancelEvent };

    Type type;
    RequestHandle* handle;

    EventInfo(Type type, RequestHandle* handle) : type(type), handle(handle) {}
  };

  static size_t RxFunction(void* ptr, size_t size, size_t nmemb,
                           RequestHandle* handle);
  static size_t HeaderFunction(char* ptr, size_t size, size_t nmemb,
                               RequestHandle* handle);

  int GetHandleIndex(CURL* handle);

  RequestHandle* GetHandle(int id, Network::Callback callback,
                           Network::HeaderCallback headerCallback,
                           Network::DataCallback dataCallback,
                           const std::shared_ptr<std::ostream>& payload);
  void ReleaseHandle(RequestHandle* handle);

  void ReleaseHandleUnlocked(RequestHandle* handle);

  void CompleteMessage(CURL* handle, CURLcode result);

  /**
   * @brief The worker thread's main method.
   */
  void Run();

  /**
   * @brief Free resources after the thread terminates.
   */
  void Teardown();

  void AddEvent(EventInfo::Type type, RequestHandle* handle);

  RequestHandle handles_[kTotalHandleCount] = { };
  std::condition_variable event_condition_;
  std::mutex mutex_;
  std::mutex init_mutex_;
  std::thread thread_;  //!< The worker thread.

  /**
   * @brief @copydoc NetworkProtocolCurl::state_
   */
  enum class WorkerState {
    Stopped,   // The worker thread is not started.
    Started,   // The worker thread is running.
    Stopping,  // The worker thread will be stopped soon.
  };
  std::atomic<WorkerState> state_{WorkerState::Stopped};  //!< The state of the worker thread.

  /**
   * @brief Checks whether the worker thread is started.
   * @return @c true if the thread is started, @c false otherwise.
   */
  inline bool IsStarted() const;

  mem::deque<EventInfo> events_{};
  CURLM* curl_{nullptr};
  bool verbose_{false};
  FILE* stderr_{nullptr};
  int pipe_[2]{};
  mem::MemoryScopeTracker tracker_{false};
#ifdef NETWORK_HAS_OPENSSL
  std::unique_ptr<std::mutex[]> ssl_mutexes_{};
#endif
};

}  // namespace network
}  // namespace olp
