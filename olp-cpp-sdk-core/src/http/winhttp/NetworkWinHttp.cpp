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
#include <vector>

#include "../NetworkUtils.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

namespace {

constexpr int kNetworkUncompressionChunkSize = 1024 * 16;

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
                          &out.front(), out.size());

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

  proxy_string_stream << std::wstring(proxy.GetUsername().begin(),
                                      proxy.GetUsername().end());
  proxy_string_stream << ':';
  proxy_string_stream << proxy.GetPort();

  return proxy_string_stream.str();
}

}  // namespace

namespace olp {
namespace http {

constexpr auto kLogTag = "WinHttp";

NetworkWinHttp::NetworkWinHttp()
    : http_session_(NULL),
      thread_(INVALID_HANDLE_VALUE),
      event_(INVALID_HANDLE_VALUE) {
  request_id_counter_.store(
      static_cast<RequestId>(RequestIdConstants::RequestIdMin));

  http_session_ = WinHttpOpen(L"OLP Http Client", WINHTTP_ACCESS_TYPE_NO_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                              WINHTTP_FLAG_ASYNC);

  if (!http_session_) {
    EDGE_SDK_LOG_ERROR(kLogTag, "WinHttpOpen failed " << GetLastError());
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
}

NetworkWinHttp::~NetworkWinHttp() {
  std::vector<std::shared_ptr<ResultData>> pending_results;
  {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    std::vector<std::shared_ptr<RequestData>> requests_to_cancel;
    for (const auto& request : http_requests_) {
      requests_to_cancel.push_back(request.second);
    }
    while (!requests_to_cancel.empty()) {
      std::shared_ptr<RequestData> request = requests_to_cancel.front();
      WinHttpCloseHandle(request->http_request);
      request->http_request = NULL;
      pending_results.push_back(request->result_data);
      requests_to_cancel.erase(requests_to_cancel.begin());
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
    EDGE_SDK_LOG_WARNING(kLogTag, "WinHttpCrackUrl failed " << request.GetUrl()
                                                            << " "
                                                            << GetLastError());
    return SendOutcome(ErrorCode::INVALID_URL_ERROR);
  }

  if ((url_components.nScheme != INTERNET_SCHEME_HTTP) &&
      (url_components.nScheme != INTERNET_SCHEME_HTTPS)) {
    EDGE_SDK_LOG_WARNING(kLogTag,
                         "Invalid scheme on request " << request.GetUrl());
    return SendOutcome(ErrorCode::IO_ERROR);
  }

  std::shared_ptr<RequestData> handle;
  {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    std::wstring server(url.data(), size_t(url_components.lpszUrlPath -
                                           url_components.lpszScheme));
    auto& connection = http_connections_[server];
    if (!connection) {
      connection = std::make_shared<ConnectionData>(shared_from_this());
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
        return SendOutcome(ErrorCode::OFFLINE_ERROR);
      }
    }

    connection->last_used = GetTickCount64();

    handle =
        std::make_shared<RequestData>(id, connection, callback, header_callback,
                                      data_callback, payload, request);
    http_requests_[id] = handle;
  }

  DWORD flags = (url_components.nScheme == INTERNET_SCHEME_HTTPS)
                    ? WINHTTP_FLAG_SECURE
                    : 0;
  LPWSTR http_verb = L"GET";
  if (request.GetVerb() == NetworkRequest::HttpVerb::POST) {
    http_verb = L"POST";
  } else if (request.GetVerb() == NetworkRequest::HttpVerb::PUT) {
    http_verb = L"PUT";
  } else if (request.GetVerb() == NetworkRequest::HttpVerb::HEAD) {
    http_verb = L"HEAD";
  } else if (request.GetVerb() == NetworkRequest::HttpVerb::DEL) {
    http_verb = L"DELETE";
  } else if (request.GetVerb() == NetworkRequest::HttpVerb::PATCH) {
    http_verb = L"PATCH";
  }

  LPCSTR content = WINHTTP_NO_REQUEST_DATA;
  DWORD content_length = 0;

  if (request.GetVerb() != NetworkRequest::HttpVerb::HEAD &&
      request.GetVerb() != NetworkRequest::HttpVerb::GET &&
      request.GetBody() != nullptr && !request.GetBody()->empty()) {
    content = (LPCSTR) & (request.GetBody()->front());
    content_length = (DWORD)request.GetBody()->size();
  }

  /* Create a request */
  auto http_request =
      WinHttpOpenRequest(handle->connection_data->http_connection, http_verb,
                         url_components.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!http_request) {
    EDGE_SDK_LOG_ERROR(kLogTag, "WinHttpOpenRequest failed " << GetLastError());

    std::unique_lock<std::recursive_mutex> lock(mutex_);

    http_requests_.erase(id);
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
      EDGE_SDK_LOG_WARNING(kLogTag, "WinHttpSetOption(Proxy) failed "
                                        << GetLastError() << " "
                                        << request.GetUrl());
    }
    if (!proxy.GetUsername().empty() && !proxy.GetUsername().empty()) {
      std::wstring proxy_username;
      const auto username_res =
          ConvertMultiByteToWideChar(proxy.GetUsername(), proxy_username);

      std::wstring proxy_password;
      const auto password_res =
          ConvertMultiByteToWideChar(proxy.GetPassword(), proxy_password);

      if (username_res && password_res) {
        LPCWSTR proxy_lpcwstr_username = proxy_username.c_str();
        if (!WinHttpSetOption(http_request, WINHTTP_OPTION_PROXY_USERNAME,
                              const_cast<LPWSTR>(proxy_lpcwstr_username),
                              wcslen(proxy_lpcwstr_username))) {
          EDGE_SDK_LOG_WARNING(
              kLogTag, "WinHttpSetOption(proxy username) failed "
                           << GetLastError() << " " << request.GetUrl());
        }

        LPCWSTR proxy_lpcwstr_password = proxy_password.c_str();
        if (!WinHttpSetOption(http_request, WINHTTP_OPTION_PROXY_PASSWORD,
                              const_cast<LPWSTR>(proxy_lpcwstr_password),
                              wcslen(proxy_lpcwstr_password))) {
          EDGE_SDK_LOG_WARNING(
              kLogTag, "WinHttpSetOption(proxy password) failed "
                           << GetLastError() << " " << request.GetUrl());
        }
      } else {
        if (!username_res) {
          EDGE_SDK_LOG_WARNING(kLogTag, "Proxy username conversion failure "
                                            << GetLastError() << " "
                                            << request.GetUrl());
        }
        if (!password_res) {
          EDGE_SDK_LOG_WARNING(kLogTag, "Proxy password conversion failure "
                                            << GetLastError() << " "
                                            << request.GetUrl());
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
    EDGE_SDK_LOG_ERROR(kLogTag,
                       "WinHttpAddRequestHeaders() failed " << GetLastError());
  }

  /* Send the request */
  if (!WinHttpSendRequest(http_request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                          (LPVOID)content, content_length, content_length,
                          (DWORD_PTR)handle.get())) {
    EDGE_SDK_LOG_ERROR(kLogTag, "WinHttpSendRequest failed " << GetLastError());

    std::unique_lock<std::recursive_mutex> lock(mutex_);

    http_requests_.erase(id);
    return SendOutcome(ErrorCode::IO_ERROR);
  }
  handle->http_request = http_request;

  return SendOutcome(id);
}

void NetworkWinHttp::Cancel(RequestId id) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  auto it = http_requests_.find(id);
  if (it == http_requests_.end()) {
    return;
  }

  // Just closing the handle cancels the request
  if (it->second->http_request) {
    WinHttpCloseHandle(it->second->http_request);
    it->second->http_request = NULL;
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
    EDGE_SDK_LOG_WARNING(kLogTag, "RequestCallback to inactive handle");
    return;
  }

  // to extend RequestData lifetime till the end of function
  std::shared_ptr<RequestData> that;

  {
    std::unique_lock<std::recursive_mutex> lock(
        handle->connection_data->self->mutex_);

    that = handle->connection_data->self->http_requests_[handle->request_id];
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

    handle->Complete();
  } else if (status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
    // We have sent request, now receive a response
    WinHttpReceiveResponse(handle->http_request, NULL);
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
          assert(convertResult == len);

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
              EDGE_SDK_LOG_ERROR(
                  kLogTag,
                  "Gzip encoding but compression no supported and no "
                  "ZLIB found");
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
    WinHttpQueryDataAvailable(handle->http_request, NULL);
  } else if (status == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
    assert(status_info_length == sizeof(DWORD));
    DWORD size = *reinterpret_cast<DWORD*>(status_info);
    if (size > 0 && 416 != handle->result_data->status) {
      // Data is available read it
      LPVOID buffer = (LPVOID)LocalAlloc(LPTR, size);
      if (!buffer) {
        EDGE_SDK_LOG_ERROR(kLogTag,
                           "Out of memory reeceiving " << size << " bytes");
        handle->result_data->status = ERROR_NOT_ENOUGH_MEMORY;
        handle->Complete();
        return;
      }
      WinHttpReadData(handle->http_request, buffer, size, NULL);
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
            EDGE_SDK_LOG_ERROR(kLogTag, "Uncompression failed");
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
                EDGE_SDK_LOG_WARNING(
                    kLogTag,
                    "Reception stream doesn't support setting write point");
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

    WinHttpQueryDataAvailable(handle->http_request, NULL);
  } else if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING) {
    // Only now is it safe to free the handle
    // See
    // https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpclosehandle
    handle->FreeHandle();
    return;
  } else {
    EDGE_SDK_LOG_ERROR(kLogTag,
                       "Unknown callback " << std::hex << status << std::dec);
  }
}

DWORD
NetworkWinHttp::Run(LPVOID arg) {
  reinterpret_cast<NetworkWinHttp*>(arg)->CompletionThread();
  return 0;
}

void NetworkWinHttp::CompletionThread() {
  std::shared_ptr<NetworkWinHttp> that = shared_from_this();

  while (that->http_session_) {
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
                     .WithCancelled(result->cancelled)
                     .WithError(str)
                     .WithRequestId(result->request_id)
                     .WithStatus(status));
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

NetworkWinHttp::ConnectionData::ConnectionData(
    std::shared_ptr<NetworkWinHttp> owner)
    : self(std::move(owner)), http_connection(NULL) {}

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
      header_callback(std::move(header_callback)),
      data_callback(std::move(data_callback)),
      http_request(NULL),
      request_id(id),
      resumed(false),
      ignore_data(request.GetVerb() == NetworkRequest::HttpVerb::HEAD),
      no_compression(false),
      uncompress(false) {}

NetworkWinHttp::RequestData::~RequestData() {
  if (http_request) {
    WinHttpCloseHandle(http_request);
    http_request = NULL;
  }
}

void NetworkWinHttp::RequestData::Complete() {
  std::shared_ptr<NetworkWinHttp> that = connection_data->self;
  {
    std::unique_lock<std::recursive_mutex> lock(that->mutex_);
    that->results_.push(result_data);
  }
  SetEvent(that->event_);
}

void NetworkWinHttp::RequestData::FreeHandle() {
  std::shared_ptr<NetworkWinHttp> that = connection_data->self;
  {
    std::unique_lock<std::recursive_mutex> lock(that->mutex_);
    that->http_requests_.erase(request_id);
  }
}

}  // namespace http
}  // namespace olp
