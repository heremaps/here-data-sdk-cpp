# Read from a stream layer (example)

On this page, you can find instructions on how to build and run the cache example project on different platforms and get catalog and partition metadata, as well as partition data using HERE Data SDK for C++.

**Before you run the example project, authorize to the HERE platform:**

1. On the [Apps & keys](https://platform.here.com/admin/apps) page, copy your application access key ID and access key secret.

   For instructions on how to get the access key ID and access key secret, see the [Register your application](https://developer.here.com/documentation/identity-access-management/dev_guide/topics/plat-token.html#step-1-register-your-application) section in the Identity & Access Management Developer Guide.

2. In <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/examples/main.cpp" target="_blank">examples/main.cpp</a>, replace the placeholders with your access key ID, access key secret, and Here Resource Name (HRN) of the catalog.

   > #### Note
   > You can also specify these values using the command line options.

   ```cpp
   AccessKey access_key{"here.access.key.id", "here.access.key.secret"};
   ```

## Build and run on Linux

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
   ./examples/dataservice-example --example read_stream --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog"
   ```

4. (Optional) To run the example with other parameters, run the help command, and then select the needed parameter.

   ```bash
   ./examples/dataservice-example --help
   ```

After building and running the example project, you see the following information:

- `./dataservice-example` – a catalog and layer description.
- `read-stream-layer-example - Poll data - Success, messages size - 6` – a success message that displays the size of data retrieved from the layer.

```bash
./dataservice-example --example read --key_id "here.access.key.id" --key_secret "here.access.key.secret" --catalog "catalog" --layer_id "layer_id" --type-of-subscription "subscription_type"
[INFO] read-stream-layer-example - Message data: size - 16
[INFO] read-stream-layer-example - No new messages is received
[INFO] read-stream-layer-example - Poll data - Success, messages size - 6
```

## How it works

### Get data from a stream layer

You can read messages from a [stream layer](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html#stream-layers) if you subscribe to it.

**To get data from the stream layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/docs/create-platform-client-settings.md" target="_blank">Create platform client settings</a>.

2. Create the `StreamLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, layer ID, and platform client settings from step 1.

   ```cpp
   olp::dataservice::read::StreamLayerClient client(
        client::HRN catalog, std::string layer_id,
        client::OlpClientSettings settings);
   ```

3. Create the `SubscribeRequest` object with the `serial` or `parallel` subscription type.

   - If your application should read smaller volumes of data using a single subscription, use the `serial` subscription type.

   - If your application should read large volumes of data in a parallel manner, use the `parallel` subscription type and subscription ID.

   ```cpp
   auto request = olp::dataservice::read::SubscribeRequest()
                     .WithSubscriptionMode(olp::dataservice::read::SubscribeRequest::SubscriptionMode::kSerial));
   ```

4. Call the `Subscribe` method with the `SubscribeRequest` parameter.

   ```cpp
   client::CancellableFuture<SubscribeResponse> Subscribe(
      SubscribeRequest request);
   ```

   You receive a subscription ID from the requested subscription to the selected layer.

5. Call the `Poll` method.

   ```cpp
   client::CancellableFuture<PollResponse> Poll();
   ```

   You get messages with the layer data and partition metadata. The `Poll` method also commits the offsets, so you can continue polling new messages.

   If the data size is less than 1 MB, the data field is populated. If the data size is greater than 1 MB, you get a data handle that points to the object stored in the blob store.

6. If the data size is greater than 1 MB, call the `GetData` method with the `Messages` instance.

   ```cpp
    client::CancellableFuture<DataResponse> GetData(
        const model::Message& message);
   ```

You get data from the requested partition.

### Delete your subscription to a stream layer

If you want to stop message consumption, delete your subscription to a stream layer:

```cpp
auto unsubscribe_response = stream_client.Unsubscribe().GetFuture().get();
  if (unsubscribe_response.IsSuccessful()) {
      // Successfully unsubscribed.
  }
```
