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

#include <map>
#include <string>
#include <vector>

/**
 * Currently supported parameters and syntax
 *
 * --credentials=<s>
 * where <s> is the test application credentials in JSON format
 *
 * --runProdTests
 * when specified tests are run on production server
 * when not specified tests are run on staging server
 *
 */
class CustomParameters {
 public:
  static CustomParameters& getInstance();
  void init(int argc, char** argv);

  std::string getArgument(const std::string& name) const;
  bool isUsingProductionServerForTest() const;

 private:
  CustomParameters();
  std::map<std::string, std::string> arguments_;
  bool runTestsOnProductionServer_;
};
