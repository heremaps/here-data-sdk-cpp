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

#include "NetworkWinHttp.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "olp/core/http/NetworkUtils.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

namespace {

constexpr int kNetworkUncompressionChunkSize = 1024 * 16;
constexpr auto kRequestCompletionSleepTime = std::chrono::milliseconds(1);

LPCSTR
ErrorToString(DWORD err) {
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
    default:
      return "Unknown error";
  }
}

olp::http::ErrorCode WinErrorToCode(DWORD err) {
  switch (err) {
    case ERROR_SUCCESS:
      return olp::http::ErrorCode::SUCCESS;
    case ERROR_WINHTTP_INVALID_URL:
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
      return olp::http::ErrorCode::INVALID_URL_ERROR;
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
    case ERROR_WINHTTP_LOGIN_FAILURE:
    case ERROR_WINHTTP_SECURE_FAILURE:
      return olp::http::ErrorCode::AUTHORIZATION_ERROR;
    case ERROR_WINHTTP_OPERATION_CANCELLED:
      return olp::http::ErrorCode::CANCELLED_ERROR;
    case ERROR_WINHTTP_TIMEOUT:
      return olp::http::ErrorCode::TIMEOUT_ERROR;
    default:
      return olp::http::ErrorCode::UNKNOWN_ERROR;
  }
}

LPWSTR
QueryHeadervalue(HINTERNET request, DWORD header) {
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

bool ConvertMultiByteToWideChar(const std::string& in, std::wstring& out) {
  out.clear();
  if (in.empty()) {
    return true;
  }

  // Detect required buffer size.
  const auto chars_required = MultiByteToWideChar(
      CP_ACP, MB_PRECOMPOSED, in.c_str(),
      -1,       // denotes null-terminated string
      nullptr,  // output buffer is null means request required buffer size
      0);

  if (chars_required == 0) {
    // error: could not convert input string
    return false;
  }

  out.resize(chars_required);
  // Perform actual conversion from multi-byte to wide char
  const auto conversion_result =
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, in.c_str(),
                          -1,  // denotes null-terminated string
                          &out.front(), static_cast<int>(out.size()));

  // Should not happen as 1st call have succeeded.
  return conversion_result != 0;
}

std::wstring ProxyString(const olp::http::NetworkProxySettings& proxy) {
  using namespace olp::http;
  std::wostringstream proxy_string_stream;

  switch (proxy.GetType()) {
    case NetworkProxySettings::Type::NONE:
      proxy_string_stream << "http://";
      break;
    case NetworkProxySettings::Type::SOCKS4:
      proxy_string_stream << "socks4://";
      break;
    case NetworkProxySettings::Type::SOCKS5:
      proxy_string_stream << "socks5://";
      break;
    case NetworkProxySettings::Type::SOCKS4A:
      proxy_string_stream << "socks4a://";
      break;
    case NetworkProxySettings::Type::SOCKS5_HOSTNAME:
      proxy_string_stream << "socks5h://";
      break;
    default:
      proxy_string_stream << "http://";
  }

  proxy_string_stream << std::wstring(proxy.GetHostname().begin(),
                                      proxy.GetHostname().end());
  proxy_string_stream << ':';
  proxy_string_stream << proxy.GetPort();

  return proxy_string_stream.str();
}

}  // namespace

