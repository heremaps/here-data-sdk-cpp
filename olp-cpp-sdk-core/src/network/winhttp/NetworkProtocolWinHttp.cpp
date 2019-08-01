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

#include "NetworkProtocolWinHttp.h"
#include <olp/core/logging/Log.h>
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/network/NetworkResponse.h>
#include <olp/core/porting/make_unique.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define NETWORK_UNCOMPRESSION_CHUNK_SIZE (1024 * 16)

namespace {
constexpr int MaxRequestCount = 32;

LPCSTR
errorToString(DWORD err) {
  switch (err) {
    case ERROR_NOT_ENOUGH_MEMORY:
      return "Out of memory";
    case ERROR_WINHTTP_CANNOT_CONNECT:
      return "Cannot connect";
    case ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW:
      return "Parsing overflow";
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
      return "Authentication required";
    case ERROR_WINHTTP_CONNECTION_ERROR:
      return "Connection error";
    case ERROR_WINHTTP_HEADER_COUNT_EXCEEDED:
      return "Header count exceeded";
    case ERROR_WINHTTP_HEADER_SIZE_OVERFLOW:
      return "Header size overflow";
    case ERROR_WINHTTP_INCORRECT_HANDLE_STATE:
      return "Invalid handle state";
    case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
      return "Invalid handle type";
    case ERROR_WINHTTP_INTERNAL_ERROR:
      return "Internal error";
    case ERROR_WINHTTP_INVALID_SERVER_RESPONSE:
      return "Invalid server response";
    case ERROR_WINHTTP_INVALID_URL:
      return "Invalid URL";
    case ERROR_WINHTTP_LOGIN_FAILURE:
      return "Login failed";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
      return "Name not resolved";
    case ERROR_WINHTTP_OPERATION_CANCELLED:
      return "Cancelled";
    case ERROR_WINHTTP_REDIRECT_FAILED:
      return "Redirect failed";
    case ERROR_WINHTTP_RESEND_REQUEST:
      return "Resend request";
    case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW:
      return "Response overflow";
    case ERROR_WINHTTP_SECURE_FAILURE:
      return "Security error";
    case ERROR_WINHTTP_TIMEOUT:
      return "Timed out";
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
      return "Invalid scheme";
  }
  return "Unknown error";
}

int errorToCode(DWORD err) {
  if (err == ERROR_SUCCESS)
    return 0;
  else if ((err == ERROR_WINHTTP_INVALID_URL) ||
           (err == ERROR_WINHTTP_UNRECOGNIZED_SCHEME) ||
           (err == ERROR_WINHTTP_NAME_NOT_RESOLVED))
    return olp::network::Network::ErrorCode::InvalidURLError;
  else if ((err == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED) ||
           (err == ERROR_WINHTTP_LOGIN_FAILURE) ||
           (err == ERROR_WINHTTP_SECURE_FAILURE))
    return olp::network::Network::ErrorCode::AuthorizationError;
  else if (err == ERROR_WINHTTP_OPERATION_CANCELLED)
    return olp::network::Network::ErrorCode::Cancelled;
  else if (err == ERROR_WINHTTP_TIMEOUT)
    return olp::network::Network::ErrorCode::TimedOut;
  return olp::network::Network::ErrorCode::IOError;
}

LPWSTR
queryHeadervalue(HINTERNET request, DWORD header) {
  DWORD len = 0, index = WINHTTP_NO_HEADER_INDEX;
  if (WinHttpQueryHeaders(request, header, WINHTTP_HEADER_NAME_BY_INDEX,
                          WINHTTP_NO_OUTPUT_BUFFER, &len, &index))
    return NULL;
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return NULL;
  }
  LPWSTR buffer = (LPWSTR)LocalAlloc(LPTR, len);
  if (!buffer) return NULL;
  index = WINHTTP_NO_HEADER_INDEX;
  if (!WinHttpQueryHeaders(request, header, WINHTTP_HEADER_NAME_BY_INDEX,
                           buffer, &len, &index)) {
    LocalFree(buffer);
    return NULL;
  }
  return buffer;
}

void UnixTimeToFileTime(std::uint64_t t, LPFILETIME pft) {
  // Note that LONGLONG is a 64-bit value
  LONGLONG ll;

  ll = Int32x32To64(t, 10000000) + 116444736000000000;
  pft->dwLowDateTime = (DWORD)ll;
  pft->dwHighDateTime = ll >> 32;
}

