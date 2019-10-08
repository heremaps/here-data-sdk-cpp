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

#include "gtest/gtest.h"

#include <olp/dataservice/read/VolatileLayerClient.h>

namespace {
using namespace olp::dataservice::read;
using namespace ::testing;
const std::string kCatalog = "hrn:here:data:::hereos-internal-test-v2";
const std::string kLayerId = "hype-test";
const auto kHRN = olp::client::HRN::FromString(kCatalog);

TEST(VolatileLayerClient, GetData) {
  olp::client::OlpClientSettings settings;

  VolatileLayerClient client(kHRN, kLayerId, settings);

  DataRequest request;

  VolatileLayerClient::Callback<model::Data> cb;
  auto token = client.GetData(request, cb);
  token.cancel();
}
}  // namespace
