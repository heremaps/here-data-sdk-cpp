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

#include <string>

#include <boost/json/value.hpp>

#include <generated/model/Catalog.h>

namespace olp {
namespace parser {
void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Coverage& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::IndexDefinition& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::IndexProperties& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Creator& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Owner& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Partitioning& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Schema& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::StreamProperties& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Encryption& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Volume& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Layer& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Notifications& x);

void from_json(const boost::json::value& value,
               olp::dataservice::write::model::Catalog& x);
}  // namespace parser
}  // namespace olp
