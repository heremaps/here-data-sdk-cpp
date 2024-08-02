/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <chrono>
#include <memory>

#include <olp/core/client/ApiNoResult.h>
#include <olp/core/client/HRN.h>
#include <olp/dataservice/read/model/Data.h>
#include <boost/optional.hpp>

namespace olp {
namespace cache {
class KeyValueCache;
}
namespace dataservice {
namespace read {
namespace repository {

class DataCacheRepository final {
 public:
  DataCacheRepository(
      const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
      std::chrono::seconds default_expiry = std::chrono::seconds::max());

  ~DataCacheRepository() = default;

  client::ApiNoResponse Put(const model::Data& data,
                            const std::string& layer_id,
                            const std::string& data_handle);

  boost::optional<model::Data> Get(const std::string& layer_id,
                                   const std::string& data_handle);
  bool IsCached(const std::string& layer_id,
                const std::string& data_handle) const;

  void PromoteInCache(const std::string& layer_id,
                      const std::string& data_handle);

  client::ApiNoResponse Clear(const std::string& layer_id,
                              const std::string& data_handle);

 private:
  const std::string hrn_;
  std::shared_ptr<cache::KeyValueCache> cache_;
  time_t default_expiry_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
