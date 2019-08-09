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

#include <olp/core/http/Network.h>

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
namespace http {

/**
 * @brief The implementation of Network based on cURL.
 */
class NetworkCurl : public olp::http::Network,
                    public std::enable_shared_from_this<NetworkCurl> {
 public:
  /**
   * @brief NetworkCurl constructor.
   */
  NetworkCurl();

  /**
   * @brief ~NetworkCurl destructor.
   */
  ~NetworkCurl() override;

  /**
   * @brief Not copyable.
   */
  NetworkCurl(const NetworkCurl& other) = delete;

  /**
   * @brief Not movable.
   */
  NetworkCurl(NetworkCurl&& other) = delete;

  /**
   * @brief Not copy-assignable.
   */
  NetworkCurl& operator=(const NetworkCurl& other) = delete;

  /**
   * @brief Not move-assignable.
   */
  NetworkCurl& operator=(NetworkCurl&& other) = delete;

  /**
   * @brief Implementation of Send method from Network abstract class.
   */
  SendOutcome Send(NetworkRequest request,
                   std::shared_ptr<std::ostream> payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override;

  /**
   * @brief Implementation of Cancel method from Network abstract class.
   */
  void Cancel(RequestId id) override;

 private:
  /**
   * @brief Context of each individual network request.
   */
  struct RequestHandle {
    std::string etag{};
    std::string content_type{};
    std::string date{};
    std::chrono::steady_clock::time_point send_time{};
    std::shared_ptr<std::ostream> payload{};
    std::weak_ptr<NetworkCurl> self{};
    Callback callback{};
    HeaderCallback header_callback{};
    DataCallback data_callback{};
    std::uint64_t count{};
    std::uint64_t offset{};
    CURL* handle{nullptr};
    struct curl_slist* chunk{nullptr};
    std::uint32_t transfer_timeout{};
    size_t retry_count{};
    size_t max_retries{};
    int index{};
    int max_age{};
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

  /**
   * @brief POD type represents worker thread notification event.
   */
  struct EventInfo {
    /**
     * @brief Event type.
     */
    enum class Type : char {
      SEND_EVENT,    ///< New request send.
      CANCEL_EVENT,  ///< New request cancellation.
    };

    /**
     * @brief EventInfo constructor.
     */
    EventInfo(Type type, RequestHandle* handle) : type(type), handle(handle) {}

    /// Event type.
    Type type{};

    /// Associated request context.
    RequestHandle* handle{};
  };

  /// Number of CURL easy handles that are always opened.
  static constexpr int kStaticHandleCount = 8;

  /// Maximum allowed number of CURL easy handles.
  static constexpr int kTotalHandleCount = 32;

  /**
   * @brief Actual routine that sends network request.
   *
   * @param[in]  request Network request.
   * @param[in]  id Unique request id.
   * @param[out] payload Stream to store response payload data.
   * @param[in]  header_callback Callback that is called for every header from
   * response.
   * @param[in]  data-callback  Callback to be called when a chunk of data is
   * received. This callback can be triggered multiple times all prior to the
   * final Callback call.
   * @param[in]  callback Callback to be called when request is fully processed
   * or cancelled. After this call there will be no more callbacks triggered and
   * users can consider the request as done.
   * @return ErrorCode.
   */
  ErrorCode SendImplementation(const NetworkRequest& request, int id,
                               const std::shared_ptr<std::ostream>& payload,
                               Network::HeaderCallback header_callback,
                               Network::DataCallback data_callback,
                               Network::Callback callback);

  /**
   * @brief Initialize internal data structures, start worker thread.
   * @return @c true if initialized successfuly, @c false otherwise.
   */
  bool Initialize();

  /**
   * @brief Release network resources, join worker thread.
   * @return @c true if deinitialized successfuly, @c false otherwise.
   */
  void Deinitialize();

  /**
   * @brief Check whether network is initialized.
   * @return @c true if initialized, @c false otherwise.
   */
  bool Initialized() const;

  /**
   * @brief. Check whether NetworkCurl has resources to handle more requests.
   * @return @c true if curl network has free network connections,
   * @c false otherwise.
   */
  bool Ready();

  /**
   * @brief Return count of pending network requests.
   * @return Number of pending network requests.
   */
  size_t AmountPending();

  /**
   * @brief Find handle index in handles_ by handle value.
   * @param[in] handle CURL handle.
   * @return index of associated RequestHandle in handles_ array.
   */
  int GetHandleIndex(CURL* handle);

  /**
   * @brief Allocate new handle RequestHandle.
   * @param[in] id Unique request id.
   * @param[in] callback Request's callback.
   * @param[in] header_callback Request's header callback.
   * @param[in] data_callback Request's data callback.
   * @param[in] payload Stream for response body.
   * @return Pointer to allocated RequestHandle.
   */
  RequestHandle* GetHandle(int id, Network::Callback callback,
                           Network::HeaderCallback headerCallback,
                           Network::DataCallback dataCallback,
                           const std::shared_ptr<std::ostream>& payload);

  /**
   * @brief Release handle after network request is done.
   * This method handles synchronization between caller's thread and worker
   * thread.
   * @param[in] handle Request handle.
   */
  void ReleaseHandle(RequestHandle* handle);

  /**
   * @brief Release handle after network request is done.
   * @param[in] handle Request handle.
   */
  void ReleaseHandleUnlocked(RequestHandle* handle);

  /**
   * @brief Routine that is called when the last bit of response is received.
   *
   * @param[in] handle CURL handle associated with request.
   * @param[in] result CURL return code.
   */
  void CompleteMessage(CURL* handle, CURLcode result);

  /**
   * @brief CURL read callback.
   */
  static size_t RxFunction(void* ptr, size_t size, size_t nmemb,
                           RequestHandle* handle);

  /**
   * @brief CURL header callback.
   */
  static size_t HeaderFunction(char* ptr, size_t size, size_t nmemb,
                               RequestHandle* handle);

  /**
   * @brief The worker thread's main method.
   */
  void Run();

  /**
   * @brief Free resources after the thread terminates.
   */
  void Teardown();

  /**
   * @brief Notify worker thread on some event.
   * @param[in] type Event type.
   * @param[in] handle Related RequestHandle.
   */
  void AddEvent(EventInfo::Type type, RequestHandle* handle);

  /**
   * @brief Checks whether the worker thread is started.
   * @return @c true if the thread is started, @c false otherwise.
   */
  inline bool IsStarted() const;

  /// Contexts for every network request.
  RequestHandle handles_[kTotalHandleCount] = {};

  /// Condition variable used to notify worker thread on event.
  std::condition_variable event_condition_;

  /// Synchronization mutex used during event processing.
  std::mutex event_mutex_;

  /// Synchronization mutex prevents parallel initialization of network.
  std::mutex init_mutex_;

  /// Worker thread.
  std::thread thread_;

  /// Variable used to assign unique request id to each request.
  RequestId request_id_counter_{
      static_cast<RequestId>(RequestIdConstants::RequestIdMin)};

  /**
   * @brief @copydoc NetworkCurl::state_
   */
  enum class WorkerState {
    STOPPED,   ///< The worker thread is not started.
    STARTED,   ///< The worker thread is running.
    STOPPING,  ///< The worker thread will be stopped soon.
  };

  /// The state of the worker thread.
  std::atomic<WorkerState> state_{WorkerState::STOPPED};

  /// Queue of events passed to worker thread.
  std::deque<EventInfo> events_{};

  /// CURL multi handle. Shared among all network requests.
  CURLM* curl_{nullptr};

  /// Turn on and off verbose mode for CURL.
  bool verbose_{false};

  /// Set custom stderr for CURL.
  FILE* stderr_{nullptr};

  /// UNIX Pipe used to notify sleeping worker thread during select() call.
  int pipe_[2]{};

#ifdef NETWORK_HAS_OPENSSL
  /// Mutexes that are used by OpenSSL to synchronize during concurrent
  /// network transfer.
  std::unique_ptr<std::mutex[]> ssl_mutexes_{};
#endif
};

}  // namespace http
}  // namespace olp
