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

#pragma once

#include <memory>
#include <vector>

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {

/**
 * @brief Represents policy decision deny or allow.
 */
enum class DecisionType { kAllow, kDeny };

/**
 * @brief Represents each action/resource pair responce with individual policy
 * decision for that action DENY or ALLOW.
 *
 * @note Data present only if diagnostics flag is true for the DecisionRequest.
 */
class AUTHENTICATION_API ActionResult {
 public:
  ActionResult() = default;
  ~ActionResult() = default;
  /**
   * @brief Represents permition pair with action and policy decision.
   */
  using Permisions = std::pair<std::string, DecisionType>;
  /**
   * @brief Gets the overall policy decision.
   *
   * @see `DecisionApiOperatorType` for more information.
   *
   * @return The decision based on operator type.
   */
  const DecisionType& GetDecision() const { return decision_; }
  /**
   * @brief Sets the policy decision.
   *
   * @param The decision.
   */
  void SetDecision(DecisionType decision) { decision_ = decision; }
  /**
   * @brief Gets the list of permitions, which are used to evaluate against the
   action and resource.
   *
   * @note Algorithm of evaluating each permission in the set against the action
   and resource.
   * If the action matches the action in the permission and the resource
   matches* the resource in the permission, consider the permission
   * If ANY considered permission for the action has an effect of DENY, the
   individual policy decision for the action is DENY
   * If ALL considered permissions for the action have an effect of ALLOW, the
   individual policy decision for the action is ALLOW

   * @return The list permitions.
   */
  const std::vector<Permisions>& GetPermitions() const { return permisions_; }
  /**
   * @brief Sets the list of permitions.
   *
   * @param permitions vector of pair action/decision.
   */
  void SetPermitions(std::vector<Permisions> permisions) {
    permisions_ = std::move(permisions);
  }

 private:
  DecisionType decision_{DecisionType::kDeny};
  std::vector<Permisions> permisions_;
};

/**
 * @brief A model used to represent policy decision for a given request context
 * against a HERE Service.
 *
 * Collected permissions associated with the authenticated user or application,
 * requested serviceId, and requested contractId For each action/resource pair
 * in the request returned individual policy decision for that action DENY or
 * ALLOW.
 */
class AUTHENTICATION_API AuthorizeResult {
 public:
  AuthorizeResult() = default;
  ~AuthorizeResult() = default;

 private:
  DecisionType policy_decision_{DecisionType::kDeny};
  std::string client_id_;
  std::vector<ActionResult> actions_results_;

 public:
  /**
   * @brief Gets the overall policy decision.
   *
   * @see `DecisionApiOperatorType` for more information.
   *
   * @return The decision based on operator type.
   */
  const DecisionType& GetDecision() const { return policy_decision_; }
  /**
   * @brief Sets the overall policy decision.
   *
   * @param The decision.
   */
  void SetDecision(DecisionType decision) { policy_decision_ = decision; }
  /**
   * @brief Gets the client id.
   *
   * @return The client id.
   */
  const std::string& GetClientId() const { return client_id_; }
  /**
   * @brief Sets the client id.
   *
   * @param The client id.
   */
  void SetClientId(std::string client_id) { client_id_ = std::move(client_id); }
  /**
   * @brief Gets the list results for each action.
   *
   * @note Data present only if diagnostics flag is true for the
   * DecisionRequest.
   *
   * @return The list of action results.
   */
  const std::vector<ActionResult>& GetActionResults() const {
    return actions_results_;
  }
  /**
   * @brief Sets the list results for each action.
   *
   * @param actions vector of ActionResult.
   */
  void SetActionResults(std::vector<ActionResult> actions) {
    actions_results_ = std::move(actions);
  }
};

}  // namespace authentication
}  // namespace olp
