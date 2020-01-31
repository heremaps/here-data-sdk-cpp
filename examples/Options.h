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

const Option kExampleOption{"-e", "--example",
                            "Run example [read, write, cache]."};

const Option kKeyIdOption{"-i", "--key_id", "Here key ID to access OLP."};

const Option kKeySecretOption{"-s", "--key_secret",
                              "Here secret key to access OLP."};

const Option kCatalogOption{"-c", "--catalog",
                            "Catalog HRN (HERE Resource Name)."};

const Option kLayerIdOption{"-l", "--layer_id",
                            "The layer ID inside the catalog where you want to "
                            "publish data to(required for write example)."};

const Option kAllOption{"-a", "--all", "Run all examples."};

}  // namespace tools
