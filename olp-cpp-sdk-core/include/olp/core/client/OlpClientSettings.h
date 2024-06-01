/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/DefaultLookupEndpointProvider.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OauthToken.h>
#include <olp/core/client/RetrySettings.h>
#include <olp/core/http/Network.h>

namespace olp {
namespace cache {
class KeyValueCache;
}  // namespace cache

namespace thread {
class TaskScheduler;
}  // namespace thread

namespace http {
class Network;
}  // namespace http

namespace client {
/**
 * @brief An alias for the asynchronous network callback.
 *
 * Used to receive the `HttpResponse` instance.
 */
using NetworkAsyncCallback = std::function<void(HttpResponse)>;

/**
 * @brief An alias for the cancel function.
 *
 * Used to cancel the asynchronous network operation.
 */
using NetworkAsyncCancel = std::function<void()>;

/**
 * @brief A set of settings that manages the `TokenProviderCallback` and
 * `TokenProviderCancelCallback` functions.
 *
 * The `TokenProviderCallback` function requests the OAuth2
 * bearer access token. The `TokenProviderCancelCallback` function cancels that
 * request. Both functions are user-provided.
 * The struct is used internally by the `OlpClient` class.
 */
struct CORE_API AuthenticationSettings {
  /// An alias for the `ApiKey` provider.
  using ApiKeyProviderType = std::function<std::string()>;

  /**
   * @brief Implemented by the client that should return the OAuth2 bearer
   * access token if the operation is successful; an `ApiError` otherwise.
   *
   * The access token should be used as the authorization header for the service
   * calls. This allows for an external OAuth2 library to be used to provide
   * the authentication functionality for any service.
   *
   * The provided token should be authorized to access the resources
   * provided by the HERE platform Services you are trying
   * to request. Also, the token should not be expired by the time the service
   * request is sent to the server. Otherwise, a service-specific authorization
   * error is returned when calls are made.
   *
   * `CancellationContext` argument should be used to give the caller an ability
   * to cancel the operation.
   */
  using TokenProviderCancellableCallback =
      std::function<OauthTokenResponse(CancellationContext&)>;

  /**
   * @brief The user-provided function that returns the OAuth2 bearer access
   * token if the operation is successful; an `ApiError` otherwise.
   *
   * @see `TokenProviderCancellableCallback` for more details.
   */
  TokenProviderCancellableCallback token_provider = nullptr;

  /**
   * @brief The user-provided function that returns `ApiKey`.
   *
   * If this provider is set, it is used instead of the token provider.
   * The returned value, if not empty, is added as a URL parameter
   * to each request.
   *
   * @note This method must be synchronized and should not trigger any tasks on
   * `TaskScheduler` as this might result in a deadlock.
   */
  ApiKeyProviderType api_key_provider = nullptr;
};

/**
 * @brief Settings to provide URLs for API lookup requests.
 */
struct CORE_API ApiLookupSettings {
  /**
   * @brief An alias for the lookup provider function.
   *
   * Users of this provider should always return full lookup API path, e.g.
   * for "here" partition return
   * "https://api-lookup.data.api.platform.here.com/lookup/v1"
   *
   * @note Return empty string in case of an invalid or unknown partition.
   * @note This call should be synchronous without any tasks scheduled on the
   * TaskScheduler as this might result in a dead-lock.
   */
  using LookupEndpointProvider = std::function<std::string(const std::string&)>;

  /**
   * @brief An alias for the catalog endpoint provider function.
   *
   * Catalogs that have a static URL or can be accessed through
   * a proxy service can input the URL provider here. This URL provider is taken
   * by the `ApiLookupClient` and returned directly to the caller without any
   * requests to the API Lookup Service.
   *
   * @note This call should be synchronous without any tasks scheduled in
   * `TaskScheduler` as it might result in a dead-lock.
   *
   * @return An empty string if the catalog is invalid or unknown.
   */
  using CatalogEndpointProvider = std::function<std::string(const HRN&)>;

  /**
   * @brief The provider of endpoint for API lookup requests.
   *
   * The lookup API endpoint provider will be called prior to every API lookup
   * attempt to get the API Lookup URL which shall be asked for the catalog
   * URLs.
   *
   * By default `DefaultLookupEndpointProvider` is being used.
   */
  LookupEndpointProvider lookup_endpoint_provider =
      DefaultLookupEndpointProvider();

  /**
   * @brief The endpoint provider for API requests.
   *
   * If some of the catalogs have fixed URLs and do not need the API Lookup
   * Service, you can provide the static URL via `CatalogEndpointProvider`.
   * Every request will receive this URL from `ApiLookupClient` without
   * any HTTP requests to the API Lookup Service. `CatalogEndpointProvider` is
   * called before `lookup_endpoint_provider`, and if the output is not empty,
   * `lookup_endpoint_provider` is not called additionally.
   */
  CatalogEndpointProvider catalog_endpoint_provider = nullptr;
};

/**
 * @brief Configures the behavior of the `OlpClient` class.
 */
struct CORE_API OlpClientSettings {
  /**
   * @brief The retry settings.
   */
  RetrySettings retry_settings;

  /**
   * @brief API Lookup settings.
   */
  ApiLookupSettings api_lookup_settings;

  /**
   * @brief The network proxy settings.
   *
   * To remove any existing proxy settings, set to `boost::none`.
   */
  boost::optional<http::NetworkProxySettings> proxy_settings = boost::none;

  /**
   * @brief The authentication settings.
   *
   * To remove any existing authentication settings, set to `boost::none`.
   */
  boost::optional<AuthenticationSettings> authentication_settings = boost::none;

  /**
   * @brief The `TaskScheduler` instance.
   *
   * If `nullptr` is set, all request calls are performed synchronously.
   */
  std::shared_ptr<thread::TaskScheduler> task_scheduler = nullptr;

  /**
   * @brief The `Network` instance.
   *
   * Used to internally operate with the HERE platform Services.
   */
  std::shared_ptr<http::Network> network_request_handler = nullptr;

  /**
   * @brief The key-value cache that is used for storing different request
   * results such as metadata, partition data, URLs from the API Lookup Service,
   * and others.
   *
   * To only use the memory LRU cache with limited size, set to `nullptr`.
   */
  std::shared_ptr<cache::KeyValueCache> cache = nullptr;

  /**
   * @brief Set default expiration for any cache entry made by the according
   * layer or catalog client.
   *
   * This setting only applies to the mutable cache and to the in-memory cache,
   * but should not affect the protected cache as no entries are added to the
   * protected cache in read-only mode. Set to std::chrono::seconds::max() to
   * disable expiration. By default, expiration is disabled.
   *
   * @note This only makes sense for data that has an expiration limit, e.g.
   * volatile or versioned, and which is stored in cache.
   */
  std::chrono::seconds default_cache_expiration = std::chrono::seconds::max();

  /**
   * @brief The flag to enable or disable the propagation of all cache errors.
   *
   * When set to `false` only critical cache errors are propagated to the user.
   * By default, this setting is set to `false`.
   */
  bool propagate_all_cache_errors = false;
};

}  // namespace client
}  // namespace olp
