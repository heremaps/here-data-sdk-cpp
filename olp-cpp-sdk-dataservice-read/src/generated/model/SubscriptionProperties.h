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

#include <boost/optional.hpp>

namespace olp {
namespace dataservice {
namespace read {
namespace model {

/**
 * @brief This is the same as [Kafka Consumer
 * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
 * with some default values changed.
 */
class KafkaConsumerProperties {
 public:
  KafkaConsumerProperties() = default;
  virtual ~KafkaConsumerProperties() = default;

  const boost::optional<int32_t>& GetAutoCommitIntervalMs() const {
    return auto_commit_interval_ms_;
  }
  KafkaConsumerProperties& SetAutoCommitIntervalMs(int32_t value) {
    auto_commit_interval_ms_ = value;
    return *this;
  }

  const boost::optional<std::string>& GetAutoOffsetReset() const {
    return auto_offset_reset_;
  }
  KafkaConsumerProperties& SetAutoOffsetReset(const std::string& value) {
    auto_offset_reset_ = value;
    return *this;
  }

  const boost::optional<bool>& isEnableAutoCommit() const {
    return enable_auto_commit_;
  }
  KafkaConsumerProperties& SetEnableAutoCommit(bool value) {
    enable_auto_commit_ = value;
    return *this;
  }

  const boost::optional<int32_t>& GetFetchMaxBytes() const {
    return fetch_max_bytes_;
  }
  KafkaConsumerProperties& SetFetchMaxBytes(int32_t value) {
    fetch_max_bytes_ = value;
    return *this;
  }

  const boost::optional<int32_t>& GetFetchMaxWaitMs() const {
    return fetch_max_wait_ms_;
  }
  KafkaConsumerProperties& SetFetchMaxWaitMs(int32_t value) {
    fetch_max_wait_ms_ = value;
    return *this;
  }

  const boost::optional<int32_t>& GetFetchMinBytes() const {
    return fetch_min_bytes_;
  }
  KafkaConsumerProperties& SetFetchMinBytes(int32_t value) {
    fetch_min_bytes_ = value;
    return *this;
  }

  const boost::optional<std::string> GetGroupId() const { return group_id_; }
  KafkaConsumerProperties& SetGroupId(const std::string& value) {
    group_id_ = value;
    return *this;
  }

  const boost::optional<int32_t>& GetMaxPartitionFetchBytes() const {
    return max_partition_fetch_bytes_;
  }
  KafkaConsumerProperties& SetMaxPartitionFetchBytes(int32_t value) {
    max_partition_fetch_bytes_ = value;
    return *this;
  }

  const boost::optional<int32_t>& GetMaxPollRecords() const {
    return max_poll_records_;
  }
  KafkaConsumerProperties& SetMaxPollRecords(int32_t value) {
    max_poll_records_ = value;
    return *this;
  }

 private:
  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> auto_commit_interval_ms_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<std::string> auto_offset_reset_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<bool> enable_auto_commit_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> fetch_max_bytes_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> fetch_max_wait_ms_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> fetch_min_bytes_;

  /**
   * @brief For applications intending to read in parallel mode belonging to
   * same 'group.id' the values for 'catalogId, layerId, aid' should also be
   * same.
   */
  boost::optional<std::string> group_id_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> max_partition_fetch_bytes_;

  /**
   * @brief Refer to [Kafka Consumer
   * properties](https://kafka.apache.org/11/documentation.html#newconsumerconfigs)
   */
  boost::optional<int32_t> max_poll_records_;
};

class SubscriptionProperties {
 public:
  SubscriptionProperties() = default;
  virtual ~SubscriptionProperties() = default;

  const KafkaConsumerProperties& GetKafkaConsumerProperties() const {
    return kafka_consumer_properties_;
  }
  SubscriptionProperties& WithKafkaConsumerProperties(
      const KafkaConsumerProperties& value) {
    kafka_consumer_properties_ = value;
    return *this;
  }

 private:
  KafkaConsumerProperties kafka_consumer_properties_;
};

}  // namespace model
}  // namespace read
}  // namespace dataservice
}  // namespace olp
