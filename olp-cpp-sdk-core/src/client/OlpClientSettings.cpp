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

#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/client/HttpResponse.h"

namespace olp {
namespace client {
unsigned int DefaultBackdownPolicy(unsigned int milliseconds) {
  return milliseconds;
}

bool DefaultRetryCondition(const HttpResponse&) { return false; }

bool DefaultRetryConditionNew(int http_code) { return false; }

}  // namespace client
}  // namespace olp
