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

#include "NetworkCurl.h"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#include <utility>
#include "olp/core/utils/Thread.h"

#if defined(HAVE_SIGNAL_H)
#include <csignal>
#endif

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <sys/stat.h>
#include <cstdio>
#else
#include "olp/core/utils/Dir.h"
#endif

#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkInitializationSettings.h"
#include "olp/core/http/NetworkUtils.h"
#include "olp/core/logging/Log.h"
#include "olp/core/utils/Credentials.h"

namespace olp {
namespace http {

namespace {

const char* kLogTag = "CURL";
const char* kCurlThreadName = "OLPSDKCURL";

#if defined(OLP_SDK_ENABLE_ANDROID_CURL) && !defined(ANDROID_HOST)
const auto kCurlAndroidCaBundleFolder = "/system/etc/security/cacerts";

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
const char* kLookupMethodName = "DataSDKMd5Lookup";

int Md5LookupCtrl(X509_LOOKUP* ctx, int, const char*, long, char**) {
  const auto* cert_path = kCurlAndroidCaBundleFolder;
  X509_LOOKUP_set_method_data(ctx, const_cast<char*>(cert_path));
  return 1;
}

#if OPENSSL_VERSION_NUMBER >= 0x30100000L
int Md5LookupGetBySubject(X509_LOOKUP* ctx, X509_LOOKUP_TYPE type,
                          const X509_NAME* name, X509_OBJECT* ret) {
#else
int Md5LookupGetBySubject(X509_LOOKUP* ctx, X509_LOOKUP_TYPE type,
                          X509_NAME* name, X509_OBJECT* ret) {
#endif
  if (type != X509_LU_X509) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Unsupported lookup type, type=%d", type);
    return 0;
  }

  const char* base_path =
      static_cast<const char*>(X509_LOOKUP_get_method_data(ctx));
  const auto name_hash = X509_NAME_hash_old(name);

  char buf[256];
  for (auto idx = 0;; ++idx) {
    snprintf(buf, sizeof(buf), "%s/%08lx.%d", base_path, name_hash, idx);

    struct stat st {};
    if (stat(buf, &st) < 0) {
      // There is no such certificate
      break;
    }

    // `X509_load_cert_file` returns number of loaded objects
    const auto load_cert_ret = X509_load_cert_file(ctx, buf, X509_FILETYPE_PEM);
    if (load_cert_ret == 0) {
      OLP_SDK_LOG_ERROR_F(kLogTag, "Failed to load certificate file, buf=%s",
                          buf);
      return 0;
    }
  }

  // Update return result
  auto* x509_data = X509_new();
  X509_set_subject_name(x509_data, name);
  X509_OBJECT_set1_X509(ret, x509_data);

  return 1;
}
#endif

#else
const auto kCurlCaBundleName = "ca-bundle.crt";

std::string DefaultCaBundlePath() { return kCurlCaBundleName; }

std::string CaBundlePath() {
  std::string bundle_path = DefaultCaBundlePath();
  if (!utils::Dir::FileExists(bundle_path)) {
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

  err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

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
    case ProxyType::HTTPS:
      return CURLPROXY_HTTPS;
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
  }
  if (curl_code == CURLE_REMOTE_ACCESS_DENIED ||
      curl_code == CURLE_SSL_CERTPROBLEM || curl_code == CURLE_SSL_CIPHER ||
      curl_code == CURLE_LOGIN_DENIED) {
    return static_cast<int>(ErrorCode::AUTHORIZATION_ERROR);
  }
#if CURL_AT_LEAST_VERSION(7, 24, 0)
  if (curl_code == CURLE_FTP_ACCEPT_FAILED) {
    return static_cast<int>(ErrorCode::AUTHORIZATION_ERROR);
  }
#endif
  if (curl_code == CURLE_SSL_CACERT) {
    return static_cast<int>(ErrorCode::AUTHENTICATION_ERROR);
  }
  if (curl_code == CURLE_UNSUPPORTED_PROTOCOL ||
      curl_code == CURLE_URL_MALFORMAT ||
      curl_code == CURLE_COULDNT_RESOLVE_HOST) {
    return static_cast<int>(ErrorCode::INVALID_URL_ERROR);
  }
  if (curl_code == CURLE_OPERATION_TIMEDOUT) {
    return static_cast<int>(ErrorCode::TIMEOUT_ERROR);
  }
  return static_cast<int>(ErrorCode::IO_ERROR);
}

/**
 * @brief CURL get upload/download data.
 * @param[in] handle CURL easy handle.
 * @param[out] upload_bytes uploaded bytes(headers+data).
 * @param[out] download_bytes downloaded bytes(headers+data).
 */
void GetTrafficData(CURL* handle, uint64_t& upload_bytes,
                    uint64_t& download_bytes) {
  upload_bytes = 0;
  download_bytes = 0;

  long headers_size;
  if (curl_easy_getinfo(handle, CURLINFO_HEADER_SIZE, &headers_size) ==
          CURLE_OK &&
      headers_size > 0) {
    download_bytes += headers_size;
  }

#if CURL_AT_LEAST_VERSION(7, 55, 0)
  off_t length_downloaded = 0;
  if (curl_easy_getinfo(handle, CURLINFO_SIZE_DOWNLOAD_T, &length_downloaded) ==
          CURLE_OK &&
      length_downloaded > 0) {
    download_bytes += length_downloaded;
  }
#else
  double length_downloaded;
  if (curl_easy_getinfo(handle, CURLINFO_SIZE_DOWNLOAD, &length_downloaded) ==
          CURLE_OK &&
      length_downloaded > 0.0) {
    download_bytes += length_downloaded;
  }
#endif

  long length_upload;
  if (curl_easy_getinfo(handle, CURLINFO_REQUEST_SIZE, &length_upload) ==
          CURLE_OK &&
      length_upload > 0) {
    upload_bytes = length_upload;
  }
}

CURLcode SetCaBundlePaths(CURL* handle) {
  OLP_SDK_CORE_UNUSED(handle);

#if defined(OLP_SDK_ENABLE_ANDROID_CURL) && !defined(ANDROID_HOST)
  // FIXME: We could disable this lookup as it won't work on most devices
  //  (probably all of them) since OpenSSL still will be trying to find
  //  certificate with SHA1 lookup
  return curl_easy_setopt(handle, CURLOPT_CAPATH, kCurlAndroidCaBundleFolder);
#else
  const auto curl_ca_bundle = CaBundlePath();
  if (!curl_ca_bundle.empty()) {
    return curl_easy_setopt(handle, CURLOPT_CAINFO, curl_ca_bundle.c_str());
  }
#endif

  return CURLE_OK;
}

int64_t GetElapsedTime(std::chrono::steady_clock::time_point start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

std::string ConcatenateDnsAddresses(
    const std::vector<std::string>& dns_servers) {
  std::string result;

  const char* delimiter = "";
  for (const auto& dns_server : dns_servers) {
    result.append(delimiter).append(dns_server);
    delimiter = ",";
  }

  return result;
}

// Returns an integer value of the duration in specified representation.
// Casts to long, as long is a type expected by `curl_easy_setopt`
template <typename ToDuration, typename Duration>
long CountIn(Duration&& duration) {
  const auto count = std::chrono::duration_cast<ToDuration>(duration).count();
  return static_cast<long>(count);
}

std::shared_ptr<curl_slist> SetupHeaders(const Headers& headers) {
  curl_slist* list{nullptr};
  std::ostringstream ss;
  for (const auto& header : headers) {
    ss << header.first << ": " << header.second;
    list = curl_slist_append(list, ss.str().c_str());
    ss.str("");
    ss.clear();
  }
  return {list, curl_slist_free_all};
}

void SetupProxy(CURL* curl_handle, const NetworkProxySettings& proxy) {
  if (proxy.GetType() == NetworkProxySettings::Type::NONE) {
    return;
  }

  curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.GetHostname().c_str());
  curl_easy_setopt(curl_handle, CURLOPT_PROXYPORT, proxy.GetPort());

  const auto proxy_type = proxy.GetType();
  if (proxy_type != NetworkProxySettings::Type::HTTP) {
    curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE,
                     ToCurlProxyType(proxy_type));
  }

