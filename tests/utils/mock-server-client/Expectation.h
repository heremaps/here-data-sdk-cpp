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

#pragma once

#include <string>

#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/any.hpp>
#include <boost/optional.hpp>

namespace mockserver {

struct Expectation {
  struct RequestMatcher {
    boost::optional<std::string> path = boost::none;
    boost::optional<std::string> method = boost::none;
  };

  struct ResponseAction {
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
void to_json(const Expectation::RequestMatcher& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::BinaryResponse& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::ResponseAction& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
void to_json(const Expectation::ResponseTimes& x, rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

std::string serialize(const Expectation& object);

inline void to_json(const Expectation& x, rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  olp::serializer::serialize("httpRequest", x.request, value, allocator);

  if (x.action != boost::none) {
    olp::serializer::serialize("httpResponse", x.action, value, allocator);
  }
  if (x.times != boost::none) {
    olp::serializer::serialize("times", x.times, value, allocator);
  }
}

inline void to_json(const Expectation::RequestMatcher& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  olp::serializer::serialize("path", x.path, value, allocator);
  olp::serializer::serialize("method", x.method, value, allocator);
}

inline void to_json(const Expectation::BinaryResponse& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  olp::serializer::serialize("type", x.type, value, allocator);
  olp::serializer::serialize("base64Bytes", x.base64_string, value, allocator);
}

inline void to_json(const Expectation::ResponseAction& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  olp::serializer::serialize("statusCode", x.status_code, value, allocator);

  if (x.body.type() == typeid(std::string)) {
    olp::serializer::serialize("body", boost::any_cast<std::string>(x.body),
                               value, allocator);
  } else if (x.body.type() == typeid(Expectation::BinaryResponse)) {
    olp::serializer::serialize(
        "body", boost::any_cast<Expectation::BinaryResponse>(x.body), value,
        allocator);
  }
}

inline void to_json(const Expectation::ResponseTimes& x,
                    rapidjson::Value& value,
                    rapidjson::Document::AllocatorType& allocator) {
  value.SetObject();
  olp::serializer::serialize("remainingTimes", x.remaining_times, value,
                             allocator);
  olp::serializer::serialize("unlimited", x.unlimited, value, allocator);
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
