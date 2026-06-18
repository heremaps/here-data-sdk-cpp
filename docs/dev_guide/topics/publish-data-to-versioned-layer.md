# Publish data to a versioned layer

If you need to store and access the history of previous data updates, publish data to a [versioned layer](https://docs.here.com/data-api/docs/layers#versioned-layers). To achieve consistency between layers, publish any update that affects multiple versioned layers together in one publication. You can access a new catalog version when all layers are updated and the publication is finalized. Once you publish a version, you cannot change the data in that version and can remove it only by removing the whole catalog version.

**To publish data to the versioned layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

2. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer and the platform client settings from step 1.

   ```cpp
   auto versioned_client = olp::dataservice::write::VersionedLayerClient(
   olp::client::HRN{kCatalogHRN}, client_settings);
   ```

3. Create the `StartBatchRequest` object with the layer ID.

   ```cpp
   auto start_batch_request = StartBatchRequest().WithLayers(kLayer);
   ```

4. Run the `StartBatchRequest` method and wait for the response.

   ```cpp
   auto start_batch_response =
     versioned_client.StartBatch(start_batch_request).GetFuture().get();
   ```

5. Get details of the started batch with the `Publication` parameter.

   ```cpp
   auto get_batch_response =
     versioned_client.GetBatch(start_batch_response.GetResult())
         .GetFuture()
         .get();
   ```

6. Create the `PublishPartitionDataRequest` object with the data that you want to publish, layer ID, and partition ID.

   ```cpp
   auto publish_request =
       PublishPartitionDataRequest()
           .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
           .WithLayerId(kLayer)
           .WithPartitionId(kPartition);
   ```

7. Run the `PublishToBatch` method with the `Publication` and the `PublishRequest` parameters, and wait for the response.

   ```cpp
   auto publish_to_batch_response =
       versioned_client
           .PublishToBatch(start_batch_response.GetResult(), publish_request)
           .GetFuture()
           .get();
   ```

8. Run the `CompleteBatch` method with the publication details, and wait for the response.

   ```cpp
   auto complete_batch_response =
       versioned_client->CompleteBatch(get_batch_response.GetResult())
           .GetFuture()
           .get();
   ```

The `PublishDataResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()`&ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::write::PublishDataResult`
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
