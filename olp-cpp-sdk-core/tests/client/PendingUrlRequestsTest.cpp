/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include <chrono>
#include <future>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include "client/PendingUrlRequests.h"

namespace {
using olp::client::HttpResponse;
using olp::http::ErrorCode;
using ::testing::_;

namespace client = olp::client;
namespace http = olp::http;

static const std::string kGoodResponse = "Response1234";
static const std::string kBadResponse = "Cancelled";
constexpr int kCancelledStatus =
    static_cast<int>(http::ErrorCode::CANCELLED_ERROR);
constexpr http::RequestId kRequestId = 1234u;
constexpr auto kSleepFor = std::chrono::seconds(1);
constexpr auto kWaitFor = std::chrono::seconds(5);
constexpr uint64_t kBytesDownloaded = 568234u;
constexpr uint64_t kBytesUploaded = 42342u;

client::HttpResponse GetHttpResponse(ErrorCode error, std::string status) {
  return client::HttpResponse(static_cast<int>(error), status);
}

client::HttpResponse GetHttpResponse(int http_status, std::string status,
                                     http::Headers headers = {}) {
  std::stringstream stream;
  stream.str(std::move(status));
  return client::HttpResponse(http_status, std::move(stream),
                              std::move(headers));
}

client::HttpResponse GetCancelledResponse() {
  return {kCancelledStatus, "Operation cancelled"};
}

TEST(HttpResponseTest, Copy) {
  {
    SCOPED_TRACE("Error response");

    auto response = GetHttpResponse(ErrorCode::CANCELLED_ERROR, kBadResponse);
    response.SetNetworkStatistics({kBytesUploaded, kBytesDownloaded});
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);

    auto copy_response = response;

    std::string status;
    std::string copy_status;
    response.GetResponse(status);
    copy_response.GetResponse(copy_status);

    EXPECT_FALSE(status.empty());
    EXPECT_FALSE(copy_status.empty());
    EXPECT_TRUE(response.GetHeaders().empty());
    EXPECT_TRUE(copy_response.GetHeaders().empty());
    EXPECT_EQ(kBadResponse, status);
    EXPECT_EQ(response.GetStatus(),
              static_cast<int>(ErrorCode::CANCELLED_ERROR));
    EXPECT_EQ(copy_response.GetStatus(), response.GetStatus());
    EXPECT_EQ(status, copy_status);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);
  }

  {
    SCOPED_TRACE("Valid response");

    http::Headers headers = {{"header1", "value1"}, {"header2", "value2"}};
    auto response =
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse, headers);
    response.SetNetworkStatistics({kBytesUploaded, kBytesDownloaded});
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);

    auto copy_response = response;

    std::string status;
    std::string copy_status;
    response.GetResponse(status);
    copy_response.GetResponse(copy_status);

    EXPECT_FALSE(status.empty());
    EXPECT_FALSE(copy_status.empty());
    EXPECT_EQ(response.GetHeaders(), headers);
    EXPECT_EQ(copy_response.GetHeaders(), headers);
    EXPECT_EQ(kGoodResponse, status);
    EXPECT_EQ(response.GetStatus(), http::HttpStatusCode::OK);
    EXPECT_EQ(copy_response.GetStatus(), response.GetStatus());
    EXPECT_EQ(status, copy_status);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);
  }

  {
    SCOPED_TRACE("Copy assign");

    auto response = GetHttpResponse(ErrorCode::CANCELLED_ERROR, kBadResponse);
    response.SetNetworkStatistics({kBytesUploaded, kBytesDownloaded});
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);

    HttpResponse copy_response;
    copy_response = response;

    std::string status;
    std::string copy_status;
    response.GetResponse(status);
    copy_response.GetResponse(copy_status);

    EXPECT_FALSE(status.empty());
    EXPECT_FALSE(copy_status.empty());
    EXPECT_TRUE(response.GetHeaders().empty());
    EXPECT_TRUE(copy_response.GetHeaders().empty());
    EXPECT_EQ(kBadResponse, status);
    EXPECT_EQ(response.GetStatus(),
              static_cast<int>(ErrorCode::CANCELLED_ERROR));
    EXPECT_EQ(copy_response.GetStatus(), response.GetStatus());
    EXPECT_EQ(status, copy_status);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesUploaded(),
              kBytesUploaded);
    EXPECT_EQ(copy_response.GetNetworkStatistics().GetBytesDownloaded(),
              kBytesDownloaded);
  }
}

