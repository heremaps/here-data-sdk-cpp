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

#include <string>

namespace olp {
namespace parser {
void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Coverage& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::IndexDefinition& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::IndexProperties& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Creator& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Owner& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Partitioning& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Schema& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::StreamProperties& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Encryption& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Volume& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Layer& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Notifications& x);

void from_json(const rapidjson::Value& value,
               olp::dataservice::read::model::Catalog& x);
}  // namespace parser
}  // namespace olp
