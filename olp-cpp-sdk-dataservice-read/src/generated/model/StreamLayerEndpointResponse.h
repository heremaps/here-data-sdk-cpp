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

namespace olp {
namespace dataservice {
namespace read {
namespace model {

class BootstrapServer {
 public:
  BootstrapServer() = default;
  virtual ~BootstrapServer() = default;

  const std::string& GetHostname() const { return m_Hostname; }
  void SetHostname(const std::string& value) { m_Hostname = value; }

  int32_t GetPort() const { return m_Port; }
  void SetPort(int32_t value) { m_Port = value; }

 private:
  std::string m_Hostname;
  int32_t m_Port;
};

/**
 * @brief Information to use to connect to the stream layer.
 */
class StreamLayerEndpointResponse {
 public:
  StreamLayerEndpointResponse() = default;
  virtual ~StreamLayerEndpointResponse() = default;

  const std::vector<std::string>& GetKafkaProtocolVersion() {
    return kafka_protocol_version_;
  }
  void SetKafkaProtocolVersion(const std::vector<std::string>& value) {
    kafka_protocol_version_ = value;
  }

  const std::string& GetTopic() const { return topic_; }
  void SetTopic(const std::string& value) { topic_ = value; }

  const std::string& GetClientId() const { return client_id_; }
  void SetClientId(const std::string& value) { client_id_ = value; }

  const std::string& GetConsumerGroupPrefix() const {
    return consumer_group_prefix_;
  }
  void SetConsumerGroupPrefix(const std::string& value) {
    consumer_group_prefix_ = value;
  }

  const std::vector<BootstrapServer>& GetBootstrapServers() const {
    return bootstrap_servers_;
  }
  void SetBootstrapServers(const std::vector<BootstrapServer>& value) {
    bootstrap_servers_ = value;
  }

  const std::vector<BootstrapServer>& GetBootstrapServersInternal() const {
    return bootstrap_servers_internal_;
  }
  void SetBootstrapServersInternal(const std::vector<BootstrapServer>& value) {
    bootstrap_servers_internal_ = value;
  }

 private:
  std::vector<std::string> kafka_protocol_version_;

  std::string topic_;

  std::string client_id_;

  /**
   * @brief If present, this should be used as the prefix of `group.id` [Kafka
   * Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs).
   */
  std::string consumer_group_prefix_;

  std::vector<BootstrapServer> bootstrap_servers_;

  std::vector<BootstrapServer> bootstrap_servers_internal_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