bool convertMultiByteToWideChar(const std::string& s_in, std::wstring& s_out) {
  s_out.clear();
  if (s_in.empty()) {
    return true;
  }

  // Detect required buffer size.
  const auto chars_required = MultiByteToWideChar(
      CP_ACP, MB_PRECOMPOSED, s_in.c_str(),
      -1,       // denotes null-terminated string
      nullptr,  // output buffer is null means request required buffer size
      0);

  if (chars_required == 0) {
    // error: could not convert input string
    return false;
  }

  s_out.resize(chars_required);
  // Perform actual conversion from multi-byte to wide char
  const auto conversion_result =
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, s_in.c_str(),
                          -1,  // denotes null-terminated string
                          &s_out.front(), s_out.size());

  if (conversion_result == 0) {
    // Should not happen as 1st call have succeeded.
    return false;
  }
  return true;
}
}  // namespace

namespace olp {
namespace network {

#define LOGTAG "WinHttp"

NetworkProtocolWinHttp::NetworkProtocolWinHttp()
    : m_session(NULL),
      m_thread(INVALID_HANDLE_VALUE),
      m_event(INVALID_HANDLE_VALUE),
      m_tracker(false) {}

NetworkProtocolWinHttp::~NetworkProtocolWinHttp() { Deinitialize(); }

bool NetworkProtocolWinHttp::Initialize() {
  m_session = WinHttpOpen(L"NGMOS CLient", WINHTTP_ACCESS_TYPE_NO_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                          WINHTTP_FLAG_ASYNC);

  if (!m_session) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "WinHttpOpen failed " << GetLastError());
    return false;
  }

  WinHttpSetStatusCallback(
      m_session,
      (WINHTTP_STATUS_CALLBACK)&NetworkProtocolWinHttp::requestCallback,
      WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES, 0);

  m_event = CreateEvent(NULL, TRUE, FALSE, NULL);

  // Store the memory tracking state during initialization so that it can be
  // used by the thread.
  m_tracker.Capture();
  m_thread = CreateThread(NULL, 0, NetworkProtocolWinHttp::run, this, 0, NULL);
  SetThreadPriority(m_thread, THREAD_PRIORITY_ABOVE_NORMAL);

  return true;
}

void NetworkProtocolWinHttp::Deinitialize() {
  std::vector<std::shared_ptr<ResultData>> pendingResults;
  {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<RequestData>> requestsToCancel;
    for (auto& request : m_requests) {
      requestsToCancel.push_back(request.second);
    }
    while (!requestsToCancel.empty()) {
      std::shared_ptr<RequestData> request = requestsToCancel.front();
      WinHttpCloseHandle(request->m_request);
      request->m_request = NULL;
      pendingResults.push_back(request->m_result);
      requestsToCancel.erase(requestsToCancel.begin());
    }
  }

  if (m_session) {
    WinHttpCloseHandle(m_session);
    m_session = NULL;
  }

  if (m_event != INVALID_HANDLE_VALUE) SetEvent(m_event);
  if (m_thread != INVALID_HANDLE_VALUE) {
    if (GetCurrentThreadId() != GetThreadId(m_thread)) {
      WaitForSingleObject(m_thread, INFINITE);
    }
  }
  CloseHandle(m_event);
  CloseHandle(m_thread);
  m_thread = m_event = INVALID_HANDLE_VALUE;

  {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_connections.clear();
    while (!m_results.empty()) {
      pendingResults.push_back(m_results.front());
      m_results.pop();
    }
  }

  for (auto& result : pendingResults) {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if (result->m_callback) {
      result->m_callback(
          NetworkResponse(result->m_id, Network::Offline, "Offline"));
      result->m_callback = nullptr;
    }
  }
}

bool NetworkProtocolWinHttp::Initialized() const { return (m_session != NULL); }

bool NetworkProtocolWinHttp::Ready() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  return m_requests.size() < MaxRequestCount;
}

size_t NetworkProtocolWinHttp::AmountPending() {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  return m_requests.size();
}

