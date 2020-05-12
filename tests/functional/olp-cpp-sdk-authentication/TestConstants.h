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

#pragma once

#include <string>

namespace olp {
namespace authentication {
static const std::string kHereAccountStagingURL =
    "https://stg.account.api.here.com";
static const std::string kHereAccountOauth2Endpoint = "/oauth2/token";

constexpr auto kMaxRetryCount = 3u;
constexpr auto kRetryDelayInSecs = 2u;

constexpr auto kLimitExpiry = 86400u;

constexpr auto kExpiryTime = 3600u;
constexpr auto kMaxExpiry = kExpiryTime + 30u;
constexpr auto kMinExpiry = kExpiryTime - 10u;

constexpr auto kCustomExpiryTime = 6000u;
constexpr auto kMaxCustomExpiry = kCustomExpiryTime + 30u;
constexpr auto kMinCustomExpiry = kCustomExpiryTime - 10u;

constexpr auto kExtendedExpiryTime = 2 * kExpiryTime;
constexpr auto kMaxExtendedExpiry = kExtendedExpiryTime + 30u;
constexpr auto kMinExtendedExpiry = kExtendedExpiryTime - 10u;

constexpr auto kMaxLimitExpiry = kLimitExpiry + 30u;
constexpr auto kMinLimitExpiry = kLimitExpiry - 10u;

static const std::string kTestUserName = "HA-UnitTestUser-Edge";
static const std::string kTestAppKeyId = "Ha-Integration-Test-App";
static const std::string kTestAppKeySecret = "ha-test-secret-1";

static const std::string kAccessToken = "access_token";

static const std::string kQuestionParam = "?";
static const std::string kEqualsParam = "=";
static const std::string kAndParam = "&";

// HTTP errors codes
constexpr auto kErrorAccountNotFoundCode = 401600;
constexpr auto kErrorBlacklistedPasswordCode = 400205;
constexpr auto kErrorFieldsCode = 400200;
constexpr auto kErrorIllegalEmailCode = 400240;
constexpr auto kErrorPreconditionCreatedCode = 412001;
constexpr auto kErrorPreconditionFailedCode = 412001;
constexpr auto kErrorRefreshFailedCode = 400601;
constexpr auto kErrorUnauthorizedCode = 401300;

// HTTP response messages
static const std::string kErrorAccountNotFoundMessage =
    "No account found for given account Id.";
static const std::string kErrorBlacklistedPassword = "Black listed password.";
static const std::string kErrorFieldsMessage = "Received invalid data.";
static const std::string kErrorIllegalEmail = "Illegal email.";
static const std::string kErrorNoContent = "No Content";
static const std::string kErrorOk = "OK";
static const std::string kErrorPreconditionCreatedMessage = "Created";
static const std::string kErrorPreconditionFailedMessage =
    "Precondition Failed";
static const std::string kErrorRefreshFailedMessage = "Invalid accessToken.";
static const std::string kErrorSignUpCreated = "Created";
static const std::string kErrorUnauthorizedMessage =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";


}  // namespace authentication
}  // namespace olp