TEST(HttpResponseTest, Move) {
  {
    SCOPED_TRACE("Error response");

    auto response = GetHttpResponse(ErrorCode::CANCELLED_ERROR, kBadResponse);
    auto copy_response = std::move(response);

    std::string status;
    std::string copy_status;
    response.GetResponse(status);
    copy_response.GetResponse(copy_status);

    EXPECT_TRUE(status.empty());
    EXPECT_FALSE(copy_status.empty());
    EXPECT_TRUE(response.GetHeaders().empty());
    EXPECT_TRUE(copy_response.GetHeaders().empty());
    EXPECT_EQ(kBadResponse, copy_status);
    EXPECT_EQ(copy_response.GetStatus(),
              static_cast<int>(ErrorCode::CANCELLED_ERROR));
  }

  {
    SCOPED_TRACE("Valid response");

    http::Headers headers = {{"header1", "value1"}, {"header2", "value2"}};
    auto response =
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse, headers);
    auto copy_response = std::move(response);

    std::string status;
    std::string copy_status;
    response.GetResponse(status);
    copy_response.GetResponse(copy_status);

    EXPECT_TRUE(status.empty());
    EXPECT_FALSE(copy_status.empty());
    EXPECT_TRUE(response.GetHeaders().empty());
    EXPECT_EQ(copy_response.GetHeaders(), headers);
    EXPECT_EQ(kGoodResponse, copy_status);
    EXPECT_EQ(copy_response.GetStatus(), http::HttpStatusCode::OK);
  }
}

void CheckHttResponse(client::HttpResponse& in, int status,
                      const std::string& response,
                      const http::Headers& headers) {
  std::string response_in;
  in.GetResponse(response_in);
  EXPECT_EQ(response_in, response);
  EXPECT_EQ(in.GetHeaders(), headers);
  EXPECT_EQ(in.GetStatus(), status);
}

