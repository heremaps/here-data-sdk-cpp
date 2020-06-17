/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "NetworkCurl.h"

#include <algorithm>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#ifdef OLP_SDK_NETWORK_HAS_OPENSSL
#include <openssl/crypto.h>
#endif
#ifdef NETWORK_USE_TIMEPROVIDER
#include <openssl/ssl.h>
#endif

#ifdef NETWORK_USE_TIMEPROVIDER
#include "timeprovider/TimeProvider.h"
#endif
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkUtils.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/platform.h"
#include "olp/core/utils/Dir.h"

namespace olp {
namespace http {

namespace {

const char* kLogTag = "CURL";

#ifdef OLP_SDK_NETWORK_HAS_OPENSSL

const auto curl_ca_bundle_name = "ca-bundle.crt";

std::string DefaultCaBundlePath() { return curl_ca_bundle_name; }

std::string AlternativeCaBundlePath() { return curl_ca_bundle_name; }

std::string CaBundlePath() {
  std::string bundle_path;
  bundle_path = DefaultCaBundlePath();
  if (!olp::utils::Dir::FileExists(bundle_path)) {
    bundle_path = AlternativeCaBundlePath();
  }
  if (!olp::utils::Dir::FileExists(bundle_path)) {
    bundle_path.clear();
  }
  return bundle_path;
}
#endif

#if defined(IGNORE_SIGPIPE)

// Block SIGPIPE signals for the current thread and all threads it creates.
int BlockSigpipe() {
  sigset_t sigset;
  int err;

  if (0 != (err = sigemptyset(&sigset))) {
    return err;
  }

  if (0 != (err = sigaddset(&sigset, SIGPIPE))) {
    return err;
  }

  err = pthread_sigmask(SIG_BLOCK, &sigset, NULL);

  return err;
}

// Curl7.35+OpenSSL can write into closed sockets sometimes which results
// in the process being terminated with SIGPIPE on Linux. Here's
// a workaround for that bug. It block SIGPIPE for the startup thread and
// hence for all other threads in the application. The variable itself is
// not used but can be examined.
int BlockSigpipeResult = BlockSigpipe();

#endif  // IGNORE_SIGPIPE

// To avoid static_cast and possible values changes in CURL
curl_proxytype ToCurlProxyType(olp::http::NetworkProxySettings::Type type) {
  using ProxyType = olp::http::NetworkProxySettings::Type;
  switch (type) {
    case ProxyType::HTTP:
      return CURLPROXY_HTTP;
    case ProxyType::SOCKS4:
      return CURLPROXY_SOCKS4;
    case ProxyType::SOCKS5:
      return CURLPROXY_SOCKS5;
    case ProxyType::SOCKS4A:
      return CURLPROXY_SOCKS4A;
    case ProxyType::SOCKS5_HOSTNAME:
      return CURLPROXY_SOCKS5_HOSTNAME;
    default:
      return CURLPROXY_HTTP;
  }
}

int ConvertErrorCode(CURLcode curl_code) {
  if (curl_code == CURLE_OK) {
    return 0;
  } else if ((curl_code == CURLE_REMOTE_ACCESS_DENIED) ||
             (curl_code == CURLE_SSL_CERTPROBLEM) ||
             (curl_code == CURLE_SSL_CIPHER) ||
             (curl_code == CURLE_LOGIN_DENIED)) {
    return static_cast<int>(ErrorCode::AUTHORIZATION_ERROR);
  } else if (curl_code == CURLE_SSL_CACERT) {
    return static_cast<int>(ErrorCode::AUTHENTICATION_ERROR);
  } else if ((curl_code == CURLE_UNSUPPORTED_PROTOCOL) ||
             (curl_code == CURLE_URL_MALFORMAT)) {
    return static_cast<int>(ErrorCode::INVALID_URL_ERROR);
#if (LIBCURL_VERSION_MAJOR >= 7) && (LIBCURL_VERSION_MINOR >= 24)
  } else if (curl_code == CURLE_FTP_ACCEPT_FAILED) {
    return static_cast<int>(ErrorCode::AUTHORIZATION_ERROR);
#endif
  } else if (curl_code == CURLE_COULDNT_RESOLVE_HOST) {
    return static_cast<int>(ErrorCode::INVALID_URL_ERROR);
  } else if (curl_code == CURLE_OPERATION_TIMEDOUT) {
    return static_cast<int>(ErrorCode::TIMEOUT_ERROR);
  } else {
    return static_cast<int>(ErrorCode::IO_ERROR);
  }
}

#ifdef OLP_SDK_NETWORK_HAS_OPENSSL
#ifdef NETWORK_USE_TIMEPROVIDER
static curl_code SslctxFunction(CURL* curl, void* sslctx, void*) {
  // get the current time in seconds since epoch
  std::uint64_t time = static_cast<std::uint64_t>(
      TimeProvider::getClock()->timeSinceEpochMs() / 1000);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  X509_STORE* store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(sslctx));
  X509_VERIFY_PARAM_set_time(store->param, time_t(time));
#else
  X509_VERIFY_PARAM* param = X509_VERIFY_PARAM_new();
  X509_VERIFY_PARAM_set_time(param, time_t(time));
  SSL_CTX_set1_param(static_cast<SSL_CTX*>(sslctx), param);
  X509_VERIFY_PARAM_free(param);
#endif
  return CURLE_OK;
}
#endif
#endif

/**
 * @brief CURL get upload/download data.
 * @param[in] handle CURL easy handle.
 * @param[out] upload_bytes uploaded bytes(headers+data).
 * @param[out] download_bytes downloaded bytes(headers+data).
 */
void GetTraficData(CURL* handle, uint64_t& upload_bytes,
                   uint64_t& download_bytes) {
  upload_bytes = 0;
  download_bytes = 0;
  long headers_size;
  double length_downloaded;
  if (curl_easy_getinfo(handle, CURLINFO_HEADER_SIZE, &headers_size) ==
          CURLE_OK &&
      headers_size > 0) {
    download_bytes += headers_size;
  }
  if (curl_easy_getinfo(handle, CURLINFO_SIZE_DOWNLOAD, &length_downloaded) ==
          CURLE_OK &&
      length_downloaded > 0.0) {
    download_bytes += length_downloaded;
  }

  long length_upload;
  if (curl_easy_getinfo(handle, CURLINFO_REQUEST_SIZE, &length_upload) ==
          CURLE_OK &&
      length_upload > 0) {
    upload_bytes = length_upload;
  }
}

}  // anonymous namespace

NetworkCurl::NetworkCurl(size_t max_requests_count)
    : handles_(max_requests_count),
      static_handle_count_(
          std::max(static_cast<size_t>(1u), max_requests_count / 4)) {
  OLP_SDK_LOG_TRACE(kLogTag, "Created NetworkCurl with address="
                                 << this
                                 << ", handles_count=" << max_requests_count);
  auto error = curl_global_init(CURL_GLOBAL_ALL);
  curl_initialized_ = (error == CURLE_OK);
  if (!curl_initialized_) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Error initializing Curl. Error: %i",
                        static_cast<int>(error));
  }
}

