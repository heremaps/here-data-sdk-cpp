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

#include "ArcGisAuthenticationOnlineTest.h"

#include "olp/core/http/HttpStatusCode.h"

using namespace olp::authentication;

// The ArcGIS refresh token will eventually expire. This requires a manual
// update of ARCGIS_TEST_accessToken in ArcGisTestUtils.cpp
TEST_F(ArcGisAuthenticationOnlineTest, SignInArcGis) {
  std::string email = GetEmail();
  std::cout << "Creating account for: " << email << std::endl;

  AuthenticationClient::SignInUserResponse response = SignInArcGis(email);
  EXPECT_EQ(olp::http::HttpStatusCode::CREATED,
            response.GetResult().GetStatus());
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_CODE,
            response.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_PRECONDITION_CREATED_MESSAGE,
            response.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response.GetResult().GetUserIdentifier().empty());
  EXPECT_FALSE(response.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_FALSE(response.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_FALSE(response.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_FALSE(response.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_FALSE(response.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::SignInUserResponse response2 = AcceptTerms(response);
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT,
            response2.GetResult().GetStatus());
  EXPECT_EQ(ERROR_NO_CONTENT, response2.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response2.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response2.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response2.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response2.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response2.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationClient::AuthenticationClient::SignInUserResponse response3 =
      SignInArcGis(email);
  EXPECT_EQ(olp::http::HttpStatusCode::OK, response3.GetResult().GetStatus());
  EXPECT_EQ(ERROR_OK, response3.GetResult().GetErrorResponse().message);
  EXPECT_FALSE(response3.GetResult().GetAccessToken().empty());
  EXPECT_FALSE(response3.GetResult().GetTokenType().empty());
  EXPECT_FALSE(response3.GetResult().GetRefreshToken().empty());
  EXPECT_FALSE(response3.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response3.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response3.GetResult().GetPrivatePolicyUrlJson().empty());

  AuthenticationUtils::DeleteUserResponse response4 =
      DeleteUser(response3.GetResult().GetAccessToken());
  EXPECT_EQ(olp::http::HttpStatusCode::NO_CONTENT, response4.status);
  EXPECT_STREQ(ERROR_NO_CONTENT.c_str(), response4.error.c_str());

  // SignIn with invalid token
  AuthenticationClient::SignInUserResponse response5 =
      SignInArcGis(email, "12345");
  EXPECT_EQ(olp::http::HttpStatusCode::UNAUTHORIZED,
            response5.GetResult().GetStatus());
  EXPECT_EQ(ERROR_ARCGIS_FAILED_CODE,
            response5.GetResult().GetErrorResponse().code);
  EXPECT_EQ(ERROR_ARCGIS_FAILED_MESSAGE,
            response5.GetResult().GetErrorResponse().message);
  EXPECT_TRUE(response5.GetResult().GetAccessToken().empty());
  EXPECT_TRUE(response5.GetResult().GetTokenType().empty());
  EXPECT_TRUE(response5.GetResult().GetRefreshToken().empty());
  EXPECT_TRUE(response5.GetResult().GetUserIdentifier().empty());
  EXPECT_TRUE(response5.GetResult().GetTermAcceptanceToken().empty());
  EXPECT_TRUE(response5.GetResult().GetTermsOfServiceUrl().empty());
  EXPECT_TRUE(response5.GetResult().GetTermsOfServiceUrlJson().empty());
  EXPECT_TRUE(response5.GetResult().GetPrivatePolicyUrl().empty());
  EXPECT_TRUE(response5.GetResult().GetPrivatePolicyUrlJson().empty());
}
