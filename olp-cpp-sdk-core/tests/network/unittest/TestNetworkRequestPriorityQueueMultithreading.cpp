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

#include "NetworkRequestPriorityQueue.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <future>
#include <map>
#include <memory>
#include <vector>

using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::Not;
using ::testing::NotNull;

using olp::network::Network;
using olp::network::NetworkRequest;
using olp::network::NetworkRequestPriorityQueue;
using olp::network::RequestContext;
using olp::network::RequestContextPtr;

static const auto defaultUrl = "url";

namespace {
RequestContextPtr createRequestContextPtr(const std::string& url) {
  return std::make_shared<RequestContext>(NetworkRequest(url),
                                          Network::NetworkRequestIdMin, nullptr,
                                          nullptr, nullptr, nullptr, nullptr);
}

template <int nThreads>
class NetworkRequestPriorityQueueMultiThreadingTest : public ::testing::Test {
 public:
  static const auto s_numberOfThreads = nThreads;

 protected:
  void waitForSetupThenStartTest() {
    for_each(begin(m_threadReadySignals), end(m_threadReadySignals),
             [](std::promise<void>& threadReadySignal) {
               threadReadySignal.get_future().wait();
             });

    m_startTestSignal.set_value();
  }

  // These are tools necessary to ensure that all launched thread are really run
  // in parallel (see i.e. Anthony Williams, C++-Concurrency In Action:
  // Practical Multithreading, Ch. 9)
  std::promise<void> m_startTestSignal;
  std::shared_future<void> m_startTest{m_startTestSignal.get_future()};
  std::array<std::promise<void>, s_numberOfThreads> m_threadReadySignals;
};

MATCHER_P(ContainsRequestContextWith, url, "") {
  return find_if(begin(arg), end(arg), [this](RequestContextPtr ptr) {
           return ptr && ptr->request_.Url() == url;
         }) != end(arg);
}

MATCHER_P(IsContainedIn, container, "") {
  return find_if(begin(container), end(container),
                 [&arg](const std::string& url) { return url == arg; }) !=
         end(container);
}
}  // namespace

using NetworkRequestPriorityQueueMultiThreadingWithTwoThreadsTest =
    NetworkRequestPriorityQueueMultiThreadingTest<2>;

TEST_F(
    NetworkRequestPriorityQueueMultiThreadingWithTwoThreadsTest,
    If_OneThreadCallsPushAnotherCallsPop_Then_QueueContainsMaxOneEntryAndIfNotThePushedEntryIsPopped) {
  NetworkRequestPriorityQueue queue;

  auto pushDone = std::async(std::launch::async, [this, &queue] {
    auto entry = createRequestContextPtr(defaultUrl);

    m_threadReadySignals[0].set_value();
    m_startTest.wait();

    queue.Push(std::move(entry));
  });

  auto popDone = std::async(std::launch::async, [this, &queue] {
    m_threadReadySignals[1].set_value();
    m_startTest.wait();

    return queue.Pop();
  });

  waitForSetupThenStartTest();

  // wait until test has finished
  pushDone.get();
  const auto entry = popDone.get();

  // verify results
  if (entry) {
    EXPECT_THAT(entry->request_.Url(), Eq(defaultUrl));
    EXPECT_THAT(queue.Size(), 0u);
  } else {
    EXPECT_THAT(queue.Size(), 1u);
  }
}

using NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest =
    NetworkRequestPriorityQueueMultiThreadingTest<8>;

TEST_F(
    NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest,
    If_MultipleThreadsCallPushOnceToEmptyQueue_Then_ElementsFromEachThreadAreInTheQueue) {
  const auto nThreads =
      NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest::
          s_numberOfThreads;
  NetworkRequestPriorityQueue queue;

  std::vector<std::future<void> > done;
  std::vector<std::string> urls;
  for (auto i = 0; i < nThreads; ++i) {
    const auto url = "url" + std::to_string(i);
    urls.emplace_back(url);
    auto asyncDone = std::async(std::launch::async, [this, i, &queue, url] {
      auto entry = createRequestContextPtr(url.c_str());

      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      queue.Push(std::move(entry));
    });

    done.push_back(std::move(asyncDone));
  }

  waitForSetupThenStartTest();

  // wait until test has_finished
  for_each(begin(done), end(done),
           [](const std::future<void>& end) { end.wait(); });

  // verify result
  auto foundUrls = std::map<std::string, bool>();
  for_each(begin(urls), end(urls), [&foundUrls](const std::string& url) {
    return foundUrls[url] = false;
  });
  while (auto entry = queue.Pop()) {
    foundUrls[entry->request_.Url()] = true;
  }

  using EntryType = decltype(*begin(foundUrls));
  const auto found_all_entries =
      all_of(begin(foundUrls), end(foundUrls),
             [](EntryType& entry) { return entry.second; });

  EXPECT_TRUE(found_all_entries);
}

