/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include "Examples.h"

/**
  * @brief Dataservice cache example.
 *
 * Gets the partition data using the HERE Open Location Platform with mutable cache. 
 * Makes the mutable cache path
 * protected. Reads the same data from the protected cache.
 * @param access_key Your access key ID and access key secret.
 * @param The HERE Resource Name (HRN) of the catalog from which you want to read data.
 * @return 0 if data was published successfully.
 */
int RunExampleProtectedCache(const AccessKey& access_key, const std::string& catalog);
