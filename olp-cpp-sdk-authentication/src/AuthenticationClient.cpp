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

#include "olp/authentication/AuthenticationClient.h"

#include <chrono>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Constants.h"
#include "Crypto.h"
#include "SignInResultImpl.h"
#include "SignInUserResultImpl.h"
#include "SignOutResultImpl.h"
#include "SignUpResultImpl.h"
#include "olp/authentication/AuthenticationError.h"
#include "olp/core/client/CancellationToken.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/network/HttpStatusCode.h"
#include "olp/core/network/Network.h"
#include "olp/core/network/NetworkRequest.h"
#include "olp/core/network/NetworkResponse.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/Atomic.h"
#include "olp/core/utils/Base64.h"
#include "olp/core/utils/LruCache.h"
#include "olp/core/utils/Url.h"

using namespace rapidjson;
using namespace std;
using namespace olp::network;
using namespace olp::thread;
using namespace olp::utils;

// Helper characters
static const std::string PARAM_ADD = "&";
static const std::string PARAM_COMMA = ",";
static const std::string PARAM_EQUALS = "=";
static const std::string PARAM_QUOTE = "\"";
static const char LINE_FEED = '\n';

// Tags
static const std::string APPLICATION_JSON = "application/json";
static const std::string OAUTH_POST = "POST";
static const std::string OAUTH_CONSUMER_KEY = "oauth_consumer_key";
static const std::string OAUTH_NONCE = "oauth_nonce";
static const std::string OAUTH_SIGNATURE = "oauth_signature";
static const std::string OAUTH_TIMESTAMP = "oauth_timestamp";
static const std::string OAUTH_VERSION = "oauth_version";
static const std::string OAUTH_SIGNATURE_METHOD = "oauth_signature_method";
static const std::string OAUTH_ENDPOINT = "/oauth2/token";
static const std::string SIGNOUT_ENDPOINT = "/logout";
static const std::string TERMS_ENDPOINT = "/terms";
static const std::string USER_ENDPOINT = "/user";

// JSON fields
static const char* COUNTRY_CODE = "countryCode";
static const char* DATE_OF_BIRTH = "dob";
static const char* EMAIL = "email";
static const char* FIRST_NAME = "firstname";
static const char* GRANT_TYPE = "grantType";
static const char* INVITE_TOKEN = "inviteToken";
static const char* LANGUAGE = "language";
static const char* LAST_NAME = "lastname";
static const char* MARKETING_ENABLED = "marketingEnabled";
static const char* PASSWORD = "password";
static const char* PHONE_NUMBER = "phoneNumber";
static const char* REALM = "realm";
static const char* TERMS_REACCEPTANCE_TOKEN = "termsReacceptanceToken";

static const char* CLIENT_GRANT_TYPE = "client_credentials";
static const char* USER_GRANT_TYPE = "password";
static const char* FACEBOOK_GRANT_TYPE = "facebook";
static const char* GOOGLE_GRANT_TYPE = "google";
static const char* ARCGIS_GRANT_TYPE = "arcgis";
static const char* REFRESH_GRANT_TYPE = "refresh_token";

// Values
static const std::string VERSION = "1.0";
static const std::string HMAC = "HMAC-SHA256";

namespace olp {
namespace authentication {

enum class FederatedSignInType { FacebookSignIn, GoogleSignIn, ArcgisSignIn };

class AuthenticationClient::Impl final {
 public:
  class ScopedNetwork final {
   public:
    ScopedNetwork(const NetworkConfig& config) : network() {
      network.Start(config);
    }

    Network& getNetwork() { return network; }

   private:
    Network network;
  };

  using Scopednetwork_ptr = std::shared_ptr<ScopedNetwork>;
  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl(const std::string& authentication_server_url, size_t token_cache_limit);

  /**
   * @brief Sign in with client credentials
   * @param credentials Client credentials obtained when registering application
   *                    on HERE developer portal.
   * @param callback  The method to be called when request is completed.
   * @param expiresIn The number of seconds until the new access token expires.
   * @return CancellationToken that can be used to cancel the request.
   */
  client::CancellationToken SignInClient(
      const AuthenticationCredentials& credentials,
      const std::chrono::seconds& expires_in,
      const AuthenticationClient::SignInClientCallback& callback);

  client::CancellationToken SignInHereUser(
      const AuthenticationCredentials& credentials,
      const UserProperties& properties,
      const AuthenticationClient::SignInUserCallback& callback);