TEST_F(
    NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest,
    If_MultipleThreadsCallPopToQueueWithTwoElements_Then_TheTwoElementArePoppedFromQueue) {
  const auto nThreads =
      NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest::
          s_numberOfThreads;

  NetworkRequestPriorityQueue queue;
  const std::vector<std::string> urls = {"url0", "url1"};
  for_each(begin(urls), end(urls), [&queue](const std::string& url) {
    auto entry = createRequestContextPtr(url);
    queue.Push(entry);
  });
  const auto initialSize = urls.size();

  std::vector<std::future<decltype(queue.Pop())> > popped;
  for (auto i = 0; i < nThreads; ++i) {
    auto asyncDone = std::async(std::launch::async, [this, i, &queue] {
      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      return queue.Pop();
    });

    popped.emplace_back(std::move(asyncDone));
  }

  waitForSetupThenStartTest();

  // wait until test has_finished
  auto popped_entries = std::vector<RequestContextPtr>();
  transform(
      begin(popped), end(popped), back_inserter(popped_entries),
      [](std::future<RequestContextPtr>& future) { return future.get(); });

  const auto numberOfEmptyElements =
      count(begin(popped_entries), end(popped_entries), nullptr);
  const auto expectedNumberOfEmptyElements =
      static_cast<decltype(numberOfEmptyElements)>(nThreads - initialSize);
  EXPECT_THAT(numberOfEmptyElements, Eq(expectedNumberOfEmptyElements));

  for_each(begin(urls), end(urls), [&popped_entries](const std::string& url) {
    EXPECT_THAT(popped_entries, ContainsRequestContextWith(url));
  });
}

TEST_F(
    NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest,
    If_OneThreadCallsPopAndMultipleThreadsCallPushToEmptyQueue_Then_OneOrNoElementIsPoppedFromTheQueue) {
  const auto nThreads =
      NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest::
          s_numberOfThreads;
  NetworkRequestPriorityQueue queue;

  std::vector<std::future<void> > pushDone;
  for (auto i = 0; i < nThreads - 1; ++i) {
    auto asyncDone = std::async(std::launch::async, [this, i, &queue] {
      auto entry = createRequestContextPtr(defaultUrl);

      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      queue.Push(entry);
    });

    pushDone.emplace_back(std::move(asyncDone));
  }

  auto popDone = std::async(std::launch::async, [this, &queue] {
    m_threadReadySignals.back().set_value();
    m_startTest.wait();

    return queue.Pop();
  });

  waitForSetupThenStartTest();

  // wait until test has_finished
  for_each(begin(pushDone), end(pushDone),
           [](const std::future<void>& end) { end.wait(); });
  popDone.wait();

  // verify result
  const auto poppedEntry = popDone.get();

  if (poppedEntry) {
    EXPECT_THAT(poppedEntry->request_.Url(), Eq(defaultUrl));
    EXPECT_THAT(queue.Size() + 1u, Eq(pushDone.size()));
  } else {
    EXPECT_THAT(queue.Size(), Eq(pushDone.size()));
  }
}

TEST_F(
    NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest,
    If_HalfThreadsCallPopAndOtherHalfCallPushToEmptyQueue_Then_UpToNHalfElementsArePoppedFromTheQueue) {
  const auto nThreads =
      NetworkRequestPriorityQueueMultiThreadingWithEightThreadsTest::
          s_numberOfThreads;
  NetworkRequestPriorityQueue queue;

  const auto half = nThreads / 2;

  std::vector<std::string> urls;
  std::vector<std::future<void> > pushDone;
  for (auto i = 0; i < half; ++i) {
    const auto url = "url" + std::to_string(i);
    urls.emplace_back(url);
    auto asyncDone = std::async(std::launch::async, [this, i, &queue, url] {
      auto entry = createRequestContextPtr(url);

      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      queue.Push(entry);
    });

    pushDone.emplace_back(std::move(asyncDone));
  }

  std::vector<std::future<RequestContextPtr> > popDone;
  for (auto i = half; i < nThreads; ++i) {
    auto asyncDone = std::async(std::launch::async, [this, i, &queue] {
      m_threadReadySignals[i].set_value();
      m_startTest.wait();

      return queue.Pop();
    });

    popDone.emplace_back(std::move(asyncDone));
  }

  waitForSetupThenStartTest();

  // wait until test has_finished
  for_each(begin(pushDone), end(pushDone),
           [](const std::future<void>& end) { end.wait(); });
  std::vector<std::string> poppedUrls;
  for_each(begin(popDone), end(popDone),
           [&poppedUrls](std::future<RequestContextPtr>& future) {
             if (auto entry = future.get()) {
               poppedUrls.push_back(entry->request_.Url());
             }
           });

  // verify result
  std::vector<std::string> remainingUrlsInQueue;
  while (auto entry = queue.Pop()) {
    remainingUrlsInQueue.push_back(entry->request_.Url());
  }

  for_each(begin(urls), end(urls),
           [&poppedUrls, &remainingUrlsInQueue](const std::string& url) {
             EXPECT_THAT(url, AnyOf(IsContainedIn(poppedUrls),
                                    IsContainedIn(remainingUrlsInQueue)));
             EXPECT_THAT(url, Not(AllOf(IsContainedIn(poppedUrls),
                                        IsContainedIn(remainingUrlsInQueue))));
           });
}
