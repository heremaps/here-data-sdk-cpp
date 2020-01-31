/*
 * Copyright (C) 2019 HERE Europe B.V.
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

#include <olp/core/porting/deprecated.h>
#include <boost/optional.hpp>

namespace olp {
namespace cache {

/**
 * @brief Options for opening a disk cache.
 */
enum OpenOptions : unsigned char {
  Default = 0x00,  /*!< Opens the disk cache in the read-write mode. */
  ReadOnly = 0x01, /*!< Opens the disk cache in the read-only mode. */
  CheckCrc = 0x02  /*!< Verifies the checksum of all data that is read. */
};

/**
 * @brief Settings for in-memory and on-disk caching.
 */
struct CacheSettings {
  /**
   * @brief The path to the on-disk cache.
   *
   * If this parameter is not set, the downloaded data is stored
   * only in the in-memory cache that is limited by `#max_memory_cache_size`.
   *
   * @deprecated Use #disk_path_mutable instead.
   */
  OLP_SDK_DEPRECATED("Use disk_path_mutable instead. Will be removed 03.2020")
  boost::optional<std::string> disk_path = boost::none;

  /**
   * @brief The path to the mutable (read-write) on-disk cache where
   * the SDK caches and lookups the content.
   *
   * You should have write permissions.
   *
   * If this parameter is not set, the downloaded data is stored
   * only in the in-memory cache that is limited by `#max_memory_cache_size`.
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
   * The default value is 32 MB.
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
   * @brief Sets the upper limit of the in-memory data cache size (in bytes).
   *
   * If set to `0`, the in-memory cache is not used. The default value is 1 MB.
   */
  size_t max_memory_cache_size = 1024u * 1024u;

  /**
   * @brief Sets the disk cache open options.
   */
  OpenOptions openOptions = OpenOptions::Default;

  /**
   * @brief The path to the protected (read-only) cache.
   *
   * This cache is used as a primary cache for lookup. `DefaultCache` opens this
   * cache in the read-only mode and does not write to it if
   * `disk_path_mutable` is empty. Use this cash if you want to have a stable
   * fallback state or offline data that you can always access regardless of
   * the network state.
   */
  boost::optional<std::string> disk_path_protected = boost::none;
};

}  // namespace cache
}  // namespace olp