olp::network::NetworkProtocol::ErrorCode NetworkProtocolWinHttp::Send(
    const olp::network::NetworkRequest& request, int id,
    const std::shared_ptr<std::ostream>& payload,
    std::shared_ptr<NetworkConfig> config,
    Network::HeaderCallback headerCallback, Network::DataCallback dataCallback,
    Network::Callback callback) {
  if (!config->GetNetworkInterface().empty()) {
    return olp::network::NetworkProtocol::
        ErrorNetworkInterfaceOptionNotImplemented;
  }

  if (!config->GetCaCert().empty()) {
    return olp::network::NetworkProtocol::ErrorCaCertOptionNotImplemented;
  }

  URL_COMPONENTS urlComponents;
  ZeroMemory(&urlComponents, sizeof(urlComponents));
  urlComponents.dwStructSize = sizeof(urlComponents);
  urlComponents.dwSchemeLength = (DWORD)-1;
  urlComponents.dwHostNameLength = (DWORD)-1;
  urlComponents.dwUrlPathLength = (DWORD)-1;
  urlComponents.dwExtraInfoLength = (DWORD)-1;

  std::wstring url(request.Url().begin(), request.Url().end());
  if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComponents)) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "WinHttpCrackUrl failed " << GetLastError());
    return olp::network::NetworkProtocol::ErrorInvalidRequest;
  }

  std::shared_ptr<RequestData> handle;
  {
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    std::wstring server(url.data(), size_t(urlComponents.lpszUrlPath -
                                           urlComponents.lpszScheme));
    std::shared_ptr<ConnectionData> connection = m_connections[server];
    if (!connection) {
      connection = std::make_shared<ConnectionData>(shared_from_this());
      INTERNET_PORT port = urlComponents.nPort;
      if (port == 0) {
        port = (urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
                   ? INTERNET_DEFAULT_HTTPS_PORT
                   : INTERNET_DEFAULT_HTTP_PORT;
      }
      std::wstring hostName(urlComponents.lpszHostName,
                            urlComponents.dwHostNameLength);
      connection->m_connect =
          WinHttpConnect(m_session, hostName.data(), port, 0);
      if (!connection->m_connect) return NetworkProtocol::ErrorNoConnection;

      m_connections[server] = connection;
    }

    connection->m_lastUsed = GetTickCount64();

    handle =
        std::make_shared<RequestData>(id, connection, callback, headerCallback,
                                      dataCallback, payload, request);
    m_requests[id] = handle;
  }

  handle->m_ignoreOffset = request.IgnoreOffset();
  handle->m_getStatistics = request.GetStatistics();

  if ((urlComponents.nScheme != INTERNET_SCHEME_HTTP) &&
      (urlComponents.nScheme != INTERNET_SCHEME_HTTPS)) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Invalid scheme on request " << request.Url());

    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    m_requests.erase(id);
    return NetworkProtocol::ErrorIO;
  }

  DWORD flags = (urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
                    ? WINHTTP_FLAG_SECURE
                    : 0;
  LPWSTR httpVerb = L"GET";
  if (request.Verb() == NetworkRequest::HttpVerb::POST) {
    httpVerb = L"POST";
  } else if (request.Verb() == NetworkRequest::HttpVerb::PUT) {
    httpVerb = L"PUT";
  } else if (request.Verb() == NetworkRequest::HttpVerb::HEAD) {
    httpVerb = L"HEAD";
  } else if (request.Verb() == NetworkRequest::HttpVerb::DEL) {
    httpVerb = L"DELETE";
  } else if (request.Verb() == NetworkRequest::HttpVerb::PATCH) {
    httpVerb = L"PATCH";
  }

  LPCSTR content = WINHTTP_NO_REQUEST_DATA;
  DWORD contentLength = 0;

  if (request.Verb() != NetworkRequest::HttpVerb::HEAD &&
      request.Verb() != NetworkRequest::HttpVerb::GET &&
      request.Content() != nullptr && !request.Content()->empty()) {
    content = (LPCSTR) & (request.Content()->front());
    contentLength = (DWORD)request.Content()->size();
  }

  /* Create a request */
  auto httpRequest = WinHttpOpenRequest(
      handle->m_connection->m_connect, httpVerb, urlComponents.lpszUrlPath,
      NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!httpRequest) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "WinHttpOpenRequest failed " << GetLastError());

    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    m_requests.erase(id);
    return NetworkProtocol::ErrorIO;
  }

  WinHttpSetTimeouts(httpRequest, config->ConnectTimeout() * 1000,
                     config->ConnectTimeout() * 1000,
                     config->TransferTimeout() * 1000,
                     config->TransferTimeout() * 1000);

  bool sysDontVerifyCertificate;
  const auto sysConfigProxy = olp::network::Network::SystemConfig().locked(
      [&](const olp::network::NetworkSystemConfig& conf) {
        sysDontVerifyCertificate = conf.DontVerifyCertificate();
        return conf.GetProxy();
      });

  if (sysDontVerifyCertificate) {
    flags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
            SECURITY_FLAG_IGNORE_UNKNOWN_CA;
    if (!WinHttpSetOption(httpRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags,
                          sizeof(flags))) {
      EDGE_SDK_LOG_ERROR(
          LOGTAG, "WinHttpSetOption(Security) failed " << GetLastError());
    }
  }

  const NetworkProxy* proxy = &sysConfigProxy;
  if (config->Proxy().IsValid()) {
    proxy = &(config->Proxy());
  }

  if (proxy->IsValid()) {
    std::wostringstream proxy_string_stream;

    switch (proxy->ProxyType()) {
      case olp::network::NetworkProxy::Type::Http:
        proxy_string_stream << "http://";
        break;
      case olp::network::NetworkProxy::Type::Socks4:
        proxy_string_stream << "socks4://";
        break;
      case olp::network::NetworkProxy::Type::Socks5:
        proxy_string_stream << "socks5://";
        break;
      case olp::network::NetworkProxy::Type::Socks4A:
        proxy_string_stream << "socks4a://";
        break;
      case olp::network::NetworkProxy::Type::Socks5Hostname:
        proxy_string_stream << "socks5h://";
        break;
      default:
        proxy_string_stream << "http://";
    }

    proxy_string_stream << std::wstring(proxy->Name().begin(),
                                        proxy->Name().end());
    proxy_string_stream << ':';
    proxy_string_stream << proxy->Port();
    std::wstring proxy_string = proxy_string_stream.str();

    WINHTTP_PROXY_INFO proxy_info;
    proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy_info.lpszProxy = const_cast<LPWSTR>(proxy_string.c_str());
    proxy_info.lpszProxyBypass = WINHTTP_NO_PROXY_BYPASS;

    DWORD error_code = ERROR_SUCCESS;
    if (!WinHttpSetOption(httpRequest, WINHTTP_OPTION_PROXY, &proxy_info,
                          sizeof(proxy_info))) {
      error_code = GetLastError();
      EDGE_SDK_LOG_ERROR(LOGTAG,
                         "WinHttpSetOption(Proxy) failed " << error_code);
    }
    if (!proxy->UserName().empty() && !proxy->UserPassword().empty()) {
      std::wstring proxy_username;
      const auto username_res =
          convertMultiByteToWideChar(proxy->UserName(), proxy_username);

      std::wstring proxy_password;
      const auto password_res =
          convertMultiByteToWideChar(proxy->UserPassword(), proxy_password);

      if (username_res && password_res) {
        LPCWSTR proxy_lpcwstr_username = proxy_username.c_str();
        if (!WinHttpSetOption(httpRequest, WINHTTP_OPTION_PROXY_USERNAME,
                              const_cast<LPWSTR>(proxy_lpcwstr_username),
                              wcslen(proxy_lpcwstr_username))) {
          error_code = GetLastError();
          EDGE_SDK_LOG_ERROR(
              LOGTAG, "WinHttpSetOption(proxy username) failed " << error_code);
        }

        LPCWSTR proxy_lpcwstr_password = proxy_password.c_str();
        if (!WinHttpSetOption(httpRequest, WINHTTP_OPTION_PROXY_PASSWORD,
                              const_cast<LPWSTR>(proxy_lpcwstr_password),
                              wcslen(proxy_lpcwstr_password))) {
          error_code = GetLastError();
          EDGE_SDK_LOG_ERROR(
              LOGTAG, "WinHttpSetOption(proxy password) failed " << error_code);
        }
      } else {
        if (!username_res) {
          error_code = GetLastError();
          EDGE_SDK_LOG_ERROR(
              LOGTAG, "Proxy username conversion failure " << error_code);
        }
        if (!password_res) {
          error_code = GetLastError();
          EDGE_SDK_LOG_ERROR(
              LOGTAG, "Proxy password conversion failure " << error_code);
        }
      }
    }
  }

  flags = WINHTTP_DECOMPRESSION_FLAG_ALL;
  if (!WinHttpSetOption(httpRequest, WINHTTP_OPTION_DECOMPRESSION, &flags,
                        sizeof(flags)))
    handle->m_noCompression = true;

  const std::vector<std::pair<std::string, std::string>>& extraHeaders =
      request.ExtraHeaders();

  std::wostringstream headerStrm;
  _locale_t loc = _create_locale(LC_CTYPE, "C");
  bool foundContentLength = false;
  for (size_t i = 0; i < extraHeaders.size(); i++) {
    std::string headerName = extraHeaders[i].first.c_str();
    std::transform(headerName.begin(), headerName.end(), headerName.begin(),
                   ::tolower);

    if (headerName.compare("content-length") == 0) {
      foundContentLength = true;
    }

    headerStrm << headerName.c_str();
    headerStrm << L": ";
    headerStrm << extraHeaders[i].second.c_str();
    headerStrm << L"\r\n";
  }

  // Set the content-length header if it does not already exist
  if (!foundContentLength) {
    headerStrm << L"content-length: " << contentLength << L"\r\n";
  }

  _free_locale(loc);

  if (!WinHttpAddRequestHeaders(httpRequest, headerStrm.str().c_str(),
                                DWORD(-1), WINHTTP_ADDREQ_FLAG_ADD)) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "WinHttpAddRequestHeaders() failed " << GetLastError());
  }

  if (request.ModifiedSince()) {
    FILETIME ft;
    SYSTEMTIME st;
    UnixTimeToFileTime(request.ModifiedSince(), &ft);
    FileTimeToSystemTime(&ft, &st);
    WCHAR pwszTimeStr[WINHTTP_TIME_FORMAT_BUFSIZE / sizeof(WCHAR)];
    if (WinHttpTimeFromSystemTime(&st, pwszTimeStr)) {
      std::wostringstream headerStrm;
      headerStrm << L"If-Modified-Since: ";
      headerStrm << pwszTimeStr;
      if (!WinHttpAddRequestHeaders(httpRequest, headerStrm.str().c_str(),
                                    DWORD(-1), WINHTTP_ADDREQ_FLAG_ADD)) {
        EDGE_SDK_LOG_ERROR(LOGTAG,
                           "WinHttpAddRequestHeaders(if-modified-since) failed "
                               << GetLastError());
      }
    }
  }

  /* Send the request */
  if (!WinHttpSendRequest(httpRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                          (LPVOID)content, contentLength, contentLength,
                          (DWORD_PTR)handle.get())) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "WinHttpSendRequest failed " << GetLastError());

    std::unique_lock<std::recursive_mutex> lock(m_mutex);

    m_requests.erase(id);
    return NetworkProtocol::ErrorIO;
  }
  handle->m_request = httpRequest;

  return NetworkProtocol::ErrorNone;
}