NetworkCurl::~NetworkCurl() {
  OLP_SDK_LOG_TRACE(kLogTag, "Destroyed NetworkCurl object, this=" << this);
  Deinitialize();
  if (curl_initialized_) {
    curl_global_cleanup();
  }
  if (stderr_) {
    fclose(stderr_);
  }
}

bool NetworkCurl::Initialize() {
  std::lock_guard<std::mutex> init_lock(init_mutex_);
  if (!curl_initialized_) {
    OLP_SDK_LOG_ERROR(kLogTag, "Curl was not initialized.");
    return false;
  }

  if (state_ != WorkerState::STOPPED) {
    OLP_SDK_LOG_DEBUG(kLogTag, "Already initialized, this=" << this);
    return true;
  }

#ifdef OLP_SDK_NETWORK_HAS_PIPE2
  if (pipe2(pipe_, O_NONBLOCK)) {
    OLP_SDK_LOG_ERROR(kLogTag, "pipe2 failed, this=" << this);
    return false;
  }
#elif defined OLP_SDK_NETWORK_HAS_PIPE
  if (pipe(pipe_)) {
    OLP_SDK_LOG_ERROR(kLogTag, "pipe failed, this=" << this);
    return false;
  }
  // Set read and write pipes non blocking
  for (size_t i = 0; i < 2; ++i) {
    int flags = fcntl(pipe_[i], F_GETFL);
    if (flags == -1) {
      flags = 0;
    }
    if (fcntl(pipe_[i], F_SETFL, flags | O_NONBLOCK)) {
      OLP_SDK_LOG_ERROR(kLogTag, __PRETTY_FUNCTION__ << ". fcntl for pipe[" << i
                                                     << "] failed. Error "
                                                     << errno);
      return false;
    }
  }
#endif

  // cURL setup
  curl_ = curl_multi_init();
  if (!curl_) {
    OLP_SDK_LOG_ERROR(kLogTag, "curl_multi_init failed, this=" << this);
    return false;
  }

  // handles setup
  std::shared_ptr<NetworkCurl> that = shared_from_this();
  for (auto& handle : handles_) {
    handle.handle = nullptr;
    handle.in_use = false;
    handle.self = that;
  }

  std::unique_lock<std::mutex> lock(event_mutex_);
  // start worker thread
  thread_ = std::thread(&NetworkCurl::Run, this);

  event_condition_.wait(lock,
                        [this] { return state_ == WorkerState::STARTED; });

  return true;
}

