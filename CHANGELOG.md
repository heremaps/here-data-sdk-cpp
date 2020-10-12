##  v1.9.0 (10/12/2020)

**Common**

* The default logger now adds a timestamp to logs.
* Fixed a crash in the `olp::cache::DefaultCache::Put` method that occurred when the input value was `null`.
* Fixed the HTTP response error that occurred when the request was canceled during the network shutdown on Windows.
* `olp::thread::TaskScheduler` now supports tasks with priorities. Tasks with higher priority are executed first.
* Fixed the issue that occurred when the JSON parsers produced valid empty objects.

**olp-cpp-sdk-dataservice-read**

* Fixed the behavior of the `GetAggregatedData` and `PrefetchTiles` APIs for both `olp::dataservice::read::VersionedLayerClient` and `olp::dataservice::read::VolatileLayerClient`. When the aggregated parent tile is far away, i.e. more then max. depth 4, other APIs could not find it (for example, level 14 tile aggregated on level 1). Now, the aggregated tile can be found and used by other APIs.
* `PrefetchTilesRequest`, `TileRequest`, and `DataRequest` now support priorities.
* Fixed the crash that occurred when the client was destroyed during an ongoing prefetch operation.
* Changed the behavior of `olp::dataservice::read::CatalogClient::GetLatestVersion` and `olp::dataservice::read::VersionedLayerClient`. Now, if you do not specify a version, the SDK tries to get it from the network by default. Also, in the cache, the latest version now does not expire after five minutes.
* Added new API `olp::dataservice::read::VersionedLayerClient::PrefetchPartitions` which allows you to prefetch generic partitions which are not tile based.

**olp-cpp-sdk-dataservice-write**

* Fixed the uploading of encoded data to the HERE Blob Service. The missing `Content-Encoding` header is now added when the layer is configured as compressed. This applies to `olp::dataservice::write::VersionedLayerClient`, `olp::dataservice::write::VolatileLayerClient` and `olp::dataservice::write::IndexLayerClient`.
* Fix the bug in the `olp::dataservice::write::StreamLayerClient::PublishData` when the TraceId provided by the user is ignored for data bigger than 20 MB.
* **Work In Progress** Enhanced `olp::dataservice::write::VersionedLayerClient`. The `PublishToBatch` and `CancelBatch` methods now use `olp::thread::TaskScheduler` instead of network threads for asynchronous operations.
* **Work In Progress** Enhanced `olp::dataservice::write::VolatileLayerClient`. The `PublishPartitionData` method now uses `olp::thread::TaskScheduler` instead of network threads for asynchronous operations.

## v1.8.0 (08/25/2020)

**Common**

* **Work In Progress** Added the `ApiKey` support. Now, you can generate API keys on the platform and use them for reading.
* Added the `ApiLookup` client. Now, you can specify a custom lookup server or provide a custom endpoint for the services (for example, a dedicated proxy).
* Added the cache protection API. Now, you can mark keys as protected from eviction.
* Adapted HRN members to the coding style, made all HRN class members private, and added getters to access them.
* Added the `KeyValueCache::Contains` API that you can use to check if a key is in the cache.
* Fixed the CMake scripts issue that occurred when a path contained whitespaces.
* Fixed the `DefaultCache` behavior that occurred when it was opened in the read-only mode.
* Modified the `DefaultRetryCondition`. It now retries after 429 errors.
* Removed the deprecated `backdown_policy` parameter from `OlpClientSettings`.

**olp-cpp-sdk-authentication**

* Deprecated the `SignInGoogle` API. It will be removed by 12.2020.
* Fixed the bug in `DecisionAPI` (`decision_` was uninitialized).

**olp-cpp-sdk-dataservice-read**

