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

#include "NetworkEventImpl.h"

#include <atomic>
#include <sstream>

#include "olp/core/network/Settings.h"

namespace olp {
namespace network {
static std::atomic<size_t> g_content_lengths(0);
static std::atomic<size_t> g_requests(0);
static std::atomic<size_t> g_errors(0);

NetworkEventImpl::NetworkEventImpl(const std::string& url) {}

void NetworkEventImpl::Record(
    size_t content_length, int status, int priority, int request_count,
    const std::string& url,
    const std::vector<std::pair<std::string, std::string> >& extra_headers,
    size_t pending) const {
  // Record global statistics
  g_content_lengths += content_length;
  g_requests++;
  if (status < 200 || status >= 400) {
    g_errors++;
  }
}

void NetworkEventImpl::GetStatistics(size_t& content_lengths, size_t& requests,
                                     size_t& errors) {
  content_lengths = g_content_lengths;
  requests = g_requests;
  errors = g_errors;
}

}  // namespace network
}  // namespace olp
