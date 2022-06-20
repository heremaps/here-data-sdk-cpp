/*
 * Copyright (C) 2019-2022 HERE Europe B.V.
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
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "CancellationToken.h"

namespace olp {
namespace client {

/// The `ApiResponse` extension class, used to carry the payload with the
/// response.
template <typename Payload>
class ResponseExtension {
 public:
  ResponseExtension() = default;

  explicit ResponseExtension(Payload payload) : payload_(std::move(payload)) {}

  const Payload& GetPayload() const { return payload_; }

 protected:
  Payload payload_{};
};

/// The `ApiResponse` extension class specialization without any payload.
template <>
class ResponseExtension<void> {
 public:
  ResponseExtension() = default;
};

/**
 * @brief Represents a request outcome.
 *
 * Contains a successful result or failure error. Before you try to access
 * the error result, check the request outcome.
 *
 * @tparam Result The result type.
 * @tparam Error The error type.
 */
template <typename Result, typename Error, typename Payload = void>
class ApiResponse : public ResponseExtension<Payload> {
 public:
  /// The type of result.
  using ResultType = Result;

  /// The type of error.
  using ErrorType = Error;

  /// The type of additional payload.
  using PayloadType = Payload;

  ApiResponse() = default;

  /**
   * @brief Creates the `ApiResponse` instance from a similar response type
   * without payload. The payload is default initialized.
   *
   * @note Enabled only for the use cases when the input response type has no
   * payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param other The `ApiResponse` instance.
   */
  template <
      typename P = Payload, typename R = Result,
      typename = typename std::enable_if<!std::is_same<P, void>::value>::type,
      typename =
          typename std::enable_if<std::is_copy_constructible<R>::value>::type>
  ApiResponse(const ApiResponse<R, Error, void>& other)  // NOLINT
      : ResponseExtension<P>(),
        result_(other.GetResult()),
        error_(other.GetError()),
        success_(other.IsSuccessful()) {}

  /**
   * @brief Creates the `ApiResponse` instance from a similar response type
   * without payload. The payload is default initialized.
   *
   * @note Enabled only for the use cases when the input response type has no
   * payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param other The `ApiResponse` instance.
   */
  template <
      typename P = Payload, typename R = Result,
      typename = typename std::enable_if<!std::is_same<P, void>::value>::type,
      typename =
          typename std::enable_if<std::is_move_constructible<R>::value>::type>
  ApiResponse(ApiResponse<R, Error, void>&& other)  // NOLINT
      : ResponseExtension<P>(),
        result_(other.MoveResult()),
        error_(other.GetError()),
        success_(other.IsSuccessful()) {}

  /**
   * @brief Creates the `ApiResponse` instance from a similar response type
   * without payload, and a separate payload.
   *
   * @note Enabled only for the use cases when the input response type has no
   * payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param other The `ApiResponse` instance.
   * @param payload The `Payload` value.
   */
  template <typename P = Payload, typename = typename std::enable_if<
                                      !std::is_same<P, void>::value>::type>
  ApiResponse(const ApiResponse<Result, Error, void>& other, P payload)
      : ResponseExtension<P>(std::move(payload)),
        result_(other.GetResult()),
        error_(other.GetError()),
        success_(other.IsSuccessful()) {}

  /**
   * @brief Creates the `ApiResponse` instance from a similar request with a
   * payload.
   *
   * @note Enabled only for the use cases when the input with a payload is
   * sliced to the response without payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param other The `ApiResponse` instance.
   */
  template <
      typename U, typename P = Payload, typename R = Result,
      typename = typename std::enable_if<std::is_same<P, void>::value>::type,
      typename =
          typename std::enable_if<std::is_copy_constructible<R>::value>::type>
  ApiResponse(const ApiResponse<R, Error, U>& other)
      : ResponseExtension<P>(),
        result_(other.GetResult()),
        error_(other.GetError()),
        success_(other.IsSuccessful()) {}

