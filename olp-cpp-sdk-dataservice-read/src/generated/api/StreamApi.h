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
#include "generated/model/SubscribeResponse.h"

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;
}  // namespace client

namespace dataservice {
namespace read {

/// Response template type
template <typename ResultType>
using Response = client::ApiResponse<ResultType, client::ApiError>;

/**
 * @brief The `stream` service provides the ability to consume data from a
 * stream layer. With this service you can subscribe to a stream layer and
 * consume messages.
 */
class StreamApi {
 public:
  /// Subscribe response type.
  using SubscribeApiResponse = Response<model::SubscribeResponse>;

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
   * For more details see [Get Data from a Stream
   * Layer](https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/getting-data-stream.html).
   * @param consumer_id The ID to use to identify this consumer. It must be
   * unique within the consumer group. If you do not provide one, the system
   * will generate one.
   * @param subscription_properties One or more Kafka Consumer properties to use
   * for this subscription. For more information, see [Get Data from a Stream
   * Layer](https://developer.here.com/olp/documentation/data-store/data_dev_guide/rest/getting-data-stream.html).
   * @param context A CancellationContext, which can be used to cancel the
   * pending request.
   * @param[out] x_correlation_id A trace ID to use to associate this request
   * with other requests in your process. The correlation ID is the value of the
   * `/subscribe` response header `X-Correlation-Id`.
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
   * @brief Delete subscription to a layer.
   *
   * This operation removes the subscription from the service. The base path to
   * use is the value of 'nodeBaseURL' returned from /subscribe POST request.
   *
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id The ID of the stream layer.
   * @param subscription_id The subscriptionId received in the response of the
   * /subscribe request (required if mode=parallel).
   * @param mode The subscription mode of this subscriptionId (as provided in
   * /subscribe POST API).
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
      const boost::optional<std::string>& x_correlation_id,
      const client::CancellationContext& context);
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
