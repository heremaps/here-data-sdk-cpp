/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include "SignOutResultImpl.h"

#include "Constants.h"
#include "olp/core/http/HttpStatusCode.h"

namespace olp {
namespace authentication {

SignOutResultImpl::SignOutResultImpl() noexcept
    : SignOutResultImpl(http::HttpStatusCode::SERVICE_UNAVAILABLE,
                        Constants::ERROR_HTTP_SERVICE_UNAVAILABLE) {}

SignOutResultImpl::SignOutResultImpl(
    int status, std::string error,
    std::shared_ptr<rapidjson::Document> json_document) noexcept
    : BaseResult(status, std::move(error), json_document) {}

SignOutResultImpl::~SignOutResultImpl() = default;

}  // namespace authentication
}  // namespace olp
