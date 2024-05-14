/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#ifdef OLP_SDK_USE_MD5_CERT_LOOKUP
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <sys/stat.h>
#include <cstdio>
#elif OLP_SDK_NETWORK_HAS_OPENSSL
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

#ifdef OLP_SDK_ENABLE_ANDROID_CURL
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

#elif OLP_SDK_NETWORK_HAS_OPENSSL
const auto kCurlCaBundleName = "ca-bundle.crt";

std::string DefaultCaBundlePath() { return kCurlCaBundleName; }

std::string AlternativeCaBundlePath() { return kCurlCaBundleName; }

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
#if CURL_AT_LEAST_VERSION(7, 24, 0)
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

#ifdef OLP_SDK_ENABLE_ANDROID_CURL
  // FIXME: We could disable this lookup as it won't work on most devices
  //  (probably all of them) since OpenSSL still will be trying to find
  //  certificate with SHA1 lookup
  return curl_easy_setopt(handle, CURLOPT_CAPATH, kCurlAndroidCaBundleFolder);
#elif OLP_SDK_NETWORK_HAS_OPENSSL
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

  std::string curl_ca_info;
  std::string curl_ca_path;
#if CURL_AT_LEAST_VERSION(7, 70, 0)
  const auto* version_data = curl_version_info(CURLVERSION_NOW);
  curl_ca_path = version_data->capath ? version_data->capath : "<empty>";
  curl_ca_info = version_data->cainfo ? version_data->cainfo : "<empty>";
#else
  curl_ca_path = "<empty>";
  curl_ca_info = "<empty>";
#endif

  std::string ca_bundle_path;
#ifdef OLP_SDK_ENABLE_ANDROID_CURL
  ca_bundle_path = kCurlAndroidCaBundleFolder;
#elif OLP_SDK_NETWORK_HAS_OPENSSL
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
    Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, Network::Callback callback) {
  if (!IsStarted()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Send failed - network is offline, url=" << request.GetUrl());
    return ErrorCode::IO_ERROR;
  }

  const auto& config = request.GetSettings();

  RequestHandle* handle =
      GetHandle(id, std::move(callback), std::move(header_callback),
                std::move(data_callback), payload, request.GetBody());
  if (!handle) {
    return ErrorCode::NETWORK_OVERLOAD_ERROR;
  }

  OLP_SDK_LOG_DEBUG(kLogTag,
                    "Send request with url="
                        << utils::CensorCredentialsInUrl(request.GetUrl())
                        << ", id=" << id);

  handle->ignore_offset = false;  // request.IgnoreOffset();
  handle->skip_content = false;   // config->SkipContentWhenError();

  for (const auto& header : request.GetHeaders()) {
    std::ostringstream sstrm;
    sstrm << header.first;
    sstrm << ": ";
    sstrm << header.second;
    handle->chunk = curl_slist_append(handle->chunk, sstrm.str().c_str());
  }

  CURL* curl_handle = handle->handle;

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
  if (verb == NetworkRequest::HttpVerb::POST ||
      verb == NetworkRequest::HttpVerb::PUT ||
      verb == NetworkRequest::HttpVerb::PATCH) {
    if (verb == NetworkRequest::HttpVerb::POST) {
      curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    } else if (verb == NetworkRequest::HttpVerb::PUT) {
      // http://stackoverflow.com/questions/7569826/send-string-in-put-request-with-libcurl
      curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (verb == NetworkRequest::HttpVerb::PATCH) {
      curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    }
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
    if (handle->body && !handle->body->empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE,
                       handle->body->size());
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, &handle->body->front());
    } else {
      // Some services (eg. Google) require the field size even if zero
      curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, 0L);
    }
  }

  const auto& proxy = config.GetProxySettings();
  if (proxy.GetType() != NetworkProxySettings::Type::NONE) {
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.GetHostname().c_str());
    curl_easy_setopt(curl_handle, CURLOPT_PROXYPORT, proxy.GetPort());
    const auto proxy_type = proxy.GetType();
    if (proxy_type != NetworkProxySettings::Type::HTTP) {
      curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE,
                       ToCurlProxyType(proxy_type));
    }

    // We expect that both fields are empty or filled
    if (!proxy.GetUsername().empty() && !proxy.GetPassword().empty()) {
      curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERNAME,
                       proxy.GetUsername().c_str());
      curl_easy_setopt(curl_handle, CURLOPT_PROXYPASSWORD,
                       proxy.GetPassword().c_str());
    }
  }

#if CURL_AT_LEAST_VERSION(7, 24, 0)
  const auto& dns_servers = config.GetDNSServers();
  if (!dns_servers.empty()) {
    const std::string& dns_list = dns_servers.size() == 1
                                      ? dns_servers.front()
                                      : ConcatenateDnsAddresses(dns_servers);
    curl_easy_setopt(curl_handle, CURLOPT_DNS_SERVERS, dns_list.c_str());
  }
