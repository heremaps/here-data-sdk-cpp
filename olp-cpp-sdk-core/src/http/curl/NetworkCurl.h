/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include <curl/curl.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <rapidjson/ostreamwrapper.h>
#include <boost/optional.hpp>

#if defined(OLP_SDK_ENABLE_ANDROID_CURL) && !defined(ANDROID_HOST)
#include <openssl/ossl_typ.h>

#ifdef OPENSSL_NO_MD5
#error cURL enabled network implementation for Android requires MD5 from OpenSSL
#endif

// Android uses MD5 to encode certificates name, but OpenSSL uses SHA1 for
// certificates lookup -> so OpenSSL won't be able to get appropriate
// certificate as it won't be able to locate one. We need to add our custom
// lookup method that uses MD5. Old SHA1 lookup will be left as is.
#define OLP_SDK_USE_MD5_CERT_LOOKUP
#endif

// CURLOPT_CAINFO_BLOB has become available only in curl-7.77
// cf. https://curl.se/libcurl/c/CURLOPT_CAINFO_BLOB.html,
#if CURL_AT_LEAST_VERSION(7, 77, 0)
#define OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
#endif

#include "olp/core/http/CertificateSettings.h"
#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkInitializationSettings.h"
#include "olp/core/http/NetworkRequest.h"
#include "olp/core/logging/LogContext.h"

#include <olp/core/thread/Atomic.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>
#include <array>
#include <fstream>
#include <iomanip>

namespace olp {
namespace http {

class SessionRecording {
 public:
  static constexpr RequestId max_value = 100000;

  // Chrono microseconds type but with a lower range to save some memory.
  using MicroSeconds = std::chrono::duration<uint32_t, std::micro>;

  struct Timings {
    MicroSeconds queue_time;
    MicroSeconds name_lookup_time;
    MicroSeconds connect_time;
    MicroSeconds app_connect_time;
    MicroSeconds pre_transfer_time;
    MicroSeconds post_transfer_time;
    MicroSeconds start_transfer_time;
    MicroSeconds total_time;
  };

  void SetRequestParameters(
      const RequestId request_id, const std::string& url,
      const NetworkRequest::HttpVerb method,
      const std::vector<std::pair<std::string, std::string>>& headers,
      const size_t body_size) {
    if (request_id >= max_value) {
      return;
    }

    const auto url_hash = hash_(url);
    cache_[url_hash] = url;

    auto* entry = GetEntry(request_id);

    entry->url = url_hash;
    entry->method = static_cast<uint8_t>(method);

    entry->request_headers_offset = headers_.size();
    entry->request_headers_count = headers.size();

    for (const auto& header : headers) {
      cache_[hash_(header.first)] = header.first;
      cache_[hash_(header.second)] = header.second;
      headers_.emplace_back(hash_(header.first), hash_(header.second));
      entry->request_headers_size +=
          header.first.size() + header.second.size() + 4;
    }

    entry->request_body_size = body_size;

    entry->start_time = std::chrono::system_clock::now();
  }

  void SetResponseParameters(
      const RequestId request_id, const int http_version, const int status_code,
      const std::vector<std::pair<std::string, std::string>>& headers,
      const std::string& content_type, const unsigned long headers_size,
      const unsigned long body_size) {
    if (request_id >= max_value) {
      return;
    }

    auto* entry = GetEntry(request_id);

    entry->status_code = static_cast<uint8_t>(status_code);
    entry->http_version = static_cast<uint8_t>(http_version);

    entry->response_headers_offset = headers_.size();
    entry->response_headers_count = headers.size();

    for (const auto& header : headers) {
      cache_[hash_(header.first)] = header.first;
      cache_[hash_(header.second)] = header.second;
      headers_.emplace_back(hash_(header.first), hash_(header.second));
    }

    entry->response_headers_size = static_cast<uint16_t>(headers_size);
    entry->response_body_size = static_cast<uint32_t>(body_size);

    entry->duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - entry->start_time);

    cache_[hash_(content_type)] = content_type;
    entry->content_type = hash_(content_type);
  }

  void SetRequestTimings(const RequestId request_id, const Timings& timings) {
    if (request_id > timings_.size()) {
      timings_.resize(request_id);
    }

    timings_[request_id - 1] = timings;
  }

