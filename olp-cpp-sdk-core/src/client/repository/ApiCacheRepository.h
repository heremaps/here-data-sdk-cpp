/*
 * Copyright (C) 2020-2021 HERE Europe B.V.
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

#include <time.h>
#include <memory>
#include <string>

#include <olp/core/client/HRN.h>
#include <olp/core/porting/optional.hpp>

namespace olp {
namespace cache {
class KeyValueCache;
}  // namespace cache

namespace repository {

class ApiCacheRepository final {
 public:
  ApiCacheRepository(const client::HRN& hrn,
                     std::shared_ptr<cache::KeyValueCache> cache);

  ~ApiCacheRepository() = default;

  void Put(const std::string& service, const std::string& version,
           const std::string& url, porting::optional<time_t> expiry);

  porting::optional<std::string> Get(const std::string& service,
                                   const std::string& version);

 private:
  std::string hrn_;
  std::shared_ptr<cache::KeyValueCache> cache_;
};

}  // namespace repository
}  // namespace olp
