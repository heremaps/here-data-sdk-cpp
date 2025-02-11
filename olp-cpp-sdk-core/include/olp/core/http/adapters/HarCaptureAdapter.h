/*
 * Copyright (C) 2025 HERE Europe B.V.
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
#include <memory>

#include <olp/core/CoreApi.h>
#include <olp/core/http/Network.h>

namespace olp {
namespace http {

/**
 * @class HarCaptureAdapter
 * @brief A network adapter that captures HTTP requests and responses,
 * generating a HAR (HTTP Archive) file.
 *
 * HarCaptureAdapter implements the olp::http::Network interface, intercepting
 * network traffic for debugging, logging, and analysis. It records request
 * metadata, headers, and response details, allowing developers to
 * inspect network interactions in HAR format.
 *
 * @note Request timings are only available when Curl is used.
 * @note The HAR file is produced when the instance is destroyed.
 *
 * Features:
 * - Captures HTTP requests and responses.
 * - Logs request/response details, including headers, status codes and timings.
 * - Generates a HAR file for easy debugging and sharing.
 *
 * Example Usage:
 * @code
 * auto network = std::make_shared<HarCaptureAdapter>(network, "/tmp/out.har");
 * @endcode
 */
class CORE_API HarCaptureAdapter final : public Network {
 public:
  /**
   * @brief Constructs a HarCaptureAdapter instance.
   *
   * @param network The underlying network implementation to forward requests
   * to.
   * @param har_out_path The file path where the HAR (HTTP Archive) file will be
   * saved.
   */
  HarCaptureAdapter(std::shared_ptr<Network> network, std::string har_out_path);

  ~HarCaptureAdapter() override;

  /**
   * @copydoc Network::Send
   */
  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback,
                   DataCallback data_callback) override;

  /**
   * @copydoc Network::Cancel
   */
  void Cancel(RequestId id) override;

 private:
  class HarCaptureAdapterImpl;
  std::shared_ptr<HarCaptureAdapterImpl> impl_;
};

}  // namespace http
}  // namespace olp
