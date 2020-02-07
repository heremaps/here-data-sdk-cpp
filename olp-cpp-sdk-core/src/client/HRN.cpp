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

#include "olp/core/client/HRN.h"

#include <sstream>

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
  std::ostringstream ret;
  const auto generic_part =
      kSeparator + region + kSeparator + account + kSeparator;
  ret << kHrnTag << partition << kSeparator;

  switch (service) {
    case ServiceType::Data: {
      ret << kDataTag << generic_part << catalogId;
      if (!layerId.empty()) {
        ret << kSeparator << layerId;
      }
      break;
    }
    case ServiceType::Schema: {
      ret << kSchemaTag << generic_part << groupId << kSeparator << schemaName
          << kSeparator << version;
      break;
    }
    case ServiceType::Pipeline: {
      ret << kPipelineTag << generic_part << pipelineId;
      break;
    }
    default: {
      ret << generic_part;
      break;
    }
  }

  return ret.str();
}

std::string HRN::ToCatalogHRNString() const {
  if (service != ServiceType::Data) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "ToCatalogHRNString: ServiceType != Data");
    return {};
  }

  return kHrnTag + partition + kSeparator + kDataTag + kSeparator + region +
         kSeparator + account + kSeparator + catalogId;
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
    partition = tokenizer.Next();
  }

  if (tokenizer.HasNext()) {
    auto service_str = tokenizer.Next();
    if (service_str == kDataTag) {
      service = ServiceType::Data;
    } else if (service_str == kSchemaTag) {
      service = ServiceType::Schema;
    } else if (service_str == kPipelineTag) {
      service = ServiceType::Pipeline;
    } else {
      OLP_SDK_LOG_WARNING_F(kLogTag, "Constructor: invalid service=%s",
                            service_str.c_str());
      return;
    }
  }

  if (tokenizer.HasNext()) {
    region = tokenizer.Next();
  }

  if (tokenizer.HasNext()) {
    account = tokenizer.Next();
  }

  switch (service) {
    case ServiceType::Data: {
      if (tokenizer.HasNext()) {
        catalogId = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        layerId = tokenizer.Tail();
      }
      break;
    }
    case ServiceType::Schema: {
      if (tokenizer.HasNext()) {
        groupId = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        schemaName = tokenizer.Next();
      }
      if (tokenizer.HasNext()) {
        version = tokenizer.Tail();
      }
      break;
    }
    case ServiceType::Pipeline: {
      if (tokenizer.HasNext()) {
        pipelineId = tokenizer.Tail();
      }
      break;
    }
    default:
      break;
  }
}

bool HRN::operator==(const HRN& rhs) const {
  // Common sections need to match for all types
  if (partition != rhs.partition || service != rhs.service ||
      region != rhs.region || account != rhs.account) {
    return false;
  }

  switch (service) {
    case ServiceType::Data: {
      return (catalogId == rhs.catalogId && layerId == rhs.layerId);
    }
    case ServiceType::Schema: {
      return (groupId == rhs.groupId && schemaName == rhs.schemaName &&
              version == rhs.version);
    }
    case ServiceType::Pipeline: {
      return (pipelineId == rhs.pipelineId);
    }
    default: { return false; }
  }
}

bool HRN::operator!=(const HRN& rhs) const { return !operator==(rhs); }

bool HRN::IsNull() const {
  switch (service) {
    case ServiceType::Data:
      // Note: region, account, layerId fields are optional.
      return partition.empty() || catalogId.empty();
    case ServiceType::Schema:
      // Note: region, account fields are optional.
      return partition.empty() || groupId.empty() || schemaName.empty() ||
             version.empty();
    case ServiceType::Pipeline:
      // Note: region, account fields are optional.
      return partition.empty() || pipelineId.empty();
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
