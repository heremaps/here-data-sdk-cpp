# Read example

On this page, find instructions on how to build and run the read example project on different platforms and get catalog and partition metadata, as well as partition data using HERE Data SDK for C++.

**Before you run the example project, authorize to the HERE platform:**

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.

   For instructions on how to get the access key ID and access key secret, see [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

2. In <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/main.cpp" target="_blank">`examples/main.cpp`</a>, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.

   > #### Note
   >  You can also specify these values using the command line options.

   ```cpp
   AccessKey access_key{};  // Your access key ID and access key secret.
   ```

## Build and run on Linux

**To build and run the example project on Linux:**

1. Enable examples of the CMake targets.

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

- `edge-example-catalog` – a catalog description.
- `versioned-world-layer` – a layer description.
- `Request partition data - Success, data size - 3375` – a success message that displays the size of data retrieved from the requested partition.

```bash
[INFO] read-example - Catalog description: edge-example-catalog
[INFO] read-example - Layer 'versioned-world-layer' (versioned): versioned-world-layer
[INFO] read-example - Layer contains 1 partitions
[INFO] read-example - Partition: 1
[INFO] read-example - Request partition data - Success, data size - 3375
```

## Build and run on Android

To integrate the Data SDK libraries in the Android example project:

- [Set up prerequisites](#prerequisites-for-android)
- [Build the Data SDK](#build-the-data-sdk-on-android)
- [Build and run the APK](#build-and-run-the-apk)

### Prerequisites for Android

1. Set up the Android environment.
2. In <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/android/app/src/main/cpp/MainActivityNative.cpp.in" target="_blank">`examples/android/app/src/main/cpp/MainActivityNative.cpp.in`</a>, replace the placeholders with your application access key ID, access key secret, and catalog HRN and specify that the example should run `RunExampleRead`.

> #### Note
> To learn how to get the access key ID and access key secret, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

### Build the Data SDK on Android

1. Set `OLP_SDK_BUILD_EXAMPLES` to `ON`.
2. (Optional) To disable tests, set `OLP_SDK_ENABLE_TESTING` to `OFF`.
3. Specify the path to the Android NDK toolchain file using the `CMAKE_TOOLCHAIN_FILE` variable.
4. If you want to build the SDK for a specific Android platform, use the `-DANDROID_PLATFORM CMake` flag, and if you want to build the SDK for a specific Android architecture, use the `-DANDROID_ABI` flag.
   For more details, see [NDK-specific CMake variables](https://developer.android.com/ndk/guides/cmake#variables).

   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
   ```

   The CMake command generates a `Gradle` project in the <a href="https://github.com/heremaps/here-data-sdk-cpp/tree/master/examples/android" target="_blank">`build/examples/android`</a> folder.

5. Install the Data SDK libraries into the **sysroot** directory.

   ```bash
   # If necessary, execute as sudo.
   (sudo) make install
   ```

### Build and run the APK

1. In the Android Studio IDE, open the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/android/build.gradle" target="_blank">`build/examples/android/build.gradle`</a> script.
2. Provide your application access key ID, access key secret, and catalog HRN.
3. Install and run the `dataservice_example` APK.

The main screen displays the following message: "Example has finished successfully".

## Build and run on iOS

To integrate the Data SDK libraries in the iOS example project written in the Objective-C language:

- [Set up prerequisites](#prerequisites-for-ios)
- [Build the Data SDK](#build-the-data-sdk-on-ios)
- [Build and run the application](#build-and-run-the-application)

### Prerequisites for iOS

1. To set up the iOS development environment, install the Xcode and command-line tools.
2. Install external dependencies.

   For information on dependencies and installation instructions, see the <a href="https://github.com/heremaps/here-data-sdk-cpp#dependencies" target="_blank">related section</a> in the README.md file.

3. In <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/ios/ViewController.mm" target="_blank">`examples/ios/ViewController.mm`</a>, replace the placeholders with your application access key ID, access key secret, and catalog HRN and specify that the example should run `RunExampleRead`.

> #### Note
> To learn how to get the access key ID and access key secret, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

### Build the Data SDK on iOS

1. Set `OLP_SDK_BUILD_EXAMPLES` to `ON`.
2. (Optional) To disable tests, set `OLP_SDK_ENABLE_TESTING` to `OFF`.
3. Specify the path to the iOS toolchain file using the `CMAKE_TOOLCHAIN_FILE` variable.

   > #### Note
   > The iOS toolchain file is shipped together with the SDK and located under the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/cmake/toolchains/iOS.cmake" target="_blank">`<olp-sdk-root>/cmake/toolchains/iOS.cmake`</a>.

```bash
mkdir build && cd build
cmake .. -GXcode  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/iOS.cmake -DPLATFORM=iphoneos -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
```

To configure Data SDK for a simulator, set the `SIMULATOR` CMake variable to `ON`.

### Build and run the application

1. Open the generated Xcode project.

   ```bash
   open olp-cpp-sdk.xcodeproj
   ```

2. In the Xcode project, from the list of schemes, select the `dataservice-example` scheme.

3. In the `dataservice-example` target, specify your application access key ID and access key secret.

4. Build and run the example application.

The main UI screen displays the following message: "Reading the partition data from the specified catalog completed successfully". For more details, check the device logs.

If you encounter an error message, for a detailed error description, check the device logs. Example of an error message: "Failed to read data from the specified catalog!"

## How it works

### Get catalog metadata

Catalog metadata contains a list of configurations that describe the catalog and its layers. Configuration information about the catalog includes the following metadata:

- Name
- HERE Resource Name (HRN)
- Description
- Owner
- Version
- Layer information

**To get catalog metadata:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](create-platform-client-settings.md).

2. Create the `CatalogClient` object with the catalog HRN and platform client settings from step 1.

   ```cpp
   olp::dataservice::read::CatalogClient catalog_client(
           olp::client::HRN(kCatalogHRN), client_settings);
   ```

3. Create the `CatalogRequest` object.

   ```cpp
   auto request = olp::dataservice::read::CatalogRequest();
   ```

4. (Optional) Set the needed parameters. For example, to set the billing tag, set the `WithBillingTag` parameter.

   ```cpp
   request.WithBillingTag("MyBillingTag");
   ```

5. Call the `GetRequest` method with the `CatalogRequest` parameter.

   ```cpp
   auto future = catalog_client.GetCatalog(request);
   ```

6. Wait for `CatalogResponse` future.

   ```cpp
   olp::dataservice::read::CatalogResponse catalog_response = future.GetFuture().get();
   ````

The `CatalogResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` – if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()` – if the operation is successful, returns the following resultant data: `olp::dataservice::read::CatalogResult`
- `GetError()` – contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (catalog_response.IsSuccessful()) {
    const auto& response_result = catalog_response.GetResult();
    // Handle success
} else {
    auto api_error = catalog_response.GetError();
    // Handle fail
}
```

The `CatalogResult` class contains the following methods used to get details of the relevant catalog:

- `GetId` – returns the catalog ID.
- `GetHrn` – returns the catalog `HRN`.
- `GetName` – returns the catalog name.
- `GetSummary` – returns the summary description of the catalog.
- `GetDescription` – returns the full description of the catalog.
- `GetCoverage` – returns the coverage area of the catalog.
- `GetOwner` – returns the identity of the catalog owner.
- `GetTags` – returns the catalog tags collection.
- `GetBillingTags` – returns the billing tags set on the catalog.
- `GetCreated` – returns the catalog creation time.
- `GetLayers` – returns details of the layers contained in the catalog.
- `GetVersion` – returns the current catalog version number.
- `GetNotifications` – returns the catalog notification status.

The `ApiError` class contains the following methods used to get details of the incurred error:

- `GetErrorCode` – returns the `ErrorCode` value defined by the `olp::client::ErrorCode enum`. For more details, see `ErrorCode.h`.
- `GetHttpStatusCode` – returns the HTTP response code.
- `GetMessage` – returns a text description of the encountered error.
- `ShouldRetry` – returns `true` if this operation can be retried.

### Get partition metadata

Partition metadata consists of the following information about the partition:

- Data handle
- ID
- Version
- Data size
- Compressed data size
- Checksum

**To get partition metadata:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](create-platform-client-settings.md).

2. Depending on the layer type, create a versioned or volatile layer client with the HERE Resource Name (HRN), layer ID, layer version, and platform client settings from step 1.

   If you do not specify a catalog version, the latest version is used. Depending on the fetch option that you specified in your first API call to the client, the `GetLatestVersion` method automatically gets the latest version in one of the following ways:

   - For the `OnlineIfNotFound` fetch option, queries the network, and if an error occurs, checks the cache. If the online version is higher than the cache version, the cache version is updated.
   - For the `OnlineOnly` fetch option, only queries the network.
   - For the `CacheOnly` fetch option, only checks the cache.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client(
      olp::client::HRN(kCatalogHRN), layer_id, version, client_settings);
   ```

3. Create the `PartitionsRequest` object with one of the following fetch options:

   - (Default) To query network if the requested resource is not found in the cache, use `OnlineIfNotFound`.
   - To skip cache lookups and query the network right away, use `OnlineOnly`.
   - To return immediately if a cache lookup fails, use `CacheOnly`.
   - (Not for `VersionedLayerClient`) To return the requested cached resource if it is found and update the cache in the background, use `CacheWithUpdate`.

   ```cpp
   auto request =
       olp::dataservice::read::PartitionsRequest()
           .WithBillingTag("MyBillingTag")
           .WithFetchOption(
               olp::dataservice::read::FetchOptions::OnlineIfNotFound);
   ```

4. Call `GetPartitions` method with the `PartitionRequest` parameter.

   ```cpp
   auto future = layer_client.GetPartitions(request);
   ```

   > #### Note
   > If your layer uses tile keys as partition IDs, this operation can fail because of the large amount of data.

5. Wait for the `PartitionsResponse` future.

   ```cpp
   olp::dataservice::read::PartitionsResponse partitions_response =
       future.GetFuture().get();
   ```

The `PartitionsResponse` object holds the details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` – if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()` – if the operation is successful, returns the following resultant data: `olp::dataservice::read::PartitionsResult`
- `GetError()` – contains error information as a result of an error in the `olp::client::ApiError` object.

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

- `GetChecksum` – returns partition checksum.
- `GetCompressedDataSize` – returns the size of the compressed partition data.
- `GetDataHandle` – returns the handle that can be used by the `GetData` function to retrieve the partition data.
- `GetDataSize` – returns the size of the partition data.
- `GetPartition` – returns the partition ID.
- `GetVersion` – returns the latest catalog version for the partition.

### Get data from a versioned layer

You can request any data version from a [versioned layer](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/layers.html#versioned-layers). When you request a particular version of data from the versioned layer, the partition you receive in the response may have a lower version number than you requested. The version of a layer or partition represents the catalog version in which the layer or partition was last updated.

**To get data from the versioned layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](create-platform-client-settings.md).

2. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, catalog version, and the platform client settings from step 1.

   If you do not specify a catalog version, the latest version is used. Depending on the fetch option that you specified in your first API call to the client, the `GetLatestVersion` method automatically gets the latest version in one of the following ways:

   - For the `OnlineIfNotFound` fetch option, queries the network, and if an error occurs, checks the cache. If the online version is higher than the cache version, the cache version is updated.
   - For the `OnlineOnly` fetch option, only queries the network.
   - For the `CacheOnly` fetch option, only checks the cache.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client (
                       client::HRN catalog,
                       std::string layer_id,
                       porting::optional<int64_t> catalog_version,
                       client::OlpClientSettings settings);
   ```

3. Create the `DataRequest` object with the partition ID and one of the following fetch options:

   - (Default) To query the network if the requested resource is not found in the cache, use `OnlineIfNotFound`.
   - To skip cache lookups and query the network right away, use `OnlineOnly`.
   - To return immediately if a cache lookup fails, use `CacheOnly`.

   ```cpp
   auto request = olp::dataservice::read::DataRequest()
                      .WithPartitionId(partition_id)
                      .WithBillingTag("MyBillingTag")
                      .WithFetchOption(olp::dataservice::read::FetchOptions::OnlineIfNotFound);
   ```

4. Call the `GetRequest` method with the `DataRequest` parameter.

   ```cpp
   auto future = layer_client.GetData(request);
   ```

5. Wait for the `DataResponse` future.

   ```cpp
   olp::dataservice::read::DataResponse data_response = future.GetFuture().get();
   ```

The `DataResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` – if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()` – if the operation is successful, returns the following resultant data: `olp::dataservice::read::DataResult`
- `GetError()` – contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (data_response.IsSuccessful()) {
    const auto& response_result = data_response.GetResult();
    // Handle success
} else {
    auto api_error = data_response.GetError();
    // Handle fail
}
```

The `DataResult` class contains raw partition data that is an alias for a `std::shared_ptr<std::vector<unsigned char>>`.
