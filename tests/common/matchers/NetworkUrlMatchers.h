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

#pragma once

#include <gmock/gmock.h>

#include <olp/core/http/NetworkRequest.h>

MATCHER_P(IsGetRequest, url, "") {
  // uri, verb, null body
  return olp::http::NetworkRequest::HttpVerb::GET == arg.GetVerb() &&
         url == arg.GetUrl() && (!arg.GetBody() || arg.GetBody()->empty());
}

MATCHER_P(IsPutRequest, url, "") {
  return olp::http::NetworkRequest::HttpVerb::PUT == arg.GetVerb() &&
         url == arg.GetUrl();
}

MATCHER_P(IsPutRequestPrefix, url, "") {
  if (olp::http::NetworkRequest::HttpVerb::PUT != arg.GetVerb()) {
    return false;
  }

  std::string url_string(url);
  auto res =
      std::mismatch(url_string.begin(), url_string.end(), arg.GetUrl().begin());

  return (res.first == url_string.end());
}

MATCHER_P(IsPostRequest, url, "") {
  return olp::http::NetworkRequest::HttpVerb::POST == arg.GetVerb() &&
         url == arg.GetUrl();
}

MATCHER_P(IsDeleteRequest, url, "") {
  return olp::http::NetworkRequest::HttpVerb::DEL == arg.GetVerb() &&
         url == arg.GetUrl();
}

MATCHER_P(IsDeleteRequestPrefix, url, "") {
  if (olp::http::NetworkRequest::HttpVerb::DEL != arg.GetVerb()) {
    return false;
  }

  std::string url_string(url);
  auto res =
      std::mismatch(url_string.begin(), url_string.end(), arg.GetUrl().begin());

  return (res.first == url_string.end());
}
