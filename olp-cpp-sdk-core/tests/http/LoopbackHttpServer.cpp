/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include "LoopbackHttpServer.h"

#include <algorithm>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace olp {
namespace http {
namespace test {

namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;

void EnsureWinSockInitialized() {
  static std::once_flag once;
  static bool initialized = false;
  std::call_once(once, []() {
    WSADATA wsadata;
    initialized = (WSAStartup(MAKEWORD(2, 2), &wsadata) == 0);
  });

  if (!initialized) {
    throw std::runtime_error("WSAStartup failed");
  }
}

void CloseSocket(SocketHandle socket) {
  if (socket != kInvalidSocket) {
    (void)::closesocket(socket);
  }
}

#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;

void EnsureWinSockInitialized() {}

void CloseSocket(SocketHandle socket) {
  if (socket != kInvalidSocket) {
    (void)::close(socket);
  }
}
#endif

bool IsValidSocket(SocketHandle socket) { return socket != kInvalidSocket; }

}  // namespace

LoopbackHttpServer::LoopbackHttpServer() { Start(); }

LoopbackHttpServer::~LoopbackHttpServer() { Stop(); }

std::uint16_t LoopbackHttpServer::Port() const { return port_; }

std::string LoopbackHttpServer::CreateUrl() const {
  return std::string("http://127.0.0.1:") + std::to_string(port_) + "/";
}

void LoopbackHttpServer::Start() {
  EnsureWinSockInitialized();

  SocketHandle listen_socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (!IsValidSocket(listen_socket)) {
    throw std::runtime_error("Failed to create loopback test socket");
  }

  int reuse = 1;
  const int setopt_result =
      ::setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse));
  if (setopt_result != 0) {
    CloseSocket(listen_socket);
    throw std::runtime_error("Failed to set SO_REUSEADDR on loopback socket");
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(0);
  if (::bind(listen_socket, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0) {
    CloseSocket(listen_socket);
    throw std::runtime_error("Failed to bind loopback socket");
  }

  socklen_t address_length = sizeof(address);
  if (::getsockname(listen_socket, reinterpret_cast<sockaddr*>(&address),
                    &address_length) != 0) {
    CloseSocket(listen_socket);
    throw std::runtime_error("Failed to read loopback socket port");
  }
  port_ = ntohs(address.sin_port);

  if (::listen(listen_socket, 8) != 0) {
    CloseSocket(listen_socket);
    throw std::runtime_error("Failed to listen on loopback socket");
  }

#ifdef _WIN32
  listen_socket_ = static_cast<std::uintptr_t>(listen_socket);
#else
  listen_socket_ = listen_socket;
#endif

  running_.store(true);
  server_thread_ = std::thread([this]() { ServeLoop(); });
}

void LoopbackHttpServer::Stop() {
  if (!running_.exchange(false)) {
    return;
  }

#ifdef _WIN32
  const SocketHandle listen_socket = static_cast<SocketHandle>(listen_socket_);
#else
  const SocketHandle listen_socket = listen_socket_;
#endif

  // Unblock accept() so ServeLoop can stop quickly.
  const SocketHandle wake_socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (IsValidSocket(wake_socket)) {
    sockaddr_in wake_address{};
    wake_address.sin_family = AF_INET;
    wake_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    wake_address.sin_port = htons(port_);
    const int connect_result =
        ::connect(wake_socket, reinterpret_cast<sockaddr*>(&wake_address),
                  sizeof(wake_address));
    if (connect_result == -1) {
      // Best-effort wakeup only; close and continue shutdown.
    }
    CloseSocket(wake_socket);
  }

  if (server_thread_.joinable()) {
    server_thread_.join();
  }

  CloseSocket(listen_socket);
#ifdef _WIN32
  listen_socket_ = ~static_cast<std::uintptr_t>(0);
#else
  listen_socket_ = kInvalidSocket;
#endif
}

void LoopbackHttpServer::ServeLoop() {
#ifdef _WIN32
  const SocketHandle listen_socket = static_cast<SocketHandle>(listen_socket_);
#else
  const SocketHandle listen_socket = listen_socket_;
#endif

  while (running_.load()) {
    const SocketHandle client_socket =
        ::accept(listen_socket, nullptr, nullptr);
    if (!IsValidSocket(client_socket)) {
      continue;
    }

    std::string request;
    char request_buffer[1024];
    for (;;) {
      const int received =
          ::recv(client_socket, request_buffer, sizeof(request_buffer), 0);
      if (received <= 0) {
        break;
      }

      request.append(request_buffer, static_cast<std::size_t>(received));
      if (request.find("\r\n\r\n") != std::string::npos) {
        break;
      }
    }

    std::string body(64 * 1024, 'a');
    std::string response_headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: " +
        std::to_string(body.size()) + "\r\n\r\n";
    std::string response = response_headers + body;

    std::size_t written = 0;
    while (written < response.size()) {
      const std::size_t chunk_size =
          std::min<std::size_t>(response.size() - written, 16 * 1024);
      const int sent = ::send(client_socket, response.data() + written,
                              static_cast<int>(chunk_size), 0);
      if (sent <= 0) {
        break;
      }
      written += static_cast<std::size_t>(sent);
    }

#ifdef _WIN32
    (void)::shutdown(client_socket, SD_BOTH);
#else
    (void)::shutdown(client_socket, SHUT_RDWR);
#endif
    CloseSocket(client_socket);
  }
}

std::string CreateLoopbackUrl() {
  static LoopbackHttpServer server;
  return server.CreateUrl();
}

}  // namespace test
}  // namespace http
}  // namespace olp
