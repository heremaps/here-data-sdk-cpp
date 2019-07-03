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

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>

#include <olp/core/CoreApi.h>

namespace olp {
namespace network {
/**
 * @brief The NetworkRequest class represents network request abstraction for a
 * HTTP request
 */
class CORE_API NetworkRequest {
 public:
  /**
   * @brief Clock for timestamps.
   */
  using Clock = std::chrono::steady_clock;

  /**
   * @brief Type for timestamps.
   */
  using Timestamp = Clock::duration;

  /**
   * @brief The PriorityRange enum shows ranges of priority field
   */
  enum PriorityRange { PriorityMin = 0, PriorityDefault = 2, PriorityMax = 5 };

  /**
   * @brief The HttpVerb enum lists the supported HTTP Verbs
   */
  enum HttpVerb { GET = 0, POST = 1, HEAD = 2, PUT = 3, DEL = 4, PATCH = 5 };

  /**
   * @brief NetworkRequest constructor
   */
  NetworkRequest() = default;

  /**
   * @brief NetworkRequest constructor
   * @param url - Full URL encoded url
   * @param modified_since - modified since
   * @param priority - priority for the request
   * @param verb - The used HTTP verb
   */
  NetworkRequest(const std::string& url, std::uint64_t modified_since = 0,
                 int priority = PriorityDefault, HttpVerb verb = HttpVerb::GET);

  /**
   * @brief add extra header
   * @param name - name of the header
   * @param value - value of the header
   */
  void AddHeader(const std::string& name, const std::string& value);

  /**
   * @brief remove extra headers that satisfy a given condition
   * @param condition - to be satisfied in order to remove a (name,value)-pair
   */
  void RemoveHeader(
      const std::function<bool(const std::pair<std::string, std::string>&)>&
          condition);

  /**
   * @brief remove extra headers by name
   * @param name_to_remove - name of the header to remove
   */
  void RemoveHeaderByName(const std::string& name_to_remove);

  /**
   * @brief Set new url
   * @param url - url to set
   */
  void SetUrl(const std::string& url);

  /**
   * @brief url getter method
   * @return contained url
   */
  const std::string& Url() const;

  /**
   * @brief priority getter method
   * @return contained priority
   */
  int Priority() const;

  /**
   * @brief extra headers getter method
   * @return extra headers for the request
   */
  const std::vector<std::pair<std::string, std::string> >& ExtraHeaders() const;

  /**
   * @brief change the http verb for request
   * @param verb specifies the used HTTP Verb
   */
  void SetVerb(HttpVerb verb);

  /**
   * @brief verb getter method
   * @return requested verb
   */
  NetworkRequest::HttpVerb Verb() const;

  /**
   * @brief set the content (only with HTTP POST or PUT verb)
   * @param content specifies the content to be sent.
   *        the data must be not modified until the request is completed.
   */
  void SetContent(const std::shared_ptr<std::vector<unsigned char> >& content);

  /**
   * @brief Retrieve the content to be sent
   * @return content to be sent
   */
  const std::shared_ptr<std::vector<unsigned char> >& Content() const;

  /**
   * @brief get content if modified since
   * @return Last modified. Seconds since 1 Jan 1970 or 0 if not in use
   */
  std::uint64_t ModifiedSince() const;

  /**
   * @brief Set a flag if payload offset should be ignored.
   *
   * If this flag is false the payload is stored to correct offset,
   * otherwise offset is not checked.
   * Do not set if you are planning to use pause/resume.
   *
   * @param ignore - true if payload offset should be ignored
   */
  void SetIgnoreOffset(bool ignore);

  /**
   * @brief Check if payload offset should be ignored
   * @return true if payload offset should be ignored
   */
  bool IgnoreOffset() const;

  /**
   * @brief Set statistics to be gethered for this request
   */
  void SetStatistics();

  /**
   * @brief Check if statistics should be gathered
   * @return true if statistics should be gathered
   */
  bool GetStatistics() const;

  /**
   * @brief Gets the request timestamp
   * @return timestamp of this request (time of construction)
   */
  Timestamp GetTimestamp() const;

 private:
  void SetPriority(int Priority);

  std::string url_;
  std::vector<std::pair<std::string, std::string> > extra_headers_;
  std::shared_ptr<std::vector<unsigned char> > content_;
  std::uint64_t modified_since_{0};
  HttpVerb verb_{HttpVerb::GET};
  int priority_{PriorityRange::PriorityDefault};
  bool ignore_offset_{false};
  bool get_statistics_{false};
  Timestamp timestamp_{Clock::now().time_since_epoch()};
};

/**
 * @brief A function to filter specific requests according to a bool criteria
 *        (similar to a comparator for a sorting algorithm)
 */
using RequestFilter = std::function<bool(const NetworkRequest&)>;

}  // namespace network
}  // namespace olp
