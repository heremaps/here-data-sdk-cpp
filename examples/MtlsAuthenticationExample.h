/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include "Examples.h"

/**
 * @brief mTLS authentication example. Presents a client X.509 certificate
 * during the TLS handshake with the HERE mTLS token endpoint to retrieve a
 * bearer token, then calls the HERE Discover Search API with that token.
 * @param cert_path Path to the client certificate PEM file.
 * @param key_path Path to the client private key PEM file.
 * @param ca_path (Optional) Path to a CA certificate PEM file. Empty if not
 * used.
 * @param scope (Optional) The project HRN scope to request. Empty if not
 * used.
 * @return 0 if the token was retrieved and the Discover API call succeeded.
 */
EXAMPLES_API
int RunExampleMtlsAuthentication(const std::string& cert_path,
                                 const std::string& key_path,
                                 const std::string& ca_path,
                                 const std::string& scope);
