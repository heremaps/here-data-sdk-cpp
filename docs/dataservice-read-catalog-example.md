# Read example

This example shows how to retrieve catalog configuration, partition metadata, and partition data using the HERE OLP SDK for C++.

Before you run the example, authorize by replacing the placeholders in `examples/dataservice-read/example.cpp` with your app access key id and access key secret that you can find on the platform `Apps & keys` page:

```cpp
const std::string kKeyId(""); // your here.access.key.id
const std::string kKeySecret(""); // your here.access.key.secret
```

## Building and running on Linux

Configure the project with `OLP_SDK_BUILD_EXAMPLES` set to `ON` to enable examples `CMake` targets:

```bash
mkdir build && cd build
cmake -DOLP_SDK_BUILD_EXAMPLES=ON ..
```

To build the example, run the following command in the `build` folder:

```bash
cmake --build . --target dataservice-read-example
```

To execute the example, run:

```bash
./examples/dataservice-read/dataservice-read-example
```

After building and running the example, you see the following:

* `edge-example-catalog` as a catalog description
* `versioned-world-layer` as a layer description
* `Request partition data - Success, data size - 3375`

```bash
[INFO] read-example - Catalog description: edge-example-catalog
[INFO] read-example - Layer 'versioned-world-layer' (versioned): versioned-world-layer
[INFO] read-example - Layer contains 1 partitions
[INFO] read-example - Partition: 1
[INFO] read-example - Request partition data - Success, data size - 3375
```

## Building and running on Android

This example shows how to integrate the HERE OLP SDK for C++ libraries into the Android project and read data from a catalog layer and partition.

### Prerequisites

* Set up the Android environment.
* Replace the placeholders in `examples/dataservice-read/example.cpp` with your app access key id and secret access key.

### Build HERE OLP SDK for C++

Before you build the example, complete the following:

