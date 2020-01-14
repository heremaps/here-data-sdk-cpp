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

#include "EndpointParser.h"

#include <olp/core/generated/parser/ParserWrapper.h>

namespace olp {
namespace parser {
using namespace olp::dataservice::read;

void from_json(const rapidjson::Value& value, model::BootstrapServer& x) {
  x.SetHostname(parse<std::string>(value, "hostname"));
  x.SetPort(parse<int32_t>(value, "port"));
}

void from_json(const rapidjson::Value& value, model::Endpoint& x) {
  x.SetKafkaProtocolVersion(
      parse<std::vector<std::string>>(value, "kafkaProtocolVersion"));
  x.SetTopic(parse<std::string>(value, "topic"));
  x.SetClientId(parse<std::string>(value, "clientId"));
  x.SetConsumerGroupPrefix(parse<std::string>(value, "consumerGroupPrefix"));
  x.SetBootstrapServers(
      parse<std::vector<model::BootstrapServer>>(value, "bootstrapServers"));
  x.SetBootstrapServersInternal(parse<std::vector<model::BootstrapServer>>(
      value, "bootstrapServersInternal"));
}

}  // namespace parser
}  // namespace olp
