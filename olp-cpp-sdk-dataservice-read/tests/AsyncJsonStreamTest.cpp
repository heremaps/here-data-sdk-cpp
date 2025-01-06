/*
 * Copyright (C) 2023-2025 HERE Europe B.V.
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

#include <gtest/gtest.h>

#include "repositories/AsyncJsonStream.h"

namespace {

namespace repository = olp::dataservice::read::repository;

TEST(AsyncJsonStreamTest, NormalFlow) {
  repository::AsyncJsonStream stream;

  auto current_stream = stream.GetCurrentStream();

  stream.AppendContent("123", 3);

  EXPECT_EQ(current_stream->Tell(), 0u);
  EXPECT_EQ(current_stream->Peek(), '1');
  EXPECT_EQ(current_stream->Tell(), 0u);
  EXPECT_EQ(current_stream->Take(), '1');
  EXPECT_EQ(current_stream->Take(), '2');
  EXPECT_EQ(current_stream->Tell(), 2u);
  EXPECT_EQ(current_stream->Take(), '3');

  stream.ResetStream("234", 3);

  auto new_current_stream = stream.GetCurrentStream();
  EXPECT_NE(current_stream.get(), new_current_stream.get());

  EXPECT_EQ(current_stream->Peek(), '\0');
  EXPECT_EQ(current_stream->Take(), '\0');
  EXPECT_TRUE(current_stream->ReadEmpty());

  EXPECT_EQ(new_current_stream->Peek(), '2');
  EXPECT_EQ(new_current_stream->Take(), '2');
  EXPECT_EQ(new_current_stream->Take(), '3');
  EXPECT_EQ(new_current_stream->Take(), '4');
  EXPECT_TRUE(new_current_stream->ReadEmpty());

  stream.AppendContent("5", 1);
  // Read buffer is empty here because swap is on Take/Peek
  EXPECT_FALSE(new_current_stream->WriteEmpty());
  EXPECT_TRUE(new_current_stream->ReadEmpty());

  stream.CloseStream(olp::client::ApiError::Cancelled());

  EXPECT_EQ(new_current_stream->Take(), '5');
  EXPECT_EQ(new_current_stream->Take(), '\0');

  EXPECT_TRUE(stream.IsClosed());

  auto error = stream.GetError();
  EXPECT_TRUE(error &&
              error->GetErrorCode() == olp::client::ErrorCode::Cancelled);

  stream.CloseStream(olp::client::ApiError::NetworkConnection());
  EXPECT_TRUE(stream.GetError()->GetErrorCode() ==
              olp::client::ErrorCode::Cancelled);

  EXPECT_TRUE(new_current_stream->ReadEmpty());
  stream.AppendContent("17", 2);
  EXPECT_TRUE(new_current_stream->ReadEmpty());
  stream.ResetStream("4", 1);
  EXPECT_TRUE(new_current_stream->ReadEmpty());
}

}  // namespace