TEST(PendingUrlRequestsTest, IsCancelled) {
  client::PendingUrlRequests pending_requests;
  const std::string url1 = "url1";
  const std::string url2 = "url2";

  auto check_cancelled = [&](client::HttpResponse response) {
    ASSERT_EQ(response.GetStatus(), kCancelledStatus);
  };

  auto check_not_cancelled = [&](client::HttpResponse response) {
    ASSERT_NE(response.GetStatus(), kCancelledStatus);
  };

  auto complete_call = [&](http::RequestId request_id, const std::string& url) {
    std::this_thread::sleep_for(kSleepFor);
    pending_requests.OnRequestCompleted(
        request_id, url,
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse,
                        {{"header1", "value1"}, {"header2", "value2"}}));
  };

  {
    SCOPED_TRACE("Cancel one request");

    auto request_valid = pending_requests[url1];
    auto request_cancelled = pending_requests[url2];
    ASSERT_TRUE(request_valid && request_cancelled);

    request_valid->Append(check_not_cancelled);
    auto request_id = request_cancelled->Append(check_cancelled);

    ASSERT_FALSE(request_valid->IsCancelled());
    ASSERT_FALSE(request_cancelled->IsCancelled());

    std::future<void> future1, future2;

    request_valid->ExecuteOrCancelled([&](http::RequestId& id) {
      id = kRequestId;
      future1 = std::async(std::launch::async, complete_call, id, url1);
      return client::CancellationToken([] {
        // Should not be called
        EXPECT_TRUE(false) << "Cancellation called on not cancelled request";
      });
    });

    request_cancelled->ExecuteOrCancelled([&](http::RequestId& id) {
      id = kRequestId + 1;
      return client::CancellationToken([&] {
        future2 = std::async(std::launch::async, complete_call, id, url2);
      });
    });

    EXPECT_TRUE(pending_requests.Cancel(url2, request_id));
    EXPECT_FALSE(request_valid->IsCancelled());
    EXPECT_TRUE(request_cancelled->IsCancelled());

    ASSERT_EQ(future1.wait_for(kWaitFor), std::future_status::ready);
    ASSERT_EQ(future2.wait_for(kWaitFor), std::future_status::ready);

    // Cancell all and wait to leave a clean state for the next scope
    ASSERT_TRUE(pending_requests.CancelAllAndWait());
  }

  {
    SCOPED_TRACE("Cancel all requests");

    auto request1 = pending_requests[url1];
    auto request2 = pending_requests[url2];
    ASSERT_TRUE(request1 && request2);

    request1->Append(check_cancelled);
    request2->Append(check_cancelled);

    ASSERT_FALSE(request1->IsCancelled());
    ASSERT_FALSE(request2->IsCancelled());

    std::future<void> future1, future2;

    request1->ExecuteOrCancelled([&](http::RequestId& id) {
      id = kRequestId;
      return client::CancellationToken([&] {
        future1 = std::async(std::launch::async, complete_call, id, url1);
      });
    });

    request2->ExecuteOrCancelled([&](http::RequestId& id) {
      id = kRequestId + 1;
      return client::CancellationToken([&] {
        future2 = std::async(std::launch::async, complete_call, id, url2);
      });
    });

    EXPECT_TRUE(pending_requests.CancelAll());
    EXPECT_TRUE(request1->IsCancelled());
    EXPECT_TRUE(request2->IsCancelled());

    ASSERT_EQ(future1.wait_for(kWaitFor), std::future_status::ready);
    ASSERT_EQ(future2.wait_for(kWaitFor), std::future_status::ready);
  }
}

TEST(PendingUrlRequestsTest, CancelAllAndWait) {
  client::PendingUrlRequests pending_requests;
  const std::string url1 = "url1";
  const std::string url2 = "url2";

  auto request1 = pending_requests[url1];
  auto request2 = pending_requests[url2];
  ASSERT_TRUE(request1 && request2);

  auto check_cancelled = [&](client::HttpResponse response) {
    ASSERT_EQ(response.GetStatus(), kCancelledStatus);
  };

  request1->Append(check_cancelled);
  request2->Append(check_cancelled);

  auto cancel_call = [&](http::RequestId request_id, const std::string& url) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pending_requests.OnRequestCompleted(request_id, url,
                                        GetCancelledResponse());
  };

  const http::RequestId request_id = 1234;
  std::future<void> future1, future2;

  request1->ExecuteOrCancelled([&](http::RequestId& id) {
    id = request_id;
    return client::CancellationToken([&] {
      future1 = std::async(std::launch::async, cancel_call, id, url1);
    });
  });

  request2->ExecuteOrCancelled([&](http::RequestId& id) {
    id = request_id + 1;
    return client::CancellationToken([&] {
      future2 = std::async(std::launch::async, cancel_call, id, url2);
    });
  });

  // Once CancelAllAndWait is called, it should wait for all responses else we
  // face crashes
  ASSERT_TRUE(pending_requests.CancelAllAndWait());
  ASSERT_EQ(future1.wait_for(std::chrono::milliseconds(1)),
            std::future_status::ready);
  ASSERT_EQ(future2.wait_for(std::chrono::milliseconds(1)),
            std::future_status::ready);
}