void NetworkCurl::Deinitialize() {
  std::lock_guard<std::mutex> init_lock(init_mutex_);
  // Stop worker thread
  if (!IsStarted()) {
    OLP_SDK_LOG_DEBUG(kLogTag, "Already deinitialized, this=" << this);
    return;
  }

  OLP_SDK_LOG_TRACE(kLogTag, "Deinitialize NetworkCurl, this=" << this);

  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STOPPING;
  }

  // We should not destroy this thread from itself
  if (thread_.get_id() != std::this_thread::get_id()) {
    event_condition_.notify_all();
#if defined(OLP_SDK_NETWORK_HAS_PIPE) || defined(OLP_SDK_NETWORK_HAS_PIPE2)
    char tmp = 1;
    if (write(pipe_[1], &tmp, 1) < 0) {
      OLP_SDK_LOG_INFO(kLogTag, __PRETTY_FUNCTION__
                                    << ". Failed to write pipe. Error "
                                    << errno);
    }
#endif
    thread_.join();
  } else {
    // We are trying to stop the very thread we are in. This is not recommended,
    // but we try to handle it gracefully. This could happen by calling from one
    // of the static functions (rxFunction or headerFunction) that was passed to
    // the cURL as callbacks.
    thread_.detach();
  }
}

void NetworkCurl::Teardown() {
  std::vector<std::pair<RequestId, Network::Callback> > completed_messages;
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    events_.clear();

    // handles teardown
    for (auto& handle : handles_) {
      if (handle.handle) {
        if (handle.in_use) {
          curl_multi_remove_handle(curl_, handle.handle);
          completed_messages.emplace_back(handle.id, handle.callback);
        }
        curl_easy_cleanup(handle.handle);
        handle.handle = nullptr;
      }
      handle.self.reset();
    }

    // cURL teardown
    curl_multi_cleanup(curl_);
    curl_ = nullptr;

#if (defined OLP_SDK_NETWORK_HAS_PIPE) || (defined OLP_SDK_NETWORK_HAS_PIPE2)
    close(pipe_[0]);
    close(pipe_[1]);
#endif
  }

  // Handle completed messages
  if (!completed_messages.empty()) {
    for (auto& pair : completed_messages) {
      pair.second(http::NetworkResponse()
                      .WithRequestId(pair.first)
                      .WithStatus(static_cast<int>(ErrorCode::OFFLINE_ERROR))
                      .WithError("Offline: network is deinitialized"));
    }
  }
}

bool NetworkCurl::IsStarted() const { return state_ == WorkerState::STARTED; }

bool NetworkCurl::Initialized() const { return IsStarted(); }

bool NetworkCurl::Ready() {
  if (!IsStarted()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  return std::any_of(
      std::begin(handles_), std::end(handles_),
      [](const RequestHandle& handle) { return !handle.in_use; });
}

size_t NetworkCurl::AmountPending() {
  std::lock_guard<std::mutex> lock(event_mutex_);
  return static_cast<size_t>(
      std::count_if(std::begin(handles_), std::end(handles_),
                    [](const RequestHandle& handle) { return handle.in_use; }));
}

SendOutcome NetworkCurl::Send(NetworkRequest request,
                              std::shared_ptr<std::ostream> payload,
                              Network::Callback callback,
                              Network::HeaderCallback header_callback,
                              Network::DataCallback data_callback) {
  if (!Initialized()) {
    if (!Initialize()) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - network is uninitialized, url="
                                     << request.GetUrl());
      return SendOutcome(ErrorCode::OFFLINE_ERROR);
    }
  }

  RequestId request_id{
      static_cast<RequestId>(RequestIdConstants::RequestIdMin)};
  {
    std::lock_guard<std::mutex> lock(event_mutex_);

    request_id = request_id_counter_;
    if (request_id_counter_ ==
        static_cast<RequestId>(RequestIdConstants::RequestIdMax)) {
      request_id_counter_ =
          static_cast<RequestId>(RequestIdConstants::RequestIdMin);
    } else {
      request_id_counter_++;
    }
  }

  auto error_status = SendImplementation(
      request, request_id, payload, std::move(header_callback),
      std::move(data_callback), std::move(callback));

  if (error_status == ErrorCode::SUCCESS) {
    return SendOutcome(request_id);
  }

  return SendOutcome(error_status);
}