  // We expect that both fields are empty or filled
  const auto& username = proxy.GetUsername();
  const auto& password = proxy.GetPassword();
  if (!username.empty() && !password.empty()) {
    curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERNAME, username.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PROXYPASSWORD, password.c_str());
  }
}

void SetupRequestBody(CURL* curl_handle,
                      const NetworkRequest::RequestBodyType& body) {
  if (body && !body->empty()) {
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, body->size());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, &body->front());
  } else {
    // Some services (eg. Google) require the field size even if zero
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, 0L);
  }
}

void SetupDns(CURL* curl_handle, const std::vector<std::string>& dns_servers) {
#if CURL_AT_LEAST_VERSION(7, 24, 0)
  if (!dns_servers.empty()) {
    const std::string& dns_list = dns_servers.size() == 1
                                      ? dns_servers.front()
                                      : ConcatenateDnsAddresses(dns_servers);
    curl_easy_setopt(curl_handle, CURLOPT_DNS_SERVERS, dns_list.c_str());
  }
#endif
}

void WithDiagnostics(NetworkResponse& response, CURL* handle) {
#if CURL_AT_LEAST_VERSION(7, 61, 0)
  Diagnostics diagnostics;
  static const std::pair<Diagnostics::Timings, CURLINFO> available_timings[] = {
#if CURL_AT_LEAST_VERSION(8, 6, 0)
    {Diagnostics::Queue, CURLINFO_QUEUE_TIME_T},
#endif
    {Diagnostics::NameLookup, CURLINFO_NAMELOOKUP_TIME_T},
    {Diagnostics::Connect, CURLINFO_CONNECT_TIME_T},
    {Diagnostics::SSL_Handshake, CURLINFO_APPCONNECT_TIME_T},
#if CURL_AT_LEAST_VERSION(8, 10, 0)
    {Diagnostics::Send, CURLINFO_POSTTRANSFER_TIME_T},
    {Diagnostics::Wait, CURLINFO_STARTTRANSFER_TIME_T},
#else
    {Diagnostics::Send, CURLINFO_STARTTRANSFER_TIME_T},
#endif
    {Diagnostics::Receive, CURLINFO_TOTAL_TIME_T},
  };

  curl_off_t last_time_point{0};

  auto add_timing = [&](Diagnostics::Timings timing,
                        Diagnostics::MicroSeconds time) {
    diagnostics.timings[timing] = time;
    diagnostics.available_timings.set(timing);
  };

  for (const auto& available_timing : available_timings) {
    curl_off_t time_point_us = 0;
    if (curl_easy_getinfo(handle, available_timing.second, &time_point_us) ==
            CURLE_OK &&
        time_point_us > 0) {
      add_timing(available_timing.first,
                 Diagnostics::MicroSeconds(time_point_us - last_time_point));
      last_time_point = time_point_us;
    }
  }

  add_timing(Diagnostics::Total, Diagnostics::MicroSeconds(last_time_point));

  response.WithDiagnostics(diagnostics);
#else
  OLP_SDK_CORE_UNUSED(response, handle);
#endif
}

}  // anonymous namespace

