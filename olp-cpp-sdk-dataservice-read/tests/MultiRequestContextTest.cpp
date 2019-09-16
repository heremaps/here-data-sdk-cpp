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

#include <gtest/gtest.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <MultiRequestContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/dataservice/read/CatalogClient.h>

using namespace olp::client;
using namespace olp::dataservice::read;

using TestResponse = std::string;
using TestResponseCallback = std::function<void(TestResponse)>;
using TestMultiRequestContext = MultiRequestContext<TestResponse>;

TEST(MultiRequestContextTest, construct) {
  TestMultiRequestContext context("cancelled");
}

TEST(MultiRequestContextTest, executeCalled) {
  std::string key("key");

  std::promise<TestResponseCallback> executeCalled;

  auto execute_fn = [&executeCalled](TestResponseCallback callback) {
    executeCalled.set_value(callback);
    return CancellationToken(
        [callback]() { callback("cancelled by provider"); });
  };

  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };

  {  // force the context out of scope before the callback is destroyed,
     // otherwise crashy-crash. (or heap-allocate responsePromise)
    TestMultiRequestContext context("cancelled");

    context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    auto contextCallback = executeCalled.get_future().get();
    ASSERT_TRUE(contextCallback);

    auto responseFuture = responsePromise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              responseFuture.wait_for(std::chrono::milliseconds(100)));
  }
}

TEST(MultiRequestContextTest, callbackCalled) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expectedResponse("response value");

  auto execute_fn = [&expectedResponse](TestResponseCallback callback) {
    callback(expectedResponse);
    return CancellationToken();
  };

  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };

  context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  auto actualResponse = responsePromise.get_future().get();
  ASSERT_EQ(expectedResponse, actualResponse);
}

TEST(MultiRequestContextTest, multiCallbacks) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expectedResponse("response value");

  std::promise<TestResponseCallback> executeCalled;

  auto execute_fn = [&executeCalled](TestResponseCallback callback) {
    executeCalled.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };

  context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  std::promise<TestResponse> responseDupPromise;
  TestResponseCallback callbackDup_fn =
      [&responseDupPromise](TestResponse response) {
        responseDupPromise.set_value(response);
      };

  context.ExecuteOrAssociate(key, execute_fn, callbackDup_fn);

  auto contextCallback = executeCalled.get_future().get();
  ASSERT_TRUE(contextCallback);

  contextCallback(expectedResponse);

  auto actualResponse = responsePromise.get_future().get();
  ASSERT_EQ(expectedResponse, actualResponse);

  auto actualResponseDup = responseDupPromise.get_future().get();
  ASSERT_EQ(expectedResponse, actualResponseDup);
}

TEST(MultiRequestContextTest, multiRequests) {
  TestMultiRequestContext context("cancelled");

  std::string key1("key");
  TestResponse expectedResponse1("1: response value");

  std::string key2("other");
  TestResponse expectedResponse2("2: response value");

  std::promise<TestResponseCallback> executeCalled1;

  auto execute_fn1 = [&executeCalled1](TestResponseCallback callback) {
    executeCalled1.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponseCallback> executeCalled2;
  auto execute_fn2 = [&executeCalled2](TestResponseCallback callback) {
    executeCalled2.set_value(callback);
    return CancellationToken();
  };

  std::promise<TestResponse> responsePromise1;
  TestResponseCallback callback_fn1 =
      [&responsePromise1](TestResponse response) {
        responsePromise1.set_value(response);
      };

  std::promise<TestResponse> responsePromise2;
  TestResponseCallback callback_fn2 =
      [&responsePromise2](TestResponse response) {
        responsePromise2.set_value(response);
      };

  context.ExecuteOrAssociate(key1, execute_fn1, callback_fn1);
  context.ExecuteOrAssociate(key2, execute_fn2, callback_fn2);

  auto contextCallback2 = executeCalled2.get_future().get();
  ASSERT_TRUE(contextCallback2);
  contextCallback2(expectedResponse2);

  auto contextCallback1 = executeCalled1.get_future().get();
  ASSERT_TRUE(contextCallback1);
  contextCallback1(expectedResponse1);

  auto actualResponse1 = responsePromise1.get_future().get();
  ASSERT_EQ(expectedResponse1, actualResponse1);

  auto actualResponse2 = responsePromise2.get_future().get();
  ASSERT_EQ(expectedResponse2, actualResponse2);
}

TEST(MultiRequestContextTest, cancelSingle) {
  std::string key("key");

  std::promise<bool> cancelCalledPromise;
  std::promise<TestResponseCallback> callbackPromise;
  auto callbackFuture = callbackPromise.get_future();

  CancellationToken providerCancelToken(
      [&cancelCalledPromise, &callbackFuture]() {
        cancelCalledPromise.set_value(true);
        auto contextCallback = callbackFuture.get();

        contextCallback("cancelled by provider");
      });

  auto execute_fn = [&providerCancelToken,
                     &callbackPromise](TestResponseCallback callback) {
    callbackPromise.set_value(callback);
    return providerCancelToken;
  };

  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };
  {
    TestMultiRequestContext context("cancelled");

    auto contextCancelToken =
        context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    auto responseFuture = responsePromise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              responseFuture.wait_for(std::chrono::milliseconds(100)));
    std::cout << "cancel()" << std::endl;

    contextCancelToken.cancel();

    // was cancel called?
    auto f = cancelCalledPromise.get_future();
    ASSERT_EQ(std::future_status::ready,
              f.wait_for(std::chrono::milliseconds(100)));
    ASSERT_TRUE(f.get());
  }
}

