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
#include <windows.h>
#include <winhttp.h>

#ifdef NETWORK_HAS_ZLIB
#include <zlib.h>
#endif

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

namespace olp {
namespace http {

/**
 * @brief The NetworkWinHttp class is implementation of Network.
 */
class NetworkWinHttp : public Network {
 public:
  NetworkWinHttp(size_t max_request_count);
  ~NetworkWinHttp() override;

  // Non-copyable, non-movable
  NetworkWinHttp(const NetworkWinHttp&) = delete;
  NetworkWinHttp(NetworkWinHttp&&) = delete;
  NetworkWinHttp& operator=(const NetworkWinHttp&) = delete;
  NetworkWinHttp& operator=(NetworkWinHttp&&) = delete;

  SendOutcome Send(NetworkRequest request,
                   std::shared_ptr<std::ostream> payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override;

  void Cancel(RequestId id) override;

 private:
  struct ResultData {
    ResultData(RequestId id, Callback callback,
               std::shared_ptr<std::ostream> payload);

    Callback user_callback;
    std::shared_ptr<std::ostream> payload;
    std::uint64_t content_length;
    std::uint64_t count;
    std::uint64_t offset;
    RequestId request_id;
    int status;
    bool completed;
    bool cancelled;

    std::uint64_t bytes_uploaded;
    std::uint64_t bytes_downloaded;
  };

  struct ConnectionData {
    ConnectionData(HINTERNET http_connection);
    ConnectionData(const ConnectionData&) = delete;
    ConnectionData(ConnectionData&&) noexcept = delete;
    ConnectionData& operator=(const ConnectionData&) = delete;
    ConnectionData& operator=(ConnectionData&&) noexcept = delete;
    ~ConnectionData();

    HINTERNET http_connection{NULL};
    ULONGLONG last_used{0ull};
  };

  struct RequestData {
    RequestData();
    RequestData(NetworkWinHttp* self, RequestId id,
                std::shared_ptr<ConnectionData> connection, Callback callback,
                HeaderCallback header_callback, DataCallback data_callback,
                std::shared_ptr<std::ostream> payload,
                const NetworkRequest& request);
    ~RequestData();
    void Complete();
    void FreeHandle();

    NetworkWinHttp* self;

    std::shared_ptr<ConnectionData> connection_data;
    std::shared_ptr<ResultData> result_data;

    NetworkRequest::RequestBodyType body;
    HeaderCallback header_callback;
    DataCallback data_callback;

    HINTERNET http_request;
    RequestId request_id;
    bool ignore_data;
    bool no_compression;
    bool uncompress;
    bool in_use;
#ifdef NETWORK_HAS_ZLIB
    z_stream strm;
#endif
  };

  RequestData* GetHandle(RequestId id,
                         std::shared_ptr<ConnectionData> connection,
                         Callback callback, HeaderCallback header_callback,
                         DataCallback data_callback,
                         std::shared_ptr<std::ostream> payload,
                         const NetworkRequest& request);
  void FreeHandle(RequestId id);
  RequestData* FindHandle(RequestId id);

  static void CALLBACK RequestCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID,
                                       DWORD);
  static DWORD WINAPI Run(LPVOID arg);

  void CompletionThread();

  std::recursive_mutex mutex_;
  std::unordered_map<std::wstring, std::shared_ptr<ConnectionData>>
      http_connections_;
  std::vector<RequestData> http_requests_;
  std::queue<std::shared_ptr<ResultData>> results_;

  HINTERNET http_session_;
  HANDLE thread_;
  HANDLE event_;

  std::atomic<RequestId> request_id_counter_;
};

}  // namespace http
}  // namespace olp
