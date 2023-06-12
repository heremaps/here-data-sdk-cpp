/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#pragma once

#include <string>

#include <olp/core/Config.h>

#include <olp/core/CoreApi.h>

#include <olp/core/porting/deprecated.h>
#include <boost/optional.hpp>

namespace olp {
namespace cache {

#ifdef OLP_SDK_ENABLE_DEFAULT_CACHE

/**
 * @brief Options for opening a disk cache.
 */
enum OpenOptions : unsigned char {
  Default = 0x00,  /*!< Opens the disk cache in the read-write mode. */
  ReadOnly = 0x01, /*!< Opens the disk cache in the read-only mode. */
  CheckCrc = 0x02  /*!< Verifies the checksum of all data that is read. */
};

/**
 * @brief Options for mutable cache eviction policy.
 */
enum class EvictionPolicy : unsigned char {
  kNone,             /*!< Disables eviction. */
  kLeastRecentlyUsed /*!< Evict least recently used key/value. */
};

/**
 * @brief Options for database compression.
 */
enum class CompressionType {
  kNoCompression,     /*!< No compression applied on the data before storing. */
  kDefaultCompression /*!< Default compression will be applied to data before
                        storing. */
};

/**
 * @brief Settings for memory and disk caching.
 */
struct CORE_API CacheSettings {
  /**
   * @brief The path to the mutable (read-write) disk cache where
   * the SDK caches and lookups the content.
   *
   * You should have write permissions.
   *
   * If this parameter is not set, the downloaded data is stored
   * only in the memory cache that is limited by `#max_memory_cache_size`.
   */
  boost::optional<std::string> disk_path_mutable = boost::none;

  /**
   * @brief Sets the upper limit (in bytes) of the disk space that is used for
   * persistent stores.
   *
   * The default value is 32 MB. To never evict data, set `max_disk_storage` to
   * `std::uint64_t(-1)`.
   */
  std::uint64_t max_disk_storage = 1024ull * 1024ull * 32ull;

  /**
   * @brief Sets the upper limit of the runtime memory (in bytes) before it is
   * written to the disk.
   *
   * Larger values increase performance, especially during bulk loads.
   * Up to two write buffers may be held in memory at the same time,
   * so you may wish to adjust this parameter to control memory usage.
   * Also, a larger write buffer will result in a longer recovery time
   * the next time the database is opened. The default value is 32 MB.
   */
  size_t max_chunk_size = 1024u * 1024u * 32u;

  /**
   * @brief Sets the flag to indicate that continuous flushes to the disk are
   * necessary to preserve maximum data between the ignition cycles.
   */
  bool enforce_immediate_flush = true;

  /**
   * @brief Sets the maximum permissible size of one file in the storage (in
   * bytes).
   *
   * The default value is 2 MB.
   */
  size_t max_file_size = 1024u * 1024u * 2u;

  /**
   * @brief Sets the upper limit of the memory data cache size (in bytes).
   *
   * If set to `0`, the memory cache is not used. The default value is 1 MB.
   */
  size_t max_memory_cache_size = 1024u * 1024u;

  /**
   * @brief Sets the disk cache open options.
   */
  OpenOptions openOptions = OpenOptions::Default;

  /*
   * @brief This flag sets the eviction policy for the key/value cache created
   * based on the disk_path_mutable path.
   *
   * This flag will not have any effect in case the disk_path_mutable is not
   * specified and in case max_disk_storage is set to -1. The default value
   * is EvictionPolicy::kLeastRecentlyUsed.
   */
  EvictionPolicy eviction_policy = EvictionPolicy::kLeastRecentlyUsed;

  /**
   * @brief This flag sets the compression policy to be applied on the database.
   *
   * In some cases though, when all the data to be inserted is already
   * compressed by any means, e.g. protobuf or other serialization protocols, it
   * might not be worth to enable any compression at all as it will eat up some
   * CPU to compress and decompress the metadata without major gain. This
   * parameter is dynamic and can be changed between runs. If changed only new
   * values which are added will be using the new compression policy all
   * existing entries will remain unchanged. The default value is
   * CompressionType::kDefaultCompression.
   */
  CompressionType compression = CompressionType::kDefaultCompression;

  /**
   * @brief The path to the protected (read-only) cache.
   *
   * This cache will be used as the primary source for data lookup. The
   * `DefaultCache` will try to open this cache in the r/w mode to make sure the
   * database can perform on-open optimizations like write-ahead logging (WAL)
   * committing or compaction. In case we do not have permission to write on
   * the provided path, or user set explicitly ReadOnly in the
   * `CacheSettings::openOptions`, the protected cache will be opened in r/o
   * mode. In both cases the database will not be opened and user will receive a
   * `ProtectedCacheCorrupted` from `DefaultCache::Open` in case the database
   * has after open still an un-commited WAL, is uncompressed or cannot
   * guarantee a normal operation and RAM usage. Use this cache if you want to
   * have a stable fallback state or offline data that you can always access
   * regardless of the network state.
   */
  boost::optional<std::string> disk_path_protected = boost::none;

  /**
   * @brief The extend permissions flag (applicable for Unix systems).
   *
   * A boolean option that controls the default permission for file and
   * directory creation. When enabled, all permissions for files and directories
   * will be set to 0666 and 0777 respectively, which allows read, write, and
   * execute access to all users.
   * 
   * Note: the resulting permissions are affected by the umask.
   */
  bool extend_permissions = false;
};

#else

/**
 * @brief Dummy settings for default cache factory.
 */
struct CORE_API CacheSettings {};

#endif  // OLP_SDK_ENABLE_DEFAULT_CACHE

}  // namespace cache
}  // namespace olp
