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

#include <initializer_list>
#include <string>
#include <vector>

#include <boost/optional.hpp>
#include <olp/dataservice/read/DataServiceReadApi.h>

namespace olp {
namespace dataservice {
namespace read {

/**
 * @brief Represents a consumer configuration entry of one stream layer.
 *
 * The accepted variable types are string, int32, or bool.
 */
class DATASERVICE_READ_API ConsumerOption final {
 public:
  /**
   * @brief Creates the `ConsumerOption` instance.
   *
   * @param key The string representation of the key.
   * @param value The string representation of the key value.
   */
  ConsumerOption(std::string key, std::string value)
      : key_{std::move(key)}, value_{std::move(value)} {}

  /**
   * @brief Creates the `ConsumerOption` instance.
   *
   * @param key The string representation of the key.
   * @param value The string representation of the key value.
   */
  ConsumerOption(std::string key, const char* value)
      : key_{std::move(key)}, value_{value} {}

  /**
   * @brief Creates the `ConsumerOption` instance.
   *
   * @param key The string representation of the key.
   * @param value The value of the key represented as an integer.
   */
  ConsumerOption(std::string key, int32_t value)
      : key_{std::move(key)}, value_{std::to_string(value)} {}

  /**
   * @brief Creates the `ConsumerOption` instance.
   *
   * @param key The string representation of the key.
   * @param value The value of the key represented as a boolean.
   */
  ConsumerOption(std::string key, bool value)
      : key_{std::move(key)}, value_{std::to_string(value)} {}

  /**
   * @brief Gets the key of the consumer configuration entry.
   *
   * @return The key of the consumer configuration entry.
   */
  const std::string& GetKey() const { return key_; };
  /**
   * @brief Gets the value of the consumer configuration entry.
   *
   * @return The value of the consumer configuration entry.
   */
  const std::string& GetValue() const { return value_; };

 private:
  std::string key_;
  std::string value_;
};

using ConsumerOptions = std::vector<ConsumerOption>;

/**
 * @brief Holds all Kafka consumer properties that should be passed to
 * the Stream API.
 */
class DATASERVICE_READ_API ConsumerProperties final {
 public:
  /**
   * @brief Creates the `ConsumerProperties` instance.
   *
   * @param properties The `ConsumerOptions` instances.
   */
  ConsumerProperties(ConsumerOptions properties)
      : properties_{std::move(properties)} {}

  /**
   * @brief Creates the `ConsumerProperties` instance.
   *
   * @param properties The list of the `ConsumerProperties` instances.
   */
  ConsumerProperties(std::initializer_list<ConsumerOption> properties)
      : properties_{std::move(properties)} {}

  /**
   * @brief Gets the `ConsumerProperties` instance.
   *
   * @return The `ConsumerProperties` instance.
   */
  const ConsumerOptions& GetProperties() const {
    return properties_;
  };

 private:
  ConsumerOptions properties_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
