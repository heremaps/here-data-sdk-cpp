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

#include "NetworkCurl.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <curl/curl.h>

#ifdef NETWORK_HAS_OPENSSL
#include <openssl/crypto.h>
#endif
#ifdef NETWORK_USE_TIMEPROVIDER
#include <openssl/ssl.h>
#endif

#include "../NetworkUtils.h"
#ifdef NETWORK_USE_TIMEPROVIDER
#include "timeprovider/TimeProvider.h"
#endif
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/porting/platform.h"
#include "olp/core/utils/Dir.h"

namespace olp {
namespace http {

namespace {

const char* kLogTag = "CURL";
constexpr std::chrono::seconds kHandleLostTimeout(30);
constexpr std::chrono::seconds kHandleReuseTimeout(120);

std::vector<std::pair<std::string, std::string> > GetStatistics(
    CURL* handle, std::size_t retryCount) {
  std::vector<std::pair<std::string, std::string> > statistics;
  double time;
  curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME, &time);
  statistics.emplace_back("TotalTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_NAMELOOKUP_TIME, &time);
  statistics.emplace_back("NameLookupTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_CONNECT_TIME, &time);
  statistics.emplace_back("ConnectTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_APPCONNECT_TIME, &time);
  statistics.emplace_back("AppConnectTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_PRETRANSFER_TIME, &time);
  statistics.emplace_back("PreTransferTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_STARTTRANSFER_TIME, &time);
  statistics.emplace_back("StartTransferTime", std::to_string(time));
  curl_easy_getinfo(handle, CURLINFO_REDIRECT_TIME, &time);
  statistics.emplace_back("RedirectTime", std::to_string(time));
  statistics.emplace_back("Retries", std::to_string(retryCount));
  return statistics;
}

#ifdef NETWORK_HAS_OPENSSL

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
    // if we appear to still have network connectivity then this is
    // likely due to an invalid URL.
    // if (olp::network::NetworkConnectivity::IsNetworkConnected()) {
    //   return static_cast<int>(ErrorCode::INVALID_URL_ERROR);
    // } else {
    return static_cast<int>(ErrorCode::OFFLINE_ERROR);
    // }
  } else if (curl_code == CURLE_OPERATION_TIMEDOUT) {
    return static_cast<int>(ErrorCode::TIMEOUT_ERROR);
  } else {
    return static_cast<int>(ErrorCode::IO_ERROR);
  }
}

#ifdef NETWORK_HAS_OPENSSL
// Lifetime of the mutex table is managed by NetworkCurl object.
static std::mutex* gSslMutexes;

void SslLockingFunction(int mode, int n, const char* file, int line) {
  if (gSslMutexes) {
    if (mode & CRYPTO_LOCK) {
      gSslMutexes[n].lock();
    } else {
      gSslMutexes[n].unlock();
    }
  }
}

unsigned long SslIdFunction(void) {
  std::hash<std::thread::id> hasher;
  return static_cast<unsigned long>(hasher(std::this_thread::get_id()));
}

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

}  // anonymous namespace

NetworkCurl::NetworkCurl() {}

NetworkCurl::~NetworkCurl() {
  if (state_ == WorkerState::STARTED) {
    Deinitialize();
  }
  if (stderr_) {
    fclose(stderr_);
  }
}

bool NetworkCurl::Initialize() {
  if (state_ != WorkerState::STOPPED) {
    EDGE_SDK_LOG_DEBUG(kLogTag, "Already initialized");
    return true;
  }
  std::lock_guard<std::mutex> init_lock(init_mutex_);

#ifdef NETWORK_HAS_PIPE2
  if (pipe2(pipe_, O_NONBLOCK)) {
    EDGE_SDK_LOG_ERROR(kLogTag, "pipe2 failed");
    return false;
  }
#elif defined NETWORK_HAS_PIPE
  if (pipe(pipe_)) {
    EDGE_SDK_LOG_ERROR(kLogTag, "pipe failed");
    return false;
  }
  // Set pipes non blocking
  int flags;
  if (-1 == (flags = fcntl(pipe_[0], F_GETFL, 0))) {
    flags = 0;
  }
  if (fcntl(pipe_[0], F_SETFL, flags | O_NONBLOCK)) {
    EDGE_SDK_LOG_ERROR(kLogTag, "fcntl for pipe failed");
    return false;
  }
  if (-1 == (flags = fcntl(pipe_[1], F_GETFL, 0))) {
    flags = 0;
  }
  if (fcntl(pipe_[1], F_SETFL, flags | O_NONBLOCK)) {
    EDGE_SDK_LOG_ERROR(kLogTag, "fcntl for pipe failed");
    return false;
  }
#endif

#ifdef NETWORK_HAS_OPENSSL
  // OpenSSL setup
  ssl_mutexes_ = std::make_unique<std::mutex[]>(CRYPTO_num_locks());
  gSslMutexes = ssl_mutexes_.get();

  CRYPTO_set_id_callback(SslIdFunction);
  CRYPTO_set_locking_callback(SslLockingFunction);
#endif

  // cURL setup
  curl_ = curl_multi_init();
  if (!curl_) {
    EDGE_SDK_LOG_ERROR(kLogTag, "curl_multi_init failed");
    return false;
  }

  // handles setup
  std::shared_ptr<NetworkCurl> that = shared_from_this();
  for (int i = 0; i < kTotalHandleCount; i++) {
    if (i < kStaticHandleCount) {
      handles_[i].handle = curl_easy_init();
    } else {
      handles_[i].handle = nullptr;
    }
    handles_[i].index = i;
    handles_[i].in_use = false;
    handles_[i].self = that;
  }

  // start worker thread
  thread_ = std::thread(&NetworkCurl::Run, this);

  std::unique_lock<std::mutex> lock(event_mutex_);
  event_condition_.wait(lock,
                        [this] { return state_ == WorkerState::STARTED; });
  return true;
}

void NetworkCurl::Deinitialize() {
  // Stop worker thread
  if (state_ != WorkerState::STARTED) {
    EDGE_SDK_LOG_DEBUG(kLogTag, "Already deinitialized");
    return;
  }
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STOPPING;
    event_condition_.notify_one();
  }

  std::lock_guard<std::mutex> init_lock(init_mutex_);
  if (thread_.get_id() != std::this_thread::get_id()) {
    event_condition_.notify_all();
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
#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
  char tmp = 1;
  if (write(pipe_[1], &tmp, 1) < 0) {
    EDGE_SDK_LOG_INFO(kLogTag, "deinitialize - failed to write pipe " << errno);
  }
#endif

  // handles teardown
  std::vector<std::pair<RequestId, Network::Callback> > completed_messages;
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    events_.clear();

    for (int i = 0; i < kTotalHandleCount; i++) {
      if (handles_[i].handle) {
        if (handles_[i].in_use) {
          curl_multi_remove_handle(curl_, handles_[i].handle);
          completed_messages.emplace_back(handles_[i].id, handles_[i].callback);
        }
        curl_easy_cleanup(handles_[i].handle);
        handles_[i].handle = nullptr;
        handles_[i].self.reset();
      }
    }
  }

  // cURL teardown
  curl_multi_cleanup(curl_);
  curl_ = nullptr;

#ifdef NETWORK_HAS_OPENSSL
  // OpenSSL teardown
  CRYPTO_set_id_callback(nullptr);
  CRYPTO_set_locking_callback(nullptr);
  gSslMutexes = nullptr;
  ssl_mutexes_.reset();
#endif

#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
  close(pipe_[0]);
  close(pipe_[1]);
#endif

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
    const NetworkRequest& request, int id,
    const std::shared_ptr<std::ostream>& payload,
    Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, Network::Callback callback) {
  EDGE_SDK_LOG_TRACE(kLogTag, "send with id = " << id);

  if (!IsStarted()) {
    return ErrorCode::IO_ERROR;
  }

  const auto& config = request.GetSettings();

  RequestHandle* handle =
      GetHandle(id, callback, header_callback, data_callback, payload);
  if (!handle) {
    return ErrorCode::UNKNOWN_ERROR;  // NetworkProtocol::ErrorNotReady;
  }

  handle->transfer_timeout = config.GetTransferTimeout();
  handle->max_retries = config.GetRetries();
  handle->ignore_offset = false;   // request.IgnoreOffset();
  handle->get_statistics = false;  // request.GetStatistics();
  handle->skip_content = false;    // config->SkipContentWhenError();

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
    auto content = request.GetBody();
    if (content && !content->empty()) {
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDSIZE, content->size());
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDS, &content->front());
    } else {
      // Some services (eg. Google) require the field size even if zero
      curl_easy_setopt(handle->handle, CURLOPT_POSTFIELDSIZE, 0);
    }
  }

