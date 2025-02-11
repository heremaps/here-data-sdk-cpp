/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include <boost/json/value.hpp>

#include "olp/dataservice/read/model/Catalog.h"

namespace olp {
namespace serializer {
void to_json(const dataservice::read::model::Coverage& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::IndexDefinition& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::IndexProperties& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Creator& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Owner& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Partitioning& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Schema& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::StreamProperties& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Encryption& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Volume& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Layer& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Notifications& x,
             boost::json::value& value);

void to_json(const dataservice::read::model::Catalog& x,
             boost::json::value& value);

}  // namespace serializer
}  // namespace olp
