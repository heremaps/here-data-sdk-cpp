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

#include <gtest/gtest.h>
#include <olp/dataservice/read/AuthorizeRequest.h>
#include <olp/dataservice/read/model/AuthorizeResult.h>

namespace {
using namespace olp::dataservice::read;

TEST(DecisionApiClientTest, AuthorizeRequestTest) {
  EXPECT_EQ(AuthorizeRequest().WithServiceId("ServiceId").GetServiceId(),
            "ServiceId");
  EXPECT_EQ(
      AuthorizeRequest().WithContractId("ContractId").GetContractId().get(),
      "ContractId");
  auto request = AuthorizeRequest().WithAction("action1").WithAction(
      "action2", std::string("hrn::test"));
  EXPECT_EQ(AuthorizeRequest().GetDiagnostics(), false);
  EXPECT_EQ(AuthorizeRequest().WithDiagnostics(true).GetDiagnostics(), true);
  EXPECT_EQ(request.GetActions().size(), 2);
  auto actions_it = request.GetActions().begin();
  EXPECT_EQ(actions_it->first, "action1");
  EXPECT_EQ(actions_it->second, boost::none);
  ++actions_it;
  EXPECT_EQ(actions_it->first, "action2");
  EXPECT_EQ(actions_it->second.get(), "hrn::test");
  EXPECT_EQ(request.GetOperatorType(), boost::none);
  request.WithOperatorType(AuthorizeRequest::DecisionApiOperatorType::OR);
  EXPECT_EQ(request.GetOperatorType().get(),
            AuthorizeRequest::DecisionApiOperatorType::OR);
}

TEST(DecisionApiClientTest, AuthorizeResponceTest) {
  EXPECT_EQ(model::AuthorizeResult().GetDecision(), model::DecisionType::DENY);
  EXPECT_EQ(model::ActionResult().GetDecision(), model::DecisionType::DENY);
  EXPECT_EQ(model::AuthorizeResult().GetClientId(), "");
  model::ActionResult action;
  action.SetDecision(model::DecisionType::ALLOW);
  action.SetPermitions({{"read", model::DecisionType::ALLOW}});
  model::AuthorizeResult decision;
  decision.SetActionResults({action});
  EXPECT_EQ(decision.GetActionResults().front().GetPermitions().front().first,
            "read");
  EXPECT_EQ(decision.GetActionResults().front().GetPermitions().front().second,
            model::DecisionType::ALLOW);
}

}  // namespace
