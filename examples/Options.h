/*
 * Copyright (C) 2020-2026 HERE Europe B.V.
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
#include <vector>

namespace tools {
/// A command line option.
struct Option {
  std::string short_name;
  std::string long_name;
  std::string description;
};

const Option kHelpOption{"-h", "--help", "Print the help message and exit."};

const Option kExampleOption{
    "-e", "--example",
    "Run example [=read|read_stream|write|cache|mtls-authentication]."};

const Option kKeyIdOption{"-i", "--key-id", "Here key ID to access OLP."};

const Option kKeySecretOption{"-s", "--key-secret",
                              "Here secret key to access OLP."};

const Option kMtlsCertOption{
    "--cert", "--mtls-cert",
    "Path to the client certificate PEM file (required for the "
    "mtls-authentication example)."};

const Option kMtlsKeyOption{
    "--key", "--mtls-key",
    "Path to the client private key PEM file (required for the "
    "mtls-authentication example)."};

const Option kMtlsCaOption{
    "--ca", "--mtls-ca",
    "Path to a CA certificate PEM file (optional, used for the "
    "mtls-authentication example)."};

const Option kMtlsScopeOption{
    "--scope", "--mtls-scope",
    "Project HRN scope to request (optional, used for the "
    "mtls-authentication example)."};

const Option kCatalogOption{"-c", "--catalog",
                            "Catalog HRN (HERE Resource Name)."};

const Option kCatalogVersionOption{
    "-v", "--catalog-version",
    "The version of the catalog from which you wan to "
    "get data(used in read example, optional)."};

const Option kLayerIdOption{"-l", "--layer-id",
                            "The layer ID inside the catalog where you want to "
                            "publish data to(required for write example)."};

const Option kSubscriptionTypeOption{
    "-t", "--type-of-subscription",
    "[Optional] Type of subscription [=serial|parallel] (used for read_stream "
    "example). The default option is serial."};

const Option kAllOption{"-a", "--all", "Run all examples."};

}  // namespace tools
