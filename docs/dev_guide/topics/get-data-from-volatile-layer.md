# Get data from a volatile layer

You can only get the latest published data from a [volatile layer](https://docs.here.com/data-api/docs/layers#volatile-layers).

**To get data from the volatile layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

2. Create the `VolatileLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, and the platform client settings from step 1.

   ```cpp
   olp::dataservice::read::VolatileLayerClient layer_client(
       olp::client::HRN(kCatalogHRN), layer_id, client_settings);
   ```

3. Create the `DataRequest` object with the partition ID and one of the following fetch options:

   - (Default) To query network if the requested resource is not found in the cache, use `OnlineIfNotFound`.
   - To skip cache lookups and query the network right away, use `OnlineOnly`.
   - To return immediately if a cache lookup fails, use `CacheOnly`.
   - To return the requested cached resource if it is found and update the cache in the background, use `CacheWithUpdate`.

   ```cpp
   auto request = olp::dataservice::read::DataRequest()
                      .WithPartitionId(partition_id)
                      .WithBillingTag("MyBillingTag")
                      .WithFetchOption(FetchOptions::OnlineIfNotFound);
   ```

4. Call the `GetData` method with the `DataRequest` parameter.

   ```cpp
   auto future = layer_client.GetData(request);
   ```

5. Wait for the `DataResponse` future.

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
