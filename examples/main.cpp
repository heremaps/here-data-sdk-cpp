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

#include "ProtectedCacheExample.h"
#include "ReadExample.h"
#include "WriteExample.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

enum Examples : int {
  read_example = 0b1,
  write_example = 0b10,
  cache_example = 0b100,
  all_examples = read_example | write_example | cache_example
};

constexpr auto usage =
    "usuage is \n -a [--all] : run all examples \n -e [--example] : run "
    "example [read, write, cache] \n -i [--key_id] here.access.key.id \n -s "
    "[--key_secret] here.access.key.secret \n"
    " -c [--catalog] catalog HRN (HERE Resource Name). \n"
    " -l [--layer_id] t layer ID inside the catalog where you want to "
    "publish data to(required for write example). \n"
    " -h [--help]: show usage \n For "
    "instructions on how to get the access key ID and access key secret, see "
    "the [Get "
    "Credentials](https://developer.here.com/olp/documentation/access-control/"
    "user-guide/topics/get-credentials.html) section in the Terms and "
    "Permissions User Guide.";

int RunExamples(const AccessKey& access_key, int examples_to_run,
                const std::string& catalog, const std::string& layer_id) {
  if (examples_to_run & Examples::read_example) {
    std::cout << "Read Example" << std::endl;
    if (RunExampleRead(access_key, catalog)) {
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
  return 0;
}

int main(int argc, char** argv) {
  static struct option long_options[] = {
      {"example", required_argument, 0, 'e'},
      {"key_id", required_argument, 0, 'i'},
      {"key_secret", required_argument, 0, 's'},
      {"catalog", required_argument, 0, 'c'},
      {"layer_id", required_argument, 0, 'l'},
      {"all", no_argument, 0, 'a'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}};

  AccessKey access_key{};  // You can specify your here.access.key.id 
                           // and here.access.key.secret
  std::string catalog;     // the HRN of the catalog to which you to publish data
  std::string layer_id;    // the of the layer inside the catalog to which you
                           // want to publish data

  int examples_to_run = 0;
  int opt, option_index;
  while ((opt = getopt_long(argc, argv, "e:i:s:c:l:ah", long_options,
                            &option_index)) != EOF)
    switch (opt) {
      case 'c':
        catalog = optarg;
        break;
      case 'l':
        layer_id = optarg;
        break;
      case 'i':
        access_key.id = optarg;
        break;
      case 's':
        access_key.secret = optarg;
        break;
      case 'e':
        std::cout << "Run example " << optarg << std::endl;
        if (strcmp(optarg, "read") == 0)
          examples_to_run = Examples::read_example;
        else if (strcmp(optarg, "write") == 0)
          examples_to_run = Examples::write_example;
        else if (strcmp(optarg, "cache") == 0)
          examples_to_run = Examples::cache_example;
        else
          std::cout
              << "Example was not found. Please use values:read, write, cache"
              << std::endl;
        break;
      case 'a':
        examples_to_run = Examples::all_examples;
        break;
      case 'h':
        std::cout << usage << std::endl;
        return 0;
      case '?':
      default:
        fprintf(stderr, usage);
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

  if ((examples_to_run & Examples::write_example) && layer_id.empty()) {
    std::cout << "Please specify layer_id for write example. For "
                 "more information use -h [--help]"
              << std::endl;
  }

  return RunExamples(access_key, examples_to_run, catalog, layer_id);
}