ErrorCode NetworkCurl::SendImplementation(
    const NetworkRequest& request, RequestId id,
    const std::shared_ptr<std::ostream>& payload,
    Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, Network::Callback callback) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Send failed - network is offline, url=" << request.GetUrl());
    return ErrorCode::IO_ERROR;
  }

  const auto& config = request.GetSettings();

  RequestHandle* handle = GetHandle(id, callback, header_callback,
                                    data_callback, payload, request.GetBody());
  if (!handle) {
    return ErrorCode::NETWORK_OVERLOAD_ERROR;
  }

  OLP_SDK_LOG_DEBUG(
      kLogTag, "Send request with url=" << request.GetUrl() << ", id=" << id);

  handle->transfer_timeout = config.GetTransferTimeout();
  handle->ignore_offset = false;  // request.IgnoreOffset();
  handle->skip_content = false;   // config->SkipContentWhenError();

  for (const auto& header : request.GetHeaders()) {
    std::ostringstream sstrm;
    sstrm << header.first;
    sstrm << ": ";
    sstrm << header.second;
    handle->chunk = curl_slist_append(handle->chunk, sstrm.str().c_str());
  }

  if (verbose_) {
    curl_easy_setopt(handle->handle, CURLOPT_VERBOSE, 1L);
    if (stderr_ != nullptr) {
      curl_easy_setopt(handle->handle, CURLOPT_STDERR, stderr_);
    }
  } else {
    curl_easy_setopt(handle->handle, CURLOPT_VERBOSE, 0L);
  }

  const std::string& url = request.GetUrl();
  curl_easy_setopt(handle->handle, CURLOPT_URL, url.c_str());
  auto verb = request.GetVerb();
  if (verb == NetworkRequest::HttpVerb::POST ||
      verb == NetworkRequest::HttpVerb::PUT ||
      verb == NetworkRequest::HttpVerb::PATCH) {
    if (verb == NetworkRequest::HttpVerb::POST) {
      curl_easy_setopt(handle->handle, CURLOPT_POST, 1L);
    } else if (verb == NetworkRequest::HttpVerb::PUT) {
      // http://stackoverflow.com/questions/7569826/send-string-in-put-request-with-libcurl
      curl_easy_setopt(handle->handle, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (verb == NetworkRequest::HttpVerb::PATCH) {
      curl_easy_setopt(handle->handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    }
  } else if (verb == NetworkRequest::HttpVerb::DEL) {
    curl_easy_setopt(handle->handle, CURLOPT_CUSTOMREQUEST, "DELETE");
  } else if (verb == NetworkRequest::HttpVerb::OPTIONS) {
    curl_easy_setopt(handle->handle, CURLOPT_CUSTOMREQUEST, "OPTIONS");
  } else {  // GET or HEAD
    curl_easy_setopt(handle->handle, CURLOPT_POST, 0L);

    if (verb == NetworkRequest::HttpVerb::HEAD) {
      curl_easy_setopt(handle->handle, CURLOPT_NOBODY, 1L);
    }
  }

  if (verb != NetworkRequest::HttpVerb::GET &&
      verb != NetworkRequest::HttpVerb::HEAD) {
    // These can also be used to add body data to a CURLOPT_CUSTOMREQUEST
    // such as delete.
    if (handle->body && !handle->body->empty()) {
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDSIZE,
                       handle->body->size());
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDS,
                       &handle->body->front());
    } else {
      // Some services (eg. Google) require the field size even if zero
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDSIZE, 0);
    }
  }

  const auto& proxy = config.GetProxySettings();
  if (proxy.GetType() != NetworkProxySettings::Type::NONE) {
    curl_easy_setopt(handle->handle, CURLOPT_PROXY,
                     proxy.GetHostname().c_str());
    curl_easy_setopt(handle->handle, CURLOPT_PROXYPORT, proxy.GetPort());
    const auto proxy_type = proxy.GetType();
    if (proxy_type != NetworkProxySettings::Type::HTTP) {
      curl_easy_setopt(handle->handle, CURLOPT_PROXYTYPE,
                       ToCurlProxyType(proxy_type));
    }

    // We expect that both fields are empty or filled
    if (!proxy.GetUsername().empty() && !proxy.GetPassword().empty()) {
      curl_easy_setopt(handle->handle, CURLOPT_PROXYUSERNAME,
                       proxy.GetUsername().c_str());
      curl_easy_setopt(handle->handle, CURLOPT_PROXYPASSWORD,
                       proxy.GetPassword().c_str());
    }
  }

  if (handle->chunk) {
    curl_easy_setopt(handle->handle, CURLOPT_HTTPHEADER, handle->chunk);
  }

