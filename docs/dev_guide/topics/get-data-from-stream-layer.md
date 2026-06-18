# Get data from a stream layer

You can read messages from a [stream layer](https://docs.here.com/data-api/docs/layers#stream-layers) if you subscribe to it. 

**To get data from the stream layer:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

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
