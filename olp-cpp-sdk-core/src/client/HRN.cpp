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

#include <cassert>
#include <sstream>

#include "Tokenizer.h"

namespace olp {
namespace client {

std::string HRN::ToString() const {
  std::ostringstream ret;
  std::string generic_part = ":" + region + ":" + account + ":";
  ret << "hrn:" << partition << ":";
  switch (service) {
    case ServiceType::Data: {
      ret << "data" << generic_part << catalogId;
      if (!layerId.empty()) {
        ret << ":" << layerId;
      }
      break;
    }
    case ServiceType::Schema:
      ret << "schema" << generic_part << groupId << ":" << schemaName << ":"
          << version;
      break;
    case ServiceType::Pipeline:
      ret << "pipeline" << generic_part << pipelineId;
      break;
    default:
      ret << generic_part;
      break;
  }
  return ret.str();
}

std::string HRN::ToCatalogHRNString() const {
  if (service != ServiceType::Data) {
    return std::string();
  }

  return "hrn:" + partition + ":data:" + region + ":" + account + ":" +
         catalogId;
}

HRN::HRN(const std::string& input) {
  Tokenizer tokenizer(input, ':');

  // hrn must start with "hrn:"
  if (!tokenizer.HasNext()) {
    return;
  }

  std::string protocol = tokenizer.Next();

  if (protocol != "hrn") {
    return;
  }

  // fill up the rest of the fields
  if (tokenizer.HasNext()) {
    partition = tokenizer.Next();
  }

  if (tokenizer.HasNext()) {
    auto service_str = tokenizer.Next();
    if (service_str.compare("data") == 0) {
      service = ServiceType::Data;
    } else if (service_str.compare("schema") == 0) {
      service = ServiceType::Schema;
    } else if (service_str.compare("pipeline") == 0) {
      service = ServiceType::Pipeline;
    } else {
      service = ServiceType::Unknown;
      assert(false);  // invalid service type
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

bool HRN::operator==(const HRN& other) const {
  bool ret = (partition == other.partition && service == other.service &&
              region == other.region && account == other.account);

  if (ret) {
    switch (service) {
      case ServiceType::Data:
        ret = (catalogId == other.catalogId && layerId == other.layerId);
        break;
      case ServiceType::Schema:
        ret = (groupId == other.groupId && schemaName == other.schemaName &&
               version == other.version);
        break;
      case ServiceType::Pipeline:
        ret = (pipelineId == other.pipelineId);
        break;
      default:
        break;
    }
  }

  return ret;
}

bool HRN::operator!=(const HRN& other) const { return !operator==(other); }

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

HRN HRN::FromString(const std::string& input) {
  HRN result{input};
  return result;
}

std::unique_ptr<HRN> HRN::UniqueFromString(const std::string& input) {
  return std::unique_ptr<HRN>(new HRN(input));
}
}  // namespace client
}  // namespace olp