  /**
   * @brief Creates the `ApiResponse` instance from a similar request with a
   * payload.
   *
   * @note Enabled only for the use cases when the input with a payload is
   * sliced to the response without payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param other The `ApiResponse` instance.
   */
  template <
      typename U, typename P = Payload, typename R = Result,
      typename = typename std::enable_if<std::is_same<P, void>::value>::type,
      typename =
          typename std::enable_if<std::is_move_constructible<R>::value>::type>
  ApiResponse(ApiResponse<R, Error, U>&& other)
      : ResponseExtension<P>(),
        result_(other.MoveResult()),
        error_(other.GetError()),
        success_(other.IsSuccessful()) {}

  /**
   * @brief Creates the `ApiResponse` instance.
   *
   * Used for moving the successfully executed request.
   *
   * @param result The `ResultType` instance.
   */
  ApiResponse(ResultType result)  // NOLINT
      : ResponseExtension<Payload>(),
        result_(std::move(result)),
        success_(true) {}

  /**
   * @brief Creates the `ApiResponse` instance with payload.
   *
   * Used for moving the successfully executed request.
   *
   * @param result The `ResultType` instance.
   * @param payload The payload.
   */
  template <typename P = Payload, typename = typename std::enable_if<
                                      !std::is_same<P, void>::value>::type>
  ApiResponse(ResultType result, P payload)
      : ResponseExtension<Payload>(std::move(payload)),
        result_(std::move(result)),
        success_(true) {}

  /**
   * @brief Creates the `ApiResponse` instance if the request is not successful.
   *
   * @param error The `ErrorType` instance.
   */
  ApiResponse(const ErrorType& error)  // NOLINT
      : error_(error), success_(false) {}

  /**
   * @brief Creates the `ApiResponse` instance if the request is not successful.
   *
   * @param error The `ErrorType` instance.
   */
  template <typename P = Payload, typename = typename std::enable_if<
                                      !std::is_same<P, void>::value>::type>
  ApiResponse(const ErrorType& error, P payload)
      : ResponseExtension<Payload>(std::move(payload)),
        error_(error),
        success_(false) {}

  /**
   * @brief Checks the status of the request attempt.
   *
   * @return True if the request is successfully completed; false otherwise.
   */
  inline bool IsSuccessful() const { return success_; }

  /**
   * @brief Gets the result of the successfully executed request.
   *
   * @return The result instance.
   */
  inline const ResultType& GetResult() const { return result_; }

  /**
   * @brief Moves the result of the successfully executed request.
   *
   * @return The forwarding reference to the result instance.
   */
  inline ResultType&& MoveResult() { return std::move(result_); }

  /**
   * @brief Gets the error of the unsuccessful request attempt.
   *
   * @return The result instance.
   */
  inline const ErrorType& GetError() const { return error_; }

  /**
   * @brief Operator to check the status of the request attempt.
   *
   * @return True if the request is successfully completed; false otherwise.
   */
  inline explicit operator bool() const { return IsSuccessful(); }

 private:
  ResultType result_{};
  ErrorType error_{};
  bool success_{false};
};

/**
 * @brief A wrapper template that you can use to cancel a request or wait for
 * it to finalize.
 *
 * @tparam T The result type.
 */
template <typename T>
class CancellableFuture {
 public:
  /**
   * @brief The sharable promise type.
   */
  using PromisePtr = std::shared_ptr<std::promise<T>>;

  /**
   * @brief Creates the `CancellableFuture` instance with `CancellationToken`
   * and `std::promise`.
   *
   * @param cancel_token The `CancellationToken` instance.
   * @param promise The `PromisePtr` instance.
   */
  CancellableFuture(const CancellationToken& cancel_token, PromisePtr promise)
      : cancel_token_(cancel_token), promise_(std::move(promise)) {}

  /**
   * @brief Gets the `CancellationToken` reference used to cancel the ongoing
   * operation.
   *
   * @return The constant reference to the `CancellationToken` instance.
   */
  inline const CancellationToken& GetCancellationToken() const {
    return cancel_token_;
  }

  /**
   * @brief Gets the future associated with the `std::promise` that you
   * specified during initialization.
   *
   * @return The future with the result of the asynchronous request.
   */
  inline std::future<T> GetFuture() const { return promise_->get_future(); }

 private:
  CancellationToken cancel_token_;
  PromisePtr promise_;
};

}  // namespace client
}  // namespace olp
