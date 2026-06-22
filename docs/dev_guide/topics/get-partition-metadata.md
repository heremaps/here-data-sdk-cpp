# Get partition metadata

Partition metadata consists of the following information about the partition:

- Data handle
- ID
- Version
- Data size
- Compressed data size
- Checksum

**To get partition metadata:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

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

4. Call `GetPartitions` method with the `PartitionsRequest` parameter.

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