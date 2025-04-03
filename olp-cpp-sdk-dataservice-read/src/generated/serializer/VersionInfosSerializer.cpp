/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include "VersionInfosSerializer.h"

#include <olp/core/generated/serializer/SerializerWrapper.h>

namespace olp {
namespace serializer {

void to_json(const dataservice::read::model::VersionDependency& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("hrn", x.GetHrn(), object);
  serialize("version", x.GetVersion(), object);
  serialize("direct", x.GetDirect(), object);
}

void to_json(const dataservice::read::model::VersionInfo& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("dependencies", x.GetDependencies(), object);
  serialize("timestamp", x.GetTimestamp(), object);
  serialize("version", x.GetVersion(), object);
  serialize("partitionCounts", x.GetPartitionCounts(), object);
}

void to_json(const dataservice::read::model::VersionInfos& x,
             boost::json::value& value) {
  auto& object = value.emplace_object();
  serialize("versions", x.GetVersions(), object);
}
}  // namespace serializer
}  // namespace olp
