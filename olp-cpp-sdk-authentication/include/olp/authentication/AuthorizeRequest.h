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

#include <boost/optional.hpp>
#include <string>
#include <vector>

#include <olp/authentication/AuthenticationApi.h>

namespace olp {
namespace authentication {

/**
 * @brief Encapsulates the fields required to make a policy decision for a given
 * request context against a HERE Service.
 *
 * Collect all permissions associated with the authenticated user or
 * application, requested serviceId, and requested contractId For each
 * action/resource pair in the request, determine an individual policy decision
 * for that action: DENY, ALLOW.
 *
 * @note: If a contractId is not provided in the request, the system will
 * automatically determine the contractId if the user only has permissions for a
 * single contractId associated with the requested serviceId. If the user has
 * permissions for multiple contractId's, a 'Contract Required' error will be
 * returned.
 */
class AUTHENTICATION_API AuthorizeRequest final {
 public:
  /**
   * @brief Type alias for action pair. First is type of action, second is
   * recource, optional parameter.
   * @note For each action/resource pair in the request, individual policy
   * decision for that action will be determined.
   */
  using Action = std::pair<std::string, boost::optional<std::string>>;

  /**
   * @brief Type alias for vector of actions.
   */
  using Actions = std::vector<Action>;

  /**
   * @brief Determins the overall policy decision based on individual decisions
   * for each action.
   *
   * @note If operator is 'and' or missing(defaults to 'and') the next algorithm
   * applied. If ANY actions have an individual policy decision of DENY, the
   * overall policy decision returned in the response is DENY If ALL actions
   * have an individual policy decision of ALLOW, the overall policy decision
   * returned in the response is ALLOW If operator is operator is 'or' the next
   * algorithm applied. If ANY actions have an individual policy decision of
   * ALLOW, the overall policy decision returned in the response is ALLOW
   */
  enum class DecisionApiOperatorType { kAnd, kOr };

  /**
   * @brief Gets the ID of the requested service.
   *
   * @return The service ID.
   */
  inline const std::string& GetServiceId() const { return service_id_; }

  /**
   * @brief Sets the service ID.
   *
   * @param service_id The service ID.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithServiceId(std::string service_id) {
    service_id_ = std::move(service_id);
    return *this;
  }

  /**
   * @brief Get the contract id.
   *
   * @note If a contractId is not provided in the request, the system will
   * automatically determine the contractId if the user only has permissions for
   * a single contractId associated with the requested serviceId. If the user
   *  has permissions for multiple contractId's, a 'Contract Required' error
   * will be returned.
   *
   * @return The contract id.
   */
  inline const boost::optional<std::string>& GetContractId() const {
    return contract_id_;
  }

  /**
   * @brief Sets the contract id.
   *
   * @see `GetContractId` for information on the contract id.
   *
   * @param contract_id The contract id of the user.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithContractId(
      boost::optional<std::string> contract_id) {
    contract_id_ = std::move(contract_id);
    return *this;
  }

  /**
   * @brief Sets the contract id.
   *
   * @see `GetContractId` for information on the contract id.
   *
   * @param contract_id The contract id of the user.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithContractId(std::string contract_id) {
    contract_id_ = std::move(contract_id);
    return *this;
  }

  /**
   * @brief Gets all actions.
   *
   * @return The vector of the actions(action/resorce pairs).
   */
  inline const Actions& GetActions() const { return actions_; }

  /**
   * @brief Adds an action/resource pair.
   *
   * @param[in] action onv which need to determine an individual policy decision
   * for that action.
   * @param[in] resource on which will be requested decision for the action.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithAction(
      std::string action, boost::optional<std::string> resource = boost::none) {
    actions_.emplace_back(std::move(action), std::move(resource));
    return *this;
  }

  /**
   * @brief Gets the operator type.
   *
   * If operator type not set, type 'and' will be used in request
   * @see `DecisionApiOperatorType` for more information.
   *
   * @return The operator type.
   */
  inline DecisionApiOperatorType GetOperatorType()
      const {
    return operator_type_;
  }

  /**
   * @brief Sets the operator type for the request.
   *
   * @see `GetOperatorType()` for information on usage and format.
   *
   * @param operator_type The `DecisionApiOperatorType` enum.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithOperatorType(
      DecisionApiOperatorType operator_type) {
    operator_type_ = operator_type;
    return *this;
  }

  /**
   * @brief Gets diagnostics flag.
   *
   * @return diagnostics flag.
   */
  inline bool GetDiagnostics() const { return diagnostics_; }

  /**
   * @brief Sets the diagnostics flag for the request.
   *
   * @param diagnostics The flag to turn on or off diagnostic information in
   * responce.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithDiagnostics(bool diagnostics) {
    diagnostics_ = diagnostics;
    return *this;
  }

  /**
   * @brief Creates a readable format for the request.
   *
   * @return A string representation of the request.
   */
  std::string CreateKey() const;

 private:
  std::string service_id_;
  boost::optional<std::string> contract_id_;
  Actions actions_;
  DecisionApiOperatorType operator_type_{DecisionApiOperatorType::kAnd};
  bool diagnostics_{false};
};

}  // namespace authentication
}  // namespace olp
