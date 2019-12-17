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

#include "ConfigApi.h"

#include <map>
#include <memory>
#include <sstream>

#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
// clang-format off
// Ordering Required - Parser template specializations before JsonParser.h
#include "parser/CatalogParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace olp {
namespace dataservice {
namespace write {
using namespace olp::client;

CancellationToken ConfigApi::GetCatalog(
    std::shared_ptr<OlpClient> client, const std::string& catalog_hrn,
    boost::optional<std::string> billing_tag,
    const CatalogCallback& catalogCallback) {
  std::multimap<std::string, std::string> headerParams;
  headerParams.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> queryParams;
  if (billing_tag) {
    queryParams.insert(std::make_pair("billingTag", *billing_tag));
  }
  std::multimap<std::string, std::string> formParams;

  std::string catalogUri = "/catalogs/" + catalog_hrn;

  NetworkAsyncCallback callback =
      [catalogCallback](client::HttpResponse response) {
        if (response.status != 200) {
          catalogCallback(CatalogResponse(
              client::ApiError(response.status, response.response.str())));
        } else {
          catalogCallback(CatalogResponse(
              olp::parser::parse<model::Catalog>(response.response)));
        }
      };

  return client->CallApi(catalogUri, "GET", queryParams, headerParams,
                         formParams, nullptr, "", callback);
}

CatalogResponse ConfigApi::GetCatalog(const OlpClient& client,
                                      const std::string& catalog_hrn,
                                      boost::optional<std::string> billing_tag,
                                      client::CancellationContext context) {
  std::multimap<std::string, std::string> header_params;
  header_params.insert(std::make_pair("Accept", "application/json"));
  std::multimap<std::string, std::string> query_params;
  if (billing_tag) {
    query_params.insert(std::make_pair("billingTag", *billing_tag));
  }
  std::string catalog_uri = "/catalogs/" + catalog_hrn;

  client::HttpResponse response = client.CallApi(
      std::move(catalog_uri), "GET", std::move(query_params),
      std::move(header_params), {}, nullptr, std::string{}, std::move(context));
  if (response.status != olp::http::HttpStatusCode::OK) {
    return client::ApiError(response.status, response.response.str());
  }
  return olp::parser::parse<model::Catalog>(response.response);
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