  void ArchiveToFile(const std::string& out_file_path) const {
    std::ofstream file(out_file_path);

    rapidjson::OStreamWrapper os(file);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(os);
    writer.StartObject();
    writer.Key("log");
    writer.StartObject();

    // Version
    writer.Key("version");
    writer.String("1.2");

    // Creator
    writer.Key("creator");
    writer.StartObject();
    writer.Key("name");
    writer.String("DataSDK");
    writer.Key("version");
    writer.String("1.0");
    writer.EndObject();

    // Entries
    writer.Key("entries");
    writer.StartArray();
    for (auto request_index = 0u; request_index < requests_.size();
         ++request_index) {
      const auto& request = requests_[request_index];
      writer.StartObject();
      // startedDateTime
      writer.Key("startedDateTime");

      std::stringstream ss;
      std::time_t t = std::chrono::system_clock::to_time_t(request.start_time);
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    request.start_time.time_since_epoch()) %
                1000;
      ss << std::put_time(std::localtime(&t), "%FT%T") << '.'
         << std::setfill('0') << std::setw(3) << ms.count() << "Z";

      writer.String(ss.str().c_str());

      // time
      writer.Key("time");
      writer.Int(request.duration.count());
      // writer.Int(timings_[request_index].total_time.count() / 1000);

      // request
      writer.Key("request");
      writer.StartObject();
      {
        // method
        writer.Key("method");
        writer.String("GET");
        // url
        writer.Key("url");
        writer.String(cache_.at(request.url).c_str());
        // httpVersion
        writer.Key("httpVersion");
        writer.String("HTTP/1.1");
        // cookies  []
        writer.Key("cookies");
        writer.StartArray();
        writer.EndArray();
        // headers  []
        writer.Key("headers");
        writer.StartArray();
        for (auto i = 0u; i < request.request_headers_count; ++i) {
          auto header = headers_[request.request_headers_offset + i];
          writer.StartObject();
          writer.Key("name");
          writer.Key(cache_.at(header.first).c_str());
          writer.Key("value");
          writer.Key(cache_.at(header.second).c_str());
          writer.EndObject();
        }
        writer.EndArray();
        // queryString []
        writer.Key("queryString");
        writer.StartArray();
        writer.EndArray();
        // headersSize   int
        writer.Key("headersSize");
        writer.Int(request.request_headers_size);
        // bodySize  int
        writer.Key("bodySize");
        writer.Int(request.request_body_size);
      }
      writer.EndObject();

      // response
      writer.Key("response");
      writer.StartObject();
      {
        writer.Key("status");
        writer.Int(request.status_code);

        writer.Key("httpVersion");
        if (request.http_version == 1) {
          writer.String("HTTP/1.1");
        } else if (request.http_version == 2) {
          writer.String("HTTP/2");
        } else {
          writer.String("UNKNOWN");
        }

        writer.Key("cookies");
        writer.StartArray();
        writer.EndArray();

        writer.Key("headers");
        writer.StartArray();
        for (auto i = 0u; i < request.response_headers_count; ++i) {
          auto header = headers_[request.response_headers_offset + i];
          writer.StartObject();
          writer.Key("name");
          writer.String(cache_.at(header.first).c_str());
          writer.Key("value");
          writer.String(cache_.at(header.second).c_str());
          writer.EndObject();
        }
        writer.EndArray();

        writer.Key("content");
        writer.StartObject();
        writer.Key("size");
        writer.Int(request.response_body_size);
        writer.Key("mimeType");
        writer.String(cache_.at(request.content_type).c_str());
        writer.EndObject();

        writer.Key("redirectURL");
        writer.String("");

        writer.Key("headersSize");
        writer.Int(request.response_headers_size);

        writer.Key("bodySize");
        writer.Int(request.response_body_size);
      }
      writer.EndObject();

      // timings
      writer.Key("timings");
      writer.StartObject();
      if (!timings_.empty()) {
        const auto& timings = timings_[request_index];
        writer.Key("blocked");
        writer.Double(timings.queue_time.count() / 1000.0);
        writer.Key("dns");
        writer.Double((timings.name_lookup_time - timings.queue_time).count() /
                      1000.0);
        writer.Key("connect");
        writer.Double(
            (timings.connect_time - timings.name_lookup_time).count() / 1000.0);
        writer.Key("ssl");
        writer.Double(
            (timings.app_connect_time - timings.connect_time).count() / 1000.0);
        writer.Key("send");
        writer.Double(
            (timings.pre_transfer_time - timings.app_connect_time).count() /
            1000.0);
        writer.Key("wait");
        writer.Double(
            (timings.start_transfer_time - timings.pre_transfer_time).count() /
            1000.0);
        writer.Key("receive");
        writer.Double(
            (timings.total_time - timings.start_transfer_time).count() /
            1000.0);
      }
      writer.EndObject();

      writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
    writer.EndObject();
  }

 private:
  struct RequestEntry {
    size_t url{};
    size_t content_type{};

    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::milliseconds duration{};

    uint16_t request_headers_offset{};
    uint16_t request_headers_size{};
    uint16_t response_headers_offset{};
    uint16_t response_headers_size{};
    uint8_t request_headers_count{};
    uint8_t response_headers_count{};
    uint32_t request_body_size{};
    uint32_t response_body_size{};

    uint8_t method{};
    uint8_t http_version{};
    uint8_t status_code{};
  };

  RequestEntry* GetEntry(const RequestId request_id) {
    if (request_id > requests_.size()) {
      requests_.resize(request_id);
    }

    return &requests_[request_id - 1];
  }