  bool sys_dont_verify_certificate = true;

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

#ifdef NETWORK_HAS_OPENSSL
  std::string curl_ca_bundle = "";
  if (curl_ca_bundle.empty()) {
    curl_ca_bundle = CaBundlePath();
  }
  if (!curl_ca_bundle.empty()) {
    CURLcode error = curl_easy_setopt(handle->handle, CURLOPT_CAINFO,
                                      curl_ca_bundle.c_str());
    if (CURLE_OK != error) {
      return ErrorCode::UNKNOWN_ERROR;
    }
    EDGE_SDK_LOG_TRACE(kLogTag, "curl bundle path: " << curl_ca_bundle);
  }
#endif

  if (sys_dont_verify_certificate) {
    curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYHOST, 0L);
  } else {
    curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(handle->handle, CURLOPT_SSL_VERIFYHOST, 2L);
#ifdef NETWORK_USE_TIMEPROVIDER
    curl_easy_setopt(handle->handle, CURLOPT_SSL_CTX_FUNCTION, SslctxFunction);
#endif
  }

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
  // if (config && config->IsAutoDecompressionEnabled()) {
  curl_easy_setopt(handle->handle, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(handle->handle, CURLOPT_TRANSFER_ENCODING, 1L);
  // }
#endif

#if (LIBCURL_VERSION_MAJOR >= 7) && (LIBCURL_VERSION_MINOR >= 25)
  // Enable keep-alive (since Curl 7.25.0)
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(handle->handle, CURLOPT_TCP_KEEPINTVL, 60L);
#endif

  if (!IsStarted()) {
    return ErrorCode::OFFLINE_ERROR;  // NetworkProtocol::ErrorNotReady;
  }
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    AddEvent(EventInfo::Type::SEND_EVENT, handle);
  }
  return ErrorCode::SUCCESS;  // NetworkProtocol::ErrorNone;
}