#endif

  if (handle->chunk) {
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, handle->chunk);
  }

#ifdef OLP_SDK_CURL_HAS_SUPPORT_SSL_BLOBS
  if (ssl_certificates_blobs_) {
    curl_easy_setopt(curl_handle, CURLOPT_SSLCERT_BLOB,
                     &ssl_certificates_blobs_->ssl_cert_blob);
    curl_easy_setopt(curl_handle, CURLOPT_SSLKEY_BLOB,
                     &ssl_certificates_blobs_->ssl_key_blob);
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO_BLOB,
                     &ssl_certificates_blobs_->ca_info_blob);
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
  // of the representation & type expected by curl.
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
      handle.cancelled = true;
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
        curl_easy_setopt(handle.handle, CURLOPT_NOSIGNAL, 1L);
      }
      handle.in_use = true;
      handle.callback = std::move(callback);
      handle.header_callback = std::move(header_callback);
      handle.data_callback = std::move(data_callback);
      handle.id = id;
      handle.count = 0u;
      handle.offset = 0u;
      handle.chunk = nullptr;
      handle.cancelled = false;
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

void NetworkCurl::ReleaseHandle(RequestHandle* handle,
                                bool cleanup_easy_handle) {
  std::lock_guard<std::mutex> lock(event_mutex_);
  ReleaseHandleUnlocked(handle, cleanup_easy_handle);
}

