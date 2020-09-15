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
#include "MockServerHelper.h"

#include <utility>
#include <vector>

namespace mockserver {

void MockServerHelper::MockGetError(olp::client::ApiError error,
                                    const std::string& path) {
  auto str = error.GetMessage();
  paths_.push_back(path);
  mock_server_client_.MockResponse("GET", path, str, error.GetHttpStatusCode());
}

MockServerHelper::MockServerHelper(olp::client::OlpClientSettings settings,
                                   std::string catalog)
    : catalog_(std::move(catalog)), mock_server_client_(std::move(settings)) {
  mock_server_client_.Reset();
}

void MockServerHelper::MockTimestamp(time_t time) {
  mock_server_client_.MockResponse(
      "GET", "/timestamp",
      R"({"timestamp" : )" + std::to_string(time) + R"(})");
}

void MockServerHelper::MockAuth() {
  mock_server_client_.MockResponse(
      "POST", "/oauth2/token",
      R"({"accessToken": "token", "tokenType": "bearer", "expiresIn":86399})");
}

void MockServerHelper::MockGetVersionResponse(
    olp::dataservice::read::model::VersionResponse data) {
  MockGetResponse(std::move(data),
                  "/metadata/v1/catalogs/" + catalog_ + "/versions/latest");
}

void MockServerHelper::MockLookupResourceApiResponse(
    olp::dataservice::read::model::Apis data) {
  MockGetResponse(std::move(data),
                  "/lookup/v1/resources/" + catalog_ + "/apis");
}

void MockServerHelper::MockLookupPlatformApiResponse(
    olp::dataservice::read::model::Apis data) {
  MockGetResponse<olp::dataservice::read::model::Api>(
      std::move(data), "/lookup/v1/platform/apis");
}

void MockServerHelper::MockGetResponse(const std::string& layer,
                                       const std::string& data_handle,
                                       const std::string& data) {
  mock_server_client_.MockResponse("GET",
                                   "/blob/v1/catalogs/" + catalog_ +
                                       "/layers/" + layer + "/data/" +
                                       data_handle,
                                   data);
}

void MockServerHelper::MockGetResponse(const std::string& layer,
                                       olp::geo::TileKey tile, int64_t version,
                                       const std::string& tree) {
  mock_server_client_.MockResponse("GET",
                                   "/query/v1/catalogs/" + catalog_ +
                                       "/layers/" + layer + "/versions/" +
                                       std::to_string(version) + "/quadkeys/" +
                                       tile.ToHereTile() + "/depths/4",
                                   tree);
}

bool MockServerHelper::Verify() {
  return mock_server_client_.VerifySequence(paths_);
}

}  // namespace mockserver
