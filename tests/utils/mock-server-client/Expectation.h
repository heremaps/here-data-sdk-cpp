/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include <string>

#include <boost/any.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <boost/optional.hpp>

#include "JsonHelpers.h"

namespace mockserver {

struct Expectation {
  struct QueryStringParameter {
    std::string name;
    std::vector<std::string> values;
  };
  struct RequestMatcher {
    boost::optional<std::string> path = boost::none;
    boost::optional<std::string> method = boost::none;
    boost::optional<std::vector<QueryStringParameter>> query_string_parameters =
        boost::none;
  };

  struct ResponseDelay {
    int32_t value;
    std::string time_unit;
  };

  struct ResponseAction {
    boost::optional<ResponseDelay> delay = boost::none;
    boost::optional<uint16_t> status_code = boost::none;

    /// Any of BinaryResponse, std::string, boost::any
    boost::any body;
  };

  struct BinaryResponse {
    std::string type = "BINARY";
    std::string base64_string;
  };

  struct ResponseTimes {
    int64_t remaining_times = 1;
    bool unlimited = false;
  };

  RequestMatcher request;
  boost::optional<ResponseAction> action = boost::none;
  boost::optional<ResponseTimes> times = boost::none;
};

void to_json(const Expectation& x, boost::json::value& value);
void to_json(const Expectation::QueryStringParameter& x,
             boost::json::value& value);
void to_json(const Expectation::RequestMatcher& x, boost::json::value& value);
void to_json(const Expectation::BinaryResponse& x, boost::json::value& value);
void to_json(const Expectation::ResponseDelay& x, boost::json::value& value);
void to_json(const Expectation::ResponseAction& x, boost::json::value& value);
void to_json(const Expectation::ResponseTimes& x, boost::json::value& value);

std::string serialize(const Expectation& object);

inline void to_json(const Expectation& x, boost::json::value& value) {
  value.emplace_object();
  serialize("httpRequest", x.request, value);

  if (x.action != boost::none) {
    serialize("httpResponse", x.action, value);
  }
  if (x.times != boost::none) {
    serialize("times", x.times, value);
  }
}

inline void to_json(const Expectation::QueryStringParameter& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("name", x.name, value);
  serialize("values", x.values, value);
}

inline void to_json(const Expectation::RequestMatcher& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("path", x.path, value);
  serialize("method", x.method, value);
  serialize("queryStringParameters", x.query_string_parameters, value);
}

inline void to_json(const Expectation::BinaryResponse& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("type", x.type, value);
  serialize("base64Bytes", x.base64_string, value);
}

inline void to_json(const Expectation::ResponseDelay& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("timeUnit", x.time_unit, value);
  serialize("value", x.value, value);
}

inline void to_json(const Expectation::ResponseAction& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("statusCode", x.status_code, value);

  if (x.body.type() == typeid(std::string)) {
    serialize("body", boost::any_cast<std::string>(x.body), value);
  } else if (x.body.type() == typeid(Expectation::BinaryResponse)) {
    serialize("body", boost::any_cast<Expectation::BinaryResponse>(x.body),
              value);
  }

  if (x.delay) {
    serialize("delay", x.delay.get(), value);
  }
}

inline void to_json(const Expectation::ResponseTimes& x,
                    boost::json::value& value) {
  value.emplace_object();
  serialize("remainingTimes", x.remaining_times, value);
  serialize("unlimited", x.unlimited, value);
}

inline std::string serialize(const Expectation& object) {
  boost::json::value doc;
  doc.emplace_object();
  to_json(object, doc);

  return boost::json::serialize(doc);
};

}  // namespace mockserver
