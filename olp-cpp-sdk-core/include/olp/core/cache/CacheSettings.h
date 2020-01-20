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
 * @brief Options for opening the disk cache.
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
   * @deprecated Use #disk_path_mutable instead.
   */
  OLP_SDK_DEPRECATED("Use disk_path_mutable instead. Will be removed 03.2020")
  boost::optional<std::string> disk_path = boost::none;

  /**
   * @brief The path to the mutable (read-write) on-disk cache where the
   * SDK will cache and lookup the content. You should have write permissions.
   *
   * If this parameter is not set, the downloaded data is stored
   * only in the in-memory cache that is limited by `#max_memory_cache_size`.
   */
  boost::optional<std::string> disk_path_mutable = boost::none;

  /**
   * @brief Set the upper limit of disk space to use for persistent stores in
   * bytes. Default is 32 MB. Set it to std::uint64_t(-1) in order to never
   * evict data.
   */
  std::uint64_t max_disk_storage = 1024ull * 1024ull * 32ull;

  /**
   * @brief Set the upper limit of runtime memory to be used before being
   * written to disk in bytes. Default is 32 Mb.
   */
  size_t max_chunk_size = 1024u * 1024u * 32u;

  /**
   * @brief Set the to indicate that continuous flushes to disk are
   * necessary to preserve maximum data between ignition cycles
   */
  bool enforce_immediate_flush = true;

  /**
   * @brief Set the maximum permissable size of one file in storage in bytes.
   * Default is 2 MB.
   */
  size_t max_file_size = 1024u * 1024u * 2u;

  /**
   * @brief Set the upper limit of the in-memory data cache size in bytes.
   * 0 means the in-memory cache is not used. Default is 1 MB.
   */
  size_t max_memory_cache_size = 1024u * 1024u;

  /**
   * @brief Set the disk cache open options.
   */
  OpenOptions openOptions = OpenOptions::Default;

  /**
   * @brief Path to the protected (readonly) cache. This cache is used as a
   * primary cache for lookup. DefaultCache will open this cache in read-only
   * mode and will not write to it in case the disk_path_mutable is empty. This
   * path should only be used in case users would like to have a stable fallback
   * state or offline data that they can always access regardless of the network
   * state.
   */
  boost::optional<std::string> disk_path_protected = boost::none;
};

}  // namespace cache
}  // namespace olp