TEST(MultiRequestContextTest, autoCancel) {
  std::string key("key");
  std::promise<TestResponse> responsePromise;
  std::promise<TestResponseCallback> executeCalled;

  auto responseFuture = responsePromise.get_future();
  auto executeFuture = executeCalled.get_future();

  std::promise<bool> cancelPromise;

  CancellationToken providerCancelToken([&cancelPromise, &executeFuture]() {
    cancelPromise.set_value(true);
    auto contextCallback = executeFuture.get();

    contextCallback("cancelled by provider");
  });

  {
    TestMultiRequestContext context("cancelled");

    auto execute_fn = [&executeCalled,
                       &providerCancelToken](TestResponseCallback callback) {
      executeCalled.set_value(callback);
      return providerCancelToken;
    };

    TestResponseCallback callback_fn =
        [&responsePromise](TestResponse response) {
          responsePromise.set_value(response);
        };

    context.ExecuteOrAssociate(key, execute_fn, callback_fn);

    ASSERT_EQ(std::future_status::timeout,
              responseFuture.wait_for(std::chrono::milliseconds(100)));
  }
  ASSERT_EQ(std::future_status::ready,
            responseFuture.wait_for(std::chrono::milliseconds(100)));
  ASSERT_EQ("cancelled by provider", responseFuture.get());
}

TEST(MultiRequestContextTest, cancelAfterCompletion) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expectedResponse("response value");

  std::promise<bool> cancelPromise;
  CancellationToken providerCancelToken(
      [&cancelPromise]() { cancelPromise.set_value(true); });

  auto execute_fn = [&providerCancelToken,
                     &expectedResponse](TestResponseCallback callback) {
    callback(expectedResponse);
    return providerCancelToken;
  };

  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };

  auto contextCancelToken =
      context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  auto actualResponse = responsePromise.get_future().get();

  ASSERT_EQ(expectedResponse, actualResponse);

  contextCancelToken.cancel();

  // Cancel should not be called.
  ASSERT_EQ(std::future_status::timeout, cancelPromise.get_future().wait_for(
                                             std::chrono::milliseconds(100)));
}

TEST(MultiRequestContextTest, multiCancel) {
  TestMultiRequestContext context("cancelled");

  std::string key("key");
  TestResponse expectedResponse("response value");

  // Setup Execute Call
  std::promise<bool> cancelPromise;
  CancellationToken providerCancelToken(
      [&cancelPromise]() { cancelPromise.set_value(true); });

  std::promise<TestResponseCallback> executeCalled;

  auto execute_fn = [&executeCalled,
                     &providerCancelToken](TestResponseCallback callback) {
    executeCalled.set_value(callback);
    return providerCancelToken;
  };

  // Setup first request
  std::promise<TestResponse> responsePromise;
  TestResponseCallback callback_fn = [&responsePromise](TestResponse response) {
    responsePromise.set_value(response);
  };

  auto contextCancelToken =
      context.ExecuteOrAssociate(key, execute_fn, callback_fn);

  // Setup second request
  std::promise<TestResponse> responseDupPromise;
  TestResponseCallback callbackDup_fn =
      [&responseDupPromise](TestResponse response) {
        responseDupPromise.set_value(response);
      };

  auto contextCancelTokenDup =
      context.ExecuteOrAssociate(key, execute_fn, callbackDup_fn);

  // Get the context callback
  auto contextCallback = executeCalled.get_future().get();
  ASSERT_TRUE(contextCallback);

  // Cancel the first request
  contextCancelToken.cancel();

  // 1st callback should be invoked
  {
    auto future = responsePromise.get_future();
    ASSERT_EQ(std::future_status::ready,
              future.wait_for(std::chrono::milliseconds(100)));
    ASSERT_EQ("cancelled", future.get());
  }

  // Send the response
  contextCallback(expectedResponse);

  // 2nd callback should be ignored since the request was cancelled entirely.
  {
    auto f = responseDupPromise.get_future();
    ASSERT_EQ(std::future_status::timeout,
              f.wait_for(std::chrono::milliseconds(100)));
  }

  // and the cancel should not be invoked
  auto f = cancelPromise.get_future();
  ASSERT_EQ(std::future_status::timeout,
            f.wait_for(std::chrono::milliseconds(100)));
}
