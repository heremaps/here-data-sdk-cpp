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

#include <boost/optional.hpp>

namespace olp {

namespace cache {

/**
 * @brief Options for opening the disk cache
 */
enum OpenOptions : unsigned char {
  Default = 0x00,
  ReadOnly = 0x01,
  CheckCrc = 0x02
};

struct CacheSettings {
  /**
   * @brief Path to save contents on disk
   */
  boost::optional<std::string> disk_path = boost::none;

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
};

}  // namespace cache

}  // namespace olp
