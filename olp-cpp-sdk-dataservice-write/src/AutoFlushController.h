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

#include <memory>

#include <olp/dataservice/write/FlushEventListener.h>
#include <olp/dataservice/write/StreamLayerClient.h>

namespace olp {
namespace dataservice {
namespace write {

class AutoFlushController {
 public:
  AutoFlushController(const FlushSettings& flush_settings);

  template <typename ClientImpl, typename FlushResponse>
  void Enable(std::shared_ptr<ClientImpl> client_impl,
              std::shared_ptr<FlushEventListener<FlushResponse>> listener);

  std::future<void> Disable();

  void NotifyQueueEventStart();
  void NotifyQueueEventComplete();
  void NotifyFlushEvent();

  // Implmentation base class
  class AutoFlushControllerImpl {
   public:
    virtual ~AutoFlushControllerImpl() {}

    virtual void Enable() {}
    virtual std::future<void> Disable() = 0;

    virtual void NotifyQueueEventStart() = 0;
    virtual void NotifyQueueEventComplete() = 0;
    virtual void NotifyFlushEvent() = 0;
  };

 private:
  FlushSettings flush_settings_;
  std::shared_ptr<AutoFlushControllerImpl> impl_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
