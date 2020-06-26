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

#include <gmock/gmock.h>
#include <olp/core/cache/KeyValueCache.h>

class CacheMock : public olp::cache::KeyValueCache {
 public:
  CacheMock();

  ~CacheMock() override;

  MOCK_METHOD(bool, Put,
              (const std::string&, const boost::any&,
               const olp::cache::Encoder&, time_t),
              (override));

  MOCK_METHOD(bool, Put,
              (const std::string&,
               const std::shared_ptr<std::vector<unsigned char>>,
               time_t expiry),
              (override));

  MOCK_METHOD(boost::any, Get, (const std::string&, const olp::cache::Decoder&),
              (override));

  MOCK_METHOD(std::shared_ptr<std::vector<unsigned char>>, Get,
              (const std::string&), (override));

  MOCK_METHOD(bool, Remove, (const std::string&), (override));

  MOCK_METHOD(bool, RemoveKeysWithPrefix, (const std::string&), (override));
};
