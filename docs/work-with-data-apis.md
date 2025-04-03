# Work with unsupported Data REST APIs

HERE Data SDK for C++ supports only a limited number of Data REST APIs. You can find a list of them on our page with the [overall architecture](OverallArchitecture.md).

If you need to work with any Data REST APIs that are currently not supported, you can use the `olp::client::OlpClient` class from our core library for the request. It has an easy-to-use interface and a retry mechanism that uses a recommended backoff strategy for subsequent requests.

The example below demonstrates how to use the `olp::client::OlpClient` class from the Data SDK to call the API for downloading the difference between two versions of a versioned catalog.

**Example. Download the difference between two versions of a catalog.**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](create-platform-client-settings.md).

2. Configure an `OlpClient` instance in one of the following ways:
    - Use the `olp::client::ApiLookupClient` class to perform a request and get a preconfigured `olp::client::OlpClient` instance.
    - Use a base URL:
        1. Get the base URL from the lookup API service.

            For instructions, see the [API Lookup API Developer's Guide](https://www.here.com/docs/bundle/api-lookup-api-developer-guide/page/README.html)
        
        2. Create an `OlpClient` instance with the client settings from step 1 and the base URL.

            ```cpp
            olp::client OlpClient client(client_settings, base_url);
            ```

3. Prepare the REST API request parameters and the metadata URL.

    ```cpp
    std::multimap<std::string, std::string> header_params;
    header_params.emplace("Accept", "application/json");

    // Replace "my_layer" with the actual layer name.
    std::string metadata_url = "/layers/my_layer/changes";

    std::multimap<std::string, std::string> query_params;

    // Required parameters.
    query_params.emplace("startVersion", "0");
    query_params.emplace("endVersion", "1");

    // Optional parameters.
    query_params.emplace("additionalFields", "dataSize,checksum");
    query_params.emplace("billingTag", "Billing1234");

    // Can be used to cancel the ongoing request. Needs to be triggered
    // from another thread as `olp::client::OlpClient::CallApi()` is a blocking call.
    CancellationContext context;
    olp::client::HttpResponse response = client.CallApi(
        metadata_url, "GET", query_params, header_params, {}, nullptr, "", context);
    ```

4. Check the response and, if needed, parse JSON.

    ```cpp
    if (response.GetStatus() != olp::http::HttpStatusCode::OK) {
        // If the response is not OK, handle error cases.
    }
    ```

5. If the check passed and the status of the received `olp::client::HttpResponse` is `olp::http::HttpStatusCode::OK` (that is 200 OK), extract the encoded JSON from the response and parse it using JSON parsing library.

    ```cpp
    std::string response_json;
    response.GetResponse(response_json);
    ```

    You get the information on the difference between the specified versions of the versioned catalog.
