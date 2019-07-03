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

#include "PublishDataRequestSerializer.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::write::model::PublishDataRequest& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator) {
  if (x.GetData()) {
    const auto& data = x.GetData().get();
    auto data_stringref = rapidjson::StringRef(
        reinterpret_cast<char*>(data->data()), data->size());
    value.AddMember("data", std::move(data_stringref), allocator);
  }

  value.AddMember("layerId", rapidjson::StringRef(x.GetLayerId().c_str()),
                  allocator);

  if (x.GetTraceId()) {
    value.AddMember("traceId",
                    rapidjson::StringRef(x.GetTraceId().get().c_str()),
                    allocator);
  }

  if (x.GetBillingTag()) {
    value.AddMember("billingTag",
                    rapidjson::StringRef(x.GetBillingTag().get().c_str()),
                    allocator);
  }

  if (x.GetChecksum()) {
    value.AddMember("checksum",
                    rapidjson::StringRef(x.GetChecksum().get().c_str()),
                    allocator);
  }
}

}  // namespace serializer

}  // namespace olp