NetworkCurl::NetworkCurl(NetworkInitializationSettings settings)
    : handles_(settings.max_requests_count),
      static_handle_count_(
          std::max(static_cast<size_t>(1u), settings.max_requests_count / 4u)),
      certificate_settings_(std::move(settings.certificate_settings)) {
  OLP_SDK_LOG_TRACE(kLogTag, "Created NetworkCurl with address="
                                 << this << ", handles_count="
                                 << settings.max_requests_count);
  auto error = curl_global_init(CURL_GLOBAL_ALL);
  curl_initialized_ = (error == CURLE_OK);
  if (!curl_initialized_) {
    OLP_SDK_LOG_ERROR_F(kLogTag, "Error initializing Curl. Error: %i",
                        static_cast<int>(error));
  }

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  SetupCertificateBlobs();
#else
  OLP_SDK_LOG_INFO_F(
      kLogTag,
      "CURL does not support SSL info with blobs, required 7.77.0, detected %s",
      LIBCURL_VERSION);
#endif

  const auto* version_data = curl_version_info(CURLVERSION_NOW);

  std::string curl_ca_info;
  std::string curl_ca_path;
#if CURL_AT_LEAST_VERSION(7, 70, 0)
  curl_ca_path = version_data->capath ? version_data->capath : "<empty>";
  curl_ca_info = version_data->cainfo ? version_data->cainfo : "<empty>";
#else
  curl_ca_path = "<empty>";
  curl_ca_info = "<empty>";
#endif

  std::string ca_bundle_path;
#if defined(OLP_SDK_ENABLE_ANDROID_CURL) && !defined(ANDROID_HOST)
  ca_bundle_path = kCurlAndroidCaBundleFolder;
#else
  ca_bundle_path = CaBundlePath();
  if (ca_bundle_path.empty()) {
    ca_bundle_path = "<empty>";
  }
#endif

  OLP_SDK_LOG_INFO_F(kLogTag,
                     "Certificate options, curl_ca_path=%s, curl_ca_info=%s, "
                     "ca_bundle_path=%s",
                     curl_ca_path.c_str(), curl_ca_info.c_str(),
                     ca_bundle_path.c_str());

  OLP_SDK_LOG_INFO_F(
      kLogTag, "TLS backend: %s",
      version_data->ssl_version ? version_data->ssl_version : "<empty>");
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
  for (size_t i = 0u; i < 2u; ++i) {
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

  // Multi handle uses the cache of connections to reuse hot ones. The size of
  // this cache is four times the number of added easy handles. Due to
  // dynamic nature of implementation, number of added easy handles changes a
  // lot, which results in the connection cache thrashing. Connection cache
  // thrashing may result in hard to debug errors, when system returns connected
  // socket with fd > 1024, which then cannot be used in the select due to
  // limitations of FS_SET structure. Use speculatively high number for
  // connection cache size, to accommodate for fluctuations in number of added
  // easy handles.
  const auto connects_cache_size = handles_.size() * 4;
  curl_multi_setopt(curl_, CURLMOPT_MAXCONNECTS, connects_cache_size);

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
  std::vector<std::pair<RequestId, Callback> > completed_messages;
  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    events_.clear();

    // handle teardown
    for (auto& handle : handles_) {
      if (handle.curl_handle) {
        if (handle.in_use) {
          curl_multi_remove_handle(curl_, handle.curl_handle.get());
          completed_messages.emplace_back(handle.id,
                                          handle.out_completion_callback);
        }
        handle.curl_handle = nullptr;
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
                              Callback callback, HeaderCallback header_callback,
                              DataCallback data_callback) {
  if (!Initialized()) {
    if (!Initialize()) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - network is uninitialized, url="
                                     << request.GetUrl());
      return SendOutcome(ErrorCode::OFFLINE_ERROR);
    }
  }

  RequestId request_id{};
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

  const auto error_status = SendImplementation(
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
    HeaderCallback header_callback, DataCallback data_callback,
    Callback callback) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Send failed - network is offline, url=" << request.GetUrl());
    return ErrorCode::IO_ERROR;
  }

  const auto& config = request.GetSettings();

  RequestHandle* handle = [&] {
    std::lock_guard<std::mutex> lock(event_mutex_);

    auto* request_handle = InitRequestHandleUnsafe();

    if (request_handle) {
      request_handle->id = id;
      request_handle->out_completion_callback = std::move(callback);
      request_handle->out_header_callback = std::move(header_callback);
      request_handle->out_data_callback = std::move(data_callback);
      request_handle->out_data_stream = payload;
      request_handle->request_body = request.GetBody();
      request_handle->request_headers = SetupHeaders(request.GetHeaders());
    }

    return request_handle;
  }();

  if (!handle) {
    return ErrorCode::NETWORK_OVERLOAD_ERROR;
  }

  OLP_SDK_LOG_DEBUG(kLogTag,
                    "Send request with url="
                        << utils::CensorCredentialsInUrl(request.GetUrl())
                        << ", id=" << id);

  CURL* curl_handle = handle->curl_handle.get();

  curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

  if (verbose_) {
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    if (stderr_ != nullptr) {
      curl_easy_setopt(curl_handle, CURLOPT_STDERR, stderr_);
    }
  } else {
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
  }

  curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

  const std::string& url = request.GetUrl();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

  auto verb = request.GetVerb();
  if (verb == NetworkRequest::HttpVerb::POST) {
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
  } else if (verb == NetworkRequest::HttpVerb::PUT) {
    // http://stackoverflow.com/questions/7569826/send-string-in-put-request-with-libcurl
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
  } else if (verb == NetworkRequest::HttpVerb::PATCH) {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
  } else if (verb == NetworkRequest::HttpVerb::DEL) {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
  } else if (verb == NetworkRequest::HttpVerb::OPTIONS) {
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "OPTIONS");
  } else {  // GET or HEAD
    curl_easy_setopt(curl_handle, CURLOPT_POST, 0L);

    if (verb == NetworkRequest::HttpVerb::HEAD) {
      curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
    }
  }

  if (verb != NetworkRequest::HttpVerb::GET &&
      verb != NetworkRequest::HttpVerb::HEAD) {
    // These can also be used to add body data to a CURLOPT_CUSTOMREQUEST
    // such as delete.
    SetupRequestBody(curl_handle, handle->request_body);
  }

  SetupProxy(curl_handle, config.GetProxySettings());
  SetupDns(curl_handle, config.GetDNSServers());

  if (handle->request_headers) {
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER,
                     handle->request_headers.get());
  }

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  if (ssl_certificates_blobs_) {
    curl_easy_setopt(curl_handle, CURLOPT_SSLCERT_BLOB,
                     ssl_certificates_blobs_->ssl_cert_blob.get_ptr());
    curl_easy_setopt(curl_handle, CURLOPT_SSLKEY_BLOB,
                     ssl_certificates_blobs_->ssl_key_blob.get_ptr());
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO_BLOB,
                     ssl_certificates_blobs_->ca_info_blob.get_ptr());
  } else
