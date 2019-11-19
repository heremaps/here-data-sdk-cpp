#include <olp/core/client/PendingRequests.h>

#include <gtest/gtest.h>

namespace {

using olp::client::PendingRequests;

TEST(PendingRequestsTest, InsertNeedsGeneratedPlaceholderInAdvancePositive) {
  PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Insert(olp::client::CancellationToken(), key));
}

TEST(PendingRequestsTest, InsertNeedsGeneratedPlaceholderInAdvanceNegative) {
  PendingRequests pending_request;
  EXPECT_FALSE(pending_request.Insert(olp::client::CancellationToken(), 0));
}

TEST(PendingRequestsTest, InsertFailsAftherThePlaceholderIsRemoved) {
  PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Remove(key));
  EXPECT_FALSE(pending_request.Insert(olp::client::CancellationToken(), key));
}

TEST(PendingRequestsTest, PlaceholderCanBeRemovedAfterInsert) {
  PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Insert(olp::client::CancellationToken(), key));
  EXPECT_TRUE(pending_request.Remove(key));
}

TEST(PendingRequestsTest, RemoveMissingKeyWillFail) {
  PendingRequests pending_request;
  EXPECT_FALSE(pending_request.Remove(0));
}

TEST(PendingRequestsTest, CancelAll) {
  PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  bool cancelled = false;
  auto token = olp::client::CancellationToken([&]() { cancelled = true; });
  EXPECT_TRUE(pending_request.Insert(token, key));
  EXPECT_TRUE(pending_request.CancelAll());
  EXPECT_TRUE(cancelled);
}

}  // namespace