void NetworkCurl::Cancel(RequestId id) {
  EDGE_SDK_LOG_TRACE(kLogTag, "cancel with id = " << id);

  if (!IsStarted()) {
    return;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (int i = 0; i < kTotalHandleCount; i++) {
    if (handles_[i].in_use && (handles_[i].id == id)) {
      handles_[i].cancelled = true;
      AddEvent(EventInfo::Type::CANCEL_EVENT, &handles_[i]);
      return;
    }
  }
  EDGE_SDK_LOG_WARNING(kLogTag, "cancel for non-existing request " << id);
  return;
}

void NetworkCurl::AddEvent(EventInfo::Type type, RequestHandle* handle) {
  events_.emplace_back(type, handle);
  event_condition_.notify_all();
#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
  char tmp = 1;
  if (write(pipe_[1], &tmp, 1) < 0) {
    EDGE_SDK_LOG_INFO(kLogTag, "addEvent - failed " << errno);
  }
#else
  EDGE_SDK_LOG_WARNING(kLogTag, "addEvent - no pipe");
#endif
}

NetworkCurl::RequestHandle* NetworkCurl::GetHandle(
    int id, Network::Callback callback, Network::HeaderCallback header_callback,
    Network::DataCallback data_callback,
    const std::shared_ptr<std::ostream>& payload) {
  if (!IsStarted()) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (int i = 0; i < kTotalHandleCount; i++) {
    if (!handles_[i].in_use) {
      if (!handles_[i].handle) {
        handles_[i].handle = curl_easy_init();
        if (!handles_[i].handle) {
          return nullptr;
        }
      }
      handles_[i].in_use = true;
      handles_[i].callback = callback;
      handles_[i].header_callback = header_callback;
      handles_[i].data_callback = data_callback;
      handles_[i].max_age = -1;
      handles_[i].expires = -1;
      handles_[i].id = id;
      handles_[i].count = 0;
      handles_[i].offset = 0;
      handles_[i].chunk = nullptr;
      handles_[i].range_out = false;
      handles_[i].cancelled = false;
      handles_[i].transfer_timeout = 30;
      handles_[i].retry_count = 0;
      handles_[i].etag.clear();
      handles_[i].content_type.clear();
      handles_[i].date.clear();
      handles_[i].payload = payload;
      handles_[i].send_time = std::chrono::steady_clock::now();
      handles_[i].error_text[0] = 0;
      handles_[i].get_statistics = false;
      handles_[i].skip_content = false;

      return &handles_[i];
    }
  }
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
}

size_t NetworkCurl::RxFunction(void* ptr, size_t size, size_t nmemb,
                               RequestHandle* handle) {
  const size_t len = size * nmemb;

  EDGE_SDK_LOG_TRACE(kLogTag, "Received " << len << " bytes");

  std::shared_ptr<NetworkCurl> that = handle->self.lock();
  if (!that) {
    return len;
  }
  long status = 0;
  curl_easy_getinfo(handle->handle, CURLINFO_RESPONSE_CODE, &status);
  if (handle->skip_content && status != 200 && status != 206 && status != 201 &&
      status != 0) {
    return len;
  }

  if (that->IsStarted() && !handle->range_out && !handle->cancelled) {
    if (handle->data_callback) {
      handle->data_callback(reinterpret_cast<uint8_t*>(ptr),
                            handle->offset + handle->count, len);
    }
    if (handle->payload) {
      if (!handle->ignore_offset) {
        if (handle->payload->tellp() != std::streampos(handle->count)) {
          handle->payload->seekp(handle->count);
          if (handle->payload->fail()) {
            EDGE_SDK_LOG_WARNING(
                kLogTag,
                "Reception stream doesn't support setting write point");
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
    if (http_status >= 400) {
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
  if (pos == std::string::npos || pos + 2 >= str.size()) {
    return len;
  }
  if (handle->header_callback) {
    std::string key = str.substr(0, pos);
    std::string value = str.substr(pos + 2);
    handle->header_callback(key, value);
  }

  if (NetworkUtils::CaseInsensitiveStartsWith(str, "Date:")) {
    handle->date = str.substr(6);
  } else if (NetworkUtils::CaseInsensitiveStartsWith(str, "Cache-Control:")) {
    std::size_t index = NetworkUtils::CaseInsensitiveFind(str, "max-age=", 8);
    if (index != std::string::npos) {
      handle->max_age = std::stoi(str.substr(index + 8));
    }
  } else if (NetworkUtils::CaseInsensitiveStartsWith(str, "Expires:")) {
    std::string date_string = str.substr(9);
    if (date_string == "0") {
      handle->expires = 0;
    } else if (date_string == "-1") {
      handle->expires = -1;
    } else {
      handle->expires = curl_getdate(date_string.c_str(), nullptr);
    }
  } else if (NetworkUtils::CaseInsensitiveStartsWith(str, "ETag:")) {
    handle->etag = str.substr(6);
  } else if (NetworkUtils::CaseInsensitiveStartsWith(str, "Content-Type:")) {
    handle->content_type = str.substr(14);
  } else if (NetworkUtils::CaseInsensitiveStartsWith(str, "Content-Range:")) {
    if (NetworkUtils::CaseInsensitiveStartsWith(str, "bytes ", 15)) {
      if ((str[21] == '*') && (str[22] == '/')) {
        // We have requested range over end of the file
        handle->range_out = true;
      } else if ((str[21] >= '0') && (str[21] <= '9')) {
        handle->offset = std::stoll(str.substr(21));
      } else {
        EDGE_SDK_LOG_WARNING(kLogTag, "Invalid Content-Range header: " << str);
      }
    } else {
      EDGE_SDK_LOG_WARNING(kLogTag, "Invalid Content-Range header: " << str);
    }
  }
  return len;
}

void NetworkCurl::CompleteMessage(CURL* handle, CURLcode result) {
  std::unique_lock<std::mutex> lock(event_mutex_);
  int index;
  for (index = 0; index < kTotalHandleCount; index++) {
    if (handles_[index].in_use && (handles_[index].handle == handle)) {
      break;
    }
  }

  std::vector<std::pair<std::string, std::string> > statistics;
  if (index < kTotalHandleCount) {
    if (handles_[index].get_statistics) {
      statistics =
          GetStatistics(handles_[index].handle, handles_[index].retry_count);
    }
    if (handles_[index].cancelled) {
      auto callback = handles_[index].callback;
      auto response =
          NetworkResponse()
              .WithRequestId(handles_[index].id)
              .WithStatus(static_cast<int>(ErrorCode::CANCELLED_ERROR))
              .WithError("Cancelled");
      ReleaseHandleUnlocked(&handles_[index]);

      lock.unlock();
      callback(response);
      return;
    }

    int max_age = handles_[index].max_age;
    time_t expires = handles_[index].expires;
    auto callback = handles_[index].callback;
    std::string etag = handles_[index].etag;
    std::string contentType = handles_[index].content_type;
    size_t count = handles_[index].count;
    size_t offset = handles_[index].offset;
    if (!callback) {
      EDGE_SDK_LOG_WARNING(kLogTag, "Complete to request without callback");
      ReleaseHandleUnlocked(&handles_[index]);
      return;
    }

    lock.unlock();
    std::string error("Success");
    int status;
    if ((result == CURLE_OK) || (result == CURLE_HTTP_RETURNED_ERROR)) {
      long http_status = 0;
      curl_easy_getinfo(handles_[index].handle, CURLINFO_RESPONSE_CODE,
                        &http_status);
      status = static_cast<int>(http_status);
      if ((handles_[index].offset == 0) && (status == 206)) status = 200;
      // for local file there is no server response so status is 0
      if ((status == 0) && (result == CURLE_OK)) status = 200;
      error = HttpErrorToString(status);
    } else {
      handles_[index].error_text[CURL_ERROR_SIZE - 1] = '\0';
      if (std::strlen(handles_[index].error_text) > 0) {
        error = handles_[index].error_text;
      } else {
        error = curl_easy_strerror(result);
      }
      status = ConvertErrorCode(result);
      // it happens sporadically that some requests fail with errors
      // "transfer closed with .... bytes remaining to read", after ~60
      // seconds This might be a server or lower network layer terminating
      // connection by timeout. Indicate such cases as timeouts, thus client
      // code would be able to retry immediately
      if (result == CURLE_PARTIAL_FILE) {
        double time = 0;
        CURLcode code = curl_easy_getinfo(handles_[index].handle,
                                          CURLINFO_TOTAL_TIME, &time);
        if (code == CURLE_OK &&
            time >= static_cast<double>(handles_[index].transfer_timeout)) {
          status =
              static_cast<int>(ErrorCode::TIMEOUT_ERROR);  // Network::TimedOut;
        }
      }
    }

    if ((status > 0) && ((status < 200) || (status >= 500))) {
      if (!handles_[index].cancelled &&
          (handles_[index].retry_count++ < handles_[index].max_retries)) {
        const char* url;
        curl_easy_getinfo(handles_[index].handle, CURLINFO_EFFECTIVE_URL, &url);
        EDGE_SDK_LOG_DEBUG(kLogTag, "Retry after: " << error << "; " << url);
        handles_[index].count = 0;
        lock.lock();
        events_.emplace_back(EventInfo::Type::SEND_EVENT, &handles_[index]);
        return;
      }
    }

    EDGE_SDK_LOG_TRACE(
        kLogTag, "Completed message " << handles_[index].id << " " << error);

    auto response = NetworkResponse()
                        .WithRequestId(handles_[index].id)
                        .WithStatus(status)
                        .WithError(error);
    ReleaseHandle(&handles_[index]);
    callback(response);
  } else {
    EDGE_SDK_LOG_WARNING(kLogTag, "Complete to unknown message");
  }
}

int NetworkCurl::GetHandleIndex(CURL* handle) {
  for (int index = 0; index < kTotalHandleCount; index++) {
    if (handles_[index].in_use && (handles_[index].handle == handle)) {
      return index;
    }
  }
  return -1;
}

void NetworkCurl::Run() {
  std::shared_ptr<NetworkCurl> keep_this_alive = shared_from_this();

  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STARTED;
    event_condition_.notify_one();
  }

  while (IsStarted()) {
    std::vector<CURL*> msgs;
    {
      std::lock_guard<std::mutex> lock(event_mutex_);
      while (!events_.empty() && IsStarted()) {
        EventInfo event = events_.front();
        events_.pop_front();
        switch (event.type) {
          case EventInfo::Type::SEND_EVENT: {
            if (event.handle->in_use) {
              CURLMcode res =
                  curl_multi_add_handle(curl_, event.handle->handle);
              if ((res != CURLM_OK) && (res != CURLM_CALL_MULTI_PERFORM)) {
                EDGE_SDK_LOG_ERROR(kLogTag, "Send failed with "
                                                << res << " "
                                                << curl_multi_strerror(res));
                msgs.push_back(event.handle->handle);
              }
            }
            break;
          }
          case EventInfo::Type::CANCEL_EVENT:
            if (event.handle->in_use) {
              curl_multi_remove_handle(curl_, event.handle->handle);
              event_mutex_.unlock();
              CompleteMessage(event.handle->handle, CURLE_OPERATION_TIMEDOUT);
              event_mutex_.lock();
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

    // Handle completed messages
    int left;
    bool completed = false;
    CURLMsg* msg(nullptr);
    {
      std::unique_lock<std::mutex> lock(event_mutex_);
      while (IsStarted() && (msg = curl_multi_info_read(curl_, &left))) {
        CURL* handle = msg->easy_handle;
        if (msg->msg == CURLMSG_DONE) {
          completed = true;
          CURLcode result = msg->data.result;
          curl_multi_remove_handle(curl_, msg->easy_handle);
          lock.unlock();
          CompleteMessage(handle, result);
          lock.lock();
        } else {
          EDGE_SDK_LOG_ERROR(
              kLogTag, "Message complete with unknown state " << msg->msg);
          int handle_index = GetHandleIndex(handle);
          if (handle_index >= 0) {
            if (!handles_[handle_index].callback) {
              EDGE_SDK_LOG_WARNING(
                  kLogTag,
                  "Complete to request with unknown state without "
                  "callback");
            } else {
              lock.unlock();
              auto response =
                  NetworkResponse()
                      .WithRequestId(handles_[handle_index].id)
                      .WithStatus(static_cast<int>(ErrorCode::IO_ERROR))
                      .WithError("CURL error");
              handles_[handle_index].callback(response);
              lock.lock();
              curl_multi_remove_handle(curl_, handles_[handle_index].handle);
            }
          } else {
            EDGE_SDK_LOG_ERROR(
                kLogTag,
                "No handle index of message complete with unknown state");
          }
        }
      }
    }

    if (!IsStarted() || completed) {
      continue;
    }
    // The QNX curl_multi_fdset implementation often returns -1 in max_fd
    // even when outstanding operations are in progress. According to the
    // docs: "When libcurl returns -1 in max_fd, it is because libcurl
    // currently does something that isn't possible for your application to
    // monitor with a socket and unfortunately you can then not know exactly
    // when the current action is completed using select(). You then need to
    // wait a while before you proceed and call curl_multi_perform anyway.
    // How long to wait? We suggest 100 milliseconds at least, but you may
    // want to test it out in your own particular conditions to find a
    // suitable value."
    constexpr long WAIT_MSEC = 100;

    int maxfd = 0;
    fd_set rfds;
    FD_ZERO(&rfds);
#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
    FD_SET(pipe_[0], &rfds);
#endif
    fd_set wfds;
    FD_ZERO(&wfds);
    fd_set excfds;
    FD_ZERO(&excfds);

    if (curl_multi_fdset(curl_, &rfds, &wfds, &excfds, &maxfd) != CURLM_OK) {
      continue;
    }
    bool missing_descriptors = maxfd == -1;

#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
    if (maxfd < pipe_[0]) {
      maxfd = pipe_[0];
    }
#endif

    long timeout;
    // Curl should be thread safe, but we sometimes get crash here
    if (maxfd != -1) {
      if (curl_multi_timeout(curl_, &timeout) != CURLM_OK) {
        continue;
      }
    } else {
      timeout = -1;
    }

    if (IsStarted() && ((timeout < 0) || missing_descriptors)) {
      // If curl_multi_timeout returns a -1 timeout, it just means that
      // libcurl currently has no stored timeout value. You must not wait
      // too long (more than a few seconds perhaps) before you call
      // curl_multi_perform() again.
      std::vector<CURL*> lostHandles;

      {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(event_mutex_);
        for (int i = 0; i < kTotalHandleCount; ++i) {
          if (handles_[i].in_use) {
            double total;
            curl_easy_getinfo(handles_[i].handle, CURLINFO_TOTAL_TIME, &total);
            // if this handle was added at least 30 seconds ago but curl
            // total time is still 0 then something wrong has happened
            if ((now - handles_[i].send_time > kHandleLostTimeout) &&
                (total == 0.0)) {
              lostHandles.push_back(handles_[i].handle);
            }
          }
        }
      }
      if (!lostHandles.empty() && IsStarted()) {
        // release all lost handles
        for (CURL* handle : lostHandles) {
          char const* url;
          curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);

          const auto remove_handle_status =
              curl_multi_remove_handle(curl_, handle);

          if (remove_handle_status == CURLM_OK) {
            EDGE_SDK_LOG_WARNING(kLogTag, "Releasing lost handle for " << url);
            CompleteMessage(handle, CURLE_OPERATION_TIMEDOUT);
          } else {
            EDGE_SDK_LOG_ERROR(
                kLogTag, "lost handle curl_multi_remove_handle error %s "
                             << std::to_string(remove_handle_status) << " for "
                             << url);

            int handle_index = GetHandleIndex(handle);
            if (handle_index >= 0) {
              if (!handles_[handle_index].callback) {
                EDGE_SDK_LOG_WARNING(kLogTag,
                                     "Complete to request without callback");
              } else {
                auto response =
                    NetworkResponse()
                        .WithRequestId(handles_[handle_index].id)
                        .WithStatus(static_cast<int>(ErrorCode::IO_ERROR))
                        .WithError("CURL error");
                handles_[handle_index].callback(response);
              }

              ReleaseHandle(&handles_[handle_index]);
            }
          }
        }
      }
      if (!IsStarted()) {
        continue;
      }
      bool in_use_handles = false;

      std::unique_lock<std::mutex> lock(event_mutex_);
      for (int i = 0; (i < kTotalHandleCount) && !in_use_handles; ++i) {
        if (handles_[i].in_use) {
          in_use_handles = true;
        }
      }

      // TODO: examine this section in details. Not clear what we are waiting
      // for.
      if (timeout < 0) {
        if (!in_use_handles) {
          // Enter wait only when all handles are free
          event_condition_.wait_for(lock, std::chrono::seconds(2));
        } else {
          event_condition_.wait_for(lock, std::chrono::milliseconds(WAIT_MSEC));
        }
      } else if (in_use_handles) {
        timeout = WAIT_MSEC;
      }
    }

    if (IsStarted() && (timeout > 0)) {
      // Limit wait time to 1s so that network can be stopped in reasonable
      // time
      if (timeout > 1000) {
        timeout = 1000;
      }
      timeval interval;
      interval.tv_sec = timeout / 1000;
      interval.tv_usec = (timeout % 1000) * 1000;
      ::select(maxfd + 1, &rfds, &wfds, &excfds, &interval);
#if (defined NETWORK_HAS_PIPE) || (defined NETWORK_HAS_PIPE2)
      if (FD_ISSET(pipe_[0], &rfds)) {
        char tmp;
        while (read(pipe_[0], &tmp, 1) > 0) {
        }
      }
#endif
    }

    auto now = std::chrono::steady_clock::now();
    long usable_handles = kStaticHandleCount;
    std::lock_guard<std::mutex> lock(event_mutex_);
    for (int i = kStaticHandleCount; i < kTotalHandleCount; ++i) {
      auto& handle = handles_[i];
      if (handle.handle && !handle.in_use &&
          handle.send_time + kHandleReuseTimeout < now) {
        curl_easy_cleanup(handle.handle);
        handle.handle = nullptr;
      }
      if (handle.handle) {
        ++usable_handles;
      }
    }
    // Make CURL close only those idle connections that we no longer plan to
    // reuse
    curl_multi_setopt(curl_, CURLMOPT_MAXCONNECTS, usable_handles);
  }  // end of the main loop
  Teardown();
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STOPPED;
    event_condition_.notify_one();
  }
  EDGE_SDK_LOG_TRACE(kLogTag, "Thread exit");
}

}  // namespace http
}  // namespace olp
