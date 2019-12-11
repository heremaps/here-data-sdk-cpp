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