bool NetworkProtocolWinHttp::Cancel(int id) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto it = m_requests.find(id);
  if (it == m_requests.end()) return false;

  // Just closing the handle cancels the request
  if (it->second->m_request) {
    WinHttpCloseHandle(it->second->m_request);
    it->second->m_request = NULL;
  }

  return true;
}

void NetworkProtocolWinHttp::requestCallback(HINTERNET, DWORD_PTR context,
                                             DWORD status, LPVOID statusInfo,
                                             DWORD statusInfoLength) {
  if (context == NULL) {
    return;
  }

  RequestData* handle = reinterpret_cast<RequestData*>(context);
  if (!handle->m_connection || !handle->m_result) {
    EDGE_SDK_LOG_WARNING(LOGTAG, "RequestCallback to inactive handle");
    return;
  }

  // to extend RequestData lifetime till the end of function
  std::shared_ptr<RequestData> that;

  {
    std::unique_lock<std::recursive_mutex> lock(
        handle->m_connection->m_self->m_mutex);

    that = handle->m_connection->m_self->m_requests[handle->m_id];
  }

  // Restore the memory tracking state used to initially create the request.
  MEMORY_TRACKER_SCOPE(handle->m_tracker);

  handle->m_connection->m_lastUsed = GetTickCount64();

  if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
    // Error has occurred
    assert(statusInfoLength == sizeof(WINHTTP_ASYNC_RESULT));
    WINHTTP_ASYNC_RESULT* result =
        reinterpret_cast<WINHTTP_ASYNC_RESULT*>(statusInfo);
    handle->m_result->m_status = result->dwError;

    if (result->dwError == ERROR_WINHTTP_OPERATION_CANCELLED) {
      handle->m_result->m_cancelled = true;
    }

    handle->complete();
  } else if (status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
    // We have sent request, now receive a response
    WinHttpReceiveResponse(handle->m_request, NULL);
  } else if (status == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE) {
    Network::HeaderCallback callback = nullptr;
    {
      std::unique_lock<std::recursive_mutex> lock(
          handle->m_connection->m_self->m_mutex);
      if (handle->m_headerCallback) callback = handle->m_headerCallback;
    }

    if (callback && handle->m_request) {
      DWORD wideLen;
      WinHttpQueryHeaders(handle->m_request, WINHTTP_QUERY_RAW_HEADERS,
                          WINHTTP_HEADER_NAME_BY_INDEX,
                          WINHTTP_NO_OUTPUT_BUFFER, &wideLen,
                          WINHTTP_NO_HEADER_INDEX);
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        DWORD len = wideLen / sizeof(WCHAR);
        auto wideBuffer = std::make_unique<WCHAR[]>(len);
        if (WinHttpQueryHeaders(handle->m_request, WINHTTP_QUERY_RAW_HEADERS,
                                WINHTTP_HEADER_NAME_BY_INDEX, wideBuffer.get(),
                                &wideLen, WINHTTP_NO_HEADER_INDEX)) {
          // Text should be converted back from the wide char to properly handle
          // UTF-8.
          auto buffer = std::make_unique<char[]>(len);
          int convertResult = WideCharToMultiByte(
              CP_ACP, 0, wideBuffer.get(), len, buffer.get(), len, 0, nullptr);
          assert(convertResult == len);

          DWORD start = 0, index = 0;
          while (index < len) {
            if (buffer[index] == 0) {
              if (start != index) {
                std::string entry(&buffer[start], index - start);
                size_t pos = entry.find(':');
                if (pos != std::string::npos) {
                  std::string key(entry.begin(), entry.begin() + pos);
                  std::string value(entry.begin() + pos + 2, entry.end());
                  callback(key, value);
                }
              }
              index++;
              start = index;
            } else
              index++;
          }
        }
      }
    }

    {
      std::unique_lock<std::recursive_mutex> lock(
          handle->m_connection->m_self->m_mutex);
      if (handle->m_request) {
        LPWSTR code =
            queryHeadervalue(handle->m_request, WINHTTP_QUERY_STATUS_CODE);
        if (code) {
          std::wstring codeStr(code);
          handle->m_result->m_status = std::stoi(codeStr);
          LocalFree(code);
        } else {
          handle->m_result->m_status = -1;
        }

        LPWSTR cache =
            queryHeadervalue(handle->m_request, WINHTTP_QUERY_CACHE_CONTROL);
        if (cache) {
          std::wstring cacheStr(cache);
          std::size_t index = cacheStr.find(L"max-age=");
          if (index != std::wstring::npos)
            handle->m_result->m_maxAge = std::stoi(cacheStr.substr(index + 8));
          LocalFree(cache);
        } else {
          handle->m_result->m_maxAge = -1;
        }

        // TODO: expires

        LPWSTR etag = queryHeadervalue(handle->m_request, WINHTTP_QUERY_ETAG);
        if (etag) {
          std::wstring etagStr(etag);
          handle->m_result->m_etag.assign(etagStr.begin(), etagStr.end());
          LocalFree(etag);
        } else {
          handle->m_result->m_etag.clear();
        }

        LPWSTR date = queryHeadervalue(handle->m_request, WINHTTP_QUERY_DATE);
        if (date) {
          handle->m_date = date;
          LocalFree(date);
        } else {
          handle->m_date.clear();
        }

        LPWSTR range =
            queryHeadervalue(handle->m_request, WINHTTP_QUERY_CONTENT_RANGE);
        if (range) {
          const std::wstring rangeStr(range);
          const std::size_t index = rangeStr.find(L"bytes ");
          if (index != std::wstring::npos) {
            std::size_t offset = 6;
            if (rangeStr[6] == L'*' && rangeStr[7] == L'/') {
              offset = 8;
            }
            if (handle->m_resumed) {
              handle->m_result->m_count =
                  std::stoull(rangeStr.substr(index + offset)) -
                  handle->m_result->m_offset;
            } else {
              handle->m_result->m_offset =
                  std::stoull(rangeStr.substr(index + offset));
            }
          }
          LocalFree(range);
        } else {
          handle->m_result->m_count = 0;
        }

        LPWSTR type =
            queryHeadervalue(handle->m_request, WINHTTP_QUERY_CONTENT_TYPE);
        if (type) {
          std::wstring typeStr(type);
          handle->m_result->m_contentType.assign(typeStr.begin(),
                                                 typeStr.end());
          LocalFree(type);
        } else {
          handle->m_result->m_contentType.clear();
        }

        LPWSTR length =
            queryHeadervalue(handle->m_request, WINHTTP_QUERY_CONTENT_LENGTH);
        if (length) {
          const std::wstring lengthStr(length);
          handle->m_result->m_size = std::stoull(lengthStr);
          LocalFree(length);
        } else {
          handle->m_result->m_size = 0;
        }

        if (handle->m_noCompression) {
          LPWSTR str = queryHeadervalue(handle->m_request,
                                        WINHTTP_QUERY_CONTENT_ENCODING);
          if (str) {
            std::wstring encoding(str);
            if (encoding == L"gzip") {
#ifdef NETWORK_HAS_ZLIB
              handle->m_uncompress = true;
              /* allocate inflate state */
              handle->m_strm.zalloc = Z_NULL;
              handle->m_strm.zfree = Z_NULL;
              handle->m_strm.opaque = Z_NULL;
              handle->m_strm.avail_in = 0;
              handle->m_strm.next_in = Z_NULL;
              inflateInit2(&handle->m_strm, 16 + MAX_WBITS);
#else
              EDGE_SDK_LOG_ERROR(
                  LOGTAG,
                  "Gzip encoding but compression no supported and no "
                  "ZLIB found");
#endif
            }
            LocalFree(str);
          }
        }
      } else {
        handle->complete();
        return;
      }
    }

    // We have received headers, now receive data
    WinHttpQueryDataAvailable(handle->m_request, NULL);
  } else if (status == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
    assert(statusInfoLength == sizeof(DWORD));
    DWORD size = *(DWORD*)statusInfo;
    if (size > 0 && 416 != handle->m_result->m_status) {
      // Data is available read it
      LPVOID buffer = (LPVOID)LocalAlloc(LPTR, size);
      if (!buffer) {
        EDGE_SDK_LOG_ERROR(LOGTAG,
                           "Out of memory reeceiving " << size << " bytes");
        handle->m_result->m_status = ERROR_NOT_ENOUGH_MEMORY;
        handle->complete();
        return;
      }
      WinHttpReadData(handle->m_request, buffer, size, NULL);
    } else {
      // Request is complete
      if (handle->m_result->m_status != 416) {
        // Skip size check if manually decompressing, since it's known to not
        // match.
        if (!handle->m_ignoreData && !handle->m_uncompress &&
            handle->m_result->m_size != 0 &&
            handle->m_result->m_size != handle->m_result->m_count) {
          handle->m_result->m_status = -1;
        }
      }
      handle->m_result->m_completed = true;
      handle->complete();
    }
  } else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE) {
    // Read is complete, check if there is more
    if (statusInfo && statusInfoLength) {
      const char* dataBuffer = (const char*)statusInfo;
      DWORD dataLen = statusInfoLength;
#ifdef NETWORK_HAS_ZLIB
      if (handle->m_uncompress) {
        Bytef* compressed = (Bytef*)statusInfo;
        DWORD compressedLen = dataLen;
        dataBuffer =
            (const char*)LocalAlloc(LPTR, NETWORK_UNCOMPRESSION_CHUNK_SIZE);
        handle->m_strm.avail_in = compressedLen;
        handle->m_strm.next_in = compressed;
        dataLen = 0;
        DWORD allocSize = NETWORK_UNCOMPRESSION_CHUNK_SIZE;

        while (handle->m_strm.avail_in > 0) {
          handle->m_strm.next_out = (Bytef*)dataBuffer + dataLen;
          DWORD availableSize = allocSize - dataLen;
          handle->m_strm.avail_out = availableSize;
          int r = inflate(&handle->m_strm, Z_NO_FLUSH);

          if ((r != Z_OK) && (r != Z_STREAM_END)) {
            EDGE_SDK_LOG_ERROR(LOGTAG, "Uncompression failed");
            LocalFree((HLOCAL)compressed);
            LocalFree((HLOCAL)dataBuffer);
            handle->m_result->m_status = ERROR_INVALID_BLOCK;
            handle->complete();
            return;
          }

          dataLen += availableSize - handle->m_strm.avail_out;
          if (r == Z_STREAM_END) break;

          if (dataLen == allocSize) {
            // We ran out of space in uncompression buffer, expand the buffer
            // and loop again
            allocSize += NETWORK_UNCOMPRESSION_CHUNK_SIZE;
            char* newBuffer = (char*)LocalAlloc(LPTR, allocSize);
            memcpy(newBuffer, dataBuffer, dataLen);
            LocalFree((HLOCAL)dataBuffer);
            dataBuffer = (const char*)newBuffer;
          }
        }
        LocalFree((HLOCAL)compressed);
      }
#endif

      if (dataLen) {
        std::uint64_t totalOffset = 0;

        if (handle->m_dataCallback)
          handle->m_dataCallback(totalOffset, (const uint8_t*)dataBuffer,
                                 dataLen);

        {
          std::unique_lock<std::recursive_mutex> lock(
              handle->m_connection->m_self->m_mutex);
          if (handle->m_payload) {
            if (!handle->m_ignoreOffset) {
              if (handle->m_payload->tellp() !=
                  std::streampos(handle->m_result->m_count)) {
                handle->m_payload->seekp(handle->m_result->m_count);
                if (handle->m_payload->fail()) {
                  EDGE_SDK_LOG_WARNING(
                      LOGTAG,
                      "Reception stream doesn't support setting write point");
                  handle->m_payload->clear();
                }
              }
            }

            handle->m_payload->write(dataBuffer, dataLen);
          }
          handle->m_result->m_count += dataLen;
        }
      }
      LocalFree((HLOCAL)dataBuffer);
    }

    WinHttpQueryDataAvailable(handle->m_request, NULL);
  } else if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING) {
    // Only now is it safe to free the handle
    // See
    // https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpclosehandle
    handle->freeHandle();
    return;
  } else {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "Unknown callback " << std::hex << status << std::dec);
  }
}

