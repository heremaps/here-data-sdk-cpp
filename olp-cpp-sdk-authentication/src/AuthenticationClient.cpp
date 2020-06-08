/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "olp/authentication/AuthenticationClient.h"

#include <chrono>
#include <iomanip>
#include <sstream>

#include "AuthenticationClientImpl.h"
#include "olp/authentication/AuthenticationError.h"
#include "olp/authentication/AuthorizeRequest.h"
#include "olp/core/client/ApiError.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/porting/make_unique.h"

namespace olp {
namespace authentication {

AuthenticationClient::AuthenticationClient(AuthenticationSettings settings)
    : impl_(std::make_unique<AuthenticationClientImpl>(std::move(settings))) {}

AuthenticationClient::~AuthenticationClient() = default;

client::CancellationToken AuthenticationClient::SignInClient(
    AuthenticationCredentials credentials, SignInProperties properties,
    SignInClientCallback callback) {
  return impl_->SignInClient(std::move(credentials), std::move(properties),
                             std::move(callback));
}

client::CancellationToken AuthenticationClient::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInHereUser(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::SignInFederated(
    AuthenticationCredentials credentials, std::string request_body,
    SignInUserCallback callback) {
  return impl_->SignInFederated(std::move(credentials), std::move(request_body),
                                std::move(callback));
}

client::CancellationToken AuthenticationClient::SignInFacebook(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(
      credentials, FederatedSignInType::FacebookSignIn, properties, callback);
}

client::CancellationToken AuthenticationClient::SignInGoogle(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(credentials, FederatedSignInType::GoogleSignIn,
                                properties, callback);
}

client::CancellationToken AuthenticationClient::SignInArcGis(
    const AuthenticationCredentials& credentials,
    const FederatedProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInFederated(credentials, FederatedSignInType::ArcgisSignIn,
                                properties, callback);
}

client::CancellationToken AuthenticationClient::SignInRefresh(
    const AuthenticationCredentials& credentials,
    const RefreshProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInRefresh(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::SignUpHereUser(
    const AuthenticationCredentials& credentials,
    const SignUpProperties& properties, const SignUpCallback& callback) {
  return impl_->SignUpHereUser(credentials, properties, callback);
}

client::CancellationToken AuthenticationClient::AcceptTerms(
    const AuthenticationCredentials& credentials,
    const std::string& reacceptance_token, const SignInUserCallback& callback) {
  return impl_->AcceptTerms(credentials, reacceptance_token, callback);
}

client::CancellationToken AuthenticationClient::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& user_access_token, const SignOutUserCallback& callback) {
  return impl_->SignOut(credentials, user_access_token, callback);
}

client::CancellationToken AuthenticationClient::IntrospectApp(
    std::string access_token, IntrospectAppCallback callback) {
  return impl_->IntrospectApp(std::move(access_token), std::move(callback));
}

client::CancellationToken AuthenticationClient::Authorize(
    std::string access_token, AuthorizeRequest request,
    AuthorizeCallback callback) {
  return impl_->Authorize(std::move(access_token), std::move(request),
                          std::move(callback));
}

}  // namespace authentication
}  // namespace olp
