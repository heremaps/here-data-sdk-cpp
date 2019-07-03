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
  std::string genericPart = ":" + region + ":" + account + ":";
  ret << "hrn:" << partition << ":";
  switch (this->service) {
    case ServiceType::Data: {
      ret << "data" << genericPart << this->catalogId;
      if (!this->layerId.empty()) ret << ":" << this->layerId;
      break;
    }
    case ServiceType::Schema:
      ret << "schema" << genericPart << this->groupId << ":" << this->schemaName
          << ":" << this->version;
      break;
    case ServiceType::Pipeline:
      ret << "pipeline" << genericPart << this->pipelineId;
      break;
    default:
      ret << genericPart;
      break;
  }
  return ret.str();
}

std::string HRN::ToCatalogHRNString() const {
  if (this->service != ServiceType::Data) return std::string();

  return "hrn:" + partition + ":data:" + region + ":" + account + ":" +
         this->catalogId;
}

HRN::HRN(const std::string& input) {
  Tokenizer tokenizer(input, ':');

  // hrn must start with "hrn:"
  if (!tokenizer.hasNext()) return;

  std::string protocol = tokenizer.next();

  // as an exception, also allow passing in direct URLs. Eases the use of
  // command line tools that want to access custom catalog URLs.
  if (protocol == "http" || protocol == "https") {
    this->catalogId = input;
    this->partition = "catalog-url";
    return;
  }

  if (protocol != "hrn") return;

  // fill up the rest of the fields
  if (tokenizer.hasNext()) this->partition = tokenizer.next();
  if (tokenizer.hasNext()) {
    auto serviceStr = tokenizer.next();
    if (serviceStr.compare("data") == 0)
      this->service = ServiceType::Data;
    else if (serviceStr.compare("schema") == 0)
      this->service = ServiceType::Schema;
    else if (serviceStr.compare("pipeline") == 0)
      this->service = ServiceType::Pipeline;
    else {
      this->service = ServiceType::Unknown;
      assert(false);  // invalid service type
    }
  }
  if (tokenizer.hasNext()) this->region = tokenizer.next();
  if (tokenizer.hasNext()) this->account = tokenizer.next();

  switch (this->service) {
    case ServiceType::Data: {
      if (tokenizer.hasNext()) this->catalogId = tokenizer.next();
      if (tokenizer.hasNext()) this->layerId = tokenizer.tail();
      break;
    }
    case ServiceType::Schema: {
      if (tokenizer.hasNext()) this->groupId = tokenizer.next();
      if (tokenizer.hasNext()) this->schemaName = tokenizer.next();
      if (tokenizer.hasNext()) this->version = tokenizer.tail();
      break;
    }
    case ServiceType::Pipeline: {
      if (tokenizer.hasNext()) this->pipelineId = tokenizer.tail();
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
    switch (this->service) {
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
  bool ret = partition.empty() && region.empty() && account.empty();
  if (ret) {
    switch (this->service) {
      case ServiceType::Data: {
        ret = catalogId.empty() && layerId.empty();
        break;
      }
      case ServiceType::Schema:
        ret = this->groupId.empty() && this->schemaName.empty() &&
              this->version.empty();
        break;
      case ServiceType::Pipeline:
        ret = this->pipelineId.empty();
        break;
      default:
        break;
    }
  }

  return ret;
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
