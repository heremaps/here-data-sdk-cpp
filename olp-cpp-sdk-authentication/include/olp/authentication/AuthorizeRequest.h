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
 * request context against the HERE Service.
 *
 * Collects all permissions associated with the authenticated user or
 * application, requested service ID, and requested contract ID. For each
 * action-resource pair in the request, this class determines an individual
 * policy decision: DENY or ALLOW.
 *
 * * @note: If the contract ID is not provided in the request, one of the
 * following happens:
 * * If you have permissions for a single contract ID associated with the
 * requested service ID, the system automatically determines the contract ID.
 * * If you have permissions for multiple contract IDs, the 'Contract Required'
 * error is returned.
 */
class AUTHENTICATION_API AuthorizeRequest final {
 public:
  /**
   * @brief The type alias for the action pair.
   *
   * The first parameter is the type of action.
   * The second one is optional and represents the resource.
   *
   * @note Each action-resource pair in the request has an individual policy
   * decision.
   */
  using Action = std::pair<std::string, std::string>;

  /**
   * @brief The type alias for the vector of actions.
   */
  using Actions = std::vector<Action>;

  /**
   * @brief Determines the overall policy decision based on individual decisions
   * for each action.
   *
   * @note If the operator is 'or' and ANY actions have an individual policy
   * decision of ALLOW, the overall policy decision returned
   * in the response is ALLOW.
   * If the operator is 'and' or missing (defaults to 'and'),
   * one of the following algorithms is applied:
   * * If ANY actions have an individual policy decision of DENY,
   *  the overall policy decision returned in the response is DENY.
   * * If ALL actions have an individual policy decision of ALLOW,
   * the overall policy decision returned in the response is ALLOW.
   */
  enum class DecisionOperatorType { kAnd, kOr };

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
   * @brief Gets the contract ID.
   *
   * @note: If the contract ID is not provided in the request, one of the
   * following happens:
   * * If you have permissions for a single contract ID associated with the
   * requested service ID, the system automatically determines the contract ID.
   * * If you have permissions for multiple contract IDs, the 'Contract
   * Required' error is returned.
   *
   * @return The contract ID or an error.
   */
  inline const boost::optional<std::string>& GetContractId() const {
    return contract_id_;
  }

  /**
   * @brief Sets the contract ID.
   *
   * @see `GetContractId` for information on the contract ID.
   *
   * @param contract_id Your contract ID.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithContractId(
      boost::optional<std::string> contract_id) {
    contract_id_ = std::move(contract_id);
    return *this;
  }

  /**
   * @brief Sets the contract ID.
   *
   * @see `GetContractId` for information on the contract ID.
   *
   * @param contract_id Your contract ID.
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
   * @return The vector of the actions (action-resource pairs).
   */
  inline const Actions& GetActions() const { return actions_; }

  /**
   * @brief Adds the action-resource pair.
   *
   * @param[in] action The action that is used to determine an individual policy
   * decision.
   * @param[in] resource The resource that is used to request the decision for
   * the action.
   *
   * @return A reference to the updated `DecisionRequest` instance.
   */
  inline AuthorizeRequest& WithAction(std::string action,
                                      std::string resource = "") {
    actions_.emplace_back(std::move(action), std::move(resource));
    return *this;
  }

  /**
   * @brief Gets the operator type.
   *
   * If the operator type is not set, the 'and' type is used in the request.

   * @see `DecisionApiOperatorType` for more information.
   *
   * @return The operator type.
   */
  inline DecisionOperatorType GetOperatorType() const { return operator_type_; }

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
      DecisionOperatorType operator_type) {
    operator_type_ = operator_type;
    return *this;
  }

  /**
   * @brief Gets the diagnostics flag.
   *
   * @return The diagnostics flag.
   */
  inline bool GetDiagnostics() const { return diagnostics_; }

  /**
   * @brief Sets the diagnostics flag for the request.
   *
   * @param diagnostics The flag used to turn on or off the diagnostic
   * information in the response.
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
  DecisionOperatorType operator_type_{DecisionOperatorType::kAnd};
  bool diagnostics_{false};
};

}  // namespace authentication
}  // namespace olp