* Added the `IsCached` API for partition and tile keys.
* Added the `Protect/Release` functionality. Now, you can protect specific partitions and tiles from eviction. For example, you may want a specific region to always be present in the cache.
* **Breaking Change** Removed the deprecated `WithVersion` and `GetVersion` methods from `DataRequest`, `PrefetchTilesRequest`, and `PartitionsRequest`.
* Enhanced prefetch. Now, you can request quadtrees in parallel.
* **Breaking Change** Removed the deprecated constructor of the `VersionedLayerClient` class.
* Added the prefetch status callback. You can use it to receive the following prefetch progress information: the number of tiles prefetched, the total number of tiles requested, and total bytes transferred.
* Added an experimental option to the prefetch API that you can use to prefetch a list of tiles as aggregated.

**olp-cpp-sdk-dataservice-write**

* **Breaking Change** Removed the deprecated `CancelAll` methods from `dataservice::write::VersionedLayerClient`, `dataservice::write::VolatileLayerClient`, and `dataservice::write::IndexLayerClient`
* **Breaking Change** Removed the deprecated `CreateDefaultCache` method from `StreamLayerClient.h`.
* **Work In Progress** Enhanced `olp::dataservice::write::VersionedLayerClient`. The `StartBatch` and `CompleteBatch` methods now use `olp::thread::TaskScheduler` instead of network threads for asynchronous operations.
* Fixed the duplicate requests used to configure the service in `StreamLayerClient`.
* Fixed the upload issue in the stream layer that occurred when the layer compression was enabled on the platform at the same time as the ingest service was used.

## v1.7.0 (06/16/2020)

**Common**

* Changed the product name from HERE OLP SDK for C++ to HERE Data SDK for C++.
* Fixed formatting to be compliant with the coding style.
* Fixed the issue with `olp::client::TaskContext`. Now, it does not trigger cancellation after a successful operation.
* Added the `default_cache_expiration` to `olp::client::OlpClientSettings`. Now, you can control how long the downloaded data is considered valid. Once the data is expired, you cannot retrieve it, and it is gradually removed from the cache when new data is added.
* Added the `OLP_SDK_DISABLE_DEBUG_LOGGING` CMake option. When enabled, the SDK does not print any `TRACE` or `DEBUG` level log messages.
* Added the `compression` setting to `olp::cache::CacheSettings`. You can now pass the flag to the underlying database engine to enable or disable the data compression.
* Added the `Compact` API to `olp::cache::DefaultCache`. It is used to optimize the underlying mutable cache storage. Compaction also starts automatically once the database size reaches the maximum in a separate thread.
* Changed the `olp::cache::DefaultCache` read operation. As a result, the number of memory allocation decreased, and performance increased.
* Added support for the HTTP `OPTIONS` request in `olp::http::Network`.
* The LRU eviction now evicts expired elements first. The eviction time decreased.
* Removed the retry code from `olp::http::NetworkCurl`. Now, retries are performed by `olp::client::OlpClient`.
* Added a prewarm helper method to `olp::client::OlpClientSettingsFactory`. This method can be used to perform DNS prefetch and TLS preconnect for certain URLs that are widely used by the clients. For example, accounts and lookup.
* Enabled certificate validation in `olp::http::NetworkCurl`.

**olp-cpp-sdk-authentication**

* The `olp::authentication::TokenProvider` class now handles the concurrent by blocking all requesters until the already triggered request completes. It verifies that only one token is requested at the same time.
* The `IntrospectApp` and `Authorize` APIs now use `ExponentialBackdownStrategy` internally in case of a 5xx HTTP status code.
* `olp::authentication::AuthenticationError` class is deprecated and will be replaced by `olp::client::ApiError`. It will be removed by 12.2020.
* Switched the `SignInClient` method in the `olp::authentication::AuthenticationClient` functionality to `TaskContext`.
* Added a fallback mechanism for `SignInClient`. Now, when the system time is wrong, the server time is used.
* Changed the `olp::authentication::ActionResult` class. Permissions are now represented by the `olp::authentication::Permission` class.
* The `use_system_time` flag in `olp::authentication::Settings` is now enabled by default. Now, system time is used for token requests by default.

**olp-cpp-sdk-dataservice-read**

