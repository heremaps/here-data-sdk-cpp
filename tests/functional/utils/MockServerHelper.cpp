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
void MockServerHelper::MockGetResponse(T data, olp::client::ApiError error) {
  auto str = error.GetMessage();
  if (error.GetHttpStatusCode() == olp::http::HttpStatusCode::OK) {
    str = olp::serializer::serialize(data);
  }
  auto path = GetPathMatcher<T>();
  paths_.push_back(path);
  mock_server_client_.MockResponse(
      "GET", path, str, static_cast<uint16_t>(error.GetHttpStatusCode()));
}  // namespace mockserver

template <class T>
void MockServerHelper::MockGetResponse(std::vector<T> data,
                                       olp::client::ApiError error) {
  std::string str = "[";
  for (const auto& el : data) {
    str.append(olp::serializer::serialize(el));
    str.append(",");
  }
  str[str.length() - 1] = ']';
  auto path = GetPathMatcher<T>();
  paths_.push_back(path);
  mock_server_client_.MockResponse("GET", path, str,
                                   static_cast<uint16_t>(error.GetErrorCode()));
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

void MockServerHelper::MockGetResponse(
    olp::dataservice::read::model::VersionResponse data,
    olp::client::ApiError error) {
  MockGetResponse<olp::dataservice::read::model::VersionResponse>(
      std::move(data), std::move(error));
}

void MockServerHelper::MockGetResponse(olp::dataservice::read::model::Apis data,
                                       olp::client::ApiError error) {
  MockGetResponse<olp::dataservice::read::model::Api>(std::move(data),
                                                      std::move(error));
}

void MockServerHelper::MockGetPlatformApiResponse(
    olp::dataservice::read::model::Apis data, olp::client::ApiError error) {
  auto str = error.GetMessage();
  if (error.GetHttpStatusCode() == olp::http::HttpStatusCode::OK) {
    str = "[";
    for (const auto& el : data) {
      str.append(olp::serializer::serialize(el));
      str.append(",");
    }
    str[str.length() - 1] = ']';
  }

  auto path = "/lookup/v1/platform/apis";
  paths_.push_back(path);
  mock_server_client_.MockResponse("GET", path, str,
                                   static_cast<uint16_t>(error.GetErrorCode()));
}

void MockServerHelper::MockGetResponse(
    olp::dataservice::read::model::Partitions data,
    olp::client::ApiError error) {
  MockGetResponse<olp::dataservice::read::model::Partitions>(std::move(data),
                                                             std::move(error));
}

void MockServerHelper::MockGetResponse(
    olp::dataservice::read::model::VersionInfos data,
    olp::client::ApiError error) {
  MockGetResponse<olp::dataservice::read::model::VersionInfos>(
      std::move(data), std::move(error));
}

void MockServerHelper::MockGetResponse(
    olp::dataservice::read::model::Catalog data, olp::client::ApiError error) {
  MockGetResponse<olp::dataservice::read::model::Catalog>(std::move(data),
                                                          std::move(error));
}

bool MockServerHelper::Verify() {
  return mock_server_client_.VerifySequence(paths_);
}

}  // namespace mockserver
