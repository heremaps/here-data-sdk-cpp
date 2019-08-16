#include "PendingRequests.h"

#include <gtest/gtest.h>

TEST(PendingRequests, InsertNeedsGeneratedPlaceholderInAdvancePositive) {
  olp::dataservice::read::PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Insert(olp::client::CancellationToken(), key));
}

TEST(PendingRequests, InsertNeedsGeneratedPlaceholderInAdvanceNegative) {
  olp::dataservice::read::PendingRequests pending_request;
  EXPECT_FALSE(pending_request.Insert(olp::client::CancellationToken(), 0));
}

TEST(PendingRequests, InsertFailsAftherThePlaceholderIsRemoved) {
  olp::dataservice::read::PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Remove(key));
  EXPECT_FALSE(pending_request.Insert(olp::client::CancellationToken(), key));
}

TEST(PendingRequests, PlaceholderCanBeRemovedAfterInsert) {
  olp::dataservice::read::PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  EXPECT_TRUE(pending_request.Insert(olp::client::CancellationToken(), key));
  EXPECT_TRUE(pending_request.Remove(key));
}

TEST(PendingRequests, RemoveMissingKeyWillFail) {
  olp::dataservice::read::PendingRequests pending_request;
  EXPECT_FALSE(pending_request.Remove(0));
}

TEST(PendingRequests, CancellAllPendingRequest) {
  olp::dataservice::read::PendingRequests pending_request;
  auto key = pending_request.GenerateRequestPlaceholder();
  bool cancelled = false;
  auto token = olp::client::CancellationToken([&]() { cancelled = true; });
  EXPECT_TRUE(pending_request.Insert(token, key));
  EXPECT_TRUE(pending_request.CancelPendingRequests());
  EXPECT_TRUE(cancelled);
}