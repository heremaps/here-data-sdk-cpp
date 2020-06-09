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

#include <rapidjson/document.h>

#include "olp/dataservice/read/model/VersionInfos.h"

namespace olp {
namespace serializer {

void to_json(const dataservice::read::model::VersionDependency& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::VersionInfo& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::VersionInfos& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);
}  // namespace serializer
}  // namespace olp
