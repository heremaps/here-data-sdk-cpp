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

#include "DecisionClientImpl.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>

#include <olp/authentication/AuthenticationSettings.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/PendingRequests.h>

#include "Common.h"
#include "Constants.h"

namespace {
const std::string kDecisionEndpoint = "/decision/authorize";
const std::string kApplicationJson = "application/json";
// JSON fields
constexpr auto kServiceId = "serviceId";
constexpr auto kActions = "actions";
constexpr auto kAction = "action";
constexpr auto kResource = "resource";
constexpr auto kDiagnostics = "diagnostics";
constexpr auto kOperator = "operator";

using namespace olp::authentication;

std::shared_ptr<std::vector<std::uint8_t>> generateBody(
    AuthorizeRequest properties) {
  rapidjson::StringBuffer data;
  rapidjson::Writer<rapidjson::StringBuffer> writer(data);
  writer.StartObject();
  writer.Key(kServiceId);
  writer.String(properties.GetServiceId().c_str());
  writer.Key(kActions);
  writer.StartArray();
  for (const auto& action : properties.GetActions()) {
    writer.StartObject();
    writer.Key(kAction);
    writer.String(action.first.c_str());
    if (!action.second.empty()) {
      writer.Key(kResource);
      writer.String(action.second.c_str());
    }
    writer.EndObject();
  }

  writer.EndArray();
  writer.Key(kDiagnostics);
  writer.Bool(properties.GetDiagnostics());
  // default value is 'and', ignore parameter if operator type is 'and'
  if (properties.GetOperatorType() ==
      AuthorizeRequest::DecisionOperatorType::kOr) {
    writer.Key(kOperator);
    writer.String("or");
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char>>(content,
                                                      content + data.GetSize());
}

AuthorizeResult ParceResult(rapidjson::Document& doc) {
  AuthorizeResult result;

  auto getPermition = [](const char* str) -> DecisionType {
    if (strcmp(str, "allow") == 0) {
      return DecisionType::kAllow;
    }
    return DecisionType::kDeny;
  };

  if (doc.HasMember(Constants::IDENTITY)) {
    auto uris = doc[Constants::IDENTITY].GetObject();

    if (uris.HasMember(Constants::CLIENT_ID)) {
      result.SetClientId(uris[Constants::CLIENT_ID].GetString());
    }
  }

  if (doc.HasMember(Constants::DECISION)) {
    result.SetDecision(getPermition(doc[Constants::DECISION].GetString()));
  }

  // get diagnostics if avialible
  std::vector<ActionResult> results;
  if (doc.HasMember(Constants::DIAGNOSTICS) &&
      doc[Constants::DIAGNOSTICS].IsArray()) {
    const auto& array = doc[Constants::DIAGNOSTICS].GetArray();
    for (auto& element : array) {
      ActionResult action;
      if (element.HasMember(Constants::DECISION)) {
        action.SetDecision(
            getPermition(element[Constants::DECISION].GetString()));
        // get permisions if avialible
        if (element.HasMember(Constants::PERMITIONS) &&
            element[Constants::PERMITIONS].IsArray()) {
          std::vector<ActionResult::Permisions> permitions;
          const auto& permitionsArray =
              element[Constants::PERMITIONS].GetArray();
          for (auto& permitionElement : permitionsArray) {
            ActionResult::Permisions permition;
            if (permitionElement.HasMember(Constants::ACTION)) {
              permition.first = permitionElement[Constants::ACTION].GetString();
            }
            if (permitionElement.HasMember(Constants::DECISION)) {
              permition.second = getPermition(
                  permitionElement[Constants::DECISION].GetString());
            }
            permitions.push_back(std::move(permition));
          }

          action.SetPermitions(std::move(permitions));
        }
      }
      results.push_back(std::move(action));
    }

    result.SetActionResults(std::move(results));
  }
  return result;
}
}  // namespace

namespace olp {

namespace authentication {

DecisionClientImpl::DecisionClientImpl(client::OlpClientSettings settings)
    : settings_(std::move(settings)),
      pending_requests_(std::make_shared<client::PendingRequests>()) {}

client::CancellationToken DecisionClientImpl::GetDecision(
    AuthorizeRequest request, AuthorizeCallback callback) {
  using ResponseType = client::ApiResponse<AuthorizeResult, client::ApiError>;

  auto task = [=](client::CancellationContext context) -> ResponseType {
    if (!settings_.network_request_handler) {
      return client::ApiError({static_cast<int>(http::ErrorCode::IO_ERROR),
                               "Can not send request while offline"});
    }

    client::OlpClient client;
    client.SetBaseUrl(kHereAccountProductionUrl);

    client.SetSettings(settings_);
    auto http_result =
        client.CallApi(kDecisionEndpoint, "POST", {}, {}, {},
                       generateBody(request), kApplicationJson, context);

    rapidjson::Document document;
    rapidjson::IStreamWrapper stream(http_result.response);
    document.ParseStream(stream);
    if (http_result.status != http::HttpStatusCode::OK) {
      // HttpResult response can be error message or valid json with it.
      std::string msg = http_result.response.str();
      if (!document.HasParseError() && document.HasMember(Constants::MESSAGE)) {
        msg = document[Constants::MESSAGE].GetString();
      }
      return client::ApiError({http_result.status, msg});
    }
    return ParceResult(document);
  };

  // wrap_callback needed to convert client::ApiError into AuthenticationError.
  auto wrap_callback = [callback](ResponseType response) {
    if (!response.IsSuccessful()) {
      const auto& error = response.GetError();
      callback({{error.GetHttpStatusCode(), error.GetMessage()}});
      return;
    }

    callback(response.MoveResult());
  };

  return AddTask(settings_.task_scheduler, pending_requests_, std::move(task),
                 std::move(wrap_callback));
}

client::CancellableFuture<AuthorizeResponse> DecisionClientImpl::GetDecision(
    AuthorizeRequest request) {
  auto promise = std::make_shared<std::promise<AuthorizeResponse>>();
  auto cancel_token =
      GetDecision(std::move(request), [promise](AuthorizeResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<AuthorizeResponse>(std::move(cancel_token),
                                                      std::move(promise));
}

}  // namespace authentication
}  // namespace olp
