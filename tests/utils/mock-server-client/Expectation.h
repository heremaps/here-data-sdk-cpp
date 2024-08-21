/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

void to_json(const Expectation& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::QueryStringParameter& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::RequestMatcher& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::BinaryResponse& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::ResponseDelay& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::ResponseAction& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::ResponseTimes& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

std::string serialize(const Expectation& object);

inline void to_json(const Expectation& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("httpRequest", x.request, value, allocator);

  if (x.action != boost::none) {
    serialize("httpResponse", x.action, value, allocator);
  }
  if (x.times != boost::none) {
    serialize("times", x.times, value, allocator);
  }
}

inline void to_json(const Expectation::QueryStringParameter& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("name", x.name, value, allocator);
  serialize("values", x.values, value, allocator);
}

inline void to_json(const Expectation::RequestMatcher& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("path", x.path, value, allocator);
  serialize("method", x.method, value, allocator);
  serialize("queryStringParameters", x.query_string_parameters, value,
            allocator);
}

inline void to_json(const Expectation::BinaryResponse& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("type", x.type, value, allocator);
  serialize("base64Bytes", x.base64_string, value, allocator);
}

inline void to_json(const Expectation::ResponseDelay& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("timeUnit", x.time_unit, value, allocator);
  serialize("value", x.value, value, allocator);
}

inline void to_json(const Expectation::ResponseAction& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("statusCode", x.status_code, value, allocator);

  if (x.body.type() == typeid(std::string)) {
    serialize("body", boost::any_cast<std::string>(x.body), value, allocator);
  } else if (x.body.type() == typeid(Expectation::BinaryResponse)) {
    serialize("body", boost::any_cast<Expectation::BinaryResponse>(x.body),
              value, allocator);
  }

  if (x.delay) {
    serialize("delay", x.delay.get(), value, allocator);
  }
}

inline void to_json(const Expectation::ResponseTimes& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  serialize("remainingTimes", x.remaining_times, value, allocator);
  serialize("unlimited", x.unlimited, value, allocator);
}

inline std::string serialize(const Expectation& object) {
  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();

  doc.SetObject();
  to_json(object, doc, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
};

}  // namespace mockserver
