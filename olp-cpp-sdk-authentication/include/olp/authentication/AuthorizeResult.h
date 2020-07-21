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
 * @brief Represents a policy decision: DENY or ALLOW.
 */
enum class DecisionType { kAllow, kDeny };

/**
 * @brief Represents the permission with the action, policy decision, and
 * associated resource.
 */
class AUTHENTICATION_API Permission {
 public:
  /**
   * @brief Sets the action associated with the resource.
   *
   * @param The action to associate with.
   */
  void SetAction(std::string action) { action_ = std::move(action); }

  /**
   * @brief Gets the action that is associated with the resource.
   *
   * @return A string that represents the action.
   */
  const std::string& GetAction() const { return action_; }

  /**
   * @brief Sets the resource with which the action and decision are associated.
   *
   * @param The resource to associate with the decision and action.
   */
  void SetResource(std::string resource) { resource_ = std::move(resource); }

  /**
   * @brief Gets the resource with which the action and decision are associated.
   *
   * @return The resource name.
   */
  const std::string& GetResource() const { return resource_; }

  /**
   * @brief Sets the decision associated with the resource.
   *
   * @param The decision to associate with the resource.
   */
  void SetDecision(DecisionType decision) { decision_ = decision; }

  /**
   * @brief Gets the decision associated with the resource.
   *
   * @return The decision for the associated resource.
   */
  DecisionType GetDecision() const { return decision_; }

 private:
  std::string action_;
  std::string resource_;
  DecisionType decision_{DecisionType::kDeny};
};

/**
 * @brief Represents each action-resource pair response with an individual
 * policy decision for that action: DENY or ALLOW.
 *
 * @note The data is present only if the diagnostics flag is true for
 * `DecisionRequest`.
 */
class AUTHENTICATION_API ActionResult {
 public:
  /**
   * @brief Gets the overall policy decision.
   *
   * @see `DecisionOperatorType` for more information.
   *
   * @return The decision based on the operator type.
   */
  const DecisionType& GetDecision() const { return decision_; }

  /**
   * @brief Sets the policy decision.
   *
   * @param The policy decision.
   */
  void SetDecision(DecisionType decision) { decision_ = decision; }

  /**
   * @brief Gets the list of permissions that are evaluated against the
   action and resource.
   *
   * @note The algorithm of evaluating each permission in the set against the
   * action and resource:
   * * If the action matches the action in the permission, and the resource
   * matches the resource in the permission, consider the permission.
   * * If ANY considered permission for the action results in DENY, the
   * individual policy decision for the action is DENY.
   * * If ALL considered permissions for the action result in ALLOW, the
   * individual policy decision for the action is ALLOW.
   *
   * @return The list of permissions.
   */
  const std::vector<Permission>& GetPermissions() const { return permissions_; }

  /**
   * @brief Sets the list of permissions.
   *
   * @param permissions The vector of the action-decision pair.
   */
  void SetPermissions(std::vector<Permission> permissions) {
    permissions_ = std::move(permissions);
  }

 private:
  DecisionType decision_{DecisionType::kDeny};
  std::vector<Permission> permissions_;
};

/**
 * @brief Represents the policy decision for a given request context
 * against the HERE Service.
 *
 * Collects all permissions associated with the authenticated user or
 * application, requested service ID, and requested contract ID. For each
 * action-resource pair in the request, this class determines an individual
 * policy decision: DENY or ALLOW.
 */
class AUTHENTICATION_API AuthorizeResult {
 public:
  /**
   * @brief Gets the overall policy decision.
   *
   * @see `DecisionOperatorType` for more information.
   *
   * @return The decision based on the operator type.
   */
  const DecisionType& GetDecision() const { return policy_decision_; }

  /**
   * @brief Sets the overall policy decision.
   *
   * @param decision The policy decision.
   */
  void SetDecision(DecisionType decision) { policy_decision_ = decision; }

  /**
   * @brief Gets the client ID.
   *
   * @return The client ID.
   */
  const std::string& GetClientId() const { return client_id_; }

  /**
   * @brief Sets the client ID.
   *
   * @param The client ID.
   */
  void SetClientId(std::string client_id) { client_id_ = std::move(client_id); }

  /**
   * @brief Gets the list of results for each action.
   *
   * @note The data is present only if the diagnostics flag is true for
   * `DecisionRequest`.
   *
   * @return The list of action results.
   */
  const std::vector<ActionResult>& GetActionResults() const {
    return actions_results_;
  }

  /**
   * @brief Sets the list of results for each action.
   *
   * @param actions The vector of `ActionResult`.
   */
  void SetActionResults(std::vector<ActionResult> actions) {
    actions_results_ = std::move(actions);
  }

 private:
  DecisionType policy_decision_{DecisionType::kDeny};
  std::string client_id_;
  std::vector<ActionResult> actions_results_;
};

}  // namespace authentication
}  // namespace olp