#endif
  {
    CURLcode error = SetCaBundlePaths(curl_handle);
    if (CURLE_OK != error) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - set ca bundle path failed, url="
                                     << request.GetUrl() << ", error=" << error
                                     << ", id=" << id);
      return ErrorCode::UNKNOWN_ERROR;
    }
  }

  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
  curl_easy_setopt(curl_handle, CURLOPT_SSL_CTX_FUNCTION,
                   &NetworkCurl::AddMd5LookupMethod);
  curl_easy_setopt(curl_handle, CURLOPT_SSL_CTX_DATA, handle);
#endif

  curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

  // `::count` is defined on all duration types, to be on the safe side
  // regarding durations on NetworkConfig. Refactoring of NetworkConfig to
  // return different types will be handled gracefully here. Force specific type
  // of the representation and type expected by curl.
  const long connect_timeout_ms =
      CountIn<std::chrono::milliseconds>(config.GetConnectionTimeoutDuration());
  const long timeout_ms =
      CountIn<std::chrono::milliseconds>(config.GetTransferTimeoutDuration());

  curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, timeout_ms);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION,
                   &NetworkCurl::RxFunction);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, handle);
  curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION,
                   &NetworkCurl::HeaderFunction);
  curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, handle);
  curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 0L);
  if (stderr_ == nullptr) {
    curl_easy_setopt(curl_handle, CURLOPT_STDERR, 0L);
  }
  curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, handle->error_text);

#if CURL_AT_LEAST_VERSION(7, 21, 0)
  curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl_handle, CURLOPT_TRANSFER_ENCODING, 1L);
#endif

