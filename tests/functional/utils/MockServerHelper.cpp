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

#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/CatalogSerializer.h"
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/VersionInfosSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"

#include "generated/serializer/JsonSerializer.h"

namespace mockserver {

template <class T>
void MockServerHelper::MockGetResponse(T data, const std::string& path) {
  auto str = olp::serializer::serialize(data);
  paths_.push_back(path);
  mock_server_client_.MockResponse(
      "GET", path, str, static_cast<uint16_t>(olp::http::HttpStatusCode::OK));
}

void MockServerHelper::MockGetError(olp::client::ApiError error,
                                    const std::string& path) {
  auto str = error.GetMessage();
  paths_.push_back(path);
  mock_server_client_.MockResponse(
      "GET", path, str, static_cast<uint16_t>(error.GetHttpStatusCode()));
}

template <class T>
void MockServerHelper::MockGetResponse(std::vector<T> data,
                                       const std::string& path) {
  std::string str = "[";
  for (const auto& el : data) {
    str.append(olp::serializer::serialize(el));
    str.append(",");
  }
  str[str.length() - 1] = ']';
  paths_.push_back(path);
  mock_server_client_.MockResponse(
      "GET", path, str, static_cast<uint16_t>(olp::http::HttpStatusCode::OK));
}

MockServerHelper::MockServerHelper(olp::client::OlpClientSettings settings,
                                   std::string catalog)
    : catalog_(std::move(catalog)), mock_server_client_(std::move(settings)) {
  mock_server_client_.Reset();
}

void MockServerHelper::MockTimestamp(time_t time) {
  mock_server_client_.MockResponse(
      "GET", "/timestamp",
      R"({"timestamp" : )" + std::to_string(time) + R"(})", true);
}

void MockServerHelper::MockAuth() {
  mock_server_client_.MockResponse(
      "POST", "/oauth2/token",
      R"({"access_token": "token","token_type": "bearer"})", true);
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

void MockServerHelper::MockGetPartitionsResponse(
    olp::dataservice::read::model::Partitions data) {
  MockGetResponse(std::move(data), "/metadata/v1/catalogs/" + catalog_ +
                                       "/layers/testlayer/partitions");
}

void MockServerHelper::MockGetVersionInfosResponse(
    olp::dataservice::read::model::VersionInfos data) {
  MockGetResponse(std::move(data),
                  "/metadata/v1/catalogs/" + catalog_ + "/versions");
}

void MockServerHelper::MockGetCatalogResponse(
    olp::dataservice::read::model::Catalog data) {
  MockGetResponse(std::move(data), "/config/v1/catalogs/" + catalog_);
}

bool MockServerHelper::Verify() {
  return mock_server_client_.VerifySequence(paths_);
}

void MockServerHelper::MockGetVersionError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               "/metadata/v1/catalogs/" + catalog_ + "/versions/latest");
}
void MockServerHelper::MockLookupResourceApiError(olp::client::ApiError error) {
  MockGetError(std::move(error), "/lookup/v1/resources/" + catalog_ + "/apis");
}
void MockServerHelper::MockLookupPlatformApiError(olp::client::ApiError error) {
  MockGetError(std::move(error), "/lookup/v1/platform/apis");
}

void MockServerHelper::MockServerHelper::MockGetPartitionsError(
    olp::client::ApiError error) {
  MockGetError(std::move(error), "/metadata/v1/catalogs/" + catalog_ +
                                     "/layers/testlayer/partitions");
}

void MockServerHelper::MockGetVersionInfosError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               "/metadata/v1/catalogs/" + catalog_ + "/versions");
}

void MockServerHelper::MockGetCatalogError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               std::string("/config/v1/catalogs/") + catalog_);
}

}  // namespace mockserver