* The module now resolves all service APIs for a specific HRN in a single request.
* Reduced the number of logs produced by the component.
* Merged the same concurrent Lookup API requests into a single HTTP request.
* Merged the same concurrent `GetBlob` and `GetVolatileBlob` API requests into a single HTTP request.
* Merged the same concurrent `QuadTreeIndex` API requests into a single HTTP request.
* **Work In Progress** Added a new `ListVersions` API to `olp::dataservice::read::CatalogClient`. You can use it to get information on catalog versions.
* **Work In Progress** Added a new `GetAggregatedData` API to `olp::dataservice::read::VersionedLayerClient`. You can use it to retrieve tile data (if it exists) or the nearest parent tile data. Use this API for tile-tree structures where children tile data is aggregated and stored in parent tiles.

## v1.6.0 (05/01/2020)

**Common**

* Added the LRU functionality to the mutable disk cache. LRU is enabled by default. You can disable it using `eviction_policy` in `CacheSettings`.
* Completed the network statistics implementation. It's available in `Network::GetStatistics`. Now, you can get accumulated statistics on how many bytes are processed during each operation.
* Added the missing `CORE_API` export macros.
* Updated various classes according to the Google coding style.
* Added information on backward compatibility.
* Fixed various compiler warnings.
* Minor documentation changes.
* Fixed the Android bug that made network responses incomplete.

**olp-cpp-sdk-authentication**

* Added the `use_system_time` flag to `olp::authentication::Settings`. You can use it when retrieving tokens to tell `olp::authentication::TokenProvider` to work with the system time instead of the server time.
* Added the `olp::authentication::AuthenticationClient::Authorize` method. You can use it to collect all permissions associated with the authenticated user or application.
* `olp::authentication::TokenProvider` now uses the pimpl idiom. Now, when you copy an instance of this class, it does not create a new request.
* Deprecated the `olp::authentication:AutoRefreshingToken` class. It will be removed by 10.2020.
* **Breaking Change** Removed the deprecated constructor of the `AuthenticationClient` class and the following deprecated methods: `SetNetworkProxySettings`, `SetNetwork`, and `SetTaskScheduler`.
* The deprecation of the `olp::authentication::TokenEndpoint` and `olp::authentication::TokenRequest` classes is extended until 10.2020.

**olp-cpp-sdk-dataservice-read**

* Optimized memory overhead for prefetch. 
* Removed the ambiguous `PrefetchTilesRequest::WithTileKeys` method that takes an rvalue reference.

## v1.5.0 (07/04/2020)

**Common**

* Moved the `DefaultCache` implementation to pimpl.
* Fixed various compiler warnings.
* CMake now uses the official Boost Github repository instead of downloading and unpacking the archive.
* Method `OlpClientSettingsFactory::CreateDefaultCache` now returns `nullptr` if it failes to open one of the user-defined disk caches.
* **Work In Progress** Added API to retrieve network statistics. It is not fully implemented yet, and users should not use it.

**olp-cpp-sdk-authentication**

* Added a new `use_system_time` flag to `olp::authentication::AuthenticationSettings`. You can use it to tell the authentication module to work with system time instead of server time when retrieving tokens.

**olp-cpp-sdk-dataservice-read**

