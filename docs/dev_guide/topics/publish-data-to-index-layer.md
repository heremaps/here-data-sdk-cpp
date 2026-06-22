# Publish data to an index layer

You can index and store metadata and data in a way that is optimized for batch processing in an [index layer](https://docs.here.com/data-api/docs/layers#index-layers).

**To publish data to the index layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

2. Create the `IndexLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer and the platform client settings from step 1.

   ```cpp
   auto client = olp::dataservice::write::IndexLayerClient(
   olp::client::HRN{kCatalogHRN}, client_settings);
   ```

3. Create the `Index` object.

   ```cpp
   Index index;
   std::map<IndexName, std::shared_ptr<IndexValue>> index_fields;
   index_fields.insert(std::pair<IndexName, std::shared_ptr<IndexValue>>(
       "Place",
       std::make_shared<StringIndexValue>("New York", IndexType::String)));
   index.SetIndexFields(index_fields);
   ```

4. Create the `PublishIndexRequest` object with the data that you want to publish, layer ID, and index that you created in step 3.

   ```cpp
   auto index_request = PublishIndexRequest()
                                        .WithIndex(index)
                                        .WithData(data)
                                        .WithLayerId(layer_id);
   ```

5. Call the `PublishIndex` method with the `PublishIndexRequest` parameter.

   ```cpp
   auto future = client.PublishIndex(index_request);
   ```

6. Wait for the `PublishIndexResponse` future.

   ```cpp
   auto response = future.GetFuture().get();
   ```

The `PublishIndexResponse` object holds details of the completed operation and is used to determine operation success and access resultant data:

- `IsSuccessful()` &ndash; if the operation is successful, returns `true`. Otherwise, returns `false`.
- `GetResult()`&ndash; if the operation is successful, returns the following resultant data: `olp::dataservice::write::PublishIndexResult`
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
