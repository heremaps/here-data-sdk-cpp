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

#include "CancellationTokenList.h"
#include <algorithm>
#include <atomic>

namespace olp {
namespace dataservice {
namespace write {

class CancellationTokenListCmp {
 public:
  bool operator()(
      const int &id,
      const std::tuple<int, olp::client::CancellationToken> &t) const {
    return id < std::get<0>(t);
  }

  bool operator()(const std::tuple<int, olp::client::CancellationToken> &t,
                  const int &id) const {
    return std::get<0>(t) < id;
  }
};

void CancellationTokenList::AddTask(int id,
                                    olp::client::CancellationToken token) {
  std::lock_guard<std::mutex> lg(mut_);
  tokenList_.insert(std::upper_bound(begin(tokenList_), end(tokenList_), id,
                                     CancellationTokenListCmp()),
                    std::tuple<int, olp::client::CancellationToken>(id, token));
}

void CancellationTokenList::RemoveTask(int id) {
  std::lock_guard<std::mutex> lg(mut_);
  auto it = std::lower_bound(begin(tokenList_), end(tokenList_), id,
                             CancellationTokenListCmp());
  if (it == end(tokenList_) || std::get<0>(*it) != id) return;
  tokenList_.erase(std::rotate(it, it + 1, end(tokenList_)));
}

void CancellationTokenList::CancelAll() {
  while (true) {
    auto idList = GetTaskIdList();
    if (idList.empty()) return;

    for (auto id : idList) {
      auto it = std::lower_bound(begin(tokenList_), end(tokenList_), id,
                                 CancellationTokenListCmp());
      if (it == end(tokenList_) || std::get<0>(*it) != id) continue;
      std::get<1>(*it).Cancel();
    }

    {
      std::lock_guard<std::mutex> lg(mut_);
      tokenList_.erase(
          std::remove_if(
              begin(tokenList_), end(tokenList_),
              [&idList](
                  const std::tuple<int, olp::client::CancellationToken> &t) {
                auto it = std::lower_bound(begin(idList), end(idList),
                                           std::get<0>(t));
                return it != end(idList) && std::get<0>(t) == *it;
              }),
          end(tokenList_));
    }
  }
}

std::vector<int> CancellationTokenList::GetTaskIdList() const {
  std::lock_guard<std::mutex> lg(mut_);
  std::vector<int> res;
  for (const auto &t : tokenList_) {
    res.push_back(std::get<0>(t));
  }
  return res;
}

int CancellationTokenList::GetNextId() const {
  static std::atomic<int> requestId(0);
  return requestId++;
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
