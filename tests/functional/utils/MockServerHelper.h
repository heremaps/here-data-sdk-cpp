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
#include "olp/dataservice/read/Types.h"
// clang-format off
#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"
#include "generated/serializer/JsonSerializer.h"
// clang-format on
#include "olp/core/geo/tiling/TileKey.h"
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
   * @brief Mock get latest version request.
   *
   * @note after mocking could be called only once.
   *
   * @param VersionResponse to be returned by server.
   */
  void MockGetVersionResponse(
      olp::dataservice::read::model::VersionResponse data);

  /**
   * @brief Mock get resource apis request.
   *
   * @note after mocking could be called only once.
   *
   * @param Apis to be returned by server.
   */
  void MockLookupResourceApiResponse(olp::dataservice::read::model::Apis data);

  /**
   * @brief Mock get platform apis request.
   *
   * @note after mocking could be called only once.
   *
   * @param Apis to be returned by server.
   */
  void MockLookupPlatformApiResponse(olp::dataservice::read::model::Apis data);

  /**
   * @brief Mock get blob data request.
   *
   * @note keep order of mocks for future use of Verify function.
   *
   * @param layer blob corresponds to.
   * @param data_handle of the blob.
   * @param data of the blob.
   */
  void MockGetResponse(const std::string& layer, const std::string& data_handle,
                       const std::string& data);

  /**
   * @brief Mock get quad tree index request.
   *
   * @note keep order of mocks for future use of Verify function.
   *
   * @param layer quad tree corresponds to.
   * @param tile root of the tree.
   * @param tree json  response.
   */
  void MockGetResponse(const std::string& layer, olp::geo::TileKey tile,
                       int64_t version, const std::string& tree);

  /**
   * @brief Verify if all calls were called on server and its order.
   *
   */
  bool Verify();

  /**
   * @brief Mock get request for some data.
   *
   * @note after mocking could be called only once.
   *
   * @param data to be returned by server.
   * @param path request path for type T.
   */
  template <class T>
  void MockGetResponse(T data, const std::string &path) {
    auto str = olp::serializer::serialize(data);
    paths_.push_back(path);
    mock_server_client_.MockResponse("GET", path, str);
  }

  /**
   * @brief Mock get request for some vector of data.
   *
   * @note after mocking could be called only once.
   *
   * @param data to be returned by server.
   * @param path request path for type T.
   */
  template <class T>
  void MockGetResponse(std::vector<T> data, const std::string &path) {
    std::string str = "[";
    for (const auto &el : data) {
      str.append(olp::serializer::serialize(el));
      str.append(",");
    }
    str[str.length() - 1] = ']';
    paths_.push_back(path);
    mock_server_client_.MockResponse("GET", path, str);
  }

  /**
   * @brief Mock get request to return some error.
   *
   * @note after mocking could be called only once.
   *
   * @param error to be returned by server.
   * @param path request path.
   */
  void MockGetError(olp::client::ApiError error, const std::string &path);

 private:
  std::string catalog_;
  Client mock_server_client_;
  std::vector<std::string> paths_;
};

}  // namespace mockserver