#ifdef OLP_SDK_NETWORK_HAS_OPENSSL
  std::string curl_ca_bundle = "";
  if (curl_ca_bundle.empty()) {
    curl_ca_bundle = CaBundlePath();
  }
  if (!curl_ca_bundle.empty()) {
    CURLcode error = curl_easy_setopt(handle->handle, CURLOPT_CAINFO,
                                      curl_ca_bundle.c_str());
    if (CURLE_OK != error) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - curl_easy_setopt error="
                                     << error << ", id=" << id);
      return ErrorCode::UNKNOWN_ERROR;
    }
  }
#endif

  curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYHOST, 2L);
#ifdef NETWORK_USE_TIMEPROVIDER
  curl_easy_setopt(handle->handle, CURLOPT_SSL_CTX_FUNCTION, SslctxFunction);
#endif

  curl_easy_setopt(handle->handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(handle->handle, CURLOPT_CONNECTTIMEOUT,
                   config.GetConnectionTimeout());
  curl_easy_setopt(handle->handle, CURLOPT_TIMEOUT,
                   config.GetConnectionTimeout() + config.GetTransferTimeout());
  curl_easy_setopt(handle->handle, CURLOPT_WRITEFUNCTION,
                   &NetworkCurl::RxFunction);
  curl_easy_setopt(handle->handle, CURLOPT_WRITEDATA, handle);
  curl_easy_setopt(handle->handle, CURLOPT_HEADERFUNCTION,
                   &NetworkCurl::HeaderFunction);
  curl_easy_setopt(handle->handle, CURLOPT_HEADERDATA, handle);
  curl_easy_setopt(handle->handle, CURLOPT_FAILONERROR, 0);
  if (stderr_ == nullptr) {
    curl_easy_setopt(handle->handle, CURLOPT_STDERR, 0);
  }
  curl_easy_setopt(handle->handle, CURLOPT_ERRORBUFFER, handle->error_text);

#if (LIBCURL_VERSION_MAJOR >= 7) && (LIBCURL_VERSION_MINOR >= 21)
  curl_easy_setopt(handle->handle, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(handle->handle, CURLOPT_TRANSFER_ENCODING, 1L);
#endif

#if (LIBCURL_VERSION_MAJOR >= 7) && (LIBCURL_VERSION_MINOR >= 25)
  // Enable keep-alive (since Curl 7.25.0)
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPINTVL, 60L);
#endif

  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    AddEvent(EventInfo::Type::SEND_EVENT, handle);
  }
  return ErrorCode::SUCCESS;  // NetworkProtocol::ErrorNone;
}

void NetworkCurl::Cancel(RequestId id) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Cancel failed - network is offline, id=" << id);
    return;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (auto& handle : handles_) {
    if (handle.in_use && (handle.id == id)) {
      handle.cancelled = true;
      AddEvent(EventInfo::Type::CANCEL_EVENT, &handle);

      OLP_SDK_LOG_TRACE(kLogTag, "Cancel request with id=" << id);
      return;
    }
  }
  OLP_SDK_LOG_WARNING(kLogTag, "Cancel non-existing request with id=" << id);
}

void NetworkCurl::AddEvent(EventInfo::Type type, RequestHandle* handle) {
  events_.emplace_back(type, handle);
  event_condition_.notify_all();
#if (defined OLP_SDK_NETWORK_HAS_PIPE) || (defined OLP_SDK_NETWORK_HAS_PIPE2)
  char tmp = 1;
  if (write(pipe_[1], &tmp, 1) < 0) {
    OLP_SDK_LOG_INFO(kLogTag, "AddEvent - failed for id=" << handle->id
                                                          << ", err=" << errno);
  }
#else
  OLP_SDK_LOG_WARNING(kLogTag,
                      "AddEvent for id=" << handle->id << " - no pipe");
#endif
}

NetworkCurl::RequestHandle* NetworkCurl::GetHandle(
    RequestId id, Network::Callback callback,
    Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, Network::Payload payload,
    NetworkRequest::RequestBodyType body) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(kLogTag,
                      "GetHandle failed - network is offline, id=" << id);
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (auto& handle : handles_) {
    if (!handle.in_use) {
      if (!handle.handle) {
        handle.handle = curl_easy_init();
        if (!handle.handle) {
          OLP_SDK_LOG_ERROR(kLogTag,
                            "GetHandle - curl_easy_init failed, id=" << id);
          return nullptr;
        }
        curl_easy_setopt(handle.handle, CURLOPT_NOSIGNAL, 1);
      }
      handle.in_use = true;
      handle.callback = callback;
      handle.header_callback = header_callback;
      handle.data_callback = data_callback;
      handle.id = id;
      handle.count = 0;
      handle.offset = 0;
      handle.chunk = nullptr;
      handle.cancelled = false;
      handle.transfer_timeout = 30;
      handle.payload = std::move(payload);
      handle.body = std::move(body);
      handle.send_time = std::chrono::steady_clock::now();
      handle.error_text[0] = 0;
      handle.skip_content = false;

      return &handle;
    }
  }

  OLP_SDK_LOG_DEBUG(kLogTag,
                    "GetHandle failed - all CURL handles are busy, id=" << id);
  return nullptr;
}

