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

#include "olp/core/http/NetworkRequest.h"

namespace olp {
namespace http {

NetworkRequest::NetworkRequest(std::string url) : url_{std::move(url)} {}

const NetworkRequest::RequestHeadersType& NetworkRequest::GetHeaders() const {
  return headers_;
}

const std::string& NetworkRequest::GetUrl() const { return url_; }

NetworkRequest::HttpVerb NetworkRequest::GetVerb() const { return verb_; }

NetworkRequest::RequestBodyType NetworkRequest::GetBody() const {
  return body_;
}

const NetworkSettings& NetworkRequest::GetSettings() const { return settings_; }

NetworkRequest& NetworkRequest::WithHeader(std::string name,
                                           std::string value) {
  headers_.emplace_back(std::move(name), std::move(value));
  return *this;
}

NetworkRequest& NetworkRequest::WithUrl(std::string url) {
  url_ = std::move(url);
  return *this;
}

NetworkRequest& NetworkRequest::WithVerb(HttpVerb verb) {
  verb_ = verb;
  return *this;
}

NetworkRequest& NetworkRequest::WithBody(RequestBodyType body) {
  body_ = std::move(body);
  return *this;
}

NetworkRequest& NetworkRequest::WithSettings(NetworkSettings settings) {
  settings_ = std::move(settings);
  return *this;
}

}  // namespace http
}  // namespace olp
