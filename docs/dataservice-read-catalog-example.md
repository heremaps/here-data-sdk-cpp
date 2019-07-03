# Read example

This example shows how to retrieve catalog metadata, partition metadata, and partition data using the HERE OLP Edge SDK C++.

Before you run the example, you have to replace the placeholders in `examples/dataservice-read/example.cpp` with your app key and app secret:

```cpp
const std::string gKeyId(""); // your here.access.key.id
const std::string gKeySecret(""); // your here.access.key.secret
```

## Building and running

Configure the project with `EDGE_SDK_BUILD_EXAMPLES` set to `ON` to enabled examples `CMake` targets:

```bash
mkdir build && cd build
cmake -DEDGE_SDK_BUILD_EXAMPLES=ON ..
```

To build the example, run the following command in the `build` folder:

```bash
cmake --build . --target dataservice-read-example
```

To execute the example, run:

```bash
./examples/dataservice-read/dataservice-read-example
```

If everything is fine, you would see `hrn:here:data:::here-optimized-map-for-visualization-2` catalog description, number of partitions and the size of `100000093` partition.

## How it works

### CatalogClient

The main entry point to the OLP is `CatalogClient`. This class provides a high-level interface for the retrieval of `OLP` catalog data and configuration, and defines the following operations:

* `GetCatalog`: Fetches the catalog configuration asynchronously.
* `GetPartitions`: Fetches the full list of partitions for the given generic layer asynchronously.
* `GetData`: Fetches data of a partition or data handle asynchronously.

To create a `CatalogClient` provide the corresponding `HRN` and a preconfigured `OlpClientSettings`:

```cpp
// Setup AuthenticationSettings with a default token provider that will
// retrieve an OAuth 2.0 token from OLP.
olp::client::AuthenticationSettings authSettings;
authSettings.provider =
    olp::authentication::TokenProviderDefault(gKeyId, gKeySecret);

// Setup OlpClientSettings and provide it to the CatalogClient.
auto settings = std::make_shared<olp::client::OlpClientSettings>();
settings->authentication_settings = authSettings;

// Create a CatalogClient with appropriate HRN and settings.
auto serviceClient = std::make_unique<olp::dataservice::read::CatalogClient>(
    olp::client::HRN(gCatalogHRN), settings);
```

The `OlpClientSettings` class pulls together all the different settings which can be used to customize the behavior of the client library.

* `retry_settings`: Sets the `olp::client::RetrySettings` to be used.
* `proxy_settings`: Sets the `olp::authentication::NetworkProxySettings` to be used.
* `authentication_settings`: Sets the `olp::client::AuthenticationSettings` to be used.
* `network_async_handler`: Sets the handler for asynchronous execution of network requests.

For basic usage, we need to specify only `authentication_settings`.

### Retrieve catalog metadata

To retrieve catalog metadata you need to create a `CatalogRequest`. The `CatalogRequest` class allows properties of the catalog request to be set, including:

* `WithBillingTag`: Sets the billing tag used for this request.

```cpp
// Create CatalogRequest
auto request =
    olp::dataservice::read::CatalogRequest().WithBillingTag(boost::none);
```

Then pass it to the `CatalogClient` via `GetCatalog` method:

```cpp
// Run the CatalogRequest
auto future = serviceClient->GetCatalog(request);
```

The execution result is a `CancellableFuture` that contains `CatalogResponse` object. The `CatalogResponse` class holds details of the completed operation and should be used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true if this response is considered successful, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::CatalogResult`) in the case that the operation is successful.
* `GetError`: Contains information about the error that occurred in the case of error in a `olp::client::ApiError` object.

```cpp
// Wait for the CatalogResponse response
olp::dataservice::read::CatalogResponse catalogResponse =
    future.GetFuture().get();