namespace olp {
namespace http {

constexpr auto kLogTag = "WinHttp";

NetworkWinHttp::NetworkWinHttp(size_t max_request_count)
    : http_session_(NULL),
      thread_(INVALID_HANDLE_VALUE),
      event_(INVALID_HANDLE_VALUE),
      http_requests_(max_request_count) {
  request_id_counter_.store(
      static_cast<RequestId>(RequestIdConstants::RequestIdMin));

  http_session_ = WinHttpOpen(L"OLP Http Client", WINHTTP_ACCESS_TYPE_NO_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                              WINHTTP_FLAG_ASYNC);

  if (!http_session_) {
    OLP_SDK_LOG_ERROR(kLogTag, "WinHttpOpen failed " << GetLastError());
    return;
  }

  WinHttpSetStatusCallback(
      http_session_, (WINHTTP_STATUS_CALLBACK)&NetworkWinHttp::RequestCallback,
      WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES, 0);

  event_ = CreateEvent(NULL, TRUE, FALSE, NULL);

  // Store the memory tracking state during initialization so that it can be
  // used by the thread.
  thread_ = CreateThread(NULL, 0, NetworkWinHttp::Run, this, 0, NULL);
  SetThreadPriority(thread_, THREAD_PRIORITY_ABOVE_NORMAL);

  OLP_SDK_LOG_TRACE(kLogTag, "Created NetworkWinHttp with address="
                                 << this
                                 << ", handles_count=" << max_request_count);
}

NetworkWinHttp::~NetworkWinHttp() {
  OLP_SDK_LOG_TRACE(kLogTag, "Destroying NetworkWinHttp, this=" << this);

  std::vector<std::shared_ptr<ResultData>> pending_results;
  {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    for (auto& request : http_requests_) {
      if (request.in_use) {
        pending_results.push_back(request.result_data);
        WinHttpCloseHandle(request.http_request);
        request.http_request = NULL;
      }
    }
  }

  // Before proceeding we need to be sure that request handles will not
  // be used in RequestCallback
  for (auto& request : http_requests_) {
    while (request.in_use) {
      std::this_thread::sleep_for(kRequestCompletionSleepTime);
    }
  }

  if (http_session_) {
    WinHttpCloseHandle(http_session_);
    http_session_ = NULL;
  }

  if (event_ != INVALID_HANDLE_VALUE) SetEvent(event_);
  if (thread_ != INVALID_HANDLE_VALUE) {
    if (GetCurrentThreadId() != GetThreadId(thread_)) {
      WaitForSingleObject(thread_, INFINITE);
    }
  }
  CloseHandle(event_);
  CloseHandle(thread_);
  thread_ = event_ = INVALID_HANDLE_VALUE;

  {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    http_connections_.clear();
    while (!results_.empty()) {
      pending_results.push_back(results_.front());
      results_.pop();
    }
  }

  for (auto& result : pending_results) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (result->user_callback) {
      result->user_callback(
          NetworkResponse()
              .WithRequestId(result->request_id)
              .WithStatus(static_cast<int>(ErrorCode::OFFLINE_ERROR))
              .WithError("Offline: network is deinitialized"));
      result->user_callback = nullptr;
    }
  }
}

SendOutcome NetworkWinHttp::Send(NetworkRequest request,
                                 std::shared_ptr<std::ostream> payload,
                                 Callback callback,
                                 HeaderCallback header_callback,
                                 DataCallback data_callback) {
  RequestId id = request_id_counter_.fetch_add(1);

  URL_COMPONENTS url_components;
  ZeroMemory(&url_components, sizeof(url_components));
  url_components.dwStructSize = sizeof(url_components);
  url_components.dwSchemeLength = (DWORD)-1;
  url_components.dwHostNameLength = (DWORD)-1;
  url_components.dwUrlPathLength = (DWORD)-1;
  url_components.dwExtraInfoLength = (DWORD)-1;

  std::wstring url(request.GetUrl().begin(), request.GetUrl().end());
  if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &url_components)) {
    OLP_SDK_LOG_ERROR(kLogTag, "WinHttpCrackUrl failed, url="
                                   << request.GetUrl()
                                   << ", error=" << GetLastError());
    return SendOutcome(ErrorCode::INVALID_URL_ERROR);
  }

  if ((url_components.nScheme != INTERNET_SCHEME_HTTP) &&
      (url_components.nScheme != INTERNET_SCHEME_HTTPS)) {
    OLP_SDK_LOG_ERROR(kLogTag, "Invalid scheme, url=" << request.GetUrl());
    return SendOutcome(ErrorCode::IO_ERROR);
  }

  RequestData* handle = nullptr;
  {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    std::wstring server(url.data(), size_t(url_components.lpszUrlPath -
                                           url_components.lpszScheme));
    auto& connection = http_connections_[server];
    if (!connection) {
      connection = std::make_shared<ConnectionData>(this);
      INTERNET_PORT port = url_components.nPort;
      if (port == 0) {
        port = (url_components.nScheme == INTERNET_SCHEME_HTTPS)
                   ? INTERNET_DEFAULT_HTTPS_PORT
                   : INTERNET_DEFAULT_HTTP_PORT;
      }
      std::wstring host_name(url_components.lpszHostName,
                             url_components.dwHostNameLength);
      connection->http_connection =
          WinHttpConnect(http_session_, host_name.data(), port, 0);
      if (!connection->http_connection) {
        OLP_SDK_LOG_ERROR(kLogTag, "WinHttpConnect failed, url="
                                       << request.GetUrl()
                                       << ", error=" << GetLastError());
        return SendOutcome(ErrorCode::OFFLINE_ERROR);
      }
    }

    connection->last_used = GetTickCount64();

    handle = GetHandle(id, connection, callback, header_callback, data_callback,
                       payload, request);
    if (handle == nullptr) {
      OLP_SDK_LOG_DEBUG(kLogTag,
                        "All handles are in use, url=" << request.GetUrl());
      return SendOutcome(ErrorCode::NETWORK_OVERLOAD_ERROR);
    }
  }

  const auto request_verb = request.GetVerb();

  DWORD flags = (url_components.nScheme == INTERNET_SCHEME_HTTPS)
                    ? WINHTTP_FLAG_SECURE
                    : 0;
  LPWSTR http_verb = L"GET";
  if (request_verb == NetworkRequest::HttpVerb::POST) {
    http_verb = L"POST";
  } else if (request_verb == NetworkRequest::HttpVerb::PUT) {
    http_verb = L"PUT";
  } else if (request_verb == NetworkRequest::HttpVerb::HEAD) {
    http_verb = L"HEAD";
  } else if (request_verb == NetworkRequest::HttpVerb::DEL) {
    http_verb = L"DELETE";
  } else if (request_verb == NetworkRequest::HttpVerb::PATCH) {
    http_verb = L"PATCH";
  }

  LPCSTR content = WINHTTP_NO_REQUEST_DATA;
  DWORD content_length = 0;

  const NetworkRequest::RequestBodyType& request_body = handle->body;

  if (request_verb != NetworkRequest::HttpVerb::HEAD &&
      request_verb != NetworkRequest::HttpVerb::GET &&
      request_body != nullptr && !request_body->empty()) {
    content = (LPCSTR) & (request_body->front());
    content_length = (DWORD)request_body->size();
  }

  /* Create a request */
  auto http_request =
      WinHttpOpenRequest(handle->connection_data->http_connection, http_verb,
                         url_components.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!http_request) {
    OLP_SDK_LOG_ERROR(kLogTag, "WinHttpOpenRequest failed, url="
                                   << request.GetUrl()
                                   << ", error=" << GetLastError());

    return SendOutcome(ErrorCode::IO_ERROR);
  }

  const auto& network_settings = request.GetSettings();

  WinHttpSetTimeouts(http_request,
                     network_settings.GetConnectionTimeout() * 1000,
                     network_settings.GetConnectionTimeout() * 1000,
                     network_settings.GetTransferTimeout() * 1000,
                     network_settings.GetTransferTimeout() * 1000);

  const auto& proxy = network_settings.GetProxySettings();

  if (proxy.GetType() != NetworkProxySettings::Type::NONE) {
    std::wstring proxy_string = ProxyString(proxy);

    WINHTTP_PROXY_INFO proxy_info;
    proxy_info.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    proxy_info.lpszProxy = const_cast<LPWSTR>(proxy_string.c_str());
    proxy_info.lpszProxyBypass = WINHTTP_NO_PROXY_BYPASS;

    if (!WinHttpSetOption(http_request, WINHTTP_OPTION_PROXY, &proxy_info,
                          sizeof(proxy_info))) {
      OLP_SDK_LOG_WARNING(kLogTag, "WinHttpSetOption(Proxy) failed, url="
                                       << request.GetUrl()
                                       << ", error=" << GetLastError());
    }
    if (!proxy.GetUsername().empty() && !proxy.GetPassword().empty()) {
      std::wstring proxy_username;
      const auto username_res =
          ConvertMultiByteToWideChar(proxy.GetUsername(), proxy_username);
      if (!username_res) {
        OLP_SDK_LOG_WARNING(kLogTag, "Proxy username conversion failure, url="
                                         << request.GetUrl()
                                         << ", error=" << GetLastError());
      }

      std::wstring proxy_password;
      const auto password_res =
          ConvertMultiByteToWideChar(proxy.GetPassword(), proxy_password);
      if (!password_res) {
        OLP_SDK_LOG_WARNING(kLogTag, "Proxy password conversion failure, url="
                                         << request.GetUrl()
                                         << ", error=" << GetLastError());
      }

      if (username_res && password_res) {
        LPCWSTR proxy_lpcwstr_username = proxy_username.c_str();
        if (!WinHttpSetOption(
                http_request, WINHTTP_OPTION_PROXY_USERNAME,
                const_cast<LPWSTR>(proxy_lpcwstr_username),
                static_cast<DWORD>(wcslen(proxy_lpcwstr_username)))) {
          OLP_SDK_LOG_WARNING(
              kLogTag, "WinHttpSetOption(proxy username) failed, url="
                           << request.GetUrl() << ", error=" << GetLastError());
        }

        LPCWSTR proxy_lpcwstr_password = proxy_password.c_str();
        if (!WinHttpSetOption(
                http_request, WINHTTP_OPTION_PROXY_PASSWORD,
                const_cast<LPWSTR>(proxy_lpcwstr_password),
                static_cast<DWORD>(wcslen(proxy_lpcwstr_password)))) {
          OLP_SDK_LOG_WARNING(
              kLogTag, "WinHttpSetOption(proxy password) failed, url="
                           << request.GetUrl() << ", error=" << GetLastError());
        }
      }
    }
  }

  flags = WINHTTP_DECOMPRESSION_FLAG_ALL;
  if (!WinHttpSetOption(http_request, WINHTTP_OPTION_DECOMPRESSION, &flags,
                        sizeof(flags))) {
    handle->no_compression = true;
  }

  const auto& extra_headers = request.GetHeaders();

  std::wostringstream header_stream;
  _locale_t loc = _create_locale(LC_CTYPE, "C");
  bool found_content_length = false;
  for (size_t i = 0; i < extra_headers.size(); i++) {
    std::string header_name = extra_headers[i].first.c_str();
    std::transform(header_name.begin(), header_name.end(), header_name.begin(),
                   ::tolower);

    if (header_name.compare("content-length") == 0) {
      found_content_length = true;
    }

    header_stream << header_name.c_str();
    header_stream << L": ";
    header_stream << extra_headers[i].second.c_str();
    header_stream << L"\r\n";
  }

  // Set the content-length header if it does not already exist
  if (!found_content_length) {
    header_stream << L"content-length: " << content_length << L"\r\n";
  }

  _free_locale(loc);

  if (!WinHttpAddRequestHeaders(http_request, header_stream.str().c_str(),
                                DWORD(-1), WINHTTP_ADDREQ_FLAG_ADD)) {
    OLP_SDK_LOG_WARNING(kLogTag, "WinHttpAddRequestHeaders failed, url="
                                     << request.GetUrl()
                                     << ", error=" << GetLastError());
  }

  /* Send the request */
  if (!WinHttpSendRequest(http_request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                          (LPVOID)content, content_length, content_length,
                          (DWORD_PTR)handle)) {
    OLP_SDK_LOG_ERROR(kLogTag, "WinHttpSendRequest failed, url="
                                   << request.GetUrl()
                                   << ", error=" << GetLastError());

    FreeHandle(id);
    return SendOutcome(ErrorCode::IO_ERROR);
  }
  handle->http_request = http_request;

  OLP_SDK_LOG_DEBUG(kLogTag,
                    "Send request, url=" << request.GetUrl() << ", id=" << id);

  return SendOutcome(id);
}

