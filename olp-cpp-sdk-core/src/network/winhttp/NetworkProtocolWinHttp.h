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

#include "../Memory.h"

#include <olp/core/network/NetworkProtocol.h>

#include <windows.h>
#include <winhttp.h>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef NETWORK_HAS_ZLIB
#include <zlib.h>
#endif

namespace olp {
namespace network {
/**
 * @brief The NetworkProtocolWinHttp class is implementation of NetworkProtocol
 * with WinHttp see NetworkProtocol::Implementation
 */
class NetworkProtocolWinHttp
    : public NetworkProtocol,
      public std::enable_shared_from_this<NetworkProtocolWinHttp> {
 public:
  /**
   * @brief NetworkProtocolWinHttp constructor
   * @param settings - settings for protocol configuration
   */
  NetworkProtocolWinHttp();

  ~NetworkProtocolWinHttp();

  bool Initialize() override;

  void Deinitialize() override;

  bool Initialized() const override;

  bool Ready() override;

  NetworkProtocol::ErrorCode Send(const NetworkRequest& request, int id,
                                  const std::shared_ptr<std::ostream>& payload,
                                  std::shared_ptr<NetworkConfig> config,
                                  Network::HeaderCallback headerCallback,
                                  Network::DataCallback dataCallback,
                                  Network::Callback callback) override;

  bool Cancel(int id) override;

  size_t AmountPending() override;

 private:
  NetworkProtocolWinHttp(const NetworkProtocolWinHttp& other) = delete;
  NetworkProtocolWinHttp(NetworkProtocolWinHttp&& other) = delete;
  NetworkProtocolWinHttp& operator=(const NetworkProtocolWinHttp& other) =
      delete;
  NetworkProtocolWinHttp& operator=(NetworkProtocolWinHttp&& other) = delete;

  struct ResultData {
    ResultData(Network::RequestId id, Network::Callback callback,
               std::shared_ptr<std::ostream> payload);
    ~ResultData();
    std::string m_etag;
    std::string m_contentType;
    StatisticsData m_statistics;
    Network::Callback m_callback;
    std::shared_ptr<std::ostream> m_payload;
    std::uint64_t m_size;
    std::uint64_t m_count;
    std::uint64_t m_offset;
    Network::RequestId m_id;
    int m_status;
    int m_maxAge;
    time_t m_expires;
    bool m_completed;
    bool m_cancelled;
  };

  struct ConnectionData {
    ConnectionData(const std::shared_ptr<NetworkProtocolWinHttp>& owner);
    ~ConnectionData();
    std::shared_ptr<NetworkProtocolWinHttp> m_self;
    HINTERNET m_connect;
    ULONGLONG m_lastUsed;
  };

  struct RequestData {
    RequestData(int id, std::shared_ptr<ConnectionData> connection,
                Network::Callback callback,
                Network::HeaderCallback headerCallback,
                Network::DataCallback dataCallback,
                std::shared_ptr<std::ostream> payload,
                const NetworkRequest& request);
    ~RequestData();
    void complete();
    void freeHandle();
    std::wstring m_date;
    std::shared_ptr<ConnectionData> m_connection;
    std::shared_ptr<ResultData> m_result;
    std::shared_ptr<std::ostream> m_payload;
    Network::HeaderCallback m_headerCallback;
    Network::DataCallback m_dataCallback;
    HINTERNET m_request;
    int m_id;
    bool m_resumed;
    bool m_ignoreOffset;
    bool m_ignoreData;
    bool m_getStatistics;
    bool m_noCompression;
    bool m_uncompress;
    mem::MemoryScopeTracker m_tracker;
#ifdef NETWORK_HAS_ZLIB
    z_stream m_strm;
#endif
  };

  static void CALLBACK requestCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID,
                                       DWORD);
  static DWORD WINAPI run(LPVOID arg);

  void completionThread();

  std::recursive_mutex m_mutex;
  mem::map<std::wstring, std::shared_ptr<ConnectionData> > m_connections;
  mem::map<int, std::shared_ptr<RequestData> > m_requests;
  std::queue<std::shared_ptr<ResultData>,
             mem::deque<std::shared_ptr<ResultData> > >
      m_results;
  HINTERNET m_session;
  HANDLE m_thread;
  HANDLE m_event;
  mem::MemoryScopeTracker m_tracker;
};

}  // namespace network
}  // namespace olp
