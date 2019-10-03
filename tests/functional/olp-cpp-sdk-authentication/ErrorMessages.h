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

static const std::string kErrorAccountNotFoundMessage =
    "No account found for given account Id.";

static const std::string kErrorBlacklistedPassword = "Black listed password.";

static const std::string kErrorFieldsMessage = "Received invalid data.";

static const std::string kErrorIllegalEmail = "Illegal email.";

static const std::string kErrorNoContent = "No Content";

static const std::string kErrorOk = "OK";

static const std::string kErrorPreconditionCreatedMessage = "Created";

static const std::string kErrorPreconditionFailedMessage = "Precondition Failed";

static const std::string kErrorRefreshFailedMessage = "Invalid accessToken.";

static const std::string kErrorSignUpCreated = "Created";

static const std::string kErrorUnauthorizedMessage =
    "Signature mismatch. Authorization signature or client credential is "
    "wrong.";