* Configure the SDK with `OLP_SDK_BUILD_EXAMPLES` set to `ON`.
* Optionally, disable tests with `OLP_SDK_ENABLE_TESTING` set to `OFF`.
* Specify the path to Android NDK's toolchain file set via the `CMAKE_TOOLCHAIN_FILE` variable.
* In case you want to build SDK for a specific Android platform, use `-DANDROID_PLATFORM CMake` flag, and `-DANDROID_ABI`, if you want to build for specific Android architecture. For more details, see [NDK-specific CMake variables](https://developer.android.com/ndk/guides/cmake#variables).

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
```

The `CMake` command generates a `Gradle` project in the `build/examples/dataservice-read/android` folder. Before using the project, install the HERE OLP SDK for C++ libraries into the sysroot directory:

```bash
# Execute as sudo if necessary
(sudo) make install
```

### Assemble APK and run it on Android device

After you complete the `Gradle` project configuration, you can use `gradlew` executable to build and install the APK on your Android device:

```bash
cd examples/dataservice-read/android
./gradlew assembleDebug
./gradlew installDebug
```

Alternatively, you can use Android Studio IDE by opening the `build/examples/dataservice-read/android/build.gradle` script.

After you provide your app access key id and secret access key, install and run `dataservice_read_example` APK, the main UI screen displays a message `Reading the partition data from the specified catalog completed successfully`. If you encounter an error, please check the device's logcat for the error message.

> **Note:** you can run `CMake` command directly from the `<olp-sdk-root>/examples/dataservice-read/` folder if you have already built and installed HERE OLP SDK for C++ libraries for Android. Make sure you provide the correct path to the `LevelDB` library and `OLP_SDK_NETWORK_PROTOCOL_JAR` parameter in the `CMake` command, invoked by the `build/examples/dataservice-read/android/app/build.gradle` script.

## Building and running on iOS

This example shows how to integrate the HERE OLP SDK for C++ libraries into a basic iOS application written in Objective-C language and read data from a catalog layer and partition.

### Prerequisites

* Set up the iOS development environment by installing the `XCode` and command-line tools.
* Install external dependencies by referring to the `README.md` file located under the `<olp-sdk-root>/README.md`.
* Replace the placeholders in `examples/dataservice-read/example.cpp` with your app access key id and secret access key.

### Build HERE OLP SDK for C++

Before you build the example, complete the following:

* Configure the HERE OLP SDK for C++ with `OLP_SDK_BUILD_EXAMPLES` set to `ON`.
* Optionally, disable tests with `OLP_SDK_ENABLE_TESTING` set to `OFF`.
* Specify the path to the iOS toolchain file shipped together with the SDK, located under the `<olp-sdk-root>/cmake/toolchains/iOS.cmake`:

```bash
mkdir build && cd build
cmake .. -GXcode  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/iOS.cmake -DPLATFORM=iphoneos -DOLP_SDK_BUILD_EXAMPLES=ON -DOLP_SDK_ENABLE_TESTING=OFF
```

> **Note:** to configure the HERE OLP SDK for C++ for a simulator, set the `SIMULATOR` variable to `ON`.

### Build and run the application on the device

Now open the generated `XCode` project:
```bash
open olp-cpp-sdk.xcodeproj
```

Select the `dataservice-read-example` scheme from the schemes list in `XCode` project and specify your signing credentials for the `dataservice-read-example` target.

Once everything is set up correctly, build and run the example application on your device. The main UI screen displays a message `Reading the partition data from the specified catalog completed successfully`. Check device's logs for more details. If you encounter an error message, e.g. `Failed to read data from the specified catalog!`, please check the device's logs for the detailed description of the error.

## How it works

### CatalogClient

The main entry point for the OLP is `CatalogClient`. This class provides a high-level interface to retrieve `OLP` catalog data and configuration, and defines the following operations:

* `GetCatalog`: Fetches the catalog configuration asynchronously.
* `GetLatestVersion`: Fetches the catalog version asynchronously.

To create a `CatalogClient`, provide the corresponding Here Resource Name (`HRN`) and the preconfigured `OlpClientSettings`:

```cpp
// Create a task scheduler instance
std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
    olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

// Create a network client
std::shared_ptr<olp::http::Network> http_client = olp::client::
    OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

// Initialize authentication settings
olp::authentication::Settings settings({kKeyId, kKeySecret});
settings.task_scheduler = task_scheduler;
settings.network_request_handler = http_client;

// Setup AuthenticationSettings with a default token provider that will
// retrieve an OAuth 2.0 token from OLP.
olp::client::AuthenticationSettings auth_settings;
auth_settings.provider =
    olp::authentication::TokenProviderDefault(std::move(settings));

// Setup OlpClientSettings and provide it to the CatalogClient.
olp::client::OlpClientSettings client_settings;
client_settings.authentication_settings = auth_settings;
client_settings.task_scheduler = std::move(task_scheduler);
client_settings.network_request_handler = std::move(http_client);
client_settings.cache =
    olp::client::OlpClientSettingsFactory::CreateDefaultCache({});

// Create a CatalogClient with appropriate HRN and settings.
olp::dataservice::read::CatalogClient catalog_client(
    olp::client::HRN(kCatalogHRN), client_settings);
```

The `OlpClientSettings` class pulls together all the different settings for customization of the client library behavior.

* `retry_settings`: Sets the `olp::client::RetrySettings` to use.
* `proxy_settings`: Sets the `olp::authentication::NetworkProxySettings` to use.
* `authentication_settings`: Sets the `olp::client::AuthenticationSettings` to use.
* `network_request_handler`: Sets the handler for asynchronous execution of network requests.

Configuration of the `authentication_settings` is sufficient to start using the `CatalogClient`.

### Retrieve catalog configuration

To retrieve catalog configuration, create a `CatalogRequest`. The `CatalogRequest` class allows you to set properties of the catalog request, including:

* `WithBillingTag`: Sets the billing tag used for this request.

```cpp
// Create CatalogRequest
auto request =
    olp::dataservice::read::CatalogRequest().WithBillingTag(boost::none);
```

Then pass it to the `CatalogClient` via `GetCatalog` method:

```cpp
// Run the CatalogRequest
auto future = catalog_client.GetCatalog(request);
```

The execution result is a `CancellableFuture` that contains `CatalogResponse` object. The `CatalogResponse` class holds details of the completed operation and is used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true in case of a successful response, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::CatalogResult`) in case of a successful operation.
* `GetError`: Contains the error information as a result of an error in the  `olp::client::ApiError` object.

```cpp
// Wait for the CatalogResponse response
olp::dataservice::read::CatalogResponse catalog_response =
    future.GetFuture().get();

// Check the response
if (catalog_response.IsSuccessful()) {
    const olp::dataservice::read::CatalogResult& response_result =
        catalog_response.GetResult();
    // Handle success
} else {
    // Handle fail
}
```

The `CatalogResult` class contains details of the relevant catalog, including:

