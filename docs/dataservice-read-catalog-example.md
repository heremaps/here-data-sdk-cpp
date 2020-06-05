# Read Example

On this page, find instructions on how to build and run the example project on different platforms and get catalog and partition metadata, as well as partition data using the HERE Data SDK for C++.

Before you run the example project, authorize to the HERE platform:

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.
   For instructions on how to get the access key ID and access key secret, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

2. In `examples/main.cpp`, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.
   **Note:**  You can also specify these values using the command line options.

   ```cpp
   AccessKey access_key{};  // Your access key ID and access key secret.
   ```

## Build and Run on Linux

To build and run the example project on Linux:

1. Enable examples of the `CMake` targets.

   ```bash
   mkdir build && cd build
   cmake -DOLP_SDK_BUILD_EXAMPLES=ON ..
   ```

2. In the **build** folder, build the example project.

   ```bash
   cmake --build . --target dataservice-example 
   ```

3. Execute the example project.

   ```bash
   ./examples/dataservice-example --example read --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog"
   ```

4. (Optional) To run the example with other parameters, run the help command, and then select the needed parameter. 
   ```bash
   ./examples/dataservice-example --help
   ```
After building and running the example project, you see the following information:

- `edge-example-catalog` &ndash; a catalog description
- `versioned-world-layer` &ndash; a layer description
- `Request partition data - Success, data size - 3375` - a success message that displays the size of data retrieved from the requested partition.

```bash
[INFO] read-example - Catalog description: edge-example-catalog
[INFO] read-example - Layer 'versioned-world-layer' (versioned): versioned-world-layer
[INFO] read-example - Layer contains 1 partitions
[INFO] read-example - Partition: 1
[INFO] read-example - Request partition data - Success, data size - 3375
```

## Build and Run on Android

To integrate the HERE Data SDK for C++ libraries in the Android example project:

