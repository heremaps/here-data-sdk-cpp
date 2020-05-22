# Read from a Stream Layer (Example)

On this page, you can find instructions on how to build and run the example project on different platforms and get catalog and partition metadata, as well as partition data using the HERE Data SDK for C++.

Before you run the example project, authorize to the HERE platform:

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.
   For instructions on how to get the access key ID and access key secret, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

2. In `examples/main.cpp`, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.
   **Note:**  You can also specify these values using the command line options.

   ```cpp
   AccessKey access_key{"here.access.key.id", "here.access.key.secret"};
   ```

## Build and Run on Linux

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
   ./examples/dataservice-example --example read_stream --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog"
   ```

4. (Optional) To run the example with other parameters, run the help command, and then select the needed parameter.

   ```bash
   ./examples/dataservice-example --help
   ```

After building and running the example project, you see the following information:

- `./dataservice-example` &ndash; a catalog and layer description
- `read-stream-layer-example - Poll data - Success, messages size - 6` - a success message that displays the size of data retrieved from the layer.

```bash
./dataservice-example --example read --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog" --layer_id "layer_id" --type-of-subscription "subscription_type"
[INFO] read-stream-layer-example - Message data: size - 16
[INFO] read-stream-layer-example - No new messages is received
[INFO] read-stream-layer-example - Poll data - Success, messages size - 6
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

   > Note: The `Network` client is designed and intended to be shared.

3. [Authenticate](#authenticate-to-here-olp-using-client-credentials) to the HERE platform.

4. Set up the `OlpClientSettings` object.

   ```cpp
   olp::client::OlpClientSettings client_settings;
   client_settings.authentication_settings = auth_settings;
   client_settings.task_scheduler = std::move(task_scheduler);
   client_settings.network_request_handler = std::move(http_client);
   client_settings.cache =
       olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
   ```

The `OlpClientSettings` class pulls together all the settings for customization of the client library behavior.

### Get Data from a Stream Layer

You can read messages from a [stream layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#stream-layers) if you subscribe to it.

**To get data from the stream layer:**

1. Get an access key ID and access key secret.

   For instructions, see [Authenticate to the HERE Platform Using Client Credentials](#authenticate-to-here-olp-using-client-credentials).

2. Create the `OlpClientSettings` object.

   For instructions, see [Create `OlpClientSettings`](#create-olpclientsettings).

3. Create the `StreamLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, layer ID, and platform client settings from step 2.

   ```cpp
   olp::dataservice::read::StreamLayerClient client(
        client::HRN catalog, std::string layer_id,
        client::OlpClientSettings settings);
   ```

4. Create the `SubscribeRequest` object with the `serial` or `parallel` subscription type.

   - If your application should read smaller volumes of data using a single subscription, use the `serial` subscription type.

   - If your application should read large volumes of data in a parallel manner, use the `parallel` subscription type and subscription ID.

   ```cpp
   auto request = olp::dataservice::read::SubscribeRequest()
                     .WithSubscriptionMode(olp::dataservice::read::SubscribeRequest::SubscriptionMode::kSerial));
   ```

5. Call the `Subscribe` method with the `SubscribeRequest` parameter.

   ```cpp
   client::CancellableFuture<SubscribeResponse> Subscribe(
      SubscribeRequest request);
   ```

   You receive a subscription ID from the requested subscription to the selected layer.

6. Call the `Poll` method.

   ```cpp
   client::CancellableFuture<PollResponse> Poll();
   ```

   You get messages with the layer data and partition metadata. The `Poll` method also commits the offsets, so you can continue polling new messages.

   If the data size is less than 1 MB, the `data` field is populated. If the data size is greater than 1 MB, you get a data handle that points to the object stored in the blob store.

7. If the data size is greater than 1 MB, call the `GetData` method with the `Messages` instance.

   ```cpp
    client::CancellableFuture<DataResponse> GetData(
        const model::Message& message);
   ```

You get data from the requested partition.

### Delete Your Subscription to a Stream Layer

If you want to stop message consumption, delete your subscription to a stream layer:

```cpp
auto unsubscribe_response = stream_client.Unsubscribe().GetFuture().get();
  if (unsubscribe_response.IsSuccessful()) {
      // Successfully unsubscribed.
  }
```