void NetworkWinHttp::Cancel(RequestId id) {
  OLP_SDK_LOG_TRACE(kLogTag, "Cancel request with id=" << id);

  std::unique_lock<std::recursive_mutex> lock_guard(mutex_);
  auto handle = FindHandle(id);
  if (handle) {
    WinHttpCloseHandle(handle->http_request);
    handle->http_request = NULL;
  }
}

void NetworkWinHttp::RequestCallback(HINTERNET, DWORD_PTR context, DWORD status,
                                     LPVOID status_info,
                                     DWORD status_info_length) {
  if (context == NULL) {
    return;
  }

  RequestData* handle = reinterpret_cast<RequestData*>(context);
  if (!handle->connection_data || !handle->result_data) {
    OLP_SDK_LOG_WARNING(kLogTag, "RequestCallback to inactive handle, id="
                                     << handle->request_id);
    return;
  }

  handle->connection_data->last_used = GetTickCount64();

  if (status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
    // Error has occurred
    WINHTTP_ASYNC_RESULT* result =
        reinterpret_cast<WINHTTP_ASYNC_RESULT*>(status_info);
    handle->result_data->status = result->dwError;

    if (result->dwError == ERROR_WINHTTP_OPERATION_CANCELLED) {
      handle->result_data->cancelled = true;
    }

    OLP_SDK_LOG_DEBUG(kLogTag, "RequestCallback - request error, status="
                                   << handle->result_data->status
                                   << ", id=" << handle->request_id);

    handle->Complete();
  } else if (status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
    // We have sent request, now receive a response
    if (!WinHttpReceiveResponse(handle->http_request, NULL)) {
      OLP_SDK_LOG_WARNING(kLogTag, "WinHttpReceiveResponse failed, id="
                                       << handle->request_id
                                       << ", error=" << GetLastError());
      handle->Complete();
    }
  } else if (status == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE) {
    Network::HeaderCallback callback = nullptr;
    {
      std::unique_lock<std::recursive_mutex> lock(
          handle->connection_data->self->mutex_);
      if (handle->header_callback) callback = handle->header_callback;
    }

    if (callback && handle->http_request) {
      DWORD wide_len;
      WinHttpQueryHeaders(handle->http_request, WINHTTP_QUERY_RAW_HEADERS,
                          WINHTTP_HEADER_NAME_BY_INDEX,
                          WINHTTP_NO_OUTPUT_BUFFER, &wide_len,
                          WINHTTP_NO_HEADER_INDEX);
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        DWORD len = wide_len / sizeof(WCHAR);
        auto wide_buffer = std::make_unique<WCHAR[]>(len);
        if (WinHttpQueryHeaders(handle->http_request, WINHTTP_QUERY_RAW_HEADERS,
                                WINHTTP_HEADER_NAME_BY_INDEX, wide_buffer.get(),
                                &wide_len, WINHTTP_NO_HEADER_INDEX)) {
          // Text should be converted back from the wide char to properly handle
          // UTF-8.
          auto buffer = std::make_unique<char[]>(len);
          const int convertResult = WideCharToMultiByte(
              CP_ACP, 0, wide_buffer.get(), len, buffer.get(), len, 0, nullptr);
          assert(convertResult == static_cast<int>(len));

          DWORD start = 0, index = 0;
          while (index < len) {
            if (buffer[index] == 0) {
              if (start != index) {
                std::string entry(&buffer[start], index - start);
                size_t pos = entry.find(':');
                if (pos != std::string::npos && pos + 2 < entry.size()) {
                  std::string key(entry.begin(), entry.begin() + pos);
                  std::string value(entry.begin() + pos + 2, entry.end());
                  callback(key, value);
                }
              }
              index++;
              start = index;
            } else {
              index++;
            }
          }
        }
      }
    }

    {
      std::unique_lock<std::recursive_mutex> lock(
          handle->connection_data->self->mutex_);
      if (handle->http_request) {
        LPWSTR code =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_STATUS_CODE);
        if (code) {
          std::wstring code_str(code);
          handle->result_data->status = std::stoi(code_str);
          LocalFree(code);
        } else {
          handle->result_data->status = -1;
        }

        LPWSTR cache =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_CACHE_CONTROL);
        if (cache) {
          std::wstring cache_str(cache);
          const std::size_t index = cache_str.find(L"max-age=");
          if (index != std::wstring::npos) {
            handle->result_data->max_age =
                std::stoi(cache_str.substr(index + 8));
          }
          LocalFree(cache);
        } else {
          handle->result_data->max_age = -1;
        }

        LPWSTR etag =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_ETAG);
        if (etag) {
          std::wstring etag_str(etag);
          handle->result_data->etag.assign(etag_str.begin(), etag_str.end());
          LocalFree(etag);
        } else {
          handle->result_data->etag.clear();
        }

        LPWSTR date =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_DATE);
        if (date) {
          handle->date = date;
          LocalFree(date);
        } else {
          handle->date.clear();
        }

        LPWSTR range =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_CONTENT_RANGE);
        if (range) {
          const std::wstring range_str(range);
          const std::size_t index = range_str.find(L"bytes ");
          if (index != std::wstring::npos) {
            std::size_t offset = 6;
            if (range_str[6] == L'*' && range_str[7] == L'/') {
              offset = 8;
            }
            if (handle->resumed) {
              handle->result_data->count =
                  std::stoull(range_str.substr(index + offset)) -
                  handle->result_data->offset;
            } else {
              handle->result_data->offset =
                  std::stoull(range_str.substr(index + offset));
            }
          }
          LocalFree(range);
        } else {
          handle->result_data->count = 0;
        }

        LPWSTR type =
            QueryHeadervalue(handle->http_request, WINHTTP_QUERY_CONTENT_TYPE);
        if (type) {
          std::wstring type_str(type);
          handle->result_data->content_type.assign(type_str.begin(),
                                                   type_str.end());
          LocalFree(type);
        } else {
          handle->result_data->content_type.clear();
        }

        LPWSTR length = QueryHeadervalue(handle->http_request,
                                         WINHTTP_QUERY_CONTENT_LENGTH);
        if (length) {
          const std::wstring length_str(length);
          handle->result_data->size = std::stoull(length_str);
          LocalFree(length);
        } else {
          handle->result_data->size = 0;
        }

        if (handle->no_compression) {
          LPWSTR str = QueryHeadervalue(handle->http_request,
                                        WINHTTP_QUERY_CONTENT_ENCODING);
          if (str) {
            std::wstring encoding(str);
            if (encoding == L"gzip") {
#ifdef NETWORK_HAS_ZLIB
              handle->uncompress = true;
              /* allocate inflate state */
              handle->strm.zalloc = Z_NULL;
              handle->strm.zfree = Z_NULL;
              handle->strm.opaque = Z_NULL;
              handle->strm.avail_in = 0;
              handle->strm.next_in = Z_NULL;
              inflateInit2(&handle->strm, 16 + MAX_WBITS);
#else
              OLP_SDK_LOG_ERROR(kLogTag,
                                "Gzip encoding failed - ZLIB not found, id="
                                    << handle->request_id);
#endif
            }
            LocalFree(str);
          }
        }
      } else {
        handle->Complete();
        return;
      }
    }

    // We have received headers, now receive data
    if (!WinHttpQueryDataAvailable(handle->http_request, NULL)) {
      OLP_SDK_LOG_WARNING(kLogTag, "WinHttpQueryDataAvailable failed, id="
                                       << handle->request_id
                                       << ", error=" << GetLastError());
    }
  } else if (status == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
    assert(status_info_length == sizeof(DWORD));
    DWORD size = *reinterpret_cast<DWORD*>(status_info);
    if (size > 0 && 416 != handle->result_data->status) {
      // Data is available read it
      LPVOID buffer = (LPVOID)LocalAlloc(LPTR, size);
      if (!buffer) {
        OLP_SDK_LOG_ERROR(kLogTag, "Out of memory reeceiving "
                                       << size
                                       << " bytes, id=" << handle->request_id);
        handle->result_data->status = ERROR_NOT_ENOUGH_MEMORY;
        handle->Complete();
        return;
      }
      if (!WinHttpReadData(handle->http_request, buffer, size, NULL)) {
        OLP_SDK_LOG_WARNING(kLogTag, "WinHttpReadData failed, id="
                                         << handle->request_id
                                         << ", error=" << GetLastError());
        handle->Complete();
      }
    } else {
      // Request is complete
      if (handle->result_data->status != 416) {
        // Skip size check if manually decompressing, since it's known to not
        // match.
        if (!handle->ignore_data && !handle->uncompress &&
            handle->result_data->size != 0 &&
            handle->result_data->size != handle->result_data->count) {
          handle->result_data->status = -1;
        }
      }
      handle->result_data->completed = true;
      OLP_SDK_LOG_DEBUG(
          kLogTag, "Completed request, id=" << handle->request_id << ", status="
                                            << handle->result_data->status);
      handle->Complete();
    }
  } else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE) {
    // Read is complete, check if there is more
    if (status_info && status_info_length) {
      const char* data_buffer = (const char*)status_info;
      DWORD data_len = status_info_length;
#ifdef NETWORK_HAS_ZLIB
      if (handle->uncompress) {
        Bytef* compressed = (Bytef*)status_info;
        DWORD compressed_len = data_len;
        data_buffer =
            (const char*)LocalAlloc(LPTR, kNetworkUncompressionChunkSize);
        handle->strm.avail_in = compressed_len;
        handle->strm.next_in = compressed;
        data_len = 0;
        DWORD alloc_size = kNetworkUncompressionChunkSize;

        while (handle->strm.avail_in > 0) {
          handle->strm.next_out = (Bytef*)data_buffer + data_len;
          DWORD available_size = alloc_size - data_len;
          handle->strm.avail_out = available_size;
          int r = inflate(&handle->strm, Z_NO_FLUSH);

          if ((r != Z_OK) && (r != Z_STREAM_END)) {
            OLP_SDK_LOG_ERROR(
                kLogTag, "Uncompression failed, id=" << handle->request_id);
            LocalFree((HLOCAL)compressed);
            LocalFree((HLOCAL)data_buffer);
            handle->result_data->status = ERROR_INVALID_BLOCK;
            handle->Complete();
            return;
          }

          data_len += available_size - handle->strm.avail_out;
          if (r == Z_STREAM_END) break;

          if (data_len == alloc_size) {
            // We ran out of space in uncompression buffer, expand the buffer
            // and loop again
            alloc_size += kNetworkUncompressionChunkSize;
            char* newBuffer = (char*)LocalAlloc(LPTR, alloc_size);
            memcpy(newBuffer, data_buffer, data_len);
            LocalFree((HLOCAL)data_buffer);
            data_buffer = (const char*)newBuffer;
          }
        }
        LocalFree((HLOCAL)compressed);
      }
#endif

      OLP_SDK_LOG_TRACE(kLogTag, "Received " << data_len << " bytes for id="
                                             << handle->request_id);
      if (data_len) {
        std::uint64_t total_offset = 0;

        if (handle->data_callback)
          handle->data_callback((const uint8_t*)data_buffer, total_offset,
                                data_len);

        {
          std::unique_lock<std::recursive_mutex> lock(
              handle->connection_data->self->mutex_);
          if (handle->payload) {
            if (handle->payload->tellp() !=
                std::streampos(handle->result_data->count)) {
              handle->payload->seekp(handle->result_data->count);
              if (handle->payload->fail()) {
                OLP_SDK_LOG_WARNING(kLogTag, "Payload seekp() failed, id="
                                                 << handle->request_id);
                handle->payload->clear();
              }
            }

            handle->payload->write(data_buffer, data_len);
          }
          handle->result_data->count += data_len;
        }
      }
      LocalFree((HLOCAL)data_buffer);
    }

    if (!WinHttpQueryDataAvailable(handle->http_request, NULL)) {
      OLP_SDK_LOG_WARNING(kLogTag, "WinHttpQueryDataAvailable failed, id="
                                       << handle->request_id
                                       << ", error=" << GetLastError());
      handle->Complete();
    }
  } else if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING) {
    // Only now is it safe to free the handle
    // See
    // https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpclosehandle
    handle->FreeHandle();
    return;
  } else {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Unknown callback, status=" << std::hex << status << std::dec
                                             << ", id=" << handle->request_id);
  }
}