TEST(PendingUrlRequestsTest, ExecuteOrCancelled) {
  client::PendingUrlRequests pending_requests;
  const std::string url = "url";

  auto check_cancelled = [&](client::HttpResponse response) {
    ASSERT_EQ(response.GetStatus(), kCancelledStatus);
  };

  // Check that ExecuteOrCancelled is behaving correctly
  auto request_ptr = pending_requests[url];
  ASSERT_TRUE(request_ptr);
  ASSERT_FALSE(request_ptr->IsCancelled());
  ASSERT_EQ(request_ptr.use_count(), 2);

  request_ptr->Append(check_cancelled);

  bool is_cancelled = false;
  bool cancel_func_called = false;
  std::future<void> future;

  // Set request Id
  request_ptr->ExecuteOrCancelled(
      [&](http::RequestId& id) {
        return client::CancellationToken([&] {
          is_cancelled = true;
          id = kRequestId;
          future = std::async(
              std::launch::async,
              [&](http::RequestId request_id, const std::string& url) {
                std::this_thread::sleep_for(kSleepFor);
                pending_requests.OnRequestCompleted(
                    request_id, url,
                    GetHttpResponse(
                        http::HttpStatusCode::OK, kGoodResponse,
                        {{"header1", "value1"}, {"header2", "value2"}}));
              },
              id, url);
        });
      },
      [] { EXPECT_TRUE(false) << "Cancel function should not be called!"; });

  // Now cancel request and call again
  request_ptr->CancelOperation();

  request_ptr->ExecuteOrCancelled(
      [](http::RequestId&) {
        EXPECT_TRUE(false) << "Execute function should not be called!";
        return client::CancellationToken();
      },
      [&] { cancel_func_called = true; });

  EXPECT_TRUE(is_cancelled);
  EXPECT_TRUE(cancel_func_called);

  ASSERT_EQ(future.wait_for(kWaitFor), std::future_status::ready);
}

TEST(PendingUrlRequestsTest, SameUrlAfterCancel) {
  // This test covers the use-case were you have a cancelled request which is in
  // the process of waiting for the network cancel answer and afterwards a new
  // request with the same URL. In this case both should exist at the same time,
  // one in the pending request list the other in the cancelled list.
  client::PendingUrlRequests pending_requests;
  const std::string url = "url1";
  std::future<void> future_cancelled, future_valid;

  auto check_cancelled = [&](client::HttpResponse response) {
    ASSERT_EQ(response.GetStatus(), kCancelledStatus);
  };

  auto check_not_cancelled = [&](client::HttpResponse response) {
    ASSERT_NE(response.GetStatus(), kCancelledStatus);
  };

  // Add request to be cancelled
  auto request_ptr = pending_requests[url];
  ASSERT_TRUE(request_ptr);
  ASSERT_FALSE(request_ptr->IsCancelled());
  ASSERT_EQ(request_ptr.use_count(), 2);

  auto cancel_id = request_ptr->Append(check_cancelled);

  request_ptr->ExecuteOrCancelled([&](http::RequestId& id) {
    id = kRequestId;
    return client::CancellationToken([&] {
      future_cancelled =
          std::async(std::launch::async,
                     [&](http::RequestId request_id, const std::string& url) {
                       std::this_thread::sleep_for(kSleepFor);
                       pending_requests.OnRequestCompleted(
                           request_id, url,
                           GetHttpResponse(
                               http::HttpStatusCode::OK, kGoodResponse,
                               {{"header1", "value1"}, {"header2", "value2"}}));
                     },
                     id, url);
    });
  });

  // Now cancel the request
  EXPECT_TRUE(pending_requests.Cancel(url, cancel_id));
  ASSERT_TRUE(request_ptr->IsCancelled());

  // Add second request with the same url
  auto new_request_ptr = pending_requests[url];
  ASSERT_TRUE(new_request_ptr);
  ASSERT_FALSE(new_request_ptr->IsCancelled());
  ASSERT_EQ(new_request_ptr.use_count(), 2);
  ASSERT_FALSE(new_request_ptr == request_ptr);

  new_request_ptr->Append(check_not_cancelled);

  new_request_ptr->ExecuteOrCancelled([&](http::RequestId& id) {
    id = kRequestId + 1;
    future_valid = std::async(
        std::launch::async,
        [&](http::RequestId request_id, const std::string& url) {
          std::this_thread::sleep_for(kSleepFor);
          pending_requests.OnRequestCompleted(
              request_id, url,
              GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse,
                              {{"header1", "value1"}, {"header2", "value2"}}));
        },
        id, url);

    return client::CancellationToken();
  });

  ASSERT_EQ(future_cancelled.wait_for(kWaitFor), std::future_status::ready);
  ASSERT_EQ(future_valid.wait_for(kWaitFor), std::future_status::ready);
}

