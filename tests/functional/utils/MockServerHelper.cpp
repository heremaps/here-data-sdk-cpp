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
template <>
std::string
MockServerHelper::GetPathMatcher<olp::dataservice::read::model::Partitions>() {
  return "/metadata/v1/catalogs/" + catalog_ + "/layers/testlayer/partitions";
}

template <>
std::string MockServerHelper::GetPathMatcher<
    olp::dataservice::read::model::VersionResponse>() {
  return "/metadata/v1/catalogs/" + catalog_ + "/versions/latest";
}

template <>
std::string
MockServerHelper::GetPathMatcher<olp::dataservice::read::model::Api>() {
  return "/lookup/v1/resources/" + catalog_ + "/apis";
}

template <>
std::string MockServerHelper::GetPathMatcher<
    olp::dataservice::read::model::VersionInfos>() {
  return "/metadata/v1/catalogs/" + catalog_ + "/versions";
}

template <>
std::string
MockServerHelper::GetPathMatcher<olp::dataservice::read::model::Catalog>() {
  return "/config/v1/catalogs/" + catalog_;
}

template <class T>
void MockServerHelper::MockGetResponse(T data) {
  auto str = olp::serializer::serialize(data);
  auto path = GetPathMatcher<T>();
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
                                       const std::string& in_path) {
  std::string str = "[";
  for (const auto& el : data) {
    str.append(olp::serializer::serialize(el));
    str.append(",");
  }
  str[str.length() - 1] = ']';
  auto path = in_path;
  if (in_path.empty()) {
    path = GetPathMatcher<T>();
  }
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
  MockGetResponse<olp::dataservice::read::model::VersionResponse>(
      std::move(data));
}

void MockServerHelper::MockLookupResourceApiResponse(
    olp::dataservice::read::model::Apis data) {
  MockGetResponse<olp::dataservice::read::model::Api>(std::move(data));
}

void MockServerHelper::MockLookupPlatformApiResponse(
    olp::dataservice::read::model::Apis data) {
  MockGetResponse<olp::dataservice::read::model::Api>(
      std::move(data), "/lookup/v1/platform/apis");
}

void MockServerHelper::MockGetPartitionsResponse(
    olp::dataservice::read::model::Partitions data) {
  MockGetResponse(std::move(data));
}

void MockServerHelper::MockGetVersionInfosResponse(
    olp::dataservice::read::model::VersionInfos data) {
  MockGetResponse(std::move(data));
}

void MockServerHelper::MockGetCatalogResponse(
    olp::dataservice::read::model::Catalog data) {
  MockGetResponse(std::move(data));
}

bool MockServerHelper::Verify() {
  return mock_server_client_.VerifySequence(paths_);
}

void MockServerHelper::MockGetVersionError(olp::client::ApiError error) {
  MockGetError(
      std::move(error),
      GetPathMatcher<olp::dataservice::read::model::VersionResponse>());
}
void MockServerHelper::MockLookupResourceApiError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               GetPathMatcher<olp::dataservice::read::model::Api>());
}
void MockServerHelper::MockLookupPlatformApiError(olp::client::ApiError error) {
  MockGetError(std::move(error), "/lookup/v1/platform/apis");
}

void MockServerHelper::MockServerHelper::MockGetPartitionsError(
    olp::client::ApiError error) {
  MockGetError(std::move(error),
               GetPathMatcher<olp::dataservice::read::model::Partitions>());
}

void MockServerHelper::MockGetVersionInfosError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               GetPathMatcher<olp::dataservice::read::model::VersionInfos>());
}

void MockServerHelper::MockGetCatalogError(olp::client::ApiError error) {
  MockGetError(std::move(error),
               GetPathMatcher<olp::dataservice::read::model::Catalog>());
}

}  // namespace mockserver