#if CURL_AT_LEAST_VERSION(7, 25, 0)
  // Enable keep-alive (since Curl 7.25.0)
  curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPIDLE, 120L);
  curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPINTVL, 60L);
#endif

#if CURL_AT_LEAST_VERSION(7, 80, 0)
  curl_easy_setopt(curl_handle, CURLOPT_MAXLIFETIME_CONN,
                   config.GetMaxConnectionLifetime().count());
#else
  curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE,
                   config.GetMaxConnectionLifetime().count() ? 1L : 0L);
#endif

  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    AddEvent(EventInfo::Type::SEND_EVENT, handle);
  }
  return ErrorCode::SUCCESS;  // NetworkProtocol::ErrorNone;
}  // namespace http

void NetworkCurl::Cancel(RequestId id) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Cancel failed - network is offline, id=" << id);
    return;
  }
  std::lock_guard<std::mutex> lock(event_mutex_);
  for (auto& handle : handles_) {
    if (handle.in_use && (handle.id == id)) {
      handle.is_cancelled = true;
      AddEvent(EventInfo::Type::CANCEL_EVENT, &handle);

      OLP_SDK_LOG_DEBUG(kLogTag, "Cancel request with id=" << id);
      return;
    }
  }
  OLP_SDK_LOG_WARNING(kLogTag, "Cancel non-existing request with id=" << id);
}

void NetworkCurl::AddEvent(EventInfo::Type type, RequestHandle* handle) {
  events_.emplace_back(type, handle);
  event_condition_.notify_all();

#if (defined OLP_SDK_NETWORK_HAS_PIPE) || (defined OLP_SDK_NETWORK_HAS_PIPE2)
  // Notify also trough the pipe so that we can unlock curl_multi_wait() if
  // the network thread is currently blocked there.
  char tmp = 1;
  if (write(pipe_[1], &tmp, 1) < 0) {
    OLP_SDK_LOG_WARNING(kLogTag, "AddEvent - failed for id="
                                     << handle->id << ", err=" << errno);
  }
#else
  OLP_SDK_LOG_WARNING(kLogTag,
                      "AddEvent for id=" << handle->id << " - no pipe");
#endif
}

NetworkCurl::RequestHandle* NetworkCurl::InitRequestHandleUnsafe() {
  const auto unused_handle_it =
      std::find_if(handles_.begin(), handles_.end(),
                   [](const RequestHandle& request_handle) {
                     return request_handle.in_use == false;
                   });

  if (unused_handle_it == handles_.end()) {
    return nullptr;
  }

  if (!unused_handle_it->curl_handle) {
    unused_handle_it->curl_handle = {curl_easy_init(), curl_easy_cleanup};
  }

  if (!unused_handle_it->curl_handle) {
    return nullptr;
  }

  unused_handle_it->in_use = true;
  unused_handle_it->self = shared_from_this();
  unused_handle_it->send_time = std::chrono::steady_clock::now();
  unused_handle_it->log_context = logging::GetContext();

  return &*unused_handle_it;
}

void NetworkCurl::ReleaseHandleUnlocked(RequestHandle* handle,
                                        bool cleanup_easy_handle) {
  // Reset the RequestHandle to default, but keep the curl_handle.
  std::shared_ptr<CURL> curl_handle;
  std::swap(curl_handle, handle->curl_handle);
  curl_easy_reset(curl_handle.get());
  *handle = RequestHandle{};
  std::swap(curl_handle, handle->curl_handle);

  // When using C-Ares on Android, DNS parameters are calculated in
  // curl_easy_init(). Those parameters are not reset in curl_easy_reset(...),
  // and persist in subsequent usages of easy_handle. Problem arises, if
  // curl_easy_init() was called when no good network was available, eg when
  // "Flight mode" is On. In such case, bad DNS params are stuck in the handle
  // and will persist. Http requests will continue to fail after good networks
  // become available. When such error is encountered, perform cleanup of easy
  // handle to force curl_easy_init() on next easy handle use.

#if defined(ANDROID)
  if (cleanup_easy_handle) {
    handle->curl_handle = nullptr;
  }
#endif
  OLP_SDK_CORE_UNUSED(cleanup_easy_handle);
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

  if (that->IsStarted() && !handle->is_cancelled) {
    if (handle->out_data_callback) {
      handle->out_data_callback(static_cast<uint8_t*>(ptr),
                                handle->bytes_received, len);
    }

    const auto& stream = handle->out_data_stream;
    if (stream) {
      if (stream->tellp() !=
          static_cast<std::streamoff>(handle->bytes_received)) {
        stream->seekp(static_cast<std::streamoff>(handle->bytes_received));
        if (stream->fail()) {
          OLP_SDK_LOG_WARNING(kLogTag,
                              "Payload seekp() failed, id=" << handle->id);
          stream->clear();
        }
      }

      stream->write(static_cast<const char*>(ptr),
                    static_cast<std::streamsize>(len));
    }
    handle->bytes_received += len;
  }

  // In case we have curl verbose and stderr enabled to log the error content
  if (that->stderr_) {
    long http_status = 0L;
    curl_easy_getinfo(handle->curl_handle.get(), CURLINFO_RESPONSE_CODE,
                      &http_status);
    if (http_status >= http::HttpStatusCode::BAD_REQUEST) {
      // Log the error content to help troubleshooting
      fprintf(that->stderr_, "\n---ERRORCONTENT BEGIN HANDLE=%p BLOCKSIZE=%u\n",
              handle, static_cast<uint32_t>(size * nmemb));
      fwrite(ptr, size, nmemb, that->stderr_);
      fprintf(that->stderr_, "\n---ERRORCONTENT END HANDLE=%p BLOCKSIZE=%u\n",
              handle, static_cast<uint32_t>(size * nmemb));
    }
  }

  return len;
}

