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

#include "Options.h"
#include "ProtectedCacheExample.h"
#include "ReadExample.h"
#include "WriteExample.h"
#include "StreamLayerReadExample.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

bool IsMatch(const std::string& name, const tools::Option& option) {
  return name == option.short_name || name == option.long_name;
}

enum Examples : int {
  read_example = 0b1,
  write_example = 0b10,
  cache_example = 0b100,
  read_stream_example = 0b1000,
  all_examples =
      read_example | write_example | cache_example | read_stream_example
};

constexpr auto usage =
    "usuage is \n -a,--all : run all examples \n "
    "-e,--example[=read|read_stream|write|cache]  \n\tRun "
    "example\n -i,--key-id \n\there.access.key.id \n -s, --key-secret "
    "\n\there.access.key.secret \n"
    " -c, --catalog \n\tCatalog HRN (HERE Resource Name). \n"
    " -v,--catalog-version \n\tThe version of the catalog from which you wan to"
    "get data(used in read example, optional). \n"
    " -l, --layer-id \n\tThe layer ID inside the catalog where you want to "
    "publish data to(required for write example). \n"
    " -t,--type-of-subscription[=serial|parallel] \n\tType of subscription  "
    "(used for read_stream test). If not set, used serial subscription. \n"
    " -h,--help \n\tShow usage \nFor instructions on how to get the access key "
    "ID and access key secret, see "
    "the [Get "
    "Credentials](https://developer.here.com/olp/documentation/access-control/"
    "user-guide/topics/get-credentials.html) section in the Terms and "
    "Permissions User Guide.";

int RequiredArgumentError(const tools::Option& arg) {
  std::cout << "option requires an argument -- '" << arg.short_name << '\''
            << " [" << arg.long_name << "] " << arg.description << std::endl;
  return 0;
}
int ParseArguments(const int argc, char** argv, AccessKey& access_key,
                   std::string& catalog,
                   boost::optional<int64_t>& catalog_version,
                   std::string& layer_id,
                   olp::dataservice::read::SubscribeRequest::SubscriptionMode&
                       subscription_mode) {
  int examples_to_run = 0;

  const std::vector<std::string> arguments(argv + 1, argv + argc);
  auto it = arguments.begin();

  while (it != arguments.end()) {
    if (IsMatch(*it, tools::kHelpOption)) {
      std::cout << usage << std::endl;
      return 0;
    }

    if (IsMatch(*it, tools::kKeyIdOption)) {
      // Here access key ID.
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kKeyIdOption);
      }
      access_key.id = *it;
    } else if (IsMatch(*it, tools::kKeySecretOption)) {
      // Here access key secret.
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kKeySecretOption);
      }
      access_key.secret = *it;
    } else if (IsMatch(*it, tools::kExampleOption)) {
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kExampleOption);
      }

      if (*it == "read") {
        examples_to_run = Examples::read_example;
      } else if (*it == "write") {
        examples_to_run = Examples::write_example;
      } else if (*it == "cache") {
        examples_to_run = Examples::cache_example;
      } else if (*it == "read_stream") {
        examples_to_run = Examples::read_stream_example;
            } else {
              std::cout << "Example was not found. Please use values:read, "
                           "write, cache, read_stream"
                        << std::endl;
              return 0;
      }

    } else if (IsMatch(*it, tools::kCatalogOption)) {
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kCatalogOption);
      }
      catalog = *it;
    } else if (IsMatch(*it, tools::kCatalogVersionOption)) {
      ++it;
      catalog_version = 0;
      std::stringstream ss(*it);
      ss >> *catalog_version;
      if (ss.fail() || !ss.eof()) {
        std::cout << "invalid catalog version value -- '" << *it
                  << "', but int64 is expected." << std::endl;
        catalog_version = boost::none;
      }
    } else if (IsMatch(*it, tools::kLayerIdOption)) {
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kLayerIdOption);
      }
      layer_id = *it;
    } else if (IsMatch(*it, tools::kSubscriptionTypeOption)) {
      if (++it == arguments.end()) {
        return RequiredArgumentError(tools::kSubscriptionTypeOption);
      }
      if (*it == "serial") {
        subscription_mode =
            olp::dataservice::read::SubscribeRequest::SubscriptionMode::kSerial;
      } else if (*it == "parallel") {
        subscription_mode = olp::dataservice::read::SubscribeRequest::
            SubscriptionMode::kParallel;
      } else {
        std::cout << "Could not parse subscription type. Allowed types are: "
                     "serial, parallel. Will be used default value serial."
                  << std::endl;
      }
    } else if (IsMatch(*it, tools::kAllOption)) {
      examples_to_run = Examples::all_examples;
    } else {
      fprintf(stderr, usage);
    }
    ++it;
  }

  if (examples_to_run == 0) {
    std::cout << "Please specify command line arguments." << std::endl;
    std::cout << usage << std::endl;
  }

  return examples_to_run;
}

int RunExamples(const AccessKey& access_key, int examples_to_run,
                const std::string& catalog,
                const boost::optional<int64_t>& catalog_version,
                const std::string& layer_id,
                olp::dataservice::read::SubscribeRequest::SubscriptionMode
                    subscription_mode) {
  if (examples_to_run & Examples::read_example) {
    std::cout << "Read Example" << std::endl;
    if (RunExampleRead(access_key, catalog, catalog_version)) {
      std::cout << "Read Example failed" << std::endl;
      return -1;
    }
  }

  if (examples_to_run & Examples::write_example) {
    std::cout << "Write example" << std::endl;
    if (RunExampleWrite(access_key, catalog, layer_id)) {
      std::cout << "Write Example failed" << std::endl;
      return -1;
    }
  }

  if (examples_to_run & Examples::cache_example) {
    std::cout << "Protected cache example" << std::endl;
    if (RunExampleProtectedCache(access_key, catalog)) {
      std::cout << "Protected cache Example failed" << std::endl;
      return -1;
    }
  }

  if (examples_to_run & Examples::read_stream_example) {
    std::cout << "Stream layer read example" << std::endl;
    if (RunStreamLayerExampleRead(access_key, catalog, layer_id,
                                  subscription_mode)) {
      std::cout << "Stream layer read example failed" << std::endl;
      return -1;
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  AccessKey access_key{};  // You can specify your here.access.key.id 
                           // and here.access.key.secret
  std::string catalog;     // the HRN of the catalog to which you to publish data
  std::string layer_id;    // the of the layer inside the catalog to which you
                           // want to publish data
  boost::optional<int64_t> catalog_version;  // version of the catalog.
  auto subscription_mode =
      olp::dataservice::read::SubscribeRequest::SubscriptionMode::
          kSerial;  // subscription mode for read stream layer example
  int examples_to_run =
      ParseArguments(argc, argv, access_key, catalog, catalog_version, layer_id,
                     subscription_mode);
  if (examples_to_run == 0) {
    return 0;
  }

  if (access_key.id.empty() || access_key.secret.empty()) {
    std::cout << "Please specify your access key ID and access key secret. For "
                 "more information use -h [--help]"
              << std::endl;
  }

  if (catalog.empty()) {
    std::cout << "Please specify catalog. For more information use -h [--help]"
              << std::endl;
  }

  if (((examples_to_run & Examples::write_example) ||
       (examples_to_run & Examples::read_stream_example)) &&
      (layer_id.empty())) {
    std::cout << "Please specify layer_id for write or read stream layer "
                 "example. For "
                 "more information use -h [--help]"
              << std::endl;
  }

  return RunExamples(access_key, examples_to_run, catalog, catalog_version,
                     layer_id, subscription_mode);
}