DWORD
NetworkWinHttp::Run(LPVOID arg) {
  reinterpret_cast<NetworkWinHttp*>(arg)->CompletionThread();
  return 0;
}

void NetworkWinHttp::CompletionThread() {
  while (http_session_) {
    std::shared_ptr<ResultData> result;
    {
      if (http_session_ && results_.empty()) {
        WaitForSingleObject(event_, 30000);  // Wait max 30 seconds
        ResetEvent(event_);
      }
      if (!http_session_) continue;

      std::unique_lock<std::recursive_mutex> lock(mutex_);
      if (!results_.empty()) {
        result = results_.front();
        results_.pop();
      }
    }

    if (http_session_ && result) {
      std::string str;
      int status;
      if ((result->offset == 0) && (result->status == 206))
        result->status = 200;

      if (result->completed) {
        str = HttpErrorToString(result->status);
      } else {
        str = ErrorToString(result->status);
      }

      if (result->completed) {
        status = result->status;
      } else {
        status = static_cast<int>(WinErrorToCode(result->status));
      }

      if (result->user_callback) {
        Network::Callback callback = nullptr;
        {
          std::lock_guard<std::recursive_mutex> lock(mutex_);
          // protect against multiple calls
          std::swap(result->user_callback, callback);
        }
        // must call outside lock to prevent deadlock
        callback(NetworkResponse()
                     .WithError(str)
                     .WithRequestId(result->request_id)
                     .WithStatus(status));
      }

      if (result->completed) {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        auto request = FindHandle(result->request_id);
        if (request) {
          WinHttpCloseHandle(request->http_request);
          request->http_request = NULL;
        }
      }
    }

    if (http_session_ && !http_connections_.empty()) {
      // Check for timeouted connections
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      std::vector<std::wstring> closed;
      for (const auto& conn : http_connections_) {
        constexpr auto five_minutes_ms = 1000 * 60 * 5;
        if ((GetTickCount64() - conn.second->last_used) > five_minutes_ms) {
          // This connection has not been used in 5 minutes
          closed.push_back(conn.first);
        }
      }
      for (const std::wstring& conn : closed) {
        http_connections_.erase(conn);
      }
    }
  }
}

