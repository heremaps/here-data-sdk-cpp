/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <string>

#include <boost/optional.hpp>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/dataservice/read/ConsumerProperties.h>
#include <olp/dataservice/read/Types.h>
#include <olp/dataservice/read/model/Messages.h>
#include <olp/dataservice/read/model/StreamOffsets.h>
#include "generated/model/SubscribeResponse.h"

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;
}  // namespace client

namespace dataservice {
namespace read {

/**
 * @brief Provides the ability to subscribe to a stream
 * layer and consume data from the subscribed layer.
 */
class StreamApi {
 public:
  /// The subscribe response type.
  using SubscribeApiResponse = Response<model::SubscribeResponse>;

  /// The subscribe response type.
  using ConsumeDataApiResponse = Response<model::Messages>;

  /// The `CommitOffsets` response type. Returns the status of the HTTP request
  /// on success.
  using CommitOffsetsApiResponse = Response<int>;

  /// The `SeekToOffset` response type. Returns the status of the HTTP request
  /// on success.
  using SeekToOffsetApiResponse = Response<int>;

  /// The unsubscribe response type. Returns the status of the HTTP request on
  /// success.
  using UnsubscribeApiResponse = Response<int>;

  /**
   * @brief Enables message consumption from a specific stream layer.
   *
   * Uses the base path returned from the API Lookup Service.
   *
   * @note For **mode = parallel**, one unit of parallelism currently equals to
   * 1 MBps inbound or 2 MBps outbound (whichever is greater) and is rounded up
   * to the nearest integer.
   *
   * The number of subscriptions within the same group cannot
   * exceed the parallelism allowed.
   * For more details, see [Get Data from a
   * Stream
   * Layer](https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/getting-data-stream.html).
   *
   * @param client The `OlpClient` instance used to make REST requests.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id The subscription ID. Include this parameter if you
   * want to look up the `nodeBaseURL` for the given subscription ID.
   * @param mode The subscription mode that should be used for this
   * subscription.
   * @param consumer_id The ID that identifies this consumer. It must be
   * unique within the consumer group. If you do not provide the consumer ID,
   * the system generates it.
   * @param subscription_properties One or more consumer properties that should
   * be used for this subscription.
   * @param context The `CancellationContext` instance that can be used to
   * cancel the pending request.
   * @param[out] x_correlation_id The trace ID that associates this request
   * with the next request in your process. The correlation ID is the value of
   * the `X-Correlation-Id`response header.
   *
   * @return The `client::ApiResponse` object.
   */
  static SubscribeApiResponse Subscribe(
      const client::OlpClient& client, const std::string& layer_id,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const boost::optional<std::string>& consumer_id,
      const boost::optional<ConsumerProperties>& subscription_properties,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Consumes data from a layer.
   *
   * Returns messages from a stream layer formatted similar to a [Partition
   * object](https://developer.here.com/olp/documentation/data-client-library/api_reference_scala/index.html#com.here.platform.data.client.javadsl.Partition).
   * If the data size is less than 1 MB, the `data` field is populated. If
   * the data size is greater than 1 MB, the data handle is returned pointing
   * to the object stored in the blob store. The base path to be used is the
   * value of 'nodeBaseURL' returned from the `/subscribe` POST request.
   *
   * @param client The `OlpClient` instance used to make REST requests.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id The subscription ID received in the response of the
   * `/subscribe` request (required if `mode=parallel`).
   * @param mode The subscription mode of this subscription ID (as provided in
   * the `/subscribe` POST API).
   * @param context The `CancellationContext` instance that can be used to
   * cancel the pending request.
   * @param[in,out] x_correlation_id The correlation ID (the value of
   * the 'X-Correlation-Id' response header) from the prior step in the process.
   * After the successful call, it is assigned to the correlation ID of the
   * latest response.
   * @see The [API
   * Reference](https://developer.here.com/olp/documentation/data-store/api-reference.html)
   * for information on the `stream` API.
   *
   * @return The `client::ApiResponse` object.
   */
  static ConsumeDataApiResponse ConsumeData(
      const client::OlpClient& client, const std::string& layer_id,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Commits offsets of the last read message.
   *
   * After reading data, you should commit the offset of the last read message
   * from each partition so that your application can resume reading new
   * messages from the correct partition if there is a disruption
   * to the subscription, such as an application crash. An offset can also be
   * useful if you delete a subscription, and then recreate a subscription for
   * the same layer because the new subscription can start reading data from
   * the offset. To read the already committed messages, use the `/seek`
   * endpoint, and then use `/partitions`. The base path to use is the value of
   * 'nodeBaseURL' returned from the `/subscribe` POST request.
   *
   * @param client The `OlpClient` instance used to make REST requests.
   * @param layer_id The ID of the stream layer.
   * @param commit_offsets The offsets that should be committed. They should be
   * the same as the offset of the message you wish to commit. Do not pass
   * offset + 1. The service adds one to the offset you specify.
   * @param subscription_id The subscription ID received in the response of the
   * `/subscribe` request (required if `mode=parallel`).
   * @param mode The subscription mode of this subscription ID (as provided in
   * the `/subscribe` POST API).
   * @param context The `CancellationContext` instance that can be used to
   * cancel the pending request.
   * @param[in,out] x_correlation_id The correlation ID (the value of
   * the 'X-Correlation-Id' response header) from the prior step in the process.
   * After the successful call, it is assigned to the correlation ID of the
   * latest response.
   * @see The [API
   * Reference](https://developer.here.com/olp/documentation/data-store/api-reference.html)
   * for information on the `stream` API.
   *
   * @return The `client::ApiResponse` object.
   */
  static CommitOffsetsApiResponse CommitOffsets(
      const client::OlpClient& client, const std::string& layer_id,
      const model::StreamOffsets& commit_offsets,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Seeks for a predefined offset.
   *
   * Enables you to start reading data from a specified offset. You can move
   * the message pointer to any offset in the layer (topic). Message consumption
   * starts from that offset. Once you seek for an offset, there is no
   * returning to the initial offset, unless the initial offset is saved. The
   * base path to use is the value of 'nodeBaseURL' returned from the
   * `/subscribe` POST request.
   *
   * @param client The `OlpClient` instance used to make REST requests.
   * @param layer_id The ID of the stream layer.
   * @param seek_offsets The list of the offsets and offset partitions.
   * @param subscription_id The subscription ID received in the response of the
   * `/subscribe` request (required if `mode=parallel`).
   * @param mode The subscription mode of this subscription ID (as provided in
   * the `/subscribe` POST API).
   * @param context The `CancellationContext` instance that can be used to
   * cancel the pending request.
   * @param[in,out] x_correlation_id The correlation ID (the value of
   * the 'X-Correlation-Id' response header) from the prior step in the process.
   * After the successful call, it is assigned to the correlation ID of the
   * latest response.
   * @see the [API
   * Reference](https://developer.here.com/olp/documentation/data-store/api-reference.html)
   * for information on the `stream` API.
   *
   * @return The `client::ApiResponse` object.
   */
  static SeekToOffsetApiResponse SeekToOffset(
      const client::OlpClient& client, const std::string& layer_id,
      const model::StreamOffsets& seek_offsets,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Deletes a subscription to a layer.
   *
   * This operation removes the subscription from the service. The base path to
   * use is the value of 'nodeBaseURL' returned from the `/subscribe` POST
   * request.
   *
   * @param client The `OlpClient` instance used to make REST requests.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id The subscription ID received in the response of
   * the `/subscribe` request (required if `mode=parallel`).
   * @param mode The subscription mode of this subscription ID (as provided in
   * the `/subscribe` POST API).
   * @param x_correlation_id The trace ID that associates this request with
   * the other requests in your process. The correlation ID is the value of
   * the `X-Correlation-Id` response header from the prior request in your
   * process. Once you use the correlation ID in the `/delete` request, do not
   * use it again since `/delete` marks the end of the process.
   * @param context The `CancellationContext` instance that can be used to
   * cancel the pending request.
   * @return The `client::ApiResponse` object.
   */
  static UnsubscribeApiResponse DeleteSubscription(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& subscription_id, const std::string& mode,
      const std::string& x_correlation_id,
      const client::CancellationContext& context);

 private:
  static Response<int> HandleOffsets(
      const client::OlpClient& client, const std::string& layer_id,
      const model::StreamOffsets& commit_offsets,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context, const std::string& endpoint,
      std::string& x_correlation_id);

  static bool HandleCorrelationId(const http::HeadersType& headers,
                                  std::string& x_correlation_id);
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
