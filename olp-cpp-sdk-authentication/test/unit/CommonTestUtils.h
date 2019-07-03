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

#include <memory>
#include <string>

namespace olp {
namespace authentication {
static const std::string HERE_ACCOUNT_STAGING_URL =
    "https://stg.account.api.here.com";
static const std::string HERE_ACCOUNT_OAUTH2_ENDPOINT = "/oauth2/token";

const unsigned int MAX_RETRY_COUNT = 3u;
const unsigned int RETRY_DELAY_SECS = 2u;

const static std::string TEST_USER_NAME = "HA-UnitTestUser-Edge";
const static std::string TEST_APP_KEY_ID = "Ha-Integration-Test-App";
const static std::string TEST_APP_KEY_SECRET = "ha-test-secret-1";

const static std::string ACCESS_TOKEN = "access_token";

const static std::string QUESTION_PARAM = "?";
const static std::string EQUALS_PARAM = "=";
const static std::string AND_PARAM = "&";

// HTTP errors
static const std::string ERROR_OK = "OK";
static const std::string ERROR_SIGNUP_CREATED = "Created";
static const std::string ERROR_SERVICE_UNAVAILABLE = "Service unavailable";
static const std::string ERROR_UNKNOWN = "Unknown error";
static const std::string ERROR_NO_CONTENT = "No Content";
static const int ERROR_FIEDS_CODE = 400200;
static const std::string ERROR_FIELDS_MESSAGE = "Received invalid data.";
static const int ERROR_PRECONDITION_FAILED_CODE = 412001;
static const std::string ERROR_PRECONDITION_FAILED_MESSAGE =
    "Precondition Failed";
static const int ERROR_PRECONDITION_CREATED_CODE = 412001;
static const std::string ERROR_PRECONDITION_CREATED_MESSAGE = "Created";
static const int ERROR_FB_FAILED_CODE = 400300;
static const std::string ERROR_FB_FAILED_MESSAGE = "Unexpected Facebook error.";
static const int ERROR_ARCGIS_FAILED_CODE = 400300;
static const std::string ERROR_ARCGIS_FAILED_MESSAGE = "Invalid token.";
static const int ERROR_REFRESH_FAILED_CODE = 400601;
static const std::string ERROR_REFRESH_FAILED_MESSAGE = "Invalid accessToken.";
static const std::string ERROR_UNAVAILABLE = "Service unavailable";
static const std::string ERROR_RESOURCE_EXHAUSTED =
    "Some resource has been exhausted";
static const std::string ERROR_OUT_OF_RANGE =
    "Operation was attempted past the valid range";
static const std::string ERROR_OPERATION_CANCELLED =
    "The operation was cancelled";
static const std::string ERROR_INTERNAL = "Internal server error";
static const std::string ERROR_MISSING_KEY =
    "The request does not have valid authentication credentials for the "
    "operation (key missing)";
static const std::string ERROR_MISSING_SECRET =
    "The request does not have valid authentication credentials for the "
    "operation (secret "
    "missing)";
static const std::string ERROR_PERMISSION_DENIED =
    "The caller does not have permission to execute the specified operation";
static const std::string ERROR_INVALID_ARGUMENT =
    "Client specified an invalid argument";
static const std::string ERROR_UNDEFINED = "";
static const int ERROR_BAD_REQUEST_CODE = 400002;
static const std::string ERROR_BAD_REQUEST_MESSAGE = "Invalid JSON.";
static const int ERROR_UNAUTHORIZED_CODE = 401300;
static const std::string ERROR_UNAUTHORIZED_MESSAGE =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";
static const int ERROR_NOT_FOUND_CODE = 404000;
static const std::string ERROR_USER_NOT_FOUND =
    "User for the given access token cannot be found.";
static const int ERROR_CONFLICT_CODE = 409100;
static const std::string ERROR_CONFLICT_MESSAGE =
    "A password account with the specified email address already exists.";
static const int ERROR_TOO_MANY_REQUESTS_CODE = 429002;
static const std::string ERROR_TOO_MANY_REQUESTS_MESSAGE =
    "Request blocked because too many requests were made. Please wait for a "
    "while before making a new request.";
static const int ERROR_INTERNAL_SERVER_CODE = 500203;
static const std::string ERROR_INTERNAL_SERVER_MESSAGE =
    "Missing Thing Encrypted Secret.";
static const int ERROR_ACCOUNT_NOT_FOUND_CODE = 401600;
static const std::string ERROR_ACCOUNT_NOT_FOUND_MESSAGE =
    "No account found for given account Id.";
static const int ERROR_ILLEGAL_LAST_NAME_CODE = 400203;
static const std::string ERROR_ILLEGAL_LAST_NAME = "Illegal last name.";
static const int ERROR_BLACKLISTED_PASSWORD_CODE = 400205;
static const std::string ERROR_BLACKLISTED_PASSWORD = "Black listed password.";
static const int ERROR_ILLEGAL_EMAIL_CODE = 400240;
static const std::string ERROR_ILLEGAL_EMAIL = "Illegal email.";
static const std::string ERROR_RESOLVE_PROXY = "Could not resolve proxy";

static const std::string PASSWORD = "password";
static const std::string LAST_NAME = "lastname";
static const std::string EMAIL = "email";

}  // namespace authentication
}  // namespace olp