void NetworkCurl::ReleaseHandleUnlocked(RequestHandle* handle,
                                        bool cleanup_easy_handle) {
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
    curl_easy_cleanup(handle->handle);
    handle->handle = nullptr;
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
  long status = 0L;
  curl_easy_getinfo(handle->handle, CURLINFO_RESPONSE_CODE, &status);
  if (handle->skip_content && status != http::HttpStatusCode::OK &&
      status != http::HttpStatusCode::PARTIAL_CONTENT &&
      status != http::HttpStatusCode::CREATED && status != 0L) {
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
    long http_status = 0L;
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

  if (!handle->header_callback) {
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
  handle->header_callback(key, value);

  return len;
}

void NetworkCurl::CompleteMessage(CURL* handle, CURLcode result) {
  std::unique_lock<std::mutex> lock(event_mutex_);

  // When curl returns an error of the handle, it is possible that error
  // originates from reuse of easy_handle, eg after network switch on the
  // Android.
  // To be on the safe side, do not reuse handle and caches attached to the
  // handle.
  const bool cleanup_easy_handle = result != CURLE_OK;

  int index = GetHandleIndex(handle);
  if (index >= 0 && index < static_cast<int>(handles_.size())) {
    RequestHandle& rhandle = handles_[index];

    auto callback = rhandle.callback;
    if (!callback) {
      OLP_SDK_LOG_WARNING(
          kLogTag,
          "CompleteMessage - message without callback, id=" << rhandle.id);
      ReleaseHandleUnlocked(&rhandle, cleanup_easy_handle);
      return;
    }

    uint64_t upload_bytes = 0u;
    uint64_t download_bytes = 0u;
    GetTrafficData(rhandle.handle, upload_bytes, download_bytes);

    auto response = NetworkResponse()
                        .WithRequestId(rhandle.id)
                        .WithBytesDownloaded(download_bytes)
                        .WithBytesUploaded(upload_bytes);

    if (rhandle.cancelled) {
      response.WithStatus(static_cast<int>(ErrorCode::CANCELLED_ERROR))
          .WithError("Cancelled");
      ReleaseHandleUnlocked(&rhandle, cleanup_easy_handle);

      lock.unlock();
      callback(response);
      return;
    }

    std::string error("Success");
    int status;
    if ((result == CURLE_OK) || (result == CURLE_HTTP_RETURNED_ERROR)) {
      long http_status = 0L;
      curl_easy_getinfo(rhandle.handle, CURLINFO_RESPONSE_CODE, &http_status);
      status = static_cast<int>(http_status);

      if ((rhandle.offset == 0u) &&
          (status == HttpStatusCode::PARTIAL_CONTENT)) {
        status = HttpStatusCode::OK;
      }

      // For local file there is no server response so status is 0
      if ((status == 0) && (result == CURLE_OK)) {
        status = HttpStatusCode::OK;
      }

      error = HttpErrorToString(status);
    } else {
      rhandle.error_text[CURL_ERROR_SIZE - 1u] = '\0';
      if (std::strlen(rhandle.error_text) > 0u) {
        error = rhandle.error_text;
      } else {
        error = curl_easy_strerror(result);
      }

      status = ConvertErrorCode(result);
    }

    const char* url;
    curl_easy_getinfo(rhandle.handle, CURLINFO_EFFECTIVE_URL, &url);

    OLP_SDK_LOG_DEBUG(kLogTag,
                      "Message completed, id="
                          << rhandle.id << ", url='"
                          << utils::CensorCredentialsInUrl(url) << "', status=("
                          << status << ") " << error
                          << ", time=" << GetElapsedTime(rhandle.send_time)
                          << "ms, bytes=" << download_bytes + upload_bytes);

    response.WithStatus(status).WithError(error);
    ReleaseHandleUnlocked(&rhandle, cleanup_easy_handle);

    lock.unlock();
    callback(response);
  } else {
    OLP_SDK_LOG_WARNING(kLogTag, "Message completed to unknown request");
  }
}

int NetworkCurl::GetHandleIndex(CURL* handle) {
  for (size_t index = 0u; index < handles_.size(); index++) {
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
        auto* rhandle = event.handle;
        if (!rhandle->in_use) {
          continue;
        }

        if (event.type == EventInfo::Type::SEND_EVENT) {
          auto res = curl_multi_add_handle(curl_, rhandle->handle);
          if ((res != CURLM_OK) && (res != CURLM_CALL_MULTI_PERFORM)) {
            OLP_SDK_LOG_ERROR(kLogTag,
                              "Send failed, id=" << rhandle->id << ", error="
                                                 << curl_multi_strerror(res));

            // Do not add the handle to msgs vector in case it is a duplicate
            // handle error as it will be reset in CompleteMessage handler,
            // and curl will crash in the next call of curl_multi_perform
            // function. In any other case, lets complete the message.
            if (res != CURLM_ADDED_ALREADY) {
              msgs.push_back(rhandle->handle);
            }
          }
        } else {
          // Request was cancelled, so lets remove it from curl
          auto code = curl_multi_remove_handle(curl_, rhandle->handle);
          if (code != CURLM_OK) {
            OLP_SDK_LOG_ERROR(kLogTag, "curl_multi_remove_handle failed, error="
                                           << curl_multi_strerror(code));
          }

          lock.unlock();
          CompleteMessage(rhandle->handle, CURLE_OPERATION_TIMEDOUT);
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
             (msg = curl_multi_info_read(curl_, &msgs_in_queue))) {
        CURL* handle = msg->easy_handle;
        uint64_t upload_bytes = 0u;
        uint64_t download_bytes = 0u;
        GetTrafficData(handle, upload_bytes, download_bytes);

        if (msg->msg == CURLMSG_DONE) {
          curl_multi_remove_handle(curl_, handle);
          lock.unlock();
          CompleteMessage(handle, msg->data.result);
          lock.lock();
        } else {
          // Actually this part should never be executed
          OLP_SDK_LOG_ERROR(
              kLogTag,
              "Request completed with unknown state, error=" << msg->msg);

          int handle_index = GetHandleIndex(handle);
          if (handle_index >= 0) {
            RequestHandle& rhandle = handles_[handle_index];
            auto callback = rhandle.callback;
            if (!callback) {
              OLP_SDK_LOG_WARNING(
                  kLogTag,
                  "Request completed without callback, id=" << rhandle.id);
            } else {
              lock.unlock();
              auto response =
                  NetworkResponse()
                      .WithRequestId(rhandle.id)
                      .WithStatus(static_cast<int>(ErrorCode::IO_ERROR))
                      .WithError("CURL error")
                      .WithBytesDownloaded(download_bytes)
                      .WithBytesUploaded(upload_bytes);
              callback(response);
              lock.lock();
            }

            curl_multi_remove_handle(curl_, rhandle.handle);
            const bool cleanup_easy_handle = true;
            ReleaseHandleUnlocked(&rhandle, cleanup_easy_handle);
          } else {
            OLP_SDK_LOG_ERROR(kLogTag, "Unknown handle completed");
          }
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
  if (certificate_settings_.client_cert_file_blob.empty() ||
      certificate_settings_.client_key_file_blob.empty() ||
      certificate_settings_.cert_file_blob.empty()) {
    OLP_SDK_LOG_INFO(kLogTag, "No certificate blobs provided");
    return;
  }

  auto setup_blob = [](struct curl_blob& blob, std::string& src) {
    blob.data = const_cast<char*>(src.data());
    blob.len = src.size();
    blob.flags = CURL_BLOB_NOCOPY;
  };

  ssl_certificates_blobs_ = SslCertificateBlobs{};

  setup_blob(ssl_certificates_blobs_->ssl_cert_blob,
             certificate_settings_.client_cert_file_blob);
  setup_blob(ssl_certificates_blobs_->ssl_key_blob,
             certificate_settings_.client_key_file_blob);
  setup_blob(ssl_certificates_blobs_->ca_info_blob,
             certificate_settings_.cert_file_blob);

  OLP_SDK_LOG_INFO(kLogTag, "Certificate blobs provided");
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