NetworkWinHttp::RequestData* NetworkWinHttp::GetHandle(
    RequestId id, std::shared_ptr<ConnectionData> connection,
    Network::Callback callback, Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, std::shared_ptr<std::ostream> payload,
    const NetworkRequest& request) {
  std::unique_lock<std::recursive_mutex> lock_guard(mutex_);

  auto it =
      std::find_if(http_requests_.begin(), http_requests_.end(),
                   [&](const RequestData& request) { return !request.in_use; });
  if (it != http_requests_.end()) {
    *it = std::move(RequestData(id, connection, callback, header_callback,
                                data_callback, payload, request));
    it->in_use = true;
    return &(*it);
  }
  return nullptr;
}

void NetworkWinHttp::FreeHandle(RequestData* request) {
  std::unique_lock<std::recursive_mutex> lock_guard(mutex_);
  if (request && request->in_use) {
    *request = RequestData();
  }
}

void NetworkWinHttp::FreeHandle(RequestId id) {
  std::unique_lock<std::recursive_mutex> lock_guard(mutex_);
  auto request = FindHandle(id);
  if (request) {
    *request = RequestData();
  }
}

NetworkWinHttp::RequestData* NetworkWinHttp::FindHandle(RequestId id) {
  auto it = std::find_if(http_requests_.begin(), http_requests_.end(),
                         [&](const RequestData& request) {
                           return request.in_use && request.request_id == id;
                         });

  if (it != http_requests_.end()) {
    return &(*it);
  }
  return nullptr;
}

