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

#include "olp/core/network/NetworkRequest.h"

#include <algorithm>
#include <string>
#include <vector>

namespace olp {
namespace network {
NetworkRequest::NetworkRequest(const std::string& url,
                               std::uint64_t modified_since, int priority,
                               HttpVerb verb)
    : url_(url), modified_since_(modified_since), verb_(verb) {
  SetPriority(priority);
}

void NetworkRequest::AddHeader(const std::string& name,
                               const std::string& value) {
  extra_headers_.push_back(std::pair<std::string, std::string>(name, value));
}

void NetworkRequest::RemoveHeader(
    const std::function<bool(const std::pair<std::string, std::string>&)>&
        condition) {
  extra_headers_.erase(
      std::remove_if(begin(extra_headers_), end(extra_headers_), condition),
      end(extra_headers_));
}

void NetworkRequest::RemoveHeaderByName(const std::string& name_to_remove) {
  RemoveHeader([&name_to_remove](
                   const std::pair<std::string, std::string>& name_and_value) {
    return name_to_remove == name_and_value.first;
  });
}

void NetworkRequest::SetUrl(const std::string& url) { url_ = url; }

const std::string& NetworkRequest::Url() const { return url_; }

int NetworkRequest::Priority() const { return priority_; }

void NetworkRequest::SetPriority(int priority) {
  if (priority < PriorityMin)
    priority_ = PriorityMin;
  else if (priority > PriorityMax)
    priority_ = PriorityMax;
  else
    priority_ = priority;
}

const std::vector<std::pair<std::string, std::string> >&
NetworkRequest::ExtraHeaders() const {
  return extra_headers_;
}

void NetworkRequest::SetVerb(HttpVerb verb) { verb_ = verb; }

NetworkRequest::HttpVerb NetworkRequest::Verb() const { return verb_; }

void NetworkRequest::SetContent(
    const std::shared_ptr<std::vector<unsigned char> >& content) {
  content_ = content;
}

const std::shared_ptr<std::vector<unsigned char> >& NetworkRequest::Content()
    const {
  return content_;
}

std::uint64_t NetworkRequest::ModifiedSince() const { return modified_since_; }

void NetworkRequest::SetIgnoreOffset(bool ignore) { ignore_offset_ = ignore; }

bool NetworkRequest::IgnoreOffset() const { return ignore_offset_; }

void NetworkRequest::SetStatistics() { get_statistics_ = true; }

bool NetworkRequest::GetStatistics() const { return get_statistics_; }

NetworkRequest::Timestamp NetworkRequest::GetTimestamp() const {
  return timestamp_;
}

}  // namespace network
}  // namespace olp
