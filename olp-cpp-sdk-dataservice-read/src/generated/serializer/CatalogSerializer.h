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

#pragma once

#include <rapidjson/document.h>

#include "olp/dataservice/read/model/Catalog.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::read::model::Coverage& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::IndexDefinition& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::IndexProperties& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Creator& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Owner& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Partitioning& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Schema& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::StreamProperties& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Encryption& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Volume& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Layer& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Notifications& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

void to_json(const dataservice::read::model::Catalog& x,
             rapidjson::Value& value,
             rapidjson::Document::AllocatorType& allocator);

}  // namespace serializer
}  // namespace olp
