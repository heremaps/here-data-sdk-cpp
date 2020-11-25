# Create platform client settings

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

   > #### Note
   > The `Network` client is designed and intended to be shared.

3. [Authenticate](authenticate.md) to the HERE platform.

4. (Optional) For data that is stored in the cache, to add expiration limit, set the `default_cache_expiration` to the needed expiration time.

   By default, expiration is disabled.

   > #### Note
   > You can only disable expiration for the mutable and in-memory cache. This setting does not affect the protected cache as no entries are added to the protected cache in the read-only mode.

   ```cpp
   std::chrono::seconds default_cache_expiration = std::chrono::seconds(200);
   ```

5. Set up the `OlpClientSettings` object.

   ```cpp
   olp::client::OlpClientSettings client_settings;
   client_settings.authentication_settings = auth_settings;
   client_settings.task_scheduler = task_scheduler;
   client_settings.network_request_handler = http_client;
   client_settings.cache =
       olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
   client_settings.default_cache_expiration = std::chrono::seconds(200);
   ```

The `OlpClientSettings` class pulls together all the settings for customization of the client library behavior:

- `retry_settings` – sets `olp::client::RetrySettings` to use.
- `proxy_settings` – sets `olp::authentication::NetworkProxySettings` to use.
- `authentication_settings` – sets `olp::client::AuthenticationSettings` to use.
- `network_request_handler` – sets the handler for asynchronous execution of network requests.