TEST(PendingUrlRequestsTest, CallbackCalled) {
  client::PendingUrlRequests pending_requests;
  const std::string url = "url1";

  {
    SCOPED_TRACE("Single callback");

    auto request_ptr = pending_requests[url];
    ASSERT_TRUE(request_ptr);
    ASSERT_FALSE(request_ptr->IsCancelled());
    ASSERT_EQ(request_ptr.use_count(), 2);

    // Add one callback and check that it is triggered
    client::HttpResponse response_out;
    EXPECT_EQ(0u, request_ptr->Append([&](client::HttpResponse response) {
      response_out = std::move(response);
    }));

    // Set request Id
    const http::RequestId request_id = 1234;
    request_ptr->ExecuteOrCancelled(
        [&](http::RequestId& id) {
          id = request_id;
          return client::CancellationToken();
        },
        [] { EXPECT_TRUE(false) << "Cancel function should not be called!"; });

    // Relese the pending request from local var to make sure we don't have
    // any issues there.
    request_ptr.reset();

    // Now mark the request as completed and expect the callback to be called
    http::Headers headers = {{"header1", "value1"}, {"header2", "value2"}};
    const auto response_in =
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse, headers);
    pending_requests.OnRequestCompleted(request_id, url, response_in);

    EXPECT_NO_FATAL_FAILURE(CheckHttResponse(
        response_out, response_in.GetStatus(), kGoodResponse, headers));

    ASSERT_EQ(pending_requests.Size(), 0u)
        << "Pending requests should be empty";
  }

  {
    SCOPED_TRACE("Multiple callbacks");

    auto request_ptr = pending_requests[url];
    ASSERT_TRUE(request_ptr);
    ASSERT_FALSE(request_ptr->IsCancelled());
    ASSERT_EQ(request_ptr.use_count(), 2);

    // Add one callback and check that it is triggered
    client::HttpResponse response_out_1, response_out_2;

    EXPECT_EQ(0u, request_ptr->Append([&](client::HttpResponse response) {
      response_out_1 = std::move(response);
    }));

    EXPECT_EQ(1u, request_ptr->Append([&](client::HttpResponse response) {
      response_out_2 = std::move(response);
    }));

    // Set request Id
    const http::RequestId request_id = 1234;
    request_ptr->ExecuteOrCancelled(
        [&](http::RequestId& id) {
          id = request_id;
          return client::CancellationToken();
        },
        [] { EXPECT_TRUE(false) << "Cancel function should not be called!"; });

    // Relese the pending request from local var to make sure we don't have
    // any issues there.
    request_ptr.reset();

    // Now mark the request as completed and expect the callback to be called
    http::Headers headers = {{"header1", "value1"}, {"header2", "value2"}};
    pending_requests.OnRequestCompleted(
        request_id, url,
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse, headers));

    EXPECT_NO_FATAL_FAILURE(CheckHttResponse(
        response_out_1, http::HttpStatusCode::OK, kGoodResponse, headers));

    EXPECT_NO_FATAL_FAILURE(CheckHttResponse(
        response_out_2, http::HttpStatusCode::OK, kGoodResponse, headers));

    ASSERT_EQ(pending_requests.Size(), 0u)
        << "Pending requests should be empty";
  }
  {
    SCOPED_TRACE("Multiple callbacks, one cancelled");

    auto request_ptr = pending_requests[url];
    ASSERT_TRUE(request_ptr);
    ASSERT_FALSE(request_ptr->IsCancelled());
    ASSERT_EQ(request_ptr.use_count(), 2);

    client::HttpResponse response_good, response_cancelled;

    EXPECT_EQ(0u, request_ptr->Append([&](client::HttpResponse response) {
      response_good = std::move(response);
    }));

    auto callback_id = request_ptr->Append([&](client::HttpResponse response) {
      response_cancelled = std::move(response);
    });
    EXPECT_EQ(callback_id, 1u);

    // Set request Id
    const http::RequestId request_id = 1234;
    request_ptr->ExecuteOrCancelled(
        [&](http::RequestId& id) {
          id = request_id;
          return client::CancellationToken();
        },
        [] { EXPECT_TRUE(false) << "Cancel function should not be called!"; });

    // Cancel second request and check that request is not cancelled fully
    pending_requests.Cancel(url, callback_id);
    ASSERT_FALSE(request_ptr->IsCancelled());

    http::Headers headers = {{"header1", "value1"}, {"header2", "value2"}};
    pending_requests.OnRequestCompleted(
        request_id, url,
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse, headers));

    EXPECT_NO_FATAL_FAILURE(CheckHttResponse(
        response_good, http::HttpStatusCode::OK, kGoodResponse, headers));

    std::string cancelled_response;
    auto cancelled_expected = GetCancelledResponse();
    cancelled_expected.GetResponse(cancelled_response);
    EXPECT_NO_FATAL_FAILURE(CheckHttResponse(response_cancelled,
                                             cancelled_expected.GetStatus(),
                                             cancelled_response, {}));
  }
}