DWORD
NetworkProtocolWinHttp::run(LPVOID arg) {
  reinterpret_cast<NetworkProtocolWinHttp*>(arg)->completionThread();
  return 0;
}

void NetworkProtocolWinHttp::completionThread() {
  std::shared_ptr<NetworkProtocolWinHttp> that = shared_from_this();

  // Use the allocator specified during initialization for this thread.
  MEMORY_TRACKER_SCOPE(that->m_tracker);

  while (that->m_session) {
    std::shared_ptr<ResultData> result;
    {
      if (m_session && m_results.empty()) {
        WaitForSingleObject(m_event, 30000);  // Wait max 30 seconds
        ResetEvent(m_event);
      }
      if (!m_session) continue;

      std::unique_lock<std::recursive_mutex> lock(m_mutex);
      if (!m_results.empty()) {
        result = m_results.front();
        m_results.pop();
      }
    }

    if (m_session && result) {
      std::string str;
      int status;
      if ((result->m_offset == 0) && (result->m_status == 206))
        result->m_status = 200;

      if (result->m_completed)
        str = NetworkProtocol::HttpErrorToString(result->m_status);
      else
        str = errorToString(result->m_status);
      if (result->m_completed)
        status = result->m_status;
      else
        status = errorToCode(result->m_status);
      NetworkResponse response(
          result->m_id, result->m_cancelled, status, str, result->m_maxAge,
          result->m_expires, result->m_etag, result->m_contentType,
          result->m_count, result->m_offset, result->m_payload,
          std::move(result->m_statistics));
      if (result->m_callback) {
        Network::Callback callback;
        {
          std::lock_guard<std::recursive_mutex> lock(m_mutex);
          callback = result->m_callback;
          result->m_callback = nullptr;  // protect against multiple calls
        }
        callback(
            std::move(response));  // must call outside lock to prevent deadlock
      }
    }
    if (m_session && !m_connections.empty()) {
      // Check for timeouted connections
      std::unique_lock<std::recursive_mutex> lock(m_mutex);
      std::vector<std::wstring> closed;
      for (const std::pair<std::wstring, std::shared_ptr<ConnectionData>>&
               conn : m_connections) {
        if ((GetTickCount64() - conn.second->m_lastUsed) > (1000 * 60 * 5)) {
          // This connection has not been used in 5 minutes
          closed.push_back(conn.first);
        }
      }
      for (const std::wstring& conn : closed) m_connections.erase(conn);
    }
  }
}

