/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include "Client.h"

#include <string>
#include <vector>

#include "generated/model/Api.h"
#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionResponse.h"

namespace mockserver {
/**
 * @brief Easy mock functionality.
 *
 * @note Keep orders of mocked requests if want use
 * Verify function to check calls on server. MockTimestamp and MockAuth set
 * property to be called infinite times.
 */
class MockServerHelper {
 public:
  MockServerHelper(olp::client::OlpClientSettings settings,
                   std::string catalog);
  /**
   * @brief Mock get timestamp request.
   *
   * @note after mocking timestamp request could be called infinite times.
   *
   * @param time to be returned by server.
   */
  void MockTimestamp(time_t time = 0);

  /**
   * @brief Mock get authentication tocken request.
   *
   * @note after mocking request could be called infinite times.
   */
  void MockAuth();

  /**
   * @brief Mock get version request.
   *
   * @note keep order of mocks for future use of Verify function.
   *
   * @param time to be returned by server.
   */
  void MockGetResponse(olp::dataservice::read::model::VersionResponse data);

  /**
   * @brief Mock get apis request.
   *
   * @note keep order of mocks for future use of Verify function.
   *
   * @param data apis to be returned by server.
   */
  void MockGetResponse(olp::dataservice::read::model::Apis data);

  /**
   * @brief Mock get partitions request.
   *
   * @note keep order of mocks for future use of Verify function.
   *
   * @param data partitions to be returned by server.
   */
  void MockGetResponse(olp::dataservice::read::model::Partitions data);

  /**
   * @brief Verify if all calls were called on server and its order.
   *
   */
  bool Verify();

 private:
  // template specialization for type to get path matcher
  template <class T>
  std::string GetPathMatcher();
  template <class T>
  void MockGetResponse(T data);
  template <class T>
  void MockGetResponse(std::vector<T> data);

 private:
  std::string catalog_;
  Client mock_server_client_;
  std::vector<std::string> paths__;
};

}  // namespace mockserver
