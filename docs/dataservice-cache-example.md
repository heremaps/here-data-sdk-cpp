# Cache example

On this page, find instructions on how to build and run the cache example project, get partition data, and work with a mutable and protected cache using HERE Data SDK for C++.

**Before you run the example project, authorize to the HERE platform:**

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.

   For instructions on how to get the access key ID and access key secret, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

2. In <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/main.cpp" target="_blank">`examples/main.cpp`</a>, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.

   > #### Note
   > You can also specify these values using the command line options.

   ```cpp
   AccessKey access_key{}; // Your access key ID and access key secret.
   std::string catalog;   // The HRN of the catalog to which you want to publish data.
   ```

## <a name="build"></a>Build and run on Linux

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
   ./examples/dataservice-example --example cache --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog"
   ```

4. (Optional) To run the example with other parameters, run the help command, and then select the needed parameter.

   ```bash
   ./examples/dataservice-example --help
   ```

After building and running the example project, you see the following information:

```bash
[INFO] protected-cache-example - Mutable cache path is "none"
[INFO] protected-cache-example - Protected cache path is "/tmp/catalog_client_example/cache"
[INFO] ThreadPoolTaskScheduler - Starting thread 'OLPSDKPOOL_0'
[DEBUG] CatalogCacheRepository - GetVersion -> 'hrn:here:data::olp-here-test:edge-example-catalog::latestVersion'
[DEBUG] CatalogRepository - Latest cached version, hrn='hrn:here:data::olp-here-test:edge-example-catalog', version=0
[DEBUG] PartitionsCacheRepository - Get 'hrn:here:data::olp-here-test:edge-example-catalog::versioned-world-layer::1::0::partition'
[DEBUG] PartitionsRepository - GetPartitionById found in cache, hrn='hrn:here:data::olp-here-test:edge-example-catalog', key='versioned-world-layer[1]@0^2'
[DEBUG] DataCacheRepository - Get 'hrn:here:data::olp-here-test:edge-example-catalog::versioned-world-layer::8daa637d-7c81-4322-a600-063f4ae0ef98::Data'
[DEBUG] DataRepository - GetBlobData found in cache, hrn='hrn:here:data::olp-here-test:edge-example-catalog', key='8daa637d-7c81-4322-a600-063f4ae0ef98'
[INFO] protected-cache-example - Request partition data - Success, data size - 3375
```

## How it works

### <a name="get-partition-data-mutable"></a>Get data from a versioned layer with a cache

You can get data from a [versioned layer](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/layers.html#versioned-layers) with a mutable or protected cache.

**To get data from the versioned layer with mutable cache:**

1. Create the `OlpClientSettings` object with the path to the needed cache settings properties (disk space, runtime memory limit, maximum file size, in-memory data cache size, options, and paths).

   For instructions, see [Create platform client settings](create-platform-client-settings.md).

   > #### Note
   > Perform the first call with the valid mutable cache path, and the second call – with the protected cache path.

2. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, catalog version, and the platform client settings from step 1.

   If you do not specify a catalog version, the latest version is used. Depending on the fetch option that you specified in your first API call to the client, the `GetLatestVersion` method automatically gets the latest version in one of the following ways:

   - For the `OnlineIfNotFound` fetch option, queries the network, and if an error occurs, checks the cache. If the online version is higher than the cache version, the cache version is updated.
   - For the `OnlineOnly` fetch option, only queries the network.
   - For the `CacheOnly` fetch option, only checks the cache.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client (
                       client::HRN catalog,
                       std::string layer_id,
                       boost::optional<int64_t> catalog_version,
                       client::OlpClientSettings settings);
   ```

3. Create the `DataRequest` object with the partition ID and one of the following fetch options:

   - (Default) To query network if the requested resource is not found in the cache, use `OnlineIfNotFound`.
   - To skip cache lookups and query the network right away, use `OnlineOnly`.
   - To return immediately if a cache lookup fails, use `CacheOnly`.

   ```cpp
   auto request = olp::dataservice::read::DataRequest()
                    .WithPartitionId(first_partition_id)
                    .WithBillingTag(boost::none)
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
    data_response.GetError().GetErrorCode();
    // Handle fail
}
```

The received data is stored in the cache. You can find the path to the cache in the log message.

   ```bash
   [INFO] protected-cache-example - Mutable cache path is "/tmp/catalog_client_example/cache"
   ```