* Added a stream layer read example. For more information, see our [documentation](https://github.com/heremaps/here-olp-sdk-cpp/blob/master/docs/dataservice-read-from-stream-layer-example.md).
* Added the `RemoveFromCache` method to `VolatileLayerClient`. Now, you can remove specific partitions or tiles from the mutable cache.
* Added a new `PrefetchTiles` method to `VolatileLayerClient`. Now, you can prefetch volatile tiles in the same way as versioned tiles.

## v1.4.0 (23/03/2020)

**Common**

* **Breaking Change** Removed the deprecated `disk_path` property. Use the `disk_path_mutable` property instead.
* The `DefaultCache` constructor is now explicit and takes `CacheSettings` by value.
* Fixed data that was not validated during reading from LevelDB when the `olp::cache::OpenOptions::CheckCrc` property was provided.
* Various improvements in `olp::http::NetworkCurl` implementation. Some legacy features were removed.
* Added the `SetDefaultHeaders` method to `olp::http::Network`. Now, you can set default HTTP headers for each request made by `Network`. User agents set with default headers and user agents passed with network requests are concatenated into one header.
* Reduced compiler warnings about deprecated methods and classes.

**olp-cpp-sdk-authentication**

* Removed the deprecated `AuthenticationClient::SignInClient` method.

**olp-cpp-sdk-dataservice-read**

* Added the `RemoveFromCache` method to `VersionedLayerClient`. Now, you can remove specific partitions or tiles from the mutable cache.
* `VersionedLayerClient` now triggers an error when the request is passed with `FetchOption::CacheWithUpdate`. It makes no sense to update data when it is available in a cache for `VersionedLayerClient` since the version is locked.
* Now, when you pass a request to `VersionedLayerClient` or `VolatileLayerClient` with `FetchOption::OnlineOnly`, data is not stored in a cache. It is designed for a use case when you are not interested in storing data in a cache.

**olp-cpp-sdk-dataservice-write**

* Deprecated the  `olp::dataservice::write::StreamLayerClient::CreateDefaultCache` method. It will be removed by 06.2020.

## v1.3.0 (04/03/2020)

**Common**

* Response headers are now stored in `olp::client::HttpResponse`. `olp::client::OlpClient::CallApi()` collects and adds headers to the response.
* X-Correlation-ID is now extracted from the HTTP response headers and used for subsequent requests in `olp::dataservice::read::StreamLayerClient`.
* Added a new backdown strategy to `olp::client::RetrySettings`.
  The default implementation is `olp::client::ExponentialBackdownStrategy` that restricts the maximum wait time during failed retries of network requests in the `olp::network::client::OlpClient` class (both synchronous and asynchronous versions). The accumulated wait time during retries should not be longer than the `RetrySettings.timeout` property.
* Added `curl_global_init()` and `curl_global_cleanup()` to the `olp::http::NetworkCurl` class (Linux default implementation).
  For more information, see the `olp::client::OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler()` function.
* Fixed MSVC, Clang, and GCC warnings. Now, when you build with `-Wall -Wextra`, you should not face any hidden warnings, as the codebase is protected by a pre-release verification job.
* Fixed the possible thread concurrency problem in the `olp::client::OlpClient` class that resulted in a crash in some situations.
* Reduced data copy in the JSON serializers to increase performance.

**olp-cpp-sdk-authentication**

* Added a new API to `olp::authentication::AuthenticationClient` that you can use to perform a generic federated authentication with a custom request body.
* Added the `olp::authentication::AuthenticationClient::IntrospectApp()` method that you can use to retrieve information
  about the application associated with a particular access token.
* Added a new `olp::authentication::AuthenticationClient` constructor using `olp::authentication::AuthenticationSettings` to setup network and task scheduler. It is a part of an ongoing authentication refactoring. Switch to this constructor instead of using the individual setters.

**olp-cpp-sdk-dataservice-read**

* `olp::dataservice::read::PartitionsRequest` now supports a list of partition IDs. This way, when you use `olp::dataservice::read::VersionedLayerClient` and `olp::dataservice::read::VolatileLayerClient`, you can also request certain partitions instead of the entire layer metadata.
* Added the `Seek` method to `olp::dataservice::read::StreamLayerClient`. This method can be used to start reading messages from a stream layer at any given position.
* Added the `Poll` method to `olp::dataservice::read::StreamLayerClient`. This method reads messages from a stream layer and commits successfully consumed messages before handing them over to you.
* Added the `olp::dataservice::read::TileRequest` class and corresponding `GetData` overload to `olp::dataservice::read::VersionedLayerClient`. Now, you can get data using a tile key. Internally, the metadata retrieval is optimized by using a quadtree request and saves bandwidth on subsequent `GetData` calls.
* Added additional fields to the `olp::dataservice::read::PartitionsRequest` class. Now, you can access the following additional metadata
  fields: data size, compressed data size, checksum, and CRC.
* Deprecated the `WithVersion` and the `GetVersion` methods in the `olp::dataservice::read::DataRequest`, `olp::dataservice::read::PartitionsRequest` and `olp::dataservice::read::PrefetchTilesRequest` classes. `olp::dataservice::read::VersionedLayerClient` now locks the catalog version either to the value provided on the constructor or by requesting the latest catalog version from the backend. This ensures that consistent data is provided for the instance lifecycle.
* Fixed cache keys by appending 'partition' and 'partitions' to key values in `olp::dataservice::read::VolatileLayerClient`.
* Fixed the infinite loop in the `olp::dataservice::read::VersionedLayerClient::PrefetchTiles()` method when min/max levels are not specified.

## v1.2.0 (04/02/2020)

**Common**

* Added a protected read-only cache. For details, see `olp::cache::CacheSettings`.
* Added a protected read-only cache [usage example](https://github.com/heremaps/here-olp-sdk-cpp/blob/c30c4051ec012eaca09b7aa7ba4c7e8f6cd7a576/docs/dataservice-cache-example.md).
* Added ARM platform support for Embedded Linux with GCC 5.4 (32/64 bit).
* Added the `ErrorCodeToString` function to olp/core/http/NetworkTypes.h.
* Improved the `olp::http::NetworkCurl` logging.
* Changed the `IsNull` function in the `olp::client::HRN` class to handle realms according to platform changes.
* Added the bool operator to the `olp::client::HRN` class. You can now use the operator to check whether all the service type fields in the `olp::client::HRN` class are not empty.
* Fixed a possible data race in the `olp::client::Condition` class.
* Added the `MoveResult` method to the `olp::client::ApiResponse` class. The method returns an rvalue reference to the result.
* CMake now requires RapidJSON v1.1.0.
* Various CMake cleanups.
* Improved error handling in Android `HttpClient`.
* Fixed the linking error when SDK was statically linked inside a shared library with `CMAKE_POSITION_INDEPENDENT_CODE=ON`.
* The retry policy now handles server errors automatically.

**olp-cpp-sdk-authentication**

* Added a possibility to parse credentials from a file or stream in `olp::authentication::AuthenticationCredentials`. Now, to get credentials for the `AuthenticationCredentials` class, you can use the **credentials.properties** file that is provided by the platform.
* Deprecated the `olp::authentication::TokenEndpoint` class. It will be removed by 04.2020.
* Deprecated the `olp::authentication::TokenRequest` class. It will be removed by 04.2020.
* Deprecated various methods in `olp::authentication::AuthenticationClient`.
* Authentication now gets the current timestamp from the platform and uses it to request the OAuth2 token that prevents replay attacks. As a fallback, the system time is used.

**olp-cpp-sdk-dataservice-read**

* The `olp::dataservice::read::QueryApi`, `olp::dataservice::read::MetadataApi`, `olp::dataservice::read::LookupApi`, and `olp::dataservice::read::BlobApi` classes are now blocking and use the new synchronous `CallApi` method in `OlpClient`.
* The `GetData` method of the versioned and volatile layer now triggers the `olp::client::ErrorCode::PreconditionFailed` error if both a partition ID and data handle are passed in the data request.
* **Work-in-progress**: Added reading support for streamed data using the new `olp::dataservice::read::StreamLayerClient`. Currently, you can only subscribe, unsubscribe, and get data from a stream layer message.

**olp-cpp-sdk-dataservice-write**

* Enhanced `olp::dataservice::write::StreamLayerClient` to use `olp::thread::TaskScheduler` for asynchronous operations instead of network threads.
* Removed the unused `olp::dataservice::write::ThreadSafeQueue` class.
* Deprecated the `CancelAll` methods in all layers. Use the `CancelPendingRequests` methods instead.

## v1.1.0 (11/12/2019)

**Common**

* The deprecated `olp::client::CancellationToken::cancel()` method was removed. Use `olp::client::CancellationToken::Cancel()` instead.
* The `curl` network implementation was fixed and can now compile on 32 bits architecture.
* The `disk_path` field in `olp::cache::CacheSettings` is deprecated. Use the `disk_path_mutable` field instead.
* A new synchronous `CallApi` method was added to `olp::client::OlpClient`.
* `pipe` and `pipe2` symbols detection are enhanced in curl.cmake.

**olp-cpp-sdk-authentication**

* The `SignInClient` method in `olp::authentication::AuthenticationClient` is deprecated in favor of the newly introduced `SignInClient` method with a different signature.
* The `scope` support was added to OAuth2 through `olp::authentication::SignInProperties`. You can now access the project bound resources using the `olp::authentication::SignInProperties::scope` field.
* The `error_id` field was added to the `ErrorResponse` structure. You can use it to get the `errorId` string from the authentication error response.

**olp-cpp-sdk-dataservice-read**

* The deprecated `GetCatalogMetadataVersion` method was removed from the `olp::dataservice::read::CatalogClient`.

**olp-cpp-sdk-dataservice-write**

* Legacy and unused code were removed.

## v1.0.0 (03/12/2019)

**Common**

* A new CMake flag `OLP_SDK_BOOST_THROW_EXCEPTION_EXTERNAL` was introduced to compile the SDK without exceptions. The user needs to provide the custom `boost::throw_exception` function.
* `olp::client::HttpResponse` now holds `std::stringstream` instead of `std::string`. This removes expensive copies.
* The CMake minimum version increased to 3.9.
* The `cancel` method in `CancellationToken` is deprecated in favor of the newly introduced `Cancel` method.
* Network logs improved for all platforms.
* Various bug fixes and improvements.
* The Network limit is now implemented for all platforms. When the limit is reached, the `Network::Send` method returns `olp::http::ErrorCode::NETWORK_OVERLOAD_ERROR`, and client users receive `olp::client::ErrorCode::SlowDown` as a response.
* Android network client can now be used when Java VM is embedded in a native application.

**olp-cpp-sdk-dataservice-read**

* The `VersionedLayerClient` and `VolatileLayerClient` classes are now movable and non-copyable.
* The `TaskContext` and `PendingRequests` classes moved to the core component, and are now public.
* The `GetCatalogMetadataVersion` method in `CatalogClient` is deprecated. The `GetLatestVersion` method should be used instead.
* All deprecated methods marked in v0.8.0 are now removed.
* A new optional parameter for a catalog version was added to `PrefetchTileRequest`. It is used by `VersionedLayerClient::PrefetchTiles()`. If not provided, the latest catalog version is queried from OLP instead.

**olp-cpp-sdk-dataservice-write**

* Missing `DATASERVICE_WRITE_API` was added to various classes.

## v0.8.0-beta (31/10/2019)

**Common**

* Project renamed to `here-olp-sdk-cpp`.
* Project structure was changed, functional, integration, performance tests introduced.
* Local OLP server that mimics OLP added.
* `OLP_SDK_DEPRECATED` macro is added to deprecate API.
* **Breaking Change** `SendOutcome::IsSuccessful` typo fixed.
* Missing dependencies for the iOS build as a shared library added.
* `KeyValueCache` instance was added to `OlpClientSetting`.
* Boost download source was changed.
* Added `Condition` class helping repositories sync wait on the OLP response.
* Updated read example and documentation to reflect recent changes in API.

**olp-cpp-sdk-authentication**

* **Breaking Change** `olp::authentication::Settings` requires `olp::authentication::AuthenticationCredentials`.

**olp-cpp-sdk-dataservice-read**

* Added new class `VersionedLayerClient` that is used to access versioned layers in OLP. Class implements `GetData`, `GetPartitions`, `PrefetchTiles` methods from `CatalogClient`.
* Added new class `VolatileLayerClient` that is used to access volatile layers in OLP. Class implements `GetPartitions`, `GetData` methods from `CatalogClient`.
* `GetData`, `GetPartitions`, `PrefetchTiles` methods in `CatalogClient` deprecated.
* **Breaking Change** CatalogClient constructor changed. `OlpClientSettings` must be passed by value, `KeyValueCache` is now part of `OlpClientSettings`.
* Moved all responses and callbacks aliases to  `Types.h`.

**olp-cpp-sdk-dataservice-write**

* `StreamLayerClientSettings` class introduced.
* **Breaking Change** `StreamLayerClient` constructor changed.
* **Breaking Change** `StreamLayerClient::Flush` changed. Now taking `FlushRequest` as input and a `FlushCallback` to be triggered after the flush.
* **Breaking Change** `StreamLayerClient::Enable`, `StreamLayerClient::Disable` removed.
* `VersionedLayerClient::CancelAll` added, used to cancel all current running requests.
* [CMake] Definition to export symbols added when the component is built as a shared library.
* `VolatileLayerClient`, `VersionedLayerClient`, `IndexLayerClient` now cancel pending requests when destroyed.

## v0.7.0-beta (09/09/2019)

**Common**

* Introduced new abstract `olp::thread::TaskScheduler` to be used for async context; provided default thread pool implementation.
* Added `UserAgent` HTTP header to mark all triggered requests.
* Added China Lookup API URLs.
* Introduced new HTTP abstraction layer `olp::http::Network` without singleton usage. Platform dependent implementations adapted and aligned with coding style.
* `olp::network::Network` with according implementations and helper classes removed.
* Changed `olp::client::CancellationContext` to shared pimpl so we can remove the `std::shared_ptr` wrapper when using it across multiple requests.
* Improved logging in core components.

**olp-edge-sdk-cpp-authentication**

* **Breaking Change** - Use new HTTP layer as input parameter for all requests. This is a mandatory parameter and users must provide it.
* **Breaking Change** - Use new `olp::thread::TaskScheduler`; provided as input parameter.
* Removed usage of raw `std::thread` and switched to `olp::thread::TaskScheduler`. If the `olp::thread::TaskScheduler` instance is not provided, all tasks are executed in the calling thread.

**olp-edge-sdk-cpp-dataservice-read**

* **Breaking Change** - Use new HTTP layer as input parameter for all requests. This is a mandatory parameter and users must provide it.
* **Breaking Change** - Use new `olp::thread::TaskScheduler`; provided as input parameter.
* Removed usage of raw `std::thread` and switched to `olp::thread::TaskScheduler`. If the `olp::thread::TaskScheduler` instance is not provided, all tasks are executed in the calling thread.
* Improved logging.

**olp-edge-sdk-cpp-dataservice-write**

* **Breaking Change** - Use new HTTP layer as input parameter for all requests. This is a mandatory parameter and users must provide it.
* **Breaking Change** - `olp::dataservice::write::StreamLayerClient` uses new `olp::thread::TaskScheduler` as input parameter for async context. If the `olp::thread::TaskScheduler` instance is not provided, all tasks are executed in the calling thread.

## v0.6.0-beta (27/06/2019)
Initial open source release as a _work in progress_ beta.
<br />Not all OLP features are implemented, API breaks very likely with following commits and releases.

**olp-edge-sdk-cpp-authentication**

* Sign up new user with e-mail and password.
* Sign up new user with Facebook, Google, ArcGIS credentials.
* Accept terms and log out user.
* AAA OAuth2 with registered user credentials.

**olp-edge-sdk-cpp-dataservice-read**

* Read from Catalog (configuration, layers).
* Read from Versioned Layer (partitions metadata, partition data).
* Read from Volatile Layer (partitions metadata, partition data).
* Cache results on disk for later use.

**olp-edge-sdk-cpp-dataservice-write**

* Write to Versioned Layer (initialize publication, upload data and metadata, submit publication).
* Write to Volatile Layer (initialize publication, upload metadata, submit publication, upload data).
* Write to Stream Layer (publish data, queue data for asyncronous publish, publish SDII messages, batch write).
* Write to Index Layer (publish index, delete index, update index).