void NetworkCurl::ReleaseHandle(RequestHandle* handle) {
  std::lock_guard<std::mutex> lock(event_mutex_);
  ReleaseHandleUnlocked(handle);
}

void NetworkCurl::ReleaseHandleUnlocked(RequestHandle* handle) {
  curl_easy_reset(handle->handle);
  if (handle->chunk) {
    curl_slist_free_all(handle->chunk);
    handle->chunk = nullptr;
  }
  handle->in_use = false;
  handle->callback = nullptr;
  handle->header_callback = nullptr;
  handle->data_callback = nullptr;
  handle->payload.reset();
  handle->body.reset();
}

size_t NetworkCurl::RxFunction(void* ptr, size_t size, size_t nmemb,
                               RequestHandle* handle) {
  const size_t len = size * nmemb;

  OLP_SDK_LOG_TRACE(kLogTag,
                    "Received " << len << " bytes for id=" << handle->id);

  std::shared_ptr<NetworkCurl> that = handle->self.lock();
  if (!that) {
    return len;
  }
  long status = 0;
  curl_easy_getinfo(handle->handle, CURLINFO_RESPONSE_CODE, &status);
  if (handle->skip_content && status != http::HttpStatusCode::OK &&
      status != http::HttpStatusCode::PARTIAL_CONTENT &&
      status != http::HttpStatusCode::CREATED && status != 0) {
    return len;
  }

  if (that->IsStarted() && !handle->cancelled) {
    if (handle->data_callback) {
      handle->data_callback(reinterpret_cast<uint8_t*>(ptr),
                            handle->offset + handle->count, len);
    }
    if (handle->payload) {
      if (!handle->ignore_offset) {
        if (handle->payload->tellp() != std::streampos(handle->count)) {
          handle->payload->seekp(handle->count);
          if (handle->payload->fail()) {
            OLP_SDK_LOG_WARNING(kLogTag,
                                "Payload seekp() failed, id=" << handle->id);
            handle->payload->clear();
          }
        }
      }

      const char* data = reinterpret_cast<const char*>(ptr);
      handle->payload->write(data, len);
    }
    handle->count += len;
  }

  // In case we have curl verbose and stderr enabled log the error content
  if (that->stderr_) {
    long http_status = 0;
    curl_easy_getinfo(handle->handle, CURLINFO_RESPONSE_CODE, &http_status);
    if (http_status >= http::HttpStatusCode::BAD_REQUEST) {
      // Log the error content to help troubleshooting
      fprintf(that->stderr_, "\n---ERRORCONTENT BEGIN HANDLE=%p BLOCKSIZE=%u\n",
              handle, (uint32_t)(size * nmemb));
      fwrite(ptr, size, nmemb, that->stderr_);
      fprintf(that->stderr_, "\n---ERRORCONTENT END HANDLE=%p BLOCKSIZE=%u\n",
              handle, (uint32_t)(size * nmemb));
    }
  }

  return len;
}

size_t NetworkCurl::HeaderFunction(char* ptr, size_t size, size_t nitems,
                                   RequestHandle* handle) {
  const size_t len = size * nitems;

  std::shared_ptr<NetworkCurl> that = handle->self.lock();
  if (!that || !that->IsStarted() || handle->cancelled) {
    return len;
  }
  size_t count = len;
  while ((count > 1) && ((ptr[count - 1] == '\n') || (ptr[count - 1] == '\r')))
    count--;
  if (count == 0) {
    return len;
  }
  std::string str(ptr, count);
  std::size_t pos = str.find(':');
  if (pos == std::string::npos) {
    return len;
  }
  std::string key = str.substr(0, pos);
  std::string value;
  if (pos + 2 < str.size()) {
    value = str.substr(pos + 2);
  }

  if (handle->header_callback) {
    handle->header_callback(key, value);
  }
  return len;
}

