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

#include <gtest/gtest.h>
#include <olp/core/porting/make_unique.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <chrono>
#include <iostream>
#include <thread>
#include "CommonTestUtils.h"
#include "testutils/CustomParameters.hpp"

using namespace olp::authentication;
using namespace rapidjson;

namespace {
const std::string HELP_OPTION("--help");
const std::string RUN_PRODUCTION_OPTION("--runProdTests");
const std::string GTEST_FILTER_OPTION("--gtest_filter=");
const char* GTEST_FILTER_PRODUCTION = "--gtest_filter=*Production*";
const char* CREDENTIALS = "--credentials=";

bool matches_option(const std::string& option, const char* argument) {
  return option.compare(0, option.size(), argument, option.size()) == 0;
}
}  // namespace

int main(int argc, char** argv) {
  std::vector<char*> tester_args;
  bool run_production = false;
  char* filter_option = nullptr;
  std::string credentials;

  // Check for credentials
  for (int i = 0; i < argc; ++i) {
    std::string param = argv[i];
    auto pos = param.find(CREDENTIALS);
    if (pos != std::string::npos) {
      credentials.append(param.substr(
          pos + strlen(CREDENTIALS), param.size() - strlen(CREDENTIALS) - pos));
      break;
    }
  }

  // Look for filters
  for (int i = 0; i < argc; ++i) {
    if (HELP_OPTION == argv[i]) {
      std::cout << "\n  " << RUN_PRODUCTION_OPTION
                << "\n    Run production tests only.";
      std::cout << "\n\n";

      // Do not return so gtest's help is also printed
      break;
    } else if (matches_option(RUN_PRODUCTION_OPTION, argv[i])) {
      run_production = true;

      // Do not pass this option to gtest
      break;
    } else if (matches_option(GTEST_FILTER_OPTION, argv[i])) {
      filter_option = argv[i];
      // The filter options will be added to the gtest args later
      break;
    } else {
      tester_args.push_back(argv[i]);
    }
  }

  if (run_production) {
    tester_args.push_back((char*)GTEST_FILTER_PRODUCTION);
  } else {
    if (filter_option) {
      tester_args.push_back(filter_option);
    }
  }

  tester_args.push_back(nullptr);

  int tester_argc = tester_args.size() - 1;
  char** tester_argv = tester_args.data();

  CustomParameters::getInstance().init(argc, argv);

  testing::InitGoogleTest(&tester_argc, tester_argv);

  int result = RUN_ALL_TESTS();

  // Wait for network stack to unwind
  std::this_thread::sleep_for(std::chrono::seconds(1));

  return result;
}
