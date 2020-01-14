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
#include <vector>

#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief Host/port pairs to use for establishing the initial connection to the
 * Kafka cluster.
 */
class DATASERVICE_READ_API BootstrapServer final {
 public:
  BootstrapServer() = default;

  const std::string& GetHostname() const { return hostname_; }
  void SetHostname(std::string value) { hostname_ = std::move(value); }

  int32_t GetPort() const { return port_; }
  void SetPort(int32_t value) { port_ = value; }

 private:
  std::string hostname_;
  int32_t port_;
};

/**
 * @brief Information to use to connect to the stream layer. Corresponds to
 * StreamLayerEndpointResponse OLP model.
 */
class DATASERVICE_READ_API Endpoint final {
 public:
  Endpoint() = default;

  const std::vector<std::string>& GetKafkaProtocolVersion() {
    return kafka_protocol_version_;
  }
  void SetKafkaProtocolVersion(std::vector<std::string> value) {
    kafka_protocol_version_ = std::move(value);
  }

  const std::string& GetTopic() const { return topic_; }
  void SetTopic(std::string value) { topic_ = std::move(value); }

  const std::string& GetClientId() const { return client_id_; }
  void SetClientId(std::string value) { client_id_ = std::move(value); }

  const std::string& GetConsumerGroupPrefix() const {
    return consumer_group_prefix_;
  }
  void SetConsumerGroupPrefix(std::string value) {
    consumer_group_prefix_ = std::move(value);
  }

  const std::vector<BootstrapServer>& GetBootstrapServers() const {
    return bootstrap_servers_;
  }
  void SetBootstrapServers(std::vector<BootstrapServer> value) {
    bootstrap_servers_ = std::move(value);
  }

  const std::vector<BootstrapServer>& GetBootstrapServersInternal() const {
    return bootstrap_servers_internal_;
  }
  void SetBootstrapServersInternal(std::vector<BootstrapServer> value) {
    bootstrap_servers_internal_ = std::move(value);
  }

 private:
  std::vector<std::string> kafka_protocol_version_;

  std::string topic_;

  std::string client_id_;

  /// If present, this should be used as the prefix of `group.id` [Kafka
  /// Consumer
  /// properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs).
  std::string consumer_group_prefix_;

  std::vector<BootstrapServer> bootstrap_servers_;

  std::vector<BootstrapServer> bootstrap_servers_internal_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
