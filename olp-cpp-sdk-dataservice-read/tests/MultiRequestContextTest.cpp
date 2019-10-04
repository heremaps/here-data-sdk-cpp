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

#include <iostream>
#include <map>
#include <mutex>

#include <MultiRequestContext.h>
#include <gtest/gtest.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {

using namespace olp::client;
using namespace olp::dataservice::read;

using TestResponse = std::string;
using TestResponseCallback = std::function<void(TestResponse)>;
using TestMultiRequestContext = MultiRequestContext<TestResponse>;

TEST(MultiRequestContextTest, Construct) {
  TestMultiRequestContext context("cancelled");
}

TEST(MultiRequestContextTest, ExecuteCalled) {
  std::string key("key");

  std::promise<TestResponseCallback> execute_called;

  auto execute_fn = [&execute_called](TestResponseCallback callback) {
    execute_called.set_value(callback);
    return CancellationToken(
        [callback]() { callback("cancelled by provider"); });
  };

  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };

  {  // force the context out of scope before the callback is destroyed,
     // otherwise crashy-crash. (or heap-allocate response_promise)
    TestMultiRequestContext context("cancelled");

    context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    auto context_callback = execute_called.get_future().get();
    ASSERT_TRUE(context_callback);

    auto response_future = response_promise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              response_future.wait_for(std::chrono::milliseconds(100)));
  }
}

TEST(MultiRequestContextTest, CallbackCalled) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expected_response("response value");

  auto execute_fn = [&expected_response](TestResponseCallback callback) {
    callback(expected_response);
    return CancellationToken();
  };

  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };

  context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  auto actual_response = response_promise.get_future().get();
  ASSERT_EQ(expected_response, actual_response);
}

TEST(MultiRequestContextTest, MultiCallbacks) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expected_response("response value");

  std::promise<TestResponseCallback> execute_called;

  auto execute_fn = [&execute_called](TestResponseCallback callback) {
    execute_called.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };

  context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  std::promise<TestResponse> response_dup_promise;
  TestResponseCallback callback_dup_fn =
      [&response_dup_promise](TestResponse response) {
        response_dup_promise.set_value(response);
      };

  context.ExecuteOrAssociate(key, execute_fn, callback_dup_fn);

  auto context_callback = execute_called.get_future().get();
  ASSERT_TRUE(context_callback);

  context_callback(expected_response);

  auto actual_response = response_promise.get_future().get();
  ASSERT_EQ(expected_response, actual_response);

  auto actual_response_dup = response_dup_promise.get_future().get();
  ASSERT_EQ(expected_response, actual_response_dup);
}

TEST(MultiRequestContextTest, MultiRequests) {
  TestMultiRequestContext context("cancelled");

  std::string key1("key");
  TestResponse expected_response1("1: response value");

  std::string key2("other");
  TestResponse expected_response2("2: response value");

  std::promise<TestResponseCallback> execute_called1;

  auto execute_fn1 = [&execute_called1](TestResponseCallback callback) {
    execute_called1.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponseCallback> execute_called2;
  auto execute_fn2 = [&execute_called2](TestResponseCallback callback) {
    execute_called2.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponse> response_promise1;
  TestResponseCallback callback_fn1 =
      [&response_promise1](TestResponse response) {
        response_promise1.set_value(response);
      };

  std::promise<TestResponse> response_promise2;
  TestResponseCallback callback_fn2 =
      [&response_promise2](TestResponse response) {
        response_promise2.set_value(response);
      };

  context.ExecuteOrAssociate(key1, execute_fn1, callback_fn1);
  context.ExecuteOrAssociate(key2, execute_fn2, callback_fn2);

  auto context_callback2 = execute_called2.get_future().get();
  ASSERT_TRUE(context_callback2);
  context_callback2(expected_response2);

  auto context_callback1 = execute_called1.get_future().get();
  ASSERT_TRUE(context_callback1);
  context_callback1(expected_response1);

  auto actual_response1 = response_promise1.get_future().get();
  ASSERT_EQ(expected_response1, actual_response1);

  auto actual_response2 = response_promise2.get_future().get();
  ASSERT_EQ(expected_response2, actual_response2);
}

TEST(MultiRequestContextTest, CancelSingle) {
  std::string key("key");

  std::promise<bool> cancel_called_promise;
  std::promise<TestResponseCallback> callback_promise;
  auto callback_future = callback_promise.get_future();

  CancellationToken provider_cancel_token(
      [&cancel_called_promise, &callback_future]() {
        cancel_called_promise.set_value(true);
        auto context_callback = callback_future.get();

        context_callback("cancelled by provider");
      });

  auto execute_fn = [&provider_cancel_token,
                     &callback_promise](TestResponseCallback callback) {
    callback_promise.set_value(callback);
    return provider_cancel_token;
  };

  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };
  {
    TestMultiRequestContext context("cancelled");

    auto context_cancel_token =
        context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    auto response_future = response_promise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              response_future.wait_for(std::chrono::milliseconds(100)));
    std::cout << "cancel()" << std::endl;

    context_cancel_token.cancel();

    // was cancel called?
    auto f = cancel_called_promise.get_future();
    ASSERT_EQ(std::future_status::ready,
              f.wait_for(std::chrono::milliseconds(100)));
    ASSERT_TRUE(f.get());
  }
}

