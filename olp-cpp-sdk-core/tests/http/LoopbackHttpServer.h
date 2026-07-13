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

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace olp {
namespace http {
namespace test {

class LoopbackHttpServer final {
 public:
  LoopbackHttpServer();
  ~LoopbackHttpServer();

  std::uint16_t Port() const;
  std::string CreateUrl() const;

 private:
  void Start();
  void Stop();
  void ServeLoop();

 private:
  std::atomic<bool> running_{false};
#ifdef _WIN32
  std::uintptr_t listen_socket_{~static_cast<std::uintptr_t>(0)};
#else
  int listen_socket_{-1};
#endif
  std::uint16_t port_{0};
  std::thread server_thread_;
};

std::string CreateLoopbackUrl();

}  // namespace test
}  // namespace http
}  // namespace olp