// Check the response
if (catalogResponse.IsSuccessful()) {
    const olp::dataservice::read::CatalogResult& responseResult =
        catalogResponse.GetResult();
    LOG_INFO_F("read-example", "Catalog description: %s",
                responseResult.GetDescription().c_str());
} else {
    LOG_ERROR("read-example", "Request catalog metadata - Failure");
}
```

The `CatalogResult` class contains details of the relevant catalog, including:

* `GetId`: Returns the catalog id.
* `GetHrn`: Returns the catalog `HRN` (Here Resource Name).
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

The `ApiError` class contains details with regards to the error incurred, including:

* `GetErrorCode`: Returns the ErrorCode value, defined by the olp::client::ErrorCode enum, see ErrorCode.h for more details.
* `GetHttpStatusCode`: Returns the HTTP response code.
* `GetMessage`:  Returns a text description of the error encountered.
* `ShouldRetry`: Returns true if this operation can be retried.

### Retrieve partitions metadata

To retrieve partition metadata you need to create a `PartitionsRequest`. The `PartitionsRequest` class allows properties of the catalog request to be set, including:

* `WithBillingTag`: Sets the billing tag used for this request.
* `WithLayerId`: Sets the layer id that partition metadata should be retrieved from.

```cpp
// Create a PartitionsRequest with appropriate LayerId
auto request = olp::dataservice::read::PartitionsRequest()
                    .WithLayerId(gLayerId)
                    .WithBillingTag(boost::none);
```

Then pass it to the `CatalogClient` via `GetPartitions` method:

```cpp
// Run the PartitionsRequest
auto future = serviceClient->GetPartitions(request);
```

The execution result is a `CancellableFuture` that contains `PartitionsResponse` object. The `PartitionsResponse` class holds details of the completed operation and should be used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true if this response is considered successful, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::PartitionsResult`) in the case that the operation is successful.
* `GetError`: Contains information about the error that occurred in the case of an error in a `olp::client::ApiError` object.

```cpp
// Wait for PartitionsResponse
olp::dataservice::read::PartitionsResponse partitionsResponse =
    future.GetFuture().get();

// Check the response
if (partitionsResponse.IsSuccessful()) {
    const olp::dataservice::read::PartitionsResult& responseResult =
        partitionsResponse.GetResult();
    const std::vector<olp::dataservice::read::model::Partition>& partitions =
        responseResult.GetPartitions();
    LOG_INFO_F("read-example", "Layer contains %d partitions.",
                partitions.size());
} else {
    LOG_ERROR("read-example", "Request partition metadata - Failure");
}
```

The `PartitionsResult` class contains details of the relevant layer's partition metadata, including:

* `GetPartitions`: Returns a collection of partition metadata objects, of type `olp::dataservice::read::model::Partition`.

The `Partition` class contains partition metadata, and exposes the following members:

* `GetChecksum`: Partition checksum.
* `GetCompressedDataSize`: Returns the size of the compressed partition data.
* `GetDataHandle`: Returns a handle that can be used by the `CatalogClient::GetData` function to retrieve the partition data.
* `GetDataSize`: Returns the size of the partition data.
* `GetPartition`: Returns the partition Id.
* `GetVersion`: Returns the latest catalog version for the partition.

### Retrieve partition data

To retrieve partition data you need to create a `DataRequest`. The `DataRequest` class allows parameters of the GetData function to be specified, including:

* `WithLayerId`: Sets the layer id to be used for the request.
* `WithPartitionId`: Sets the partition whose data is being requested.
* `WithDataHandle`: Sets the requested data handle, which can be found via the GetPartition call, in the `olp::dataservice::read::model::Partition::GetDataHandle` member.
* `WithBillingTag`: Sets the billing tag for the request.

```cpp
// Create a DataRequest with appropriate LayerId and PartitionId
auto request = olp::dataservice::read::DataRequest()
                    .WithLayerId(gLayerId)
                    .WithPartitionId(gPartitionId)
                    .WithBillingTag(boost::none);
```

Then pass it to the `CatalogClient` via `GetData` method:

```cpp
// Run the DataRequest
auto future = serviceClient->GetData(request);
```

The execution result is a `CancellableFuture` that contains `DataResponse` object.

```cpp
// Wait for DataResponse
olp::dataservice::read::DataResponse dataResponse =
    future.GetFuture().get();

// Check the response
if (dataResponse.IsSuccessful()) {
    const olp::dataservice::read::DataResult& responseResult =
        dataResponse.GetResult();
    LOG_INFO_F("read-example",
                "Request partition data - Success, data size - %d",
                responseResult->size());
} else {
    LOG_ERROR("read-example", "Request partition data - Failure");
}
```

The `DataResponse` class holds details of the completed operation and should be used to determine operation success and access resultant data.

* `IsSuccessful`: Returns true if this response is considered successful, false otherwise.
* `GetResult`: Returns the resultant data (`olp::dataservice::read::DataResult`) in the case that the operation is successful.
* `GetError`: Contains information about the error that occurred in the case of error in a `olp::client::ApiError` object.

The `DataResult` class contains the raw partition data, which is an alias for a std::shared_ptr<std::vector<unsigned char>>.