TEST(MultiRequestContextTest, AutoCancel) {
  std::string key("key");
  std::promise<TestResponse> response_promise;
  std::promise<TestResponseCallback> execute_called;

  auto response_future = response_promise.get_future();
  auto execute_future = execute_called.get_future();

  std::promise<bool> cancel_promise;

  CancellationToken provider_cancel_token([&cancel_promise, &execute_future]() {
    cancel_promise.set_value(true);
    auto context_callback = execute_future.get();

    context_callback("cancelled by provider");
  });

  {
    TestMultiRequestContext context("cancelled");

    auto execute_fn = [&execute_called,
                       &provider_cancel_token](TestResponseCallback callback) {
      execute_called.set_value(callback);
      return provider_cancel_token;
    };

    TestResponseCallback callback_fn =
        [&response_promise](TestResponse response) {
          response_promise.set_value(response);
        };

    context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    ASSERT_EQ(std::future_status::timeout,
              response_future.wait_for(std::chrono::milliseconds(100)));
  }
  ASSERT_EQ(std::future_status::ready,
            response_future.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ("cancelled by provider", response_future.get());
}

TEST(MultiRequestContextTest, CancelAfterCompletion) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expected_response("response value");

  std::promise<bool> cancel_promise;
  CancellationToken provider_cancel_token(
      [&cancel_promise]() { cancel_promise.set_value(true); });

  auto execute_fn = [&provider_cancel_token,
                     &expected_response](TestResponseCallback callback) {
    callback(expected_response);
    return provider_cancel_token;
  };

  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };

  auto context_cancel_token =
      context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  auto actual_response = response_promise.get_future().get();

  ASSERT_EQ(expected_response, actual_response);

  context_cancel_token.cancel();

  // Cancel should not be called.
  ASSERT_EQ(std::future_status::timeout, cancel_promise.get_future().wait_for(
                                             std::chrono::milliseconds(100)));
}

TEST(MultiRequestContextTest, MultiCancel) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expected_response("response value");

  // Setup Execute Call
  std::promise<bool> cancel_promise;
  CancellationToken provider_cancel_token(
      [&cancel_promise]() { cancel_promise.set_value(true); });

  std::promise<TestResponseCallback> execute_called;

  auto execute_fn = [&execute_called,
                     &provider_cancel_token](TestResponseCallback callback) {
    execute_called.set_value(callback);
    return provider_cancel_token;
  };

  // Setup first request
  std::promise<TestResponse> response_promise;
  TestResponseCallback callback_fn =
      [&response_promise](TestResponse response) {
        response_promise.set_value(response);
      };

  auto context_cancel_token =
      context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  // Setup second request
  std::promise<TestResponse> response_dup_promise;
  TestResponseCallback callback_dup_fn =
      [&response_dup_promise](TestResponse response) {
        response_dup_promise.set_value(response);
      };

  auto context_cancel_tokenDup =
      context.ExecuteOrAssociate(key, execute_fn, callback_dup_fn);

  // Get the context callback
  auto context_callback = execute_called.get_future().get();
  ASSERT_TRUE(context_callback);

  // Cancel the first request
  context_cancel_token.cancel();

  // 1st callback should be invoked
  {
    auto future = response_promise.get_future();
    ASSERT_EQ(std::future_status::ready,
              future.wait_for(std::chrono::milliseconds(100)));
    ASSERT_EQ("cancelled", future.get());
  }

  // Send the response
  context_callback(expected_response);

  // 2nd callback should be ignored since the request was cancelled entirely.
  {
    auto f = response_dup_promise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              f.wait_for(std::chrono::milliseconds(100)));
  }

  // and the cancel should not be invoked
  auto f = cancel_promise.get_future();
  ASSERT_EQ(std::future_status::timeout,
            f.wait_for(std::chrono::milliseconds(100)));
}

}  // namespace