NetworkWinHttp::ResultData::ResultData(RequestId id, Network::Callback callback,
                                       std::shared_ptr<std::ostream> payload)
    : user_callback(std::move(callback)),
      payload(std::move(payload)),
      size(0),
      count(0),
      offset(0),
      request_id(id),
      status(-1),
      max_age(-1),
      expires(-1),
      completed(false),
      cancelled(false) {}

NetworkWinHttp::ConnectionData::ConnectionData(NetworkWinHttp* owner)
    : self(owner), http_connection(NULL) {}

NetworkWinHttp::ConnectionData::~ConnectionData() {
  if (http_connection) {
    WinHttpCloseHandle(http_connection);
    http_connection = NULL;
  }
}

NetworkWinHttp::RequestData::RequestData(
    RequestId id, std::shared_ptr<ConnectionData> connection,
    Network::Callback callback, Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, std::shared_ptr<std::ostream> payload,
    const NetworkRequest& request)
    : connection_data(std::move(connection)),
      result_data(new ResultData(id, callback, payload)),
      payload(payload),
      body(request.GetBody()),
      header_callback(std::move(header_callback)),
      data_callback(std::move(data_callback)),
      http_request(NULL),
      request_id(id),
      resumed(false),
      ignore_data(request.GetVerb() == NetworkRequest::HttpVerb::HEAD),
      no_compression(false),
      uncompress(false),
      in_use(false) {}

NetworkWinHttp::RequestData::RequestData()
    : http_request(NULL),
      request_id(static_cast<RequestId>(RequestIdConstants::RequestIdInvalid)),
      resumed(false),
      ignore_data(),
      no_compression(false),
      uncompress(false),
      in_use(false) {}

NetworkWinHttp::RequestData::~RequestData() {
  if (http_request) {
    WinHttpCloseHandle(http_request);
    http_request = NULL;
  }
}

void NetworkWinHttp::RequestData::Complete() {
  auto that = connection_data->self;
  {
    std::unique_lock<std::recursive_mutex> lock(that->mutex_);
    that->results_.push(result_data);
  }
  SetEvent(that->event_);
}

void NetworkWinHttp::RequestData::FreeHandle() {
  auto that = connection_data->self;
  that->FreeHandle(request_id);
}

}  // namespace http
}  // namespace olp