  std::string out_file_path_;
  std::unordered_map<size_t, std::string> cache_{};
  std::vector<RequestEntry> requests_{};
  std::vector<Timings> timings_{};
  std::vector<std::pair<size_t, size_t>> headers_{};
  std::hash<std::string> hash_;
};

/**
 * @brief The implementation of Network based on cURL.
 */
class NetworkCurl : public Network,
                    public std::enable_shared_from_this<NetworkCurl> {
 public:
  /**
   * @brief NetworkCurl constructor.
   */
  explicit NetworkCurl(NetworkInitializationSettings settings);

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
  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override;

  /**
   * @brief Implementation of Cancel method from Network abstract class.
   */
  void Cancel(RequestId id) override;

 private:
  /**
   * @brief Context of each network request.
   */
  struct RequestHandle {
    std::string request_url;
    std::string request_method;
    NetworkRequest::RequestBodyType request_body;
    std::shared_ptr<curl_slist> request_headers;

    HeaderCallback out_header_callback;
    DataCallback out_data_callback;
    Payload out_data_stream;
    Callback out_completion_callback;
    std::uint64_t bytes_received{0};

    std::vector<std::pair<std::string, std::string>> response_headers;

    std::chrono::steady_clock::time_point send_time{};
    std::weak_ptr<NetworkCurl> self{};

    std::shared_ptr<CURL> curl_handle;
    RequestId id{};
    bool in_use{false};
    bool cancelled{false};
    char error_text[CURL_ERROR_SIZE]{};
    std::shared_ptr<const logging::LogContext> log_context;
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

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  /**
   * @brief Blobs required for custom certificate validation.
   */
  struct SslCertificateBlobs {
    using OptionalBlob = boost::optional<struct curl_blob>;

    /// Certificate blob.
    OptionalBlob ssl_cert_blob;

    /// Private key blob.
    OptionalBlob ssl_key_blob;

    /// Certificate authority blob.
    OptionalBlob ca_info_blob;
  };
#endif

  /**
   * @brief Actual routine that sends network request.
   *
   * @param[in]  request Network request.
   * @param[in]  id Unique request id.
   * @param[out] payload Stream to store response payload data.
   * @param[in]  header_callback Callback that is called for every header from
   * response.
   * @param[in]  data_callback  Callback to be called when a chunk of data is
   * received. This callback can be triggered multiple times all prior to the
   * final Callback call.
   * @param[in]  callback Callback to be called when request is fully processed
   * or cancelled. After this call there will be no more callbacks triggered and
   * users can consider the request as done.
   * @return ErrorCode.
   */
  ErrorCode SendImplementation(const NetworkRequest& request, RequestId id,
                               const std::shared_ptr<std::ostream>& payload,
                               HeaderCallback header_callback,
                               DataCallback data_callback, Callback callback);

  /**
   * @brief Initialize internal data structures, start worker thread.
   * @return @c true if initialized successfully, @c false otherwise.
   */
  bool Initialize();

  /**
   * @brief Release network resources, join worker thread.
   * @return @c true if deinitialized successfully, @c false otherwise.
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
   * @brief Find a handle in handles_ by curl handle.
   * @param[in] handle CURL handle.
   * @return Pointer to the allocated RequestHandle.
   */
  RequestHandle* FindRequestHandle(const CURL* handle);

  /**
   * @brief Allocate new handle RequestHandle.
   *
   * @return Pointer to the allocated RequestHandle.
   */
  RequestHandle* InitRequestHandle();

  /**
   * @brief Reset the handle after network request is done.
   * @param[in] handle Request handle.
   * @param[in] cleanup_handle If true, then a handle is completely released.
   * Otherwise, a handle is reset, which preserves DNS cache, Session ID cache,
   * cookies, and so on.
   */
  static void ReleaseHandleUnlocked(RequestHandle* handle, bool cleanup_handle);

  /**
   * @brief Routine that is called when the last bit of response is received.
   *
   * @param[in] curl_handle CURL handle associated with request.
   * @param[in] result CURL return code.
   */
  void CompleteMessage(CURL* curl_handle, CURLcode result);

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

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  /**
   * @brief Setups all necessary blobs for custom certificate settings.
   */
  void SetupCertificateBlobs();
#endif

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
  /**
   * @brief Adds new lookup method for certificates search routine.
   *
   * @param[in] curl cURL instance.
   * @param[in] ssl_ctx OpenSSL context.
   * @param[in] handle Related RequestHandle.
   * @return An error code for the operation.
   */
  static CURLcode AddMd5LookupMethod(CURL* curl, SSL_CTX* ssl_ctx,
                                     RequestHandle* handle);
#endif

  /// Contexts for every network request.
  std::vector<RequestHandle> handles_;

  /// Number of CURL easy handles that are always opened.
  const size_t static_handle_count_;

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

  /// Set custom stderr for CURL.
  FILE* stderr_{nullptr};

  /// UNIX Pipe used to notify sleeping worker thread during select() call.
  int pipe_[2]{};

  /// Stores value if `curl_global_init()` was successful on construction.
  bool curl_initialized_;

  /// Store original certificate setting in order to reference them in the SSL
  /// blobs so cURL does not need to copy them.
  CertificateSettings certificate_settings_;

  /// The path to store CURL logs
  boost::optional<std::string> curl_log_path_;

  boost::optional<thread::Atomic<SessionRecording>> session_recording_;

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  /// SSL certificate blobs.
  boost::optional<SslCertificateBlobs> ssl_certificates_blobs_;
#endif
};

}  // namespace http
}  // namespace olp