NetworkProtocolWinHttp::ResultData::ResultData(
    Network::RequestId id, Network::Callback callback,
    std::shared_ptr<std::ostream> payload)
    : m_callback(callback),
      m_payload(payload),
      m_size(0),
      m_count(0),
      m_offset(0),
      m_id(id),
      m_status(-1),
      m_maxAge(-1),
      m_expires(-1),
      m_completed(false),
      m_cancelled(false) {}

NetworkProtocolWinHttp::ResultData::~ResultData() {}

NetworkProtocolWinHttp::ConnectionData::ConnectionData(
    const std::shared_ptr<NetworkProtocolWinHttp>& owner)
    : m_self(owner), m_connect(NULL) {}

NetworkProtocolWinHttp::ConnectionData::~ConnectionData() {
  if (m_connect) {
    WinHttpCloseHandle(m_connect);
    m_connect = NULL;
  }
}

NetworkProtocolWinHttp::RequestData::RequestData(
    int id, std::shared_ptr<ConnectionData> connection,
    Network::Callback callback, Network::HeaderCallback headerCallback,
    Network::DataCallback dataCallback, std::shared_ptr<std::ostream> payload,
    const NetworkRequest& request)
    : m_connection(connection),
      m_result(new ResultData(id, callback, payload)),
      m_payload(payload),
      m_headerCallback(headerCallback),
      m_dataCallback(dataCallback),
      m_request(NULL),
      m_id(id),
      m_resumed(false),
      m_ignoreOffset(false),
      m_ignoreData(request.Verb() == NetworkRequest::HEAD),
      m_getStatistics(false),
      m_noCompression(false),
      m_uncompress(false) {}

NetworkProtocolWinHttp::RequestData::~RequestData() {
  if (m_request) {
    WinHttpCloseHandle(m_request);
    m_request = NULL;
  }
}

void NetworkProtocolWinHttp::RequestData::complete() {
  std::shared_ptr<NetworkProtocolWinHttp> that = m_connection->m_self;
  {
    std::unique_lock<std::recursive_mutex> lock(that->m_mutex);
    that->m_results.push(m_result);
  }
  SetEvent(that->m_event);
}

void NetworkProtocolWinHttp::RequestData::freeHandle() {
  std::shared_ptr<NetworkProtocolWinHttp> that = m_connection->m_self;
  {
    std::unique_lock<std::recursive_mutex> lock(that->m_mutex);
    that->m_requests.erase(m_id);
  }
}

}  // namespace network
}  // namespace olp
