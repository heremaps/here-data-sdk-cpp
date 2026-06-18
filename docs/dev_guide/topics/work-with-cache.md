# Work with a cache

You can configure a cache instance and add it to the `OlpClientSettings` instance. Otherwise, a default cache of 1 MB is automatically created and cached in memory when you create one of the following clients: `VersionedLayerClient`, `VolatileLayerClient`, `StreamLayerClient`, or `IndexLayerClient`.

For information on how to configure cache settings, see [Create platform client settings](../../create-platform-client-settings.md).

If your application is running with the cache, you can get or change cache size:

- To get the cache size, call the `Size` method with the needed cache type.

  ```cpp
  auto cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
  auto cache_mutable_size = cache->Size(olp::cache::DefaultCache::kMutable);
  ```

- To change the cache size, call the `Size` method with the new size in bytes.

  > #### Note
  > You can only change the size of the mutable cache.

  ```cpp
  auto cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
  auto bytes_evicted = cache->Size(512);
  ```

  If the new cache size is smaller than the current size, items are evicted until the cache shrinks to the specified capacity, and you get the size of the evicted data. If the new size is greater, you get `0u`.
