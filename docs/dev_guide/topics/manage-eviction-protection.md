# Manage eviction protection

You can add tile keys to a protected list if you do not want them to be evicted.
For example, you may want to protect tile keys of your home region so that they always stay in the cache and are not deleted when you travel to other places.
You can also remove tile keys from protection.

**To work with eviction protection:**

1. Create the `OlpClientSettings` object.

   For instructions, see [Create platform client settings](../../create-platform-client-settings.md).

2. Create the `VersionedLayerClient` object with the HERE Resource Name (HRN) of the catalog that contains the layer, the layer ID, catalog version, and the platform client settings from step 1.

   > #### Note
   > If a catalog version is not specified, the latest version is used.

   ```cpp
   olp::dataservice::read::VersionedLayerClient layer_client (
                       client::HRN catalog,
                       std::string layer_id,
                       boost::optional<int64_t> catalog_version,
                       client::OlpClientSettings settings);
   ```

3. Depending on how you want to manage eviction protection, do one of the following:

    - If you want to protect tile keys from eviction, make sure that the `Release` method is not in progress, and then call the `Protect` method with the list of tile keys that you want to protect.

        > #### Note
        > You can protect tile keys from eviction if their data handles are present in the cache at the time of the call.

        ```cpp
        layer_client.Protect(<vector of tile keys>);
        ```

        Data and keys of the specified tiles are added to the protected list and stored in the cache. The keys are removed from the LRU cache. Now, they cannot be evicted and do not expire. The quadtree stays protected if at least one tile key is protected.

    - If you want to remove tile keys from protection, make sure that the `Protect` method is not in progress, and then call the `Release` method with the list of tile keys that you want to remove from protection.

        ```cpp
        layer_client.Protect(<vector of tile keys>);
        ```

        Data and keys of the specified tiles are removed from the protected list. The keys are added to the LRU cache. Now, they can be evicted. Expiration value is restored, and keys can expire. The quadtree can be removed from the protected list if all tile keys are no longer protected.