- [Set up prerequisites](#prerequisites-android)
- [Build the HERE Data SDK for C++](#build-sdk-android)
- [Build and Run the APK](#build-and-run-android)

### <a name="prerequisites-android"></a>Prerequisites

Before you integrate the HERE Data SDK for C++ libraries in the Android example project:

1. Set up the Android environment.
2.  In `examples/android/app/src/main/cpp/MainActivityNative.cpp.in`, replace the placeholders with your application access key ID, access key secret, and catalog HRN and specify that the example should run `RunExampleRead`.
   For instructions on how to get the access key ID and access key secret, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

### <a name="build-sdk-android"></a>Build the HERE Data SDK for C++

To build the HERE Data SDK for C++ on Android:

1. Set `OLP_SDK_BUILD_EXAMPLES` to `ON`.
2. (Optional) To disable tests, set `OLP_SDK_ENABLE_TESTING` to `OFF`.
3. Specify the path to the Android NDK toolchain file using the `CMAKE_TOOLCHAIN_FILE` variable.
4. If you want to build the SDK for a specific Android platform, use the `-DANDROID_PLATFORM CMake` flag, and if you want to build the SDK for a specific Android architecture, use the `-DANDROID_ABI` flag.
   For more details, see [NDK-specific CMake variables](https://developer.android.com/ndk/guides/cmake#variables).

   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
   ```

   The `CMake` command generates a `Gradle` project in the `build/examples/android` folder.

5. Install the HERE Data SDK for C++ libraries into the **sysroot** directory.

   ```bash
   # If necessary, execute as sudo.
   (sudo) make install
   ```

### <a name="build-and-run-android"></a>Build and Run the APK

To build and run the APK:

1. In the Android Studio IDE, open the `build/examples/android/build.gradle` script.
2. Provide your application access key ID, access key secret, and catalog HRN.
3. Install and run the `dataservice_example` APK.

The main screen displays the following message: "Example has finished successfully"

## Build and run on iOS

To integrate the HERE Data SDK for C++ libraries in the iOS example project written in the Objective-C language:

- [Set up prerequisites](#prerequisites-ios)
- [Build the HERE Data SDK for C++](#build-sdk-ios)
- [Build and Run the Application](#build-and-run-ios)

### <a name="prerequisites-ios"></a>Prerequisites

Before you integrate the HERE Data SDK for C++ libraries in the iOS example project:

1. To set up the iOS development environment, install the `XCode` and command-line tools.
2. Install external dependencies.
   For information on dependencies and installation instructions, see the [related section](https://github.com/heremaps/here-data-sdk-cpp#dependencies) in the README.md file.
3. In `examples/ios/ViewController.mm`, replace the placeholders with your application access key ID, access key secret, and catalog HRN and specify that the example should run `RunExampleRead`.
   For instructions on how to get the access key ID and access key secret, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

### <a name="build-sdk-ios"></a>Build the HERE Data SDK for C++

To build the HERE Data SDK for C++ on iOS:

1. Set `OLP_SDK_BUILD_EXAMPLES` to `ON`.
2. (Optional) To disable tests, set `OLP_SDK_ENABLE_TESTING` to `OFF`.
3. Specify the path to the iOS toolchain file using the `CMAKE_TOOLCHAIN_FILE` variable.
   > **Note:** The iOS toolchain file is shipped together with the SDK and located under the `<olp-sdk-root>/cmake/toolchains/iOS.cmake`.

```bash
mkdir build && cd build
cmake .. -GXcode  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/iOS.cmake -DPLATFORM=iphoneos -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
```

To configure the HERE Data SDK for C++ for a simulator, set the `SIMULATOR` CMake variable to `ON`.

### <a name="build-and-run-ios"></a>Build and Run the Application

To build an run the example application on iOS:

1. Open the generated `XCode` project.

   ```bash
   open olp-cpp-sdk.xcodeproj
   ```

2. In the `XCode` project, from the list of schemes, select the `dataservice-example` scheme.

3. In the `dataservice-example` target, specify your application access key ID and access key secret.

4. Build and run the example application.

The main UI screen displays the following message: "Reading the partition data from the specified catalog completed successfully". For more details, check the device logs.

If you encounter an error message, for a detailed error description, check the device logs. Example of an error message: "Failed to read data from the specified catalog!"

## How it works

### <a name="authenticate-to-here-olp-using-client-credentials"></a>Authenticate to the HERE Platform Using Client Credentials

To authenticate with the HERE platform, you must get platform credentials that contain the access key ID and access key secret.

**To authenticate using client credentials:**

1. Get your platform credentials.

   For instructions, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

   You get the `credentials.properties` file.

2. Initialize the authentification settings using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   ```cpp
   olp::authentication::Settings settings({kKeyId, kKeySecret});
   settings.task_scheduler = task_scheduler;
   settings.network_request_handler = http_client;
   ```

3. To get the OAuth 2.0 token from the HERE platform, set up the `AuthenticationSettings` object with a default token provider.

   ```cpp
   olp::client::AuthenticationSettings auth_settings;
   auth_settings.provider =
     olp::authentication::TokenProviderDefault(std::move(settings));
   ```

You can use the `TokenProvider` object to create the `OlpClientSettings` object. For more information, see [Create OlpClientSettings](#create-olpclientsettings).

### <a name="create-olpclientsettings"></a>Create `OlpClientSettings`

You need to create the `OlpClientSettings` object to get catalog and partition metadata, retrieve versioned, volatile, and stream layer data, and publish data to the HERE platform.

**To create `OlpClientSettings` object:**

1. To perform requests asynchronously, create the `TaskScheduler` object.

   ```cpp
   std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
         olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);
   ```

2. To internally operate with the HERE platform Services, create the `Network` client.

   ```cpp
   std::shared_ptr<olp::http::Network> http_client = olp::client::
        OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();
   ```

   > Note: The `Network` client is designed and intended to be shared.

3. [Authenticate](#authenticate-to-here-olp-using-client-credentials) to the HERE platform.

4. (Optional) For data that is stored in the cache, to add expiration limit, set the `default_cache_expiration` to the needed expiration time.

   By default, expiration is disabled.

   > Note: You can only disable expiration for the mutable and in-memory cache. This setting does not affect the protected cache as no entries are added to the protected cache in the read-only mode.

   ```cpp
   std::chrono::seconds default_cache_expiration = std::chrono::seconds(200);
   ```

5. Set up the `OlpClientSettings` object.

   ```cpp
   olp::client::OlpClientSettings client_settings;
   client_settings.authentication_settings = auth_settings;
   client_settings.task_scheduler = std::move(task_scheduler);
   client_settings.network_request_handler = std::move(http_client);
   client_settings.cache =
       olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
   client_settings.default_cache_expiration = std::chrono::seconds(200);
   ```

The `OlpClientSettings` class pulls together all the settings for customization of the client library behavior:

- `retry_settings` &ndash; sets `olp::client::RetrySettings` to use.
- `proxy_settings` &ndash; sets `olp::authentication::NetworkProxySettings` to use.
- `authentication_settings` &ndash; sets `olp::client::AuthenticationSettings` to use.
- `network_request_handler` &ndash; sets the handler for asynchronous execution of network requests.

Configuration of the `authentication_settings` is sufficient to start using the `CatalogClient`.

### Get Catalog Metadata

Catalog metadata contains a list of configurations that describe the catalog and its layers. Configuration information about the catalog includes the following metadata:

- Name
- HERE Resource Name (HRN)
- Description
- Owner
- Version
- Layer information

**To get catalog metadata:**

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Create the `CatalogClient` object with the catalog HRN and platform client settings from step 2.

   ```cpp
   olp::dataservice::read::CatalogClient catalog_client(
           olp::client::HRN(catalog), client_settings);
   ```

4. Create the `CatalogRequest` object.

   ```cpp
   auto request =
   olp::dataservice::read::CatalogRequest();
   ```

5. (Optional) Set the needed parameters. For example, to set the billing tag, add the `WithBillingTag` parameter.

   ```cpp
   request.WithBillingTag("MyBillingTag");
   ```

6. Call the `GetRequest` method with the `CatalogRequest` parameter.

   ```cpp
   auto future = catalog_client.GetCatalog(request);
   ```

7. Wait for `CatalogResponse` future.

   ```cpp
   olp::dataservice::read::CatalogResponse catalog_response =
   future.GetFuture().get();
   ```

The `CatalogResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()`&ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::read::CatalogResult`
- `GetError()` &ndash; contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (catalog_response.IsSuccessful()) {
    auto response_result =
        catalog_response.GetResult();
    // Handle success
} else {
    auto api_error = catalog_response.GetError();
    // Handle fail
}
```

The `CatalogResult` class contains the following methods used to get details of the relevant catalog:

- `GetId` &ndash; returns the catalog ID.
- `GetHrn` &ndash; returns the catalog `HRN`.
- `GetName` &ndash; returns the catalog name.
- `GetSummary` &ndash; returns the summary description of the catalog.
- `GetDescription` &ndash; returns the full description of the catalog.
- `GetCoverage` &ndash; returns the coverage area of the catalog.
- `GetOwner` &ndash; returns the identity of the catalog owner.
- `GetTags` &ndash; returns the catalog tags collection.
- `GetBillingTags` &ndash; returns the billing tags set on the catalog.
- `GetCreated` &ndash; returns the catalog creation time.
- `GetLayers` &ndash; returns details of the layers contained in the catalog.
- `GetVersion` &ndash; returns the current catalog version number.
- `GetNotifications` &ndash; returns the catalog notification status.

The `ApiError` class contains the following methods used to get details of the incurred error:

- `GetErrorCode` &ndash; returns the `ErrorCode` value defined by the `olp::client::ErrorCode enum`. For more details, see `ErrorCode.h`.
- `GetHttpStatusCode` &ndash; returns the HTTP response code.
- `GetMessage` &ndash; returns a text description of the encountered error.
- `ShouldRetry`&ndash; returns `true` if this operation can be retried.

### Get Partition Metadata

Partition metadata consists of the following information about the partition:

- Data handle
- ID
- Version
- Data size
- Compressed data size
- Checksum

**To get partition metadata:**

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Depending on the layer type, create a versioned or volatile layer client with the HERE Resource Name (HRN), layer ID, and platform client settings from step 2.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client(
   olp::client::HRN(catalog), layer_id, client_settings);
   ```

4. Create the `PartitionsRequest` object.

   ```cpp
   auto request =
   olp::dataservice::read::PartitionsRequest().WithBillingTag("MyBillingTag");
   ```

5. Call `GetPartitions` method with the `PartitionRequest` parameter.

   ```cpp
   auto future = layer_client.GetPartitions(request);
   ```

   > **Note:** If your layer uses TileKeys as partition IDs, then this operation can fail because of the large amount of data.

6. Wait for the `PartitionsResponse` future.

   ```cpp
   olp::dataservice::read::PartitionsResponse partitions_response =
       future.GetFuture().get();
   ```

The `PartitionsResponse` object holds the details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()`&ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::read::PartitionsResult`
- `GetError()` &ndash; contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (partitions_response.IsSuccessful()) {
    const olp::dataservice::read::PartitionsResult& response_result =
        partitions_response.GetResult();
    // Handle success
} else {
    // Handle fail
}
```

The `PartitionsResult` class contains the `GetPartitions` method that returns a collection of partition metadata objects of the `olp::dataservice::read::model::Partition` type.

The `Partition` class contains partition metadata and exposes the following members:

- `GetChecksum` &ndash; returns partition checksum.
- `GetCompressedDataSize` &ndash; returns the size of the compressed partition data.
- `GetDataHandle` &ndash; returns the handle that can be used by the `GetData` function to retrieve the partition data.
- `GetDataSize` &ndash; returns the size of the partition data.
- `GetPartition` &ndash; returns the partition ID.
- `GetVersion` &ndash; returns the latest catalog version for the partition.

### Get Partition Data from a Versioned Layer

You can request any data version from a [versioned layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#versioned-layers). When you request a particular version of data from the versioned layer, the partition you receive in the response may have a lower version number than you requested. The version of a layer or partition represents the catalog version in which the layer or partition was last updated.

**To get data from the versioned layer:**

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, catalog version, and the platform client settings from step 2.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client(
   olp::client::HRN(catalog), layer_id, client_settings);
   ```

4. Create the `DataRequest` object with the layer version and partition ID.

   > **Note:** If a catalog version is not specified, the latest version is used.

   ```cpp
   auto request = olp::dataservice::read::DataRequest()
                      .WithVersion(version_number)
                      .WithPartitionId(partition_id)
                      .WithBillingTag("MyBillingTag");
   ```

5. Call the `GetRequest` method with the `DataRequest` parameter.

   ```cpp
   auto future = layer_client.GetData(request);
   ```

6. Wait for the `DataResponse` future.

   ```cpp
   olp::dataservice::read::DataResponse data_response =
   future.GetFuture().get();
   ```

The `DataResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()`&ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::read::DataResult`
- `GetError()` &ndash; contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (data_response.IsSuccessful()) {
    auto response_result = data_response.GetResult();
    // Handle success
} else {
    auto api_error = data_response.GetError();
    // Handle fail
}
```

The `DataResult` class contains the raw partition data that is an alias for a `std::shared_ptr<std::vector<unsigned char>>`.