void NetworkCurl::CompleteMessage(CURL* handle, CURLcode result) {
  std::unique_lock<std::mutex> lock(event_mutex_);
  int index = GetHandleIndex(handle);
  if (index >= 0 && index < static_cast<int>(handles_.size())) {
    RequestHandle& rhandle = handles_[index];
    uint64_t upload_bytes = 0;
    uint64_t download_bytes = 0;
    GetTraficData(rhandle.handle, upload_bytes, download_bytes);

    if (rhandle.cancelled) {
      auto callback = rhandle.callback;
      auto response =
          NetworkResponse()
              .WithRequestId(rhandle.id)
              .WithStatus(static_cast<int>(ErrorCode::CANCELLED_ERROR))
              .WithError("Cancelled")
              .WithBytesDownloaded(download_bytes)
              .WithBytesUploaded(upload_bytes);
      ReleaseHandleUnlocked(&rhandle);

      lock.unlock();
      callback(response);
      return;
    }

    auto callback = rhandle.callback;
    if (!callback) {
      OLP_SDK_LOG_WARNING(
          kLogTag,
          __PRETTY_FUNCTION__ << ". Complete to request without callback");
      ReleaseHandleUnlocked(&rhandle);
      return;
    }

    std::string error("Success");
    int status;
    if ((result == CURLE_OK) || (result == CURLE_HTTP_RETURNED_ERROR)) {
      long http_status = 0;
      curl_easy_getinfo(rhandle.handle, CURLINFO_RESPONSE_CODE, &http_status);
      status = static_cast<int>(http_status);
      if ((rhandle.offset == 0) &&
          (status == HttpStatusCode::PARTIAL_CONTENT)) {
        status = HttpStatusCode::OK;
      }
      // for local file there is no server response so status is 0
      if ((status == 0) && (result == CURLE_OK))
        status = HttpStatusCode::OK;
      error = HttpErrorToString(status);
    } else {
      rhandle.error_text[CURL_ERROR_SIZE - 1] = '\0';
      if (std::strlen(rhandle.error_text) > 0) {
        error = rhandle.error_text;
      } else {
        error = curl_easy_strerror(result);
      }
      status = ConvertErrorCode(result);
    }

    const char* url;
    curl_easy_getinfo(rhandle.handle, CURLINFO_EFFECTIVE_URL, &url);

    OLP_SDK_LOG_DEBUG(kLogTag, "Completed message id="
                                   << handles_[index].id << ", url=" << url
                                   << ", status=(" << status << ") " << error);

    auto response = NetworkResponse()
                        .WithRequestId(handles_[index].id)
                        .WithStatus(status)
                        .WithError(error)
                        .WithBytesDownloaded(download_bytes)
                        .WithBytesUploaded(upload_bytes);
    ReleaseHandleUnlocked(&rhandle);
    lock.unlock();
    callback(response);
  } else {
    OLP_SDK_LOG_WARNING(kLogTag, "Complete message to unknown request");
  }
}

