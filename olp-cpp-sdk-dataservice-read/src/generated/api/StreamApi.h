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
 * @brief The `stream` service provides the ability to subscribe to a stream
 * layer and consume data from subscribed layer.
 */
class StreamApi {
 public:
  /// Subscribe response type.
  using SubscribeApiResponse = Response<model::SubscribeResponse>;

  /// CommitOffsets response type. Returns status of the HTTP request on
  /// success.
  using CommitOffsetsApiResponse = Response<int>;

  /// SeekToOffset response type. Returns status of the HTTP request on success.
  using SeekToOffsetApiResponse = Response<int>;

  /// Unsubscribe response type. Returns status of the HTTP request on success.
  using UnsubscribeApiResponse = Response<int>;

  /**
   * @brief Enable message consumption from a specific stream layer.
   *
   * Use the base path returned from the API Lookup service.\nNote: For **mode =
   * parallel**, one unit of parallelism currently equals 1 MBps inbound or 2
   * MBps outbound, whichever is greater, rounded up to the nearest integer. The
   * number of subscriptions within the same group cannot exceed the parallelism
   * allowed. For more details see [Get Data from a Stream
   * Layer](https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/getting-data-stream.html).
   *
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id Include this parameter if you want to look up the
   * `nodeBaseURL` for a given subscriptionId.
   * @param mode Specifies whether to use serial or parallel subscription mode.
   * @param consumer_id The ID to use to identify this consumer. It must be
   * unique within the consumer group. If you do not provide one, the system
   * will generate one.
   * @param subscription_properties One or more Consumer properties to use for
   * this subscription.
   * @param context A CancellationContext, which can be used to cancel the
   * pending request.
   * @param[out] x_correlation_id A trace ID to use to associate this request
   * with the next request in your process. The correlation ID is the value of
   * the response header `X-Correlation-Id`.
   * @return The result of operation as a client::ApiResponse object.
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
   * @brief Commits offsets of the last message read.
   *
   * After reading data, you should commit the offset of the last message read
   * from each partition so that your application can resume reading new
   * messages from the correct partition in the event that there is a disruption
   * to the subscription, such as an application crash. An offset can also be
   * useful if you delete a subscription then recreate a subscription for the
   * same layer, because the new subscription can start reading data from the
   * offset. To read messages already committed, use the `/seek` endpoint, then
   * use `/partitions`. The base path to use is the value of 'nodeBaseURL'
   * returned from `/subscribe` POST request.
   *
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the stream layer.
   * @param commit_offsets The offsets to commit. It should be same as the
   * offset of the message you wish to commit. Do not pass offset + 1. The
   * service adds one to the offset you specify.
   * @param subscription_id The subscriptionId received in the response of the
   * `/subscribe` request (required if mode=parallel).
   * @param mode The subscription mode of this subscriptionId (as provided in
   * `/subscribe` POST API).
   * @param context A CancellationContext, which can be used to cancel the
   * pending request.
   * @param[in,out] x_correlation_id The correlation-id (value of Response
   * Header 'X-Correlation-Id') from prior step in the process. See the [API
   * Reference](https://developer.here.com/olp/documentation/data-store/api-reference.html)
   * for the `stream` API. After the call it will be assigned to the
   * correlation-id of the latest response in case of successful call.
   * @return The result of operation as a client::ApiResponse object.
   */
  static CommitOffsetsApiResponse CommitOffsets(
      const client::OlpClient& client, const std::string& layer_id,
      const model::StreamOffsets& commit_offsets,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Seek to predefined offset.
   *
   * Enables you to start reading data from a specified offset. You can move the
   * message pointer to any offset in the layer (topic). Message consumption
   * will start from that offset. Once you seek to an offset, there is no
   * returning to the initial offset, unless the initial offset is saved. The
   * base path to use is the value of 'nodeBaseURL' returned from `/subscribe`
   * POST request.
   *
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the stream layer.
   * @param seek_offsets List of offsets and offset partitions.
   * @param subscription_id The subscriptionId received in the response of the
   * /subscribe request (required if mode=parallel).
   * @param mode The subscription mode of this subscriptionId (as provided in
   * /subscribe POST API).
   * @param context A CancellationContext, which can be used to cancel the
   * pending request.
   * @param[in,out] x_correlation_id The correlation-id (value of Response
   * Header 'X-Correlation-Id') from prior step in the process. See the [API
   * Reference](https://developer.here.com/olp/documentation/data-store/api-reference.html)
   * for the `stream` API. After the call it will be assigned to the
   * correlation-id of the latest response in case of successful call.
   * @return The result of operation as a client::ApiResponse object.
   */
  static SeekToOffsetApiResponse SeekToOffset(
      const client::OlpClient& client, const std::string& layer_id,
      const model::StreamOffsets& seek_offsets,
      const boost::optional<std::string>& subscription_id,
      const boost::optional<std::string>& mode,
      const client::CancellationContext& context,
      std::string& x_correlation_id);

  /**
   * @brief Delete subscription to a layer.
   *
   * This operation removes the subscription from the service. The base path to
   * use is the value of 'nodeBaseURL' returned from `/subscribe` POST request.
   *
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id The subscriptionId received in the response of the
   * `/subscribe` request (required if mode=parallel).
   * @param mode The subscription mode of this subscriptionId (as provided in
   * `/subscribe` POST API).
   * @param x_correlation_id A trace ID to use to associate this request with
   * other requests in your process. The correlation ID is the value of the
   * response header `X-Correlation-Id` from the prior request in your process.
   * Once you use a correlation ID in a `/delete` request, do not use it again
   * since `/delete` marks the end of a process.
   * @param context A CancellationContext, which can be used to cancel the
   * pending request.
   * @return The result of operation as a client::ApiResponse object.
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
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
