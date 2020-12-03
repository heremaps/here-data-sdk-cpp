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

#include <olp/authentication/AuthenticationError.h>
#include <olp/authentication/AuthorizeResult.h>
#include <olp/authentication/IntrospectAppResult.h>
#include <olp/authentication/UserAccountInfoResponse.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>

namespace olp {
namespace authentication {

/// The response template type.
template <typename ResultType>
using Response = client::ApiResponse<ResultType, AuthenticationError>;

/// The callback template type.
template <typename ResultType>
using Callback = std::function<void(Response<ResultType>)>;

/// Defines the callback signature when the user request is completed.
using IntrospectAppResponse = Response<IntrospectAppResult>;

/// Called when the user introspect app request is completed.
using IntrospectAppCallback = Callback<IntrospectAppResult>;

/// The decision API response alias.
using AuthorizeResponse = Response<AuthorizeResult>;

/// Called when the user decision request is completed.
using AuthorizeCallback = Callback<AuthorizeResult>;

/// The UserAccountInfoResponse response alias.
using UserAccountInfoResponse = Response<model::UserAccountInfoResponse>;

/// Called when the get user account request is completed.
using UserAccountInfoCallback = Callback<model::UserAccountInfoResponse>;
}  // namespace authentication
}  // namespace olp
