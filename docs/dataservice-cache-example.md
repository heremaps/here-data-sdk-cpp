# Cache Example

On this page, find instructions on how to build and run the example project, get partition data, and work with a mutable and protected cache using the HERE Data SDK for C++.

Before you run the example project, authorize to the HERE platform:

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.
   For instructions on how to get the access key ID and access key secret, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

2. In `examples/main.cpp`, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.
   **Note:**  You can also specify these values using the command line options.
   ```cpp
   AccessKey access_key{}; // Your access key ID and access key secret.
   std::string catalog;   // The HRN of the catalog to which you want to publish data.
   ```

## <a name="build"></a>Build and Run on Linux

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
   ./examples/dataservice-example --example cache --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog"
   ```

4. (Optional) To run the example with other parameters, run the help command, and then select the needed parameter.
   ```bash
   ./examples/dataservice-example --help
   ```

After building and running the example project, you see the following information:

```bash
[INFO] protected-cache-example - Mutable cache path is "none"
[INFO] protected-cache-example - Protected cache path is "/tmp/cata.log_client_example/cache"
[INFO] ThreadPoolTaskScheduler - Starting thread 'OLPSDKPOOL_0'
[INFO] CatalogRepository - cache catalog '@0^0' found!
[INFO] PartitionsCacheRepository - Get 'hrn:here:data::olp-here-test:edge-example-catalog::versioned-world-layer::1::0::partition'
[INFO] PartitionsRepository - cache data 'versioned-world-layer[1]@0^0' found!
[INFO] DataRepository - cache data 'versioned-world-layer[1]@0^0' found!
[INFO] protected-cache-example - Request partition data - Success, data size - 3375
```

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

   > **Note:** The `Network` client is designed and intended to be shared.

3. [Authenticate](#authenticate-to-here-olp-using-client-credentials) to the HERE platform.

4. Set up the `OlpClientSettings` object with the path to the needed cache settings properties (the limit of runtime memory, disk space, maximum file size, memory data cache size, options, and paths).
  For instructions on how to get the path to the cache settings, see the [Build and Run on Linux](#build) section.
  > **Note:** Perform the first call with the valid mutable cache path, and the second call – with the protected cache path.

   ```cpp
   client_settings.cache =
       olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
   ```

### <a name="get-partition-data-mutable"></a>Get Partition Data from a Versioned Layer with a Mutable Cache

You can request any data version from a [versioned layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#versioned-layers). When you request a particular version of data from the versioned layer, the partition you receive in the response may have a lower version number than you requested. The version of a layer or partition represents the catalog version in which the layer or partition was last updated.

**To get data from the versioned layer with mutable cache:**

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object with the path to the mutable cache settings.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, and the platform client settings from step 2.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client(
   olp::client::HRN(kCatalogHRN), layer_id, client_settings);
   ```

4. Create the `DataRequest` object with the layer version and partition ID.

   > **Note:** If a catalog version is not specified, the latest version is used.

   ```cpp
   auto request = olp::dataservice::read::DataRequest()
                    .WithPartitionId(first_partition_id)
                    .WithBillingTag(boost::none);
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

- `IsSuccessful()` - if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()` - if the operation is successful, returns the following resultant data: `olp::dataservice::read::DataResult`
- `GetError()` - contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (data_response.IsSuccessful()) {
    const auto& response_result = data_response.GetResult();
    // Handle success
} else {
    data_response.GetError().GetErrorCode();
    // Handle fail
}
```
The received data is stored in the cache. You can find the path to the cache in the log message.
   ```bash
   [INFO] protected-cache-example - Mutable cache path is "/tmp/cata.log_client_example/cache"
   ```
### Get Partition Data from a Versioned Layer with a Protected Cache

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object with the path to the protected cache settings.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Do steps 3–6 of the instruction on how to [get partition data from a versioned layer with a mutable cache](#get-partition-data-mutable).
The received data is stored in the protected cache. You can find the path to the cache in the log message.
    ```bash
    [INFO] protected-cache-example - Mutable cache path is "none"
    [INFO] protected-cache-example - Protected cache path is "/tmp/cata.log_client_example/cache"
    [INFO] ThreadPoolTaskScheduler - Starting thread 'OLPSDKPOOL_0'
    [INFO] CatalogRepository - cache catalog '@0^0' found!
    [INFO] PartitionsCacheRepository - Get 'hrn:here:data::olp-here-test:edge-example-catalog::versioned-world-layer::1::0::partition'
    [INFO] PartitionsRepository - cache data 'versioned-world-layer[1]@0^0' found!
    [INFO] DataRepository - cache data 'versioned-world-layer[1]@0^0' found!
    [INFO] protected-cache-example - Request partition data - Success, data size - 3375
    ```
