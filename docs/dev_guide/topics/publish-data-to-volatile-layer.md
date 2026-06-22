# Publish data to a volatile layer

If you need to store only the current version of data, use a [volatile layer](https://docs.here.com/data-api/docs/layers#volatile-layers). As new data is published to the volatile layer, old data is overwritten.

**To publish data to the volatile layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

2. Create the `VolatileLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer and the platform client settings from step 1.

   ```cpp
   auto client = olp::dataservice::write::VolatileLayerClient(
   olp::client::HRN{kCatalogHRN}, client_settings);
   ```

3. Create the `PublishPartitionDataRequest` object with the data that you want to publish, layer ID, and partition ID.

   ```cpp
   auto request = PublishPartitionDataRequest().WithData(buffer).WithLayerId(kLayer).WithPartitionId(kPartition);
   ```

4. Call the `PublishPartitionData` method with the `PublishPartitionDataRequest` parameter.

   ```cpp
   auto futureResponse = client.PublishPartitionData(request);
   ```

5. Wait for the `PublishPartitionDataResponse` future.

   ```cpp
   auto response = futureResponse.GetFuture().get();
   ```

The `PublishPartitionDataResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()` &ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::write::PublishDataResult`
- `GetError()` &ndash; contains error information as a result of an error in the `olp::client::ApiError` object.

```cpp
if (response.IsSuccessful()) {
    auto response_result = response.GetResult();
    // Handle success
} else {
    auto api_error = response.GetError();
    // Handle fail
}
```
