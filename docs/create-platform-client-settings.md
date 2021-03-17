# Create platform client settings

The `OlpClientSettings` class pulls together all the settings for customization of the client library behavior.
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

3. To configure handling of failed requests, create the `RetrySettings` object with the following parameters:

   - `retry_condition` – the HTTP status codes that you want to retry.
   - `backdown_strategy` – the delay between retries.
   - `max_attempts` – the number of attempts.
   - `timeout` – the connection timeout limit (in seconds).
   - `initial_backdown_period` – the period between the error and the first retry attempt (in milliseconds).

   > #### Note
   > We recommend that your application includes retry logic for handling HTTP 429 and 5xx errors. Use exponential backoff in the retry logic.

   ```cpp
   olp::client::RetrySettings retry_settings;
   retry_settings.retry_condition = [](const olp::client::HttpResponse& response) {
     return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
   };
   retry_settings.backdown_strategy = ExponentialBackdownStrategy();
   retry_settings.max_attempts = 3;
   retry_settings.timeout = 60;
   retry_settings.initial_backdown_period = 200;
   ```

4. [Authenticate](authenticate.md) to the HERE platform.

5. (Optional) To configure a cache:

    1. Create the `CacheSettings` instance with the cache that you want to enable and, if needed, the maximum cache size in bytes.

         > #### Note
         > By default, the downloaded data is cached in memory, and the maximum size of it is 1 MB.

         ```cpp
         olp::cache::CacheSettings cache_settings;
         //On iOS, the path is relative to the Application Data folder.
         cache_settings.disk_path_mutable = "path to mutable cache";
         cache_settings.disk_path_protected = "path to protected cache";
         cache_settings.max_disk_storage = 1024ull * 1024ull * 32ull;
         ```

    2. Add the `CacheSettings` instance to the `DefaultCache` constructor.

       ```cpp
       olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
       ```

       To learn how to get or change cache size, see [Work with a cache](https://developer.here.com/documentation/sdk-cpp/dev_guide/docs/work-with-cache.html).

6. Set up the `OlpClientSettings` object, and if you want to add the expiration limit for the data that is stored in the cache, set the `default_cache_expiration` to the needed expiration time.

   By default, expiration is disabled.

   > #### Note
   > You can only disable expiration for the mutable and in-memory cache. This setting does not affect the protected cache as no entries are added to the protected cache in the read-only mode.

   ```cpp
   olp::client::OlpClientSettings client_settings;
   client_settings.authentication_settings = auth_settings;
   client_settings.task_scheduler = task_scheduler;
   client_settings.network_request_handler = http_client;
   client_settings.retry_settings = retry_settings;
   client_settings.cache =
       olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
   client_settings.default_cache_expiration = std::chrono::seconds(200);
   ```
