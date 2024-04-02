/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "olp/core/client/HRN.h"

#include <cstring>

#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include "Tokenizer.h"

namespace {
constexpr auto kLogTag = "HRN";
constexpr auto kDataTag = "data";
constexpr auto kPipelineTag = "pipeline";
constexpr auto kSchemaTag = "schema";
constexpr auto kHrnTag = "hrn:";
constexpr char kSeparator = ':';
}  // namespace

namespace olp {
namespace client {

std::string HRN::ToString() const {
  std::string ret;

  switch (service_) {
    case ServiceType::Data: {
      ret.reserve(std::strlen(kHrnTag) + std::strlen(kDataTag) +
                  5 * sizeof(kSeparator) + partition_.size() + region_.size() +
                  account_.size() + catalog_id_.size() + layer_id_.size());
      ret.append(kHrnTag)
          .append(partition_)
          .append(1, kSeparator)
          .append(kDataTag)
          .append(1, kSeparator)
          .append(region_)
          .append(1, kSeparator)
          .append(account_)
          .append(1, kSeparator)
          .append(catalog_id_);
      if (!layer_id_.empty()) {
        ret.append(1, kSeparator).append(layer_id_);
      }
      break;
    }
    case ServiceType::Schema: {
      ret.reserve(std::strlen(kHrnTag) + std::strlen(kSchemaTag) +
                  6 * sizeof(kSeparator) + partition_.size() + region_.size() +
                  account_.size() + group_id_.size() + schema_name_.size() +
                  version_.size());
      ret.append(kHrnTag)
          .append(partition_)
          .append(1, kSeparator)
          .append(kSchemaTag)
          .append(1, kSeparator)
          .append(region_)
          .append(1, kSeparator)
          .append(account_)
          .append(1, kSeparator)
          .append(group_id_)
          .append(1, kSeparator)
          .append(schema_name_)
          .append(1, kSeparator)
          .append(version_);
      break;
    }
    case ServiceType::Pipeline: {
      ret.reserve(std::strlen(kHrnTag) + std::strlen(kPipelineTag) +
                  4 * sizeof(kSeparator) + partition_.size() + region_.size() +
                  account_.size() + pipeline_id_.size());
      ret.append(kHrnTag)
          .append(partition_)
          .append(1, kSeparator)
          .append(kPipelineTag)
          .append(1, kSeparator)
          .append(region_)
          .append(1, kSeparator)
          .append(account_)
          .append(1, kSeparator)
          .append(pipeline_id_);
      break;
    }
    default: {
      ret.reserve(std::strlen(kHrnTag) + 4 * sizeof(kSeparator) +
                  partition_.size() + region_.size() + account_.size());
      ret.append(kHrnTag)
          .append(partition_)
          .append(1, kSeparator)
          .append(1, kSeparator)
          .append(region_)
          .append(1, kSeparator)
          .append(account_)
          .append(1, kSeparator);
      break;
    }
  }

  return ret;
}

std::string HRN::ToCatalogHRNString() const {
  std::string ret;
  if (service_ != ServiceType::Data) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "ToCatalogHRNString: ServiceType != Data");
    return ret;
  }
  ret.reserve(std::strlen(kHrnTag) + std::strlen(kDataTag) +
              4 * sizeof(kSeparator) + partition_.size() + region_.size() +
              account_.size() + catalog_id_.size());
  ret.append(kHrnTag)
      .append(partition_)
      .append(1, kSeparator)
      .append(kDataTag)
      .append(1, kSeparator)
      .append(region_)
      .append(1, kSeparator)
      .append(account_)
      .append(1, kSeparator)
      .append(catalog_id_);
  return ret;
}

HRN::HRN(const std::string& input) {
  Tokenizer tokenizer(input, kSeparator);

  // Must start with "hrn:"
  if (!tokenizer.HasNext()) {
    return;
  }

  const auto protocol = tokenizer.Next();
  if (protocol != "hrn") {
    return;
  }

  if (tokenizer.HasNext()) {
    partition_ = tokenizer.Next();
  }

  if (tokenizer.HasNext()) {
    auto service_str = tokenizer.Next();
    if (service_str == kDataTag) {
      service_ = ServiceType::Data;
    } else if (service_str == kSchemaTag) {
      service_ = ServiceType::Schema;
    } else if (service_str == kPipelineTag) {
      service_ = ServiceType::Pipeline;
    } else {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Constructor: invalid service=%s",
                            service_str.c_str());
      return;
    }
  }

  if (tokenizer.HasNext()) {
    region_ = tokenizer.Next();
  }

  if (tokenizer.HasNext()) {
    account_ = tokenizer.Next();
  }

  switch (service_) {
    case ServiceType::Data: {
      if (tokenizer.HasNext()) {
        catalog_id_ = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        layer_id_ = tokenizer.Tail();
      }
      break;
    }
    case ServiceType::Schema: {
      if (tokenizer.HasNext()) {
        group_id_ = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        schema_name_ = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        version_ = tokenizer.Tail();
      }
      break;
    }
    case ServiceType::Pipeline: {
      if (tokenizer.HasNext()) {
        pipeline_id_ = tokenizer.Tail();
      }
      break;
    }
    default:
      break;
  }
}

bool HRN::operator==(const HRN& rhs) const {
  // Common sections need to match for all types
  if (partition_ != rhs.partition_ || service_ != rhs.service_ ||
      region_ != rhs.region_ || account_ != rhs.account_) {
    return false;
  }

  switch (service_) {
    case ServiceType::Data: {
      return (catalog_id_ == rhs.catalog_id_ && layer_id_ == rhs.layer_id_);
    }
    case ServiceType::Schema: {
      return (group_id_ == rhs.group_id_ && schema_name_ == rhs.schema_name_ &&
              version_ == rhs.version_);
    }
    case ServiceType::Pipeline: {
      return (pipeline_id_ == rhs.pipeline_id_);
    }
    default: { return false; }
  }
}

bool HRN::operator!=(const HRN& rhs) const { return !operator==(rhs); }

bool HRN::IsNull() const {
  switch (service_) {
    case ServiceType::Data:
      // Note: region, account, layerId fields are optional.
      return partition_.empty() || catalog_id_.empty();
    case ServiceType::Schema:
      // Note: region, account fields are optional.
      return partition_.empty() || group_id_.empty() || schema_name_.empty() ||
             version_.empty();
    case ServiceType::Pipeline:
      // Note: region, account fields are optional.
      return partition_.empty() || pipeline_id_.empty();
    default:
      return true;
  }
}

HRN::operator bool() const { return !IsNull(); }

HRN HRN::FromString(const std::string& input) { return HRN(input); }

std::unique_ptr<HRN> HRN::UniqueFromString(const std::string& input) {
  return std::make_unique<HRN>(input);
}

}  // namespace client
}  // namespace olp