int NetworkCurl::GetHandleIndex(CURL* handle) {
  for (size_t index = 0; index < handles_.size(); index++) {
    if (handles_[index].in_use && (handles_[index].handle == handle)) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

void NetworkCurl::Run() {
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STARTED;
    event_condition_.notify_one();
  }

  while (IsStarted()) {
    std::vector<CURL*> msgs;
    {
      std::unique_lock<std::mutex> lock(event_mutex_);
      while (!events_.empty() && IsStarted()) {
        EventInfo event = events_.front();
        events_.pop_front();
        switch (event.type) {
          case EventInfo::Type::SEND_EVENT: {
            if (event.handle->in_use) {
              CURLMcode res =
                  curl_multi_add_handle(curl_, event.handle->handle);
              if ((res != CURLM_OK) && (res != CURLM_CALL_MULTI_PERFORM)) {
                OLP_SDK_LOG_ERROR(
                    kLogTag, "Send failed for id="
                                 << event.handle->id << " with result=" << res
                                 << ", error=" << curl_multi_strerror(res));
                if (res == CURLM_ADDED_ALREADY) {
                  // something wrong happened, we tried to add an easy handle
                  // already existed in the multi handle
                  OLP_SDK_LOG_ERROR(kLogTag,
                                    " Handle already in use. Handle="
                                        << event.handle->handle
                                        << ", id=" << event.handle->id);
                } else {
                  // do not add the handle to msgs vector as it will be reset in
                  // CompleteMessage handler, and curl will crash in the next
                  // call of curl_multi_perform function.
                  msgs.push_back(event.handle->handle);
                }
              }
            }
            break;
          }
          case EventInfo::Type::CANCEL_EVENT:
            if (event.handle->in_use) {
              auto code = curl_multi_remove_handle(curl_, event.handle->handle);
              if (code != CURLM_OK) {
                OLP_SDK_LOG_ERROR(
                    kLogTag,
                    __PRETTY_FUNCTION__
                        << ". curl_multi_remove_handle has failed. Code="
                        << code << ", " << curl_multi_strerror(code));
              }
              lock.unlock();
              CompleteMessage(event.handle->handle, CURLE_OPERATION_TIMEDOUT);
              lock.lock();
            }
            break;
        }
      }
    }

    if (!IsStarted()) {
      continue;
    }
    if (!msgs.empty()) {
      for (CURL* msg : msgs) {
        CompleteMessage(msg, CURLE_COULDNT_CONNECT);
      }
    }

    // Run cURL queue
    int running = 0;
    {
      do {
      } while (IsStarted() &&
               curl_multi_perform(curl_, &running) == CURLM_CALL_MULTI_PERFORM);
    }

    {  // lock block
      // Handle completed messages
      int msgs_in_queue = 0;
      CURLMsg* msg(nullptr);
      std::unique_lock<std::mutex> lock(event_mutex_);
      while (IsStarted() &&
             (msg = curl_multi_info_read(curl_, &msgs_in_queue))) {
        CURL* handle = msg->easy_handle;
        uint64_t upload_bytes = 0;
        uint64_t download_bytes = 0;
        GetTraficData(handle, upload_bytes, download_bytes);

        if (msg->msg == CURLMSG_DONE) {
          CURLcode result = msg->data.result;
          curl_multi_remove_handle(curl_, handle);
          lock.unlock();
          CompleteMessage(handle, result);
          lock.lock();
        } else {
          // actually this part should never be executed.
          OLP_SDK_LOG_ERROR(kLogTag,
                            "Message complete with unknown state " << msg->msg);
          int handle_index = GetHandleIndex(handle);
          if (handle_index >= 0) {
            if (!handles_[handle_index].callback) {
              OLP_SDK_LOG_WARNING(
                  kLogTag,
                  "Complete to request with unknown state without "
                  "callback");
            } else {
              lock.unlock();
              auto response =
                  NetworkResponse()
                      .WithRequestId(handles_[handle_index].id)
                      .WithStatus(static_cast<int>(ErrorCode::IO_ERROR))
                      .WithError("CURL error")
                      .WithBytesDownloaded(download_bytes)
                      .WithBytesUploaded(upload_bytes);
              handles_[handle_index].callback(response);
              lock.lock();
            }
            curl_multi_remove_handle(curl_, handles_[handle_index].handle);
            ReleaseHandleUnlocked(&handles_[handle_index]);
          } else {
            OLP_SDK_LOG_ERROR(
                kLogTag,
                "No handle index of message complete with unknown state");
          }
        }
      }
    }

    if (!IsStarted()) {
      continue;
    }

    {
      int numfds = 0;
#if defined(OLP_SDK_NETWORK_HAS_PIPE) || defined(OLP_SDK_NETWORK_HAS_PIPE2)
      curl_waitfd waitfd[1];
      waitfd[0].fd = pipe_[0];
      waitfd[0].events = CURL_WAIT_POLLIN;
      waitfd[0].revents = 0;
      auto mc = curl_multi_wait(curl_, waitfd, 1, 1000, &numfds);
      // read pipe data if it is signaled
      if (mc == CURLM_OK && numfds != 0 && waitfd[0].revents != 0) {
        char tmp;
        while (read(waitfd[0].fd, &tmp, 1) > 0) {
        }
      }
#else
      // Without pipe limit wait time to 100ms
      // so that network events can be handled in a reasonable time
      auto mc = curl_multi_wait(curl_, nullptr, 0, 100, &numfds);
#endif
      if (mc != CURLM_OK) {
        OLP_SDK_LOG_INFO(kLogTag, __PRETTY_FUNCTION__
                                      << ". curl_multi_wait: Failed. error="
                                      << mc);
        continue;
      }
      // 'numfds' being zero means either a timeout or no file descriptors to
      // wait for.
      if (numfds == 0) {
        std::unique_lock<std::mutex> lock(event_mutex_);

        bool in_use_handles = std::any_of(
            handles_.begin(), handles_.end(),
            [](const RequestHandle& handle) { return handle.in_use; });

        if (!IsStarted()) {
          continue;
        }

        if (!in_use_handles) {
          // Enter wait only when all handles are free
          event_condition_.wait_for(lock, std::chrono::seconds(2));
        } else {
          constexpr int kWaitMsec = 100;
          event_condition_.wait_for(lock, std::chrono::milliseconds(kWaitMsec));
        }
      }
    }
  }

  Teardown();
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STOPPED;
  }
  OLP_SDK_LOG_DEBUG(kLogTag, "Thread exit, this=" << this);
}

}  // namespace http
}  // namespace olp
