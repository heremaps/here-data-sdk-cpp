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

#include "testutils/CustomParameters.hpp"
#include <boost/program_options.hpp>
#include <vector>

const std::vector<std::string> param_list = {
  "service_id",
  "service_secret",
  "production_service_id",
  "production_service_secret",
  "facebook_access_token",
  "facebook_app_id",
  "google_client_id",
  "google_client_secret",
  "google_client_token",
  "arcgis_app_id",
  "arcgis_access_token",
  "integration_production_service_id",
  "integration_production_service_secret",
  "endpoint",
  "appid",
  "secret",
  "catalog",
  "layer",
  "layer2",
  "layer-sdii",
  "versioned-layer",
  "volatile-layer",
  "index-layer"
};

namespace po = boost::program_options;

CustomParameters::CustomParameters() : runTestsOnProductionServer_(false) {}

CustomParameters& CustomParameters::getInstance() {
  static CustomParameters c;
  return c;
}

void CustomParameters::init(int argc, char** argv) {
  if (argc == 0) {
    return;
  }

  boost::program_options::options_description desc;
  for (std::string param : param_list) {
    desc.add_options()(param.c_str(), po::value<std::string>(), "");
  }
  desc.add_options()("runProdTests", boost::program_options::bool_switch(
                                         &runTestsOnProductionServer_));

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv)
          .options(desc)
          .allow_unregistered()
          .run(),
      vm);
  boost::program_options::notify(vm);
  for (std::string param : param_list) {
    arguments_.emplace(param,
                       vm.count(param) ? vm[param].as<std::string>() : "");
  }
}

std::string CustomParameters::getArgument(const std::string& name) const {
  return arguments_.at(name);
}

bool CustomParameters::isUsingProductionServerForTest() const {
  return runTestsOnProductionServer_;
}