  client::CancellationToken SignInFederated(
      const AuthenticationCredentials& credentials,
      const FederatedSignInType& type,
      const AuthenticationClient::FederatedProperties& properties,
      const AuthenticationClient::SignInUserCallback& callback);

  client::CancellationToken SignInRefresh(
      const AuthenticationCredentials& credentials,
      const RefreshProperties& properties, const SignInUserCallback& callback);

  client::CancellationToken AcceptTerms(
      const AuthenticationCredentials& credentials,
      const std::string& reacceptanceToken, const SignInUserCallback& callback);

  client::CancellationToken SignUpHereUser(
      const AuthenticationCredentials& credentials,
      const AuthenticationClient::SignUpProperties& properties,
      const AuthenticationClient::SignUpCallback& callback);

  client::CancellationToken SignOut(
      const AuthenticationCredentials& credentials,
      const std::string& userAccessToken, const SignOutUserCallback& callback);

  bool SetNetworkProxySettings(const NetworkProxySettings& proxy_settings);

 private:
  std::string base64Encode(const std::vector<uint8_t>& vector);

  std::string generateHeader(const AuthenticationCredentials& credentials,
                             const std::string& url);
  std::string generateBearerHeader(const std::string& bearer_token);

  std::shared_ptr<std::vector<unsigned char> > generateClientBody(
      unsigned int expires_in);
  std::shared_ptr<std::vector<unsigned char> > generateUserBody(
      const AuthenticationClient::UserProperties& properties);
  std::shared_ptr<std::vector<unsigned char> > generateFederatedBody(
      const FederatedSignInType,
      const AuthenticationClient::FederatedProperties& properties);
  std::shared_ptr<std::vector<unsigned char> > generateRefreshBody(
      const AuthenticationClient::RefreshProperties& properties);
  std::shared_ptr<std::vector<unsigned char> > generateSignUpBody(
      const SignUpProperties& properties);
  std::shared_ptr<std::vector<unsigned char> > generateAcceptTermBody(
      const std::string& reacceptance_token);

  std::string generateUid();

  Scopednetwork_ptr GetScopedNetwork();

  client::CancellationToken HandleUserRequest(
      const AuthenticationCredentials& credentials, const std::string& endpoint,
      const std::shared_ptr<std::vector<unsigned char> >& request_body,
      const AuthenticationClient::SignInUserCallback& callback);

 private:
  std::string server_url_;
  std::weak_ptr<ScopedNetwork> network_ptr_;
  std::mutex network_ptr_lock_;
  std::shared_ptr<
      olp::thread::Atomic<utils::LruCache<std::string, SignInResult> > >
      client_token_cache_;
  std::shared_ptr<
      olp::thread::Atomic<utils::LruCache<std::string, SignInUserResult> > >
      user_token_cache_;
  olp::network::NetworkConfig network_config_;

