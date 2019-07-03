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

#include <map>
#include <type_traits>
#include <unordered_map>
#include <utility>

/*
 * Workaround for not having std::map::try_emplace and
 * std::unordered_map::try_emplace
 * until c++17 is available.
 * Note - std::piecewise_construct and emplace are not supported by gcc 4.7
 * therefore for this compiler first argument is not deduced as map::key_type
 * and insert is used
 * instead of emplace.
 */

namespace olp {
namespace porting {
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ <= 7) && \
    defined(__GLIBCXX__) && !defined(__clang__)

template <typename MAP_TYPE_CONT, typename FIRST, typename SECOND>
inline std::pair<typename MAP_TYPE_CONT::iterator, bool> try_emplace(
    MAP_TYPE_CONT& map_type_cont, FIRST&& first, SECOND&& second) {
  return map_type_cont.insert(
      std::make_pair(std::forward<FIRST>(first), std::forward<SECOND>(second)));
}

template <typename MAP_TYPE_CONT, typename FIRST, typename SECOND,
          typename... ARGS>
inline std::pair<typename MAP_TYPE_CONT::iterator, bool> try_emplace(
    MAP_TYPE_CONT& map_type_cont, FIRST&& first, SECOND&& second,
    ARGS&&... args) {
  return map_type_cont.insert(std::make_pair(
      std::forward<FIRST>(first),
      typename MAP_TYPE_CONT::mapped_type(std::forward<SECOND>(second),
                                          std::forward<ARGS>(args)...)));
}

#else

template <class T>
struct IsMap : public std::false_type {};

template <class... ARGS>
struct IsMap<std::map<ARGS...> > : public std::true_type {};

template <class T>
struct IsUnorderedMap : public std::false_type {};

template <class... ARGS>
struct IsUnorderedMap<std::unordered_map<ARGS...> > : public std::true_type {};

/**** For std::map ****/

template <class MAP_TYPE_CONT, class... ARGS>
inline typename std::enable_if<
    IsMap<MAP_TYPE_CONT>::value,
    std::pair<typename MAP_TYPE_CONT::iterator, bool> >::type
try_emplace(MAP_TYPE_CONT& map_type_cont,
            typename MAP_TYPE_CONT::key_type&& key, ARGS&&... args) {
  auto it = map_type_cont.lower_bound(key);
  if (it == map_type_cont.end() || map_type_cont.key_comp()(key, it->first)) {
    return {map_type_cont.emplace_hint(
                it, std::piecewise_construct,
                std::forward_as_tuple(
                    std::forward<typename MAP_TYPE_CONT::key_type>(key)),
                std::forward_as_tuple(std::forward<ARGS>(args)...)),
            true};
  }
  return {it, false};
}

template <class MAP_TYPE_CONT, class... ARGS>
inline typename std::enable_if<
    IsMap<MAP_TYPE_CONT>::value,
    std::pair<typename MAP_TYPE_CONT::iterator, bool> >::type
try_emplace(MAP_TYPE_CONT& map_type_cont,
            const typename MAP_TYPE_CONT::key_type& key, ARGS&&... args) {
  auto it = map_type_cont.lower_bound(key);
  if (it == map_type_cont.end() || map_type_cont.key_comp()(key, it->first)) {
    return {map_type_cont.emplace_hint(
                it, std::piecewise_construct, std::forward_as_tuple(key),
                std::forward_as_tuple(std::forward<ARGS>(args)...)),
            true};
  }
  return {it, false};
}

/**** For std::unordered_map ****/

template <class MAP_TYPE_CONT, class... ARGS>
inline typename std::enable_if<
    IsUnorderedMap<MAP_TYPE_CONT>::value,
    std::pair<typename MAP_TYPE_CONT::iterator, bool> >::type
try_emplace(MAP_TYPE_CONT& map_type_cont,
            typename MAP_TYPE_CONT::key_type&& key, ARGS&&... args) {
  auto it = map_type_cont.find(key);
  if (it == map_type_cont.end()) {
    return map_type_cont.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(
            std::forward<typename MAP_TYPE_CONT::key_type>(key)),
        std::forward_as_tuple(std::forward<ARGS>(args)...));
  }
  return {it, false};
}

template <class MAP_TYPE_CONT, class... ARGS>
inline typename std::enable_if<
    IsUnorderedMap<MAP_TYPE_CONT>::value,
    std::pair<typename MAP_TYPE_CONT::iterator, bool> >::type
try_emplace(MAP_TYPE_CONT& map_type_cont,
            const typename MAP_TYPE_CONT::key_type& key, ARGS&&... args) {
  auto it = map_type_cont.find(key);
  if (it == map_type_cont.end()) {
    return map_type_cont.emplace(
        std::piecewise_construct, std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<ARGS>(args)...));
  }
  return {it, false};
}

#endif
}  // namespace porting
}  // namespace olp