size_t NetworkCurl::HeaderFunction(char* ptr, size_t size, size_t nitems,
                                   RequestHandle* handle) {
  const size_t len = size * nitems;

  std::shared_ptr<NetworkCurl> that = handle->self.lock();
  if (!that || !that->IsStarted() || handle->is_cancelled) {
    return len;
  }

  if (!handle->out_header_callback) {
    return len;
  }

  // Drop trailing '\r' and '\n'
  size_t count = len;
  while ((count > 1u) &&
         ((ptr[count - 1u] == '\n') || (ptr[count - 1u] == '\r')))
    count--;
  if (count == 0u) {
    return len;
  }

  std::string str(ptr, count);
  std::size_t pos = str.find(':');
  if (pos == std::string::npos) {
    return len;
  }

  std::string key(str.substr(0u, pos));
  std::string value;
  if (pos + 2u < str.size()) {
    value = str.substr(pos + 2u);
  }

  // Callback with header key+value
  handle->out_header_callback(key, value);

  return len;
}

void NetworkCurl::CompleteMessage(CURL* curl_handle, CURLcode result) {
  std::unique_lock<std::mutex> lock(event_mutex_);

  // When curl returns an error of the handle, it is possible that error
  // originates from reuse of easy_handle, eg after network switch on the
  // Android.
  // To be on the safe side, do not reuse handle and caches attached to the
  // handle.
  const bool cleanup_easy_handle = result != CURLE_OK;

  RequestHandle* request_handle = FindRequestHandle(curl_handle);
  if (!request_handle) {
    OLP_SDK_LOG_WARNING(kLogTag, "Message completed to unknown request");
    return;
  }

  logging::ScopedLogContext scopedLogContext(request_handle->log_context);
  auto callback = std::move(request_handle->out_completion_callback);

  if (!callback) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "CompleteMessage - message without callback, id="
                            << request_handle->id);
    return ReleaseHandleUnlocked(request_handle, cleanup_easy_handle);
  }

  uint64_t upload_bytes = 0u;
  uint64_t download_bytes = 0u;
  GetTrafficData(curl_handle, upload_bytes, download_bytes);

  auto response = NetworkResponse()
                      .WithRequestId(request_handle->id)
                      .WithBytesDownloaded(download_bytes)
                      .WithBytesUploaded(upload_bytes);

  WithDiagnostics(response, curl_handle);

  if (request_handle->is_cancelled) {
    response.WithStatus(static_cast<int>(ErrorCode::CANCELLED_ERROR))
        .WithError("Cancelled");
    ReleaseHandleUnlocked(request_handle, cleanup_easy_handle);

    lock.unlock();
    callback(response);
    return;
  }

  std::string error("Success");
  int status;
  if ((result == CURLE_OK) || (result == CURLE_HTTP_RETURNED_ERROR)) {
    long http_status = 0L;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_status);
    status = static_cast<int>(http_status);

    if (status == HttpStatusCode::PARTIAL_CONTENT) {
      status = HttpStatusCode::OK;
    }

    // For local file, there is no server response so status is 0
    if ((status == 0) && (result == CURLE_OK)) {
      status = HttpStatusCode::OK;
    }

    error = HttpErrorToString(status);
  } else {
    request_handle->error_text[CURL_ERROR_SIZE - 1u] = '\0';
    if (std::strlen(request_handle->error_text) > 0u) {
      error = request_handle->error_text;
    } else {
      error = curl_easy_strerror(result);
    }

    status = ConvertErrorCode(result);
  }

  const char* url;
  curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);

  OLP_SDK_LOG_DEBUG(
      kLogTag, "Message completed, id="
                   << request_handle->id << ", url='"
                   << utils::CensorCredentialsInUrl(url) << "', status=("
                   << status << ") " << error
                   << ", time=" << GetElapsedTime(request_handle->send_time)
                   << "ms, bytes=" << download_bytes + upload_bytes);

  response.WithStatus(status).WithError(error);
  ReleaseHandleUnlocked(request_handle, cleanup_easy_handle);

  lock.unlock();
  callback(response);
}

NetworkCurl::RequestHandle* NetworkCurl::FindRequestHandle(const CURL* handle) {
  for (auto& request_handle : handles_) {
    if (request_handle.in_use && request_handle.curl_handle.get() == handle) {
      return &request_handle;
    }
  }
  return nullptr;
}

