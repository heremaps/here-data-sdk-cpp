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

#include <future>
#include <string>

#include "CancellationToken.h"

namespace olp {
namespace client {

/**
 * Template class representing the outcome of making a request.  It will contain
 * either a successful result or the failure error.  The caller must check
 * whether the outcome of the request was a success before attempting to access
 *  the result or the error.
 */
template <typename Result, typename Error>
class ApiResponse {
 public:
  using ResultType = Result;
  using ErrorType = Error;

  /**
   * @brief ApiResponse Default constructor.
   */
  ApiResponse() = default;

  /**
   * @brief ApiResponse Constructor for moving a successfully executed request.
   */
  ApiResponse(ResultType result) : result_(std::move(result)), success_(true) {}

  /**
   * @brief ApiResponse Constructor if request unsuccessfully executed
   */
  ApiResponse(const ErrorType& error) : error_(error), success_(false) {}

  /**
   * @brief ApiResponse Copy constructor.
   */
  ApiResponse(const ApiResponse& r)
      : result_(r.result_), error_(r.error_), success_(r.success_) {}

  /**
   * @brief IsSuccessful Gets the status of the request attempt.
   * @return True is request successfully completed.
   */
  inline bool IsSuccessful() const { return success_; }

  /**
   * @brief GetResult Gets the result for a succcessfully executed request
   * @return A valid Result if IsSuccessful() returns true. Undefined,
   * otherwise.
   */
  inline const ResultType& GetResult() const { return result_; }
  /**
   * @brief MoveResult Moves the result for a succcessfully executed request
   * @return A valid Result if IsSuccessful() returns true. Undefined,
   * otherwise.
   */
  inline ResultType&& MoveResult() { return std::move(result_); }

  /**
   * @brief GetError Gets the error for an unsucccessful request attempt.
   * @return A valid Error if IsSuccessful() returns false. Undefined,
   * otherwise.
   */
  inline const ErrorType& GetError() const { return error_; }

 private:
  ResultType result_{};
  ErrorType error_{};
  bool success_{false};
};

template <typename T>
class CancellableFuture {
 public:
  CancellableFuture(const CancellationToken& cancel_token,
                    const std::shared_ptr<std::promise<T>>& promise)
      : cancel_token_(cancel_token), promise_(promise) {}

  inline const CancellationToken& GetCancellationToken() const {
    return cancel_token_;
  }

  inline std::future<T> GetFuture() const { return promise_->get_future(); }

 private:
  CancellationToken cancel_token_;
  std::shared_ptr<std::promise<T>> promise_;
};
}  // namespace client
}  // namespace olp
