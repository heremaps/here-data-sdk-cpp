# Get catalog metadata

Catalog metadata contains a list of configurations that describe the catalog and its layers. Configuration information about the catalog includes the following metadata:

- Name
- HERE Resource Name (HRN)
- Description
- Owner
- Version
- Layer information

**To get catalog metadata:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

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

5. Call the `GetCatalog` method with the `CatalogRequest` parameter.

   ```cpp
   auto future = catalog_client.GetCatalog(request);
   ```

6. Wait for `CatalogResponse` future.

   ```cpp
   olp::dataservice::read::CatalogResponse catalog_response = future.GetFuture().get();
   ```

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