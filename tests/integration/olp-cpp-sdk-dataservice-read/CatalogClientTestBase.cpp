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

#include "CatalogClientTestBase.h"

#include <matchers/NetworkUrlMatchers.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "HttpResponses.h"

namespace olp {
namespace tests {
namespace integration {

namespace http = olp::http;
using ::testing::_;

std::ostream& operator<<(std::ostream& os, const CacheType cache_type) {
  switch (cache_type) {
    case CacheType::IN_MEMORY:
      return os << "In-memory cache";
    case CacheType::DISK:
      return os << "Disk cache";
    case CacheType::BOTH:
      return os << "In-memory & disk cache";
    default:
      return os << "Unknown cache type";
  }
}

CatalogClientTestBase::CatalogClientTestBase() = default;

CatalogClientTestBase::~CatalogClientTestBase() = default;

std::string CatalogClientTestBase::GetTestCatalog() {
  return "hrn:here:data::olp-here-test:hereos-internal-test-v2";
}

std::string CatalogClientTestBase::ApiErrorToString(
    const olp::client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

void CatalogClientTestBase::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = olp::client::OlpClientSettings();
  settings_.network_request_handler = network_mock_;
  olp::cache::CacheSettings cache_settings;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
  client_ = olp::client::OlpClientFactory::Create(settings_);
  settings_.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  SetUpCommonNetworkMockCalls();
}

void CatalogClientTestBase::TearDown() {
  client_.reset();
  ::testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
}

void CatalogClientTestBase::SetUpCommonNetworkMockCalls() {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LATEST_CATALOG_VERSION));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LAYER_VERSIONS));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITIONS));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITION_269));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_269));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITION_3), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITION_3));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LAYER_VERSIONS_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITIONS_V2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_V2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITION_269_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269_V2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_269_V2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_V10), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                             HTTP_RESPONSE_INVALID_VERSION_V10));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_VN1), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                             HTTP_RESPONSE_INVALID_VERSION_VN1));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LAYER_VERSIONS_V10), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                             HTTP_RESPONSE_INVALID_VERSION_V10));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LAYER_VERSIONS_VN1), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(GetResponse(http::HttpStatusCode::BAD_REQUEST),
                             HTTP_RESPONSE_INVALID_VERSION_VN1));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG_V2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_CONFIG_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_23618364), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_23618364));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_1476147), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_1476147));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_92259), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_92259));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_369036), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_369036));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_1));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_3), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_3));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_4), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_4));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_5), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_5));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_7), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(GetResponse(http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_7));

  // Catch any non-interesting network calls that don't need to be verified
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
}

}  // namespace integration
}  // namespace tests
}  // namespace olp