  mutable std::mutex token_mutex_;
};

AuthenticationClient::Impl::Scopednetwork_ptr
AuthenticationClient::Impl::GetScopedNetwork() {
  std::lock_guard<std::mutex> lock(network_ptr_lock_);
  auto net_ptr = network_ptr_.lock();
  if (net_ptr) {
    return net_ptr;
  }
  auto result = std::make_shared<ScopedNetwork>(network_config_);
  network_ptr_ = result;
  return result;
}

bool AuthenticationClient::Impl::SetNetworkProxySettings(
    const NetworkProxySettings& proxy_settings) {
  auto network_proxy = NetworkProxy(
      proxy_settings.host, proxy_settings.port, NetworkProxy::Type::Http,
      proxy_settings.username, proxy_settings.password);

  if (!network_proxy.IsValid()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(network_ptr_lock_);
  auto net_ptr = network_ptr_.lock();
  if (net_ptr) {
    return false;
  }

  network_config_.SetProxy(network_proxy);
  return true;
}

AuthenticationClient::Impl::Impl(const std::string& authentication_server_url,
                                 size_t token_cache_limit)
    : server_url_(authentication_server_url),
      client_token_cache_(std::make_shared<
                          Atomic<utils::LruCache<std::string, SignInResult> > >(
          token_cache_limit)),
      user_token_cache_(
          std::make_shared<
              Atomic<utils::LruCache<std::string, SignInUserResult> > >(
              token_cache_limit)),
      network_config_() {}

client::CancellationToken AuthenticationClient::Impl::SignInClient(
    const AuthenticationCredentials& credentials,
    const chrono::seconds& expires_in,
    const AuthenticationClient::SignInClientCallback& callback) {
  std::string url = server_url_;
  url.append(OAUTH_ENDPOINT);
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::POST);
  request.AddHeader(http::kAuthorizationHeader,
                    generateHeader(credentials, url));
  request.AddHeader(http::kContentTypeHeader, APPLICATION_JSON);
  request.AddHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.SetContent(
      generateClientBody(static_cast<unsigned int>(expires_in.count())));
  auto network_ptr = GetScopedNetwork();
  Network& network = network_ptr->getNetwork();
  auto cache = client_token_cache_;
  auto request_id = network_ptr->getNetwork().Send(
      request, payload,
      [network_ptr, callback, payload, credentials,
       cache](const NetworkResponse& network_response) {
        auto response_status = network_response.Status();
        auto error_msg = network_response.Error();

        // Network not available, use cached token if available
        // See olp::network::ErrorCode for negative values
        if (response_status < 0) {
          // Request cancelled, return
          if (response_status == Network::ErrorCode::Cancelled) {
            AuthenticationError error(Network::ErrorCode::Cancelled, error_msg);
            callback(error);
            return;
          }

          auto cached_response_found =
              cache->locked([credentials, callback](
                                utils::LruCache<std::string, SignInResult>& c) {
                auto it = c.Find(credentials.GetKey());
                if (it != c.end()) {
                  SignInClientResponse response(it->value());
                  callback(response);
                  return true;
                }
                return false;
              });

          if (!cached_response_found) {
            // Return an error response
            SignInClientResponse error_response(
                AuthenticationError(response_status, error_msg));
            callback(error_response);
          }

          return;
        }

        std::shared_ptr<Document> document = std::make_shared<Document>();
        IStreamWrapper stream(*payload.get());
        document->ParseStream(stream);

        std::shared_ptr<SignInResultImpl> resp_impl =
            std::make_shared<SignInResultImpl>(response_status, error_msg,
                                               document);
        SignInResult response(resp_impl);

        if (!resp_impl->IsValid()) {
          callback(response);
          return;
        }

        switch (response_status) {
          case HttpStatusCode::Ok: {
            // Cache the response
            cache->locked([credentials, &response](
                              utils::LruCache<std::string, SignInResult>& c) {
              return c.InsertOrAssign(credentials.GetKey(), response);
            });
            // intentially do not break
          }
          default:
            callback(response);
            break;
        }
      });
  return client::CancellationToken(
      [&network, request_id]() { network.Cancel(request_id); });
}

client::CancellationToken AuthenticationClient::Impl::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties,
    const AuthenticationClient::SignInUserCallback& callback) {
  return HandleUserRequest(credentials, OAUTH_ENDPOINT,
                           generateUserBody(properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::SignInRefresh(
    const AuthenticationCredentials& credentials,
    const RefreshProperties& properties, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, OAUTH_ENDPOINT,
                           generateRefreshBody(properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::SignInFederated(
    const AuthenticationCredentials& credentials,
    const FederatedSignInType& type, const FederatedProperties& properties,
    const AuthenticationClient::SignInUserCallback& callback) {
  return HandleUserRequest(credentials, OAUTH_ENDPOINT,
                           generateFederatedBody(type, properties), callback);
}

client::CancellationToken AuthenticationClient::Impl::AcceptTerms(
    const AuthenticationCredentials& credentials,
    const std::string& reacceptanceToken, const SignInUserCallback& callback) {
  return HandleUserRequest(credentials, TERMS_ENDPOINT,
                           generateAcceptTermBody(reacceptanceToken), callback);
}

client::CancellationToken AuthenticationClient::Impl::HandleUserRequest(
    const AuthenticationCredentials& credentials, const std::string& endpoint,
    const std::shared_ptr<std::vector<unsigned char> >& request_body,
    const SignInUserCallback& callback) {
  std::string url = server_url_;
  url.append(endpoint);
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::POST);
  request.AddHeader(http::kAuthorizationHeader,
                    generateHeader(credentials, url));
  request.AddHeader(http::kContentTypeHeader, APPLICATION_JSON);
  request.AddHeader(olp::http::kUserAgentHeader, olp::http::kOlpSdkUserAgent);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.SetContent(request_body);
  auto network_ptr = GetScopedNetwork();
  Network& network = network_ptr->getNetwork();
  auto cache = user_token_cache_;
  auto request_id = network_ptr->getNetwork().Send(
      request, payload,
      [network_ptr, callback, payload, credentials,
       cache](const NetworkResponse& network_response) {
        auto response_status = network_response.Status();
        auto error_msg = network_response.Error();

        // Network not available, use cached token if available
        if (response_status < 0) {
          // Request cancelled, return
          if (response_status == Network::ErrorCode::Cancelled) {
            AuthenticationError error(Network::ErrorCode::Cancelled, error_msg);
            callback(error);
            return;
          }

          auto cached_response_found = cache->locked(
              [credentials,
               callback](utils::LruCache<std::string, SignInUserResult>& c) {
                auto it = c.Find(credentials.GetKey());
                if (it != c.end()) {
                  callback(it->value());
                  return true;
                }
                return false;
              });

          if (!cached_response_found) {
            // Return an error response
            SignInUserResult response(std::make_shared<SignInUserResultImpl>(
                response_status, error_msg));
            callback(response);
          }

          return;
        }

        std::shared_ptr<Document> document = std::make_shared<Document>();
        IStreamWrapper stream(*payload.get());
        document->ParseStream(stream);

        std::shared_ptr<SignInUserResultImpl> resp_impl =
            std::make_shared<SignInUserResultImpl>(response_status, error_msg,
                                                   document);
        SignInUserResult response(resp_impl);

        if (!resp_impl->IsValid()) {
          callback(response);
          return;
        }

        switch (response_status) {
          case HttpStatusCode::Ok: {
            // Cache the response
            cache->locked(
                [credentials,
                 &response](utils::LruCache<std::string, SignInUserResult>& c) {
                  return c.InsertOrAssign(credentials.GetKey(), response);
                });
          }
          default:
            callback(response);
            break;
        }
      });

  return client::CancellationToken(
      [&network, request_id]() { network.Cancel(request_id); });
}

client::CancellationToken AuthenticationClient::Impl::SignUpHereUser(
    const AuthenticationCredentials& credentials,
    const SignUpProperties& properties, const SignUpCallback& callback) {
  std::string url = server_url_;
  url.append(USER_ENDPOINT);
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::POST);
  request.AddHeader(http::kAuthorizationHeader,
                    generateHeader(credentials, url));
  request.AddHeader(http::kContentTypeHeader, APPLICATION_JSON);
  request.AddHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  request.SetContent(generateSignUpBody(properties));
  auto network_ptr = GetScopedNetwork();
  Network& network = network_ptr->getNetwork();
  auto request_id = network_ptr->getNetwork().Send(
      request, payload,
      [network_ptr, callback, payload,
       credentials](const NetworkResponse& network_response) {
        auto response_status = network_response.Status();
        auto error_msg = network_response.Error();

        if (response_status < 0) {
          // Network error response
          AuthenticationError error(response_status, error_msg);
          callback(error);
          return;
        }

        std::shared_ptr<Document> document = std::make_shared<Document>();
        IStreamWrapper stream(*payload.get());
        document->ParseStream(stream);

        std::shared_ptr<SignUpResultImpl> resp_impl =
            std::make_shared<SignUpResultImpl>(response_status, error_msg,
                                               document);
        SignUpResult response(resp_impl);
        callback(response);
      });
  return client::CancellationToken(
      [&network, request_id]() { network.Cancel(request_id); });
}

client::CancellationToken AuthenticationClient::Impl::SignOut(
    const AuthenticationCredentials& credentials,
    const std::string& userAccessToken, const SignOutUserCallback& callback) {
  std::string url = server_url_;
  url.append(SIGNOUT_ENDPOINT);
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::POST);
  request.AddHeader(http::kAuthorizationHeader,
                    generateBearerHeader(userAccessToken));
  request.AddHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  auto network_ptr = GetScopedNetwork();
  Network& network = network_ptr->getNetwork();
  auto request_id = network_ptr->getNetwork().Send(
      request, payload,
      [network_ptr, callback, payload,
       credentials](const NetworkResponse& network_response) {
        auto response_status = network_response.Status();
        auto error_msg = network_response.Error();

        if (response_status < 0) {
          // Network error response not available
          AuthenticationError error(response_status, error_msg);
          callback(error);
          return;
        }

        std::shared_ptr<Document> document = std::make_shared<Document>();
        IStreamWrapper stream(*payload.get());
        document->ParseStream(stream);

        std::shared_ptr<SignOutResultImpl> resp_impl =
            std::make_shared<SignOutResultImpl>(response_status, error_msg,
                                                document);
        SignOutResult response(resp_impl);
        callback(response);
      });
  return client::CancellationToken(
      [&network, request_id]() { network.Cancel(request_id); });
}

std::string AuthenticationClient::Impl::base64Encode(
    const std::vector<uint8_t>& vector) {
  std::string ret = olp::utils::Base64Encode(vector);
  // Base64 encode sometimes return multiline with garbage at the end
  if (!ret.empty()) {
    auto loc = ret.find(LINE_FEED);
    if (loc != std::string::npos) ret = ret.substr(0, loc);
  }
  return ret;
}

std::string AuthenticationClient::Impl::generateHeader(
    const AuthenticationCredentials& credentials, const std::string& url) {
  std::string uid = generateUid();
  const std::string currentTime = std::to_string(std::time(nullptr));
  const std::string encodedUri = Url::Encode(url);
  const std::string encodedQuery = Url::Encode(
      OAUTH_CONSUMER_KEY + PARAM_EQUALS + credentials.GetKey() + PARAM_ADD +
      OAUTH_NONCE + PARAM_EQUALS + uid + PARAM_ADD + OAUTH_SIGNATURE_METHOD +
      PARAM_EQUALS + HMAC + PARAM_ADD + OAUTH_TIMESTAMP + PARAM_EQUALS +
      currentTime + PARAM_ADD + OAUTH_VERSION + PARAM_EQUALS + VERSION);
  const std::string signatureBase =
      OAUTH_POST + PARAM_ADD + encodedUri + PARAM_ADD + encodedQuery;
  const std::string encodeKey = credentials.GetSecret() + PARAM_ADD;
  auto hmacResult = Crypto::hmac_sha256(encodeKey, signatureBase);
  auto signature = base64Encode(hmacResult);
  std::string authorization =
      "OAuth " + OAUTH_CONSUMER_KEY + PARAM_EQUALS + PARAM_QUOTE +
      Url::Encode(credentials.GetKey()) + PARAM_QUOTE + PARAM_COMMA +
      OAUTH_NONCE + PARAM_EQUALS + PARAM_QUOTE + Url::Encode(uid) +
      PARAM_QUOTE + PARAM_COMMA + OAUTH_SIGNATURE_METHOD + PARAM_EQUALS +
      PARAM_QUOTE + HMAC + PARAM_QUOTE + PARAM_COMMA + OAUTH_TIMESTAMP +
      PARAM_EQUALS + PARAM_QUOTE + Url::Encode(currentTime) + PARAM_QUOTE +
      PARAM_COMMA + OAUTH_VERSION + PARAM_EQUALS + PARAM_QUOTE + VERSION +
      PARAM_QUOTE + PARAM_COMMA + OAUTH_SIGNATURE + PARAM_EQUALS + PARAM_QUOTE +
      Url::Encode(signature) + PARAM_QUOTE;

  return authorization;
}

std::string AuthenticationClient::Impl::generateBearerHeader(
    const std::string& bearer_token) {
  std::string authorization = http::kBearer + std::string(" ");
  authorization += bearer_token;
  return authorization;
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateClientBody(unsigned int expires_in) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(GRANT_TYPE);
  writer.String(CLIENT_GRANT_TYPE);

  if (expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(expires_in);
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateUserBody(const UserProperties& properties) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(GRANT_TYPE);
  writer.String(USER_GRANT_TYPE);

  if (!properties.email.empty()) {
    writer.Key(EMAIL);
    writer.String(properties.email.c_str());
  }
  if (!properties.password.empty()) {
    writer.Key(PASSWORD);
    writer.String(properties.password.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateFederatedBody(
    const FederatedSignInType type, const FederatedProperties& properties) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(GRANT_TYPE);
  switch (type) {
    case FederatedSignInType::FacebookSignIn:
      writer.String(FACEBOOK_GRANT_TYPE);
      break;
    case FederatedSignInType::GoogleSignIn:
      writer.String(GOOGLE_GRANT_TYPE);
      break;
    case FederatedSignInType::ArcgisSignIn:
      writer.String(ARCGIS_GRANT_TYPE);
      break;
    default:
      return nullptr;
  }

  if (!properties.access_token.empty()) {
    writer.Key(Constants::ACCESS_TOKEN);
    writer.String(properties.access_token.c_str());
  }
  if (!properties.country_code.empty()) {
    writer.Key(COUNTRY_CODE);
    writer.String(properties.country_code.c_str());
  }
  if (!properties.language.empty()) {
    writer.Key(LANGUAGE);
    writer.String(properties.language.c_str());
  }
  if (!properties.email.empty()) {
    writer.Key(EMAIL);
    writer.String(properties.email.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateRefreshBody(
    const RefreshProperties& properties) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(GRANT_TYPE);
  writer.String(REFRESH_GRANT_TYPE);

  if (!properties.access_token.empty()) {
    writer.Key(Constants::ACCESS_TOKEN);
    writer.String(properties.access_token.c_str());
  }
  if (!properties.refresh_token.empty()) {
    writer.Key(Constants::REFRESH_TOKEN);
    writer.String(properties.refresh_token.c_str());
  }
  if (properties.expires_in > 0) {
    writer.Key(Constants::EXPIRES_IN);
    writer.Uint(properties.expires_in);
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateSignUpBody(
    const SignUpProperties& properties) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  if (!properties.email.empty()) {
    writer.Key(EMAIL);
    writer.String(properties.email.c_str());
  }
  if (!properties.password.empty()) {
    writer.Key(PASSWORD);
    writer.String(properties.password.c_str());
  }
  if (!properties.date_of_birth.empty()) {
    writer.Key(DATE_OF_BIRTH);
    writer.String(properties.date_of_birth.c_str());
  }
  if (!properties.first_name.empty()) {
    writer.Key(FIRST_NAME);
    writer.String(properties.first_name.c_str());
  }
  if (!properties.last_name.empty()) {
    writer.Key(LAST_NAME);
    writer.String(properties.last_name.c_str());
  }
  if (!properties.country_code.empty()) {
    writer.Key(COUNTRY_CODE);
    writer.String(properties.country_code.c_str());
  }
  if (!properties.language.empty()) {
    writer.Key(LANGUAGE);
    writer.String(properties.language.c_str());
  }
  if (properties.marketing_enabled) {
    writer.Key(MARKETING_ENABLED);
    writer.Bool(true);
  }
  if (!properties.phone_number.empty()) {
    writer.Key(PHONE_NUMBER);
    writer.String(properties.phone_number.c_str());
  }
  if (!properties.realm.empty()) {
    writer.Key(REALM);
    writer.String(properties.realm.c_str());
  }
  if (!properties.invite_token.empty()) {
    writer.Key(INVITE_TOKEN);
    writer.String(properties.invite_token.c_str());
  }

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::shared_ptr<std::vector<unsigned char> >
AuthenticationClient::Impl::generateAcceptTermBody(
    const std::string& reacceptance_token) {
  StringBuffer data;
  Writer<StringBuffer> writer(data);
  writer.StartObject();

  writer.Key(TERMS_REACCEPTANCE_TOKEN);
  writer.String(reacceptance_token.c_str());

  writer.EndObject();
  auto content = data.GetString();
  return std::make_shared<std::vector<unsigned char> >(
      content, content + data.GetSize());
}

std::string AuthenticationClient::Impl::generateUid() {
  std::lock_guard<std::mutex> lock(token_mutex_);
  {
    static boost::uuids::random_generator gen;

    return boost::uuids::to_string(gen());
  }
}

AuthenticationClient::AuthenticationClient(
    const std::string& authenticationServerUrl, size_t tokenCacheLimit)
    : impl_(std::make_unique<Impl>(authenticationServerUrl, tokenCacheLimit)) {}

AuthenticationClient::~AuthenticationClient() = default;

client::CancellationToken AuthenticationClient::SignInClient(
    const AuthenticationCredentials& credentials,
    const SignInClientCallback& callback, const chrono::seconds& expires_in) {
  return impl_->SignInClient(credentials, expires_in, callback);
}

client::CancellationToken AuthenticationClient::SignInHereUser(
    const AuthenticationCredentials& credentials,
    const UserProperties& properties, const SignInUserCallback& callback) {
  return impl_->SignInHereUser(credentials, properties, callback);
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

bool AuthenticationClient::SetNetworkProxySettings(
    const NetworkProxySettings& proxy_settings) {
  return impl_->SetNetworkProxySettings(proxy_settings);
}

}  // namespace authentication
}  // namespace olp