* `GetId`: Returns the catalog id.
* `GetHrn`: Returns the catalog `HRN`.
* `GetName`: Returns the catalog name.
* `GetSummary`: Returns the summary description of the catalog.
* `GetDescription`: Returns the full description of the catalog.
* `GetCoverage`: Returns the coverage area of the catalog.
* `GetOwner`: Returns the identity of the owner of the catalog.
* `GetTags`: Returns the tags collection of the catalog.
* `GetBillingTags`: Returns the billing tags set on the catalog.
* `GetCreated`: Returns the catalog creation time.
* `GetLayers`: Returns details of the layers contained in the catalog.
* `GetVersion`: Returns the current catalog version number.
* `GetNotifications`: Returns the notification status of the catalog.

The `ApiError` class contains details regarding to the incurred error, including:

* `GetErrorCode`: Returns the `ErrorCode` value, defined by the `olp::client::ErrorCode enum`, see `ErrorCode.h` for more details.
* `GetHttpStatusCode`: Returns the HTTP response code.
* `GetMessage`:  Returns a text description of the encountered error.
* `ShouldRetry`: Returns `true` if this operation can be retried.

### Retrieve partitions metadata

The `GetPartition` method in the `VersionedLayerClient` object fetches partitions metadata for a particular layer. To retrieve partitions metadata, create a `PartitionsRequest`. The `PartitionsRequest` class allows you to set the properties of the partition request, including:

* `WithBillingTag`: Sets the billing tag used for this request.

```cpp
// Create a PartitionsRequest with appropriate LayerId
auto request = olp::dataservice::read::PartitionsRequest()
                    .WithBillingTag(boost::none);
```

Then pass it to the appropriate layer client, i.e. `VersionedLayerClient` via `GetPartitions` method:

```cpp
// Create appropriate layer client with HRN, layer name and settings.
olp::dataservice::read::VersionedLayerClient layer_client(
    olp::client::HRN(kCatalogHRN), first_layer_id, client_settings);

// Run the PartitionsRequest
auto future = layer_client.GetPartitions(request);
```

The execution result is a `CancellableFuture` that contains `PartitionsResponse` object. The `PartitionsResponse` class holds the details of the completed operation and is used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true in case of a successful response, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::PartitionsResult`) in case of a successful operation.
* `GetError`: Contains the error information as a result of an error in the `olp::client::ApiError` object.

```cpp
// Wait for PartitionsResponse
olp::dataservice::read::PartitionsResponse partitions_response =
    future.GetFuture().get();

// Check the response
if (partitions_response.IsSuccessful()) {
    const olp::dataservice::read::PartitionsResult& response_result =
        partitions_response.GetResult();
    // Handle success
} else {
    // Handle fail
}
```

The `PartitionsResult` class contains details of the relevant layer's partition metadata, including:

* `GetPartitions`: Returns a collection of partition metadata objects of the `olp::dataservice::read::model::Partition` type.

The `Partition` class contains partition metadata and exposes the following members:

* `GetChecksum`: Partition checksum.
* `GetCompressedDataSize`: Returns the size of the compressed partition data.
* `GetDataHandle`: Returns the handle that can be used by the `GetData` function to retrieve the partition data.
* `GetDataSize`: Returns the size of the partition data.
* `GetPartition`: Returns the partition Id.
* `GetVersion`: Returns the latest catalog version for the partition.

### Retrieve partition data

The `GetData` method in the `VersionedLayerClient` object fetches data for a partition or data handle asynchronously. To retrieve partition data, create the `DataRequest`. The `DataRequest` class allows you to specify the following parameters of the data request:

* `WithPartitionId`: Sets the partition for the data request.
* `WithDataHandle`: Sets the requested data handle which can be found via the `GetPartition` call in the `olp::dataservice::read::model::Partition::GetDataHandle` member.
* `WithBillingTag`: Sets the billing tag for the request.

```cpp
// Create a DataRequest with appropriate LayerId and PartitionId
auto request = olp::dataservice::read::DataRequest()
                    .WithPartitionId(gPartitionId)
                    .WithBillingTag(boost::none);
```

Then pass it to the `VersionedLayerClient` via `GetData` method:

```cpp
// Run the DataRequest
auto future = layer_client.GetData(request);
```

The execution result is a `CancellableFuture` that contains `DataResponse` object.

```cpp
// Wait for DataResponse
olp::dataservice::read::DataResponse data_response =
    future.GetFuture().get();

// Check the response
if (data_response.IsSuccessful()) {
    const olp::dataservice::read::DataResult& response_result =
        data_response.GetResult();
    // Handle success
} else {
    // Handle fail
}
```

The `DataResponse` class holds the details of the completed operation that are used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true in case of a successful response, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::DataResult`) in case of a successful operation.
* `GetError`: Contains the error information as a result of an error in the `olp::client::ApiError` object.

The `DataResult` class contains the raw partition data, which is an alias for a std::shared_ptr<std::vector<unsigned char>>.