TEST(PendingUrlRequestsTest, CancelCallback) {
  client::PendingUrlRequests pending_requests;
  const std::string url1 = "url1", url2 = "url2", url3 = "url3";
  http::RequestId request_id = 1234;

  auto response_call = [&](http::RequestId request_id, const std::string& url) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pending_requests.OnRequestCompleted(
        request_id, url,
        GetHttpResponse(http::HttpStatusCode::OK, kGoodResponse));
  };

  {
    SCOPED_TRACE("Single callback cancelled");

    auto request_ptr = pending_requests[url1];
    ASSERT_TRUE(request_ptr);

    // Add one callback and check that it is triggered and it is cancelled
    auto callback_id = request_ptr->Append([&](client::HttpResponse response) {
      ASSERT_EQ(response.GetStatus(), kCancelledStatus);
    });

    request_ptr->ExecuteOrCancelled([&](http::RequestId& id) {
      id = ++request_id;
      return client::CancellationToken();
    });

    EXPECT_TRUE(pending_requests.Cancel(url1, callback_id));

    // Trigger and wait for response
    std::async(std::launch::async, response_call, request_id, url1).get();
  }

  {
    SCOPED_TRACE("Multiple callbacks, one cancelled");

    auto request_ptr = pending_requests[url1];
    ASSERT_TRUE(request_ptr);

    // Add one callback and check that it is triggered and it is cancelled
    request_ptr->Append([&](client::HttpResponse response) {
      ASSERT_NE(response.GetStatus(), kCancelledStatus);
    });

    request_ptr->ExecuteOrCancelled([&](http::RequestId& id) {
      id = ++request_id;
      return client::CancellationToken();
    });

    auto callback_id = request_ptr->Append([&](client::HttpResponse response) {
      ASSERT_EQ(response.GetStatus(), kCancelledStatus);
    });

    EXPECT_TRUE(pending_requests.Cancel(url1, callback_id));

    // Trigger and wait for response
    std::async(std::launch::async, response_call, request_id, url1).get();
  }

  {
    SCOPED_TRACE("Multiple callbacks, unknown cancelled");

    auto request_ptr = pending_requests[url1];
    ASSERT_TRUE(request_ptr);

    // Add one callback and check that it is triggered and it is cancelled
    request_ptr->Append([&](client::HttpResponse response) {
      ASSERT_NE(response.GetStatus(), kCancelledStatus);
    });

    auto callback_id = request_ptr->Append([&](client::HttpResponse response) {
      ASSERT_NE(response.GetStatus(), kCancelledStatus);
    });

    request_ptr->ExecuteOrCancelled([&](http::RequestId& id) {
      id = ++request_id;
      return client::CancellationToken();
    });

    EXPECT_FALSE(pending_requests.Cancel(url1, callback_id + 15));

    // Trigger and wait for response
    std::async(std::launch::async, response_call, request_id, url1).get();
  }
}

}  // namespace