void NetworkCurl::Run() {
  olp::utils::Thread::SetCurrentThreadName(kCurlThreadName);

  {
    std::lock_guard<std::mutex> lock(event_mutex_);
    state_ = WorkerState::STARTED;
    event_condition_.notify_one();
  }

  while (IsStarted()) {
    //
    // First block handles user actions, i.e. adding or cancelling requests
    //
    std::vector<CURL*> msgs;
    {
      std::unique_lock<std::mutex> lock(event_mutex_);
      while (!events_.empty() && IsStarted()) {
        auto event = events_.front();
        events_.pop_front();

        // Only handle handles that are actually used
        auto* request_handle = event.handle;
        if (!request_handle->in_use) {
          continue;
        }

        CURL* curl_handle = request_handle->curl_handle.get();

        if (event.type == EventInfo::Type::SEND_EVENT) {
          auto res = curl_multi_add_handle(curl_, curl_handle);
          if (res != CURLM_OK && res != CURLM_CALL_MULTI_PERFORM) {
            OLP_SDK_LOG_ERROR(
                kLogTag, "Send failed, id=" << request_handle->id << ", error="
                                            << curl_multi_strerror(res));

            // Do not add the handle to msgs vector in case it is a duplicate
            // handle error as it will be reset in CompleteMessage handler,
            // and curl will crash in the next call of curl_multi_perform
            // function. In any other case, lets complete the message.
            if (res != CURLM_ADDED_ALREADY) {
              msgs.push_back(curl_handle);
            }
          }
        } else if (event.type == EventInfo::Type::CANCEL_EVENT &&
                   request_handle->is_cancelled) {
          // The Request was canceled, remove it from curl
          auto code = curl_multi_remove_handle(curl_, curl_handle);
          if (code != CURLM_OK) {
            OLP_SDK_LOG_ERROR(kLogTag, "curl_multi_remove_handle failed, error="
                                           << curl_multi_strerror(code));
          }

          lock.unlock();
          CompleteMessage(curl_handle, CURLE_OPERATION_TIMEDOUT);
          lock.lock();
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

    //
    // Run cURL queue, i.e. upload/download
    //
    int running = 0;
    {
      do {
      } while (IsStarted() &&
               curl_multi_perform(curl_, &running) == CURLM_CALL_MULTI_PERFORM);
    }

    //
    // Handle completed messages
    //
    {
      int msgs_in_queue = 0;
      CURLMsg* msg(nullptr);

      std::unique_lock<std::mutex> lock(event_mutex_);

      while (IsStarted() &&
             ((msg = curl_multi_info_read(curl_, &msgs_in_queue)))) {
        CURL* curl_handle = msg->easy_handle;
        uint64_t upload_bytes = 0u;
        uint64_t download_bytes = 0u;
        GetTrafficData(curl_handle, upload_bytes, download_bytes);

        if (msg->msg == CURLMSG_DONE) {
          curl_multi_remove_handle(curl_, curl_handle);
          lock.unlock();
          CompleteMessage(curl_handle, msg->data.result);
          lock.lock();
        } else {
          // Actually this part should never be executed
          OLP_SDK_LOG_ERROR(
              kLogTag,
              "Request completed with unknown state, error=" << msg->msg);

          auto* request_handle = FindRequestHandle(curl_handle);
          if (!request_handle) {
            OLP_SDK_LOG_ERROR(kLogTag, "Unknown handle completed");
            continue;
          }

          logging::ScopedLogContext scopedLogContext(
              request_handle->log_context);

          auto callback = std::move(request_handle->out_completion_callback);

          if (!callback) {
            OLP_SDK_LOG_WARNING(kLogTag,
                                "Request completed without callback, id="
                                    << request_handle->id);
          } else {
            lock.unlock();
            auto response =
                NetworkResponse()
                    .WithRequestId(request_handle->id)
                    .WithStatus(static_cast<int>(ErrorCode::IO_ERROR))
                    .WithError("CURL error")
                    .WithBytesDownloaded(download_bytes)
                    .WithBytesUploaded(upload_bytes);

            WithDiagnostics(response, curl_handle);

            callback(response);
            lock.lock();
          }

          curl_multi_remove_handle(curl_, curl_handle);
          const bool cleanup_easy_handle = true;
          ReleaseHandleUnlocked(request_handle, cleanup_easy_handle);
        }
      }
    }

    if (!IsStarted()) {
      continue;
    }

    //
    // Wait for next action or upload/download
    //
    {
      // NOTE: curl_multi_wait has a fatal flow in it and it was corrected by
      // curl_multi_poll in libcurl 7.66.0.
      // If no extra file descriptors are provided and libcurl has no file
      // descriptor to offer to wait for, curl_multi_wait will return directly
      // without waiting the provided timeout. This needs to be handled by the
      // user accordingly with another wait.

      // In case we have pipes we can wait longer as it will return once any of
      // the

      int numfds = 0;
#if defined(OLP_SDK_NETWORK_HAS_PIPE) || defined(OLP_SDK_NETWORK_HAS_PIPE2)
      curl_waitfd waitfd[1];
      waitfd[0].fd = pipe_[0];
      waitfd[0].events = CURL_WAIT_POLLIN;
      waitfd[0].revents = 0;
      auto mc = curl_multi_wait(curl_, waitfd, 1, 1000, &numfds);
      if (mc == CURLM_OK && numfds != 0 && waitfd[0].revents != 0) {
        // Empty pipe data to make sure we are clear for the next wait
        char tmp;
        while (read(waitfd[0].fd, &tmp, 1) > 0) {
        }
      }
#else
      // Without pipe limit wait time to 100ms so that network events can be
      // handled in a reasonable time
      auto mc = curl_multi_wait(curl_, nullptr, 0, 100, &numfds);
#endif
      if (mc != CURLM_OK) {
        OLP_SDK_LOG_INFO(kLogTag, " Run - curl_multi_wait failed, error="
                                      << curl_multi_strerror(mc));
        continue;
      }

      // 'numfds' being zero means either a timeout or no file descriptors to
      // wait for
      if (numfds == 0) {
        std::unique_lock<std::mutex> lock(event_mutex_);

        bool in_use_handles = std::any_of(
            handles_.begin(), handles_.end(),
            [](const RequestHandle& handle) { return handle.in_use; });

        if (!IsStarted()) {
          continue;
        }

        if (!in_use_handles) {
          // Enter wait only when all handles are free as this will overcome the
          // curl_multi_wait issue on skipping timeout when no FDs are present.
          event_condition_.wait_for(lock, std::chrono::seconds(2));
        }

        // If handles are in use do not wait aditionally as this will increase
        // download latency to at least the wait time for each download. If
        // there are pending request then curl_multi_wait with 1000/100ms is
        // more then enough sleep and we should handle incoming/outgoing data as
        // soon as curl_multi_wait tells us to do so.
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

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
void NetworkCurl::SetupCertificateBlobs() {
  if (certificate_settings_.client_cert_file_blob.empty() &&
      certificate_settings_.client_key_file_blob.empty() &&
      certificate_settings_.cert_file_blob.empty()) {
    OLP_SDK_LOG_INFO(kLogTag, "No certificate blobs provided");
    return;
  }

  auto setup_blob = [](SslCertificateBlobs::OptionalBlob& blob,
                       std::string& src) {
    if (src.empty()) {
      blob.reset();
      return;
    }
    blob = SslCertificateBlobs::OptionalBlob::value_type{};
    blob->data = const_cast<char*>(src.data());
    blob->len = src.size();
    blob->flags = CURL_BLOB_NOCOPY;
  };

  ssl_certificates_blobs_ = SslCertificateBlobs{};

  setup_blob(ssl_certificates_blobs_->ssl_cert_blob,
             certificate_settings_.client_cert_file_blob);
  setup_blob(ssl_certificates_blobs_->ssl_key_blob,
             certificate_settings_.client_key_file_blob);
  setup_blob(ssl_certificates_blobs_->ca_info_blob,
             certificate_settings_.cert_file_blob);

  auto to_log_str = [](const SslCertificateBlobs::OptionalBlob& blob) {
    return blob ? "<provided>" : "<empty>";
  };

  OLP_SDK_LOG_INFO(kLogTag,
                   "Certificate blobs provided, client_cert_blob="
                       << to_log_str(ssl_certificates_blobs_->ssl_cert_blob)
                       << ", client_key_blob="
                       << to_log_str(ssl_certificates_blobs_->ssl_key_blob)
                       << ", ca_info_blob="
                       << to_log_str(ssl_certificates_blobs_->ca_info_blob));
}
#endif

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
CURLcode NetworkCurl::AddMd5LookupMethod(CURL*, SSL_CTX* ssl_ctx,
                                         RequestHandle* handle) {
  auto self = handle->self.lock();
  if (!self) {
    OLP_SDK_LOG_ERROR(kLogTag, "Unable to lock cURL handle");
    return CURLE_ABORTED_BY_CALLBACK;
  }

  auto* md5_lookup_method = X509_LOOKUP_meth_new(kLookupMethodName);
  if (!md5_lookup_method) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to allocate MD5 lookup method");
    return CURLE_ABORTED_BY_CALLBACK;
  }

  X509_LOOKUP_meth_set_ctrl(md5_lookup_method, Md5LookupCtrl);
  X509_LOOKUP_meth_set_get_by_subject(md5_lookup_method, Md5LookupGetBySubject);

  auto* cert_store = SSL_CTX_get_cert_store(ssl_ctx);
  auto* lookup = X509_STORE_add_lookup(cert_store, md5_lookup_method);
  if (lookup) {
    X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_PEM);
  } else {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to add MD5 lookup method");
    return CURLE_ABORTED_BY_CALLBACK;
  }

  return CURLE_OK;
}
#endif

}  // namespace http
}  // namespace olp
