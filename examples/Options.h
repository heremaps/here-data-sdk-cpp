/* Copyright (C) 2020 HERE Global B.V. and its affiliate(s).
 * All rights reserved.
 *
 * This software and other materials contain proprietary information
 * controlled by HERE and are protected by applicable copyright legislation.
 * Any use and utilization of this software and other materials and
 * disclosure to any third parties is conditional upon having a separate
 * agreement with HERE for the access, use, utilization or disclosure of this
 * software. In the absence of such agreement, the use of the software is not
 * allowed.
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
    "-e", "--example", "Run example [read, write, cache, stream_layer_read]."};

const Option kKeyIdOption{"-i", "--key_id", "Here key ID to access OLP."};

const Option kKeySecretOption{"-s", "--key_secret",
                              "Here secret key to access OLP."};

const Option kCatalogOption{"-c", "--catalog",
                            "Catalog HRN (HERE Resource Name)."};

const Option kCatalogVersionOption{
    "-v", "--catalog_version",
    "The version of the catalog from which you wan to "
    "get data(used in read example, optional)."};

const Option kLayerIdOption{"-l", "--layer_id",
                            "The layer ID inside the catalog where you want to "
                            "publish data to(required for write example)."};

const Option kSubscriptionTypeOption{
    "-t", "--type_of_subscription",
    "Optional parameter type of subscription [serial, parallel] (used for "
    "stream_layer_read test). If not set, used serial subscription."};

const Option kAllOption{"-a", "--all", "Run all examples."};

}  // namespace tools
