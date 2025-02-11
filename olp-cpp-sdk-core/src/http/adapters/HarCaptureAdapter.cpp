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

#include <olp/core/http/adapters/HarCaptureAdapter.h>

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <deque>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "olp/core/logging/Log.h"

namespace olp {
namespace http {
namespace {

const char* VerbToString(const NetworkRequest::HttpVerb verb) {
  switch (verb) {
    case NetworkRequest::HttpVerb::GET:
      return "GET";
    case NetworkRequest::HttpVerb::POST:
      return "POST";
    case NetworkRequest::HttpVerb::HEAD:
      return "HEAD";
    case NetworkRequest::HttpVerb::PUT:
      return "PUT";
    case NetworkRequest::HttpVerb::DEL:
      return "DEL";
    case NetworkRequest::HttpVerb::PATCH:
      return "PATCH";
    case NetworkRequest::HttpVerb::OPTIONS:
      return "OPTIONS";
    default:
      return "UNKNOWN";
  }
}

std::string FormatTime(const std::chrono::system_clock::time_point timestamp) {
  std::stringstream ss;
  const std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      timestamp.time_since_epoch()) %
                  1000;
  ss << std::put_time(std::localtime(&time), "%FT%T") << '.'
     << std::setfill('0') << std::setw(3) << ms.count() << "Z";
  return ss.str();
}

class JsonFileSerializer {
 public:
  explicit JsonFileSerializer(std::ofstream& file)
      : out_stream_(file), writer_(out_stream_) {}

  void Object(const std::function<void()>& body) {
    writer_.StartObject();
    body();
    writer_.EndObject();
  }

  void Object(const char* key, const std::function<void()>& body) {
    writer_.Key(key);
    writer_.StartObject();
    body();
    writer_.EndObject();
  }

  void Array(const char* key, const std::function<void()>& body) {
    writer_.Key(key);
    writer_.StartArray();
    body();
    writer_.EndArray();
  }

  void String(const char* key, const std::string& value) {
    writer_.Key(key);
    writer_.String(value.c_str(), value.size());
  }

  void Int(const char* key, const int value) {
    writer_.Key(key);
    writer_.Int(value);
  }

  void Double(const char* key, const double value) {
    writer_.Key(key);
    writer_.Double(value);
  }

  void EmptyArray(const char* key) {
    writer_.Key(key);
    writer_.StartArray();
    writer_.EndArray();
  }

 private:
  rapidjson::OStreamWrapper out_stream_;
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer_{};
};

}  // namespace

class HarCaptureAdapter::HarCaptureAdapterImpl final : public Network {
 public:
  HarCaptureAdapterImpl(std::shared_ptr<Network> network,
                        std::string har_out_path)
      : network_{std::move(network)}, har_out_path_{std::move(har_out_path)} {}

  ~HarCaptureAdapterImpl() override { SaveSessionToFile(); }

  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback,
                   DataCallback data_callback) override {
    const auto session_request_id = RecordRequest(request);

    const auto response_headers = std::make_shared<Headers>();

    auto header_callback_proxy = [=](std::string key, std::string value) {
      header_callback(key, value);
      response_headers->emplace_back(std::move(key), std::move(value));
    };

    auto callback_proxy = [=](NetworkResponse response) {
      RecordResponse(session_request_id, response, *response_headers);
      callback(std::move(response));
    };

    return network_->Send(
        std::move(request), std::move(payload), std::move(callback_proxy),
        std::move(header_callback_proxy), std::move(data_callback));
  }

  void Cancel(const RequestId id) override { network_->Cancel(id); }

 private:
  struct RequestEntry {
    size_t url{};
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    uint8_t method{};
    uint8_t status_code{};
    uint16_t request_headers_offset{};
    uint16_t request_headers_count{};
    uint16_t response_headers_offset{};
    uint16_t response_headers_count{};
    uint32_t transfer_size{};
  };

  RequestId RecordRequest(const NetworkRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    constexpr std::hash<std::string> hasher{};

    const auto size = requests_.size();

    RequestEntry request_entry;

    const auto& url = request.GetUrl();
    request_entry.url = hasher(url);
    cache_[request_entry.url] = request.GetUrl();

    request_entry.method = static_cast<uint8_t>(request.GetVerb());
    request_entry.start_time = std::chrono::system_clock::now();

    const auto& request_headers = request.GetHeaders();
    request_entry.request_headers_offset = headers_.size();
    request_entry.request_headers_count = request_headers.size();

    for (const auto& header : request_headers) {
      cache_[hasher(header.first)] = header.first;
      cache_[hasher(header.second)] = header.second;
      headers_.emplace_back(hasher(header.first), hasher(header.second));
    }

    requests_.emplace_back(request_entry);

    return size;
  }

  void RecordResponse(const RequestId request_id,
                      const NetworkResponse& response,
                      const Headers& response_headers) {
    std::lock_guard<std::mutex> lock(mutex_);
    constexpr std::hash<std::string> hasher{};

    RequestEntry& request_entry = requests_[request_id];
    request_entry.status_code = response.GetStatus();
    request_entry.end_time = std::chrono::system_clock::now();
    request_entry.transfer_size =
        response.GetBytesUploaded() + response.GetBytesDownloaded();

    request_entry.response_headers_offset = headers_.size();
    request_entry.response_headers_count = response_headers.size();

    for (const auto& header : response_headers) {
      cache_[hasher(header.first)] = header.first;
      cache_[hasher(header.second)] = header.second;
      headers_.emplace_back(hasher(header.first), hasher(header.second));
    }

    const auto& diagnostics = response.GetDiagnostics();
    if (diagnostics) {
      if (diagnostics_.size() <= request_id) {
        diagnostics_.resize(request_id + 1);
      }

      diagnostics_[request_id] = *diagnostics;
    }
  }

  void SaveSessionToFile() const {
    std::ofstream file(har_out_path_);
    if (!file.is_open()) {
      OLP_SDK_LOG_ERROR("HarCaptureAdapter::SaveSession",
                        "Failed to save session.");
      return;
    }

    JsonFileSerializer serializer{file};

    serializer.Object([&] {
      serializer.Object("log", [&] {
        serializer.String("version", "1.2");

        serializer.Object("creator", [&] {
          serializer.String("name", "DataSDK");
          serializer.String("version", OLP_SDK_VERSION_STRING);
        });

        serializer.Array("entries", [&] {
          for (auto request_index = 0u; request_index < requests_.size();
               ++request_index) {
            const auto& request = requests_[request_index];
            const auto diagnostics = diagnostics_.size() > request_index
                                         ? diagnostics_[request_index]
                                         : Diagnostics{};

            // return duration in milliseconds as float
            auto duration = [&](const Diagnostics::Timings timing,
                                const double default_value = -1.0) {
              return diagnostics.available_timings[timing]
                         ? diagnostics.timings[timing].count() / 1000.0
                         : default_value;
            };

            const double total_time = duration(
                Diagnostics::Total,
                static_cast<double>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        request.end_time - request.start_time)
                        .count()) /
                    1000.0);

            auto output_headers = [&](const uint16_t headers_offset,
                                      const uint16_t headers_count) {
              serializer.Array("headers", [&] {
                for (auto i = 0u; i < headers_count; ++i) {
                  serializer.Object([&] {
                    auto header = headers_[headers_offset + i];
                    serializer.String("name", cache_.at(header.first));
                    serializer.String("value", cache_.at(header.second));
                  });
                }
              });
            };

            serializer.Object([&] {
              serializer.String("startedDateTime",
                                FormatTime(request.start_time));
              serializer.Double("time", total_time);

              serializer.Object("request", [&] {
                serializer.String(
                    "method",
                    VerbToString(
                        static_cast<NetworkRequest::HttpVerb>(request.method)));
                serializer.String("url", cache_.at(request.url));
                serializer.String("httpVersion", "UNSPECIFIED");
                serializer.EmptyArray("cookies");
                output_headers(request.request_headers_offset,
                               request.request_headers_count);
                serializer.EmptyArray("queryString");
                serializer.Int("headersSize", -1);
                serializer.Int("bodySize", -1);
              });

              // response
              serializer.Object("response", [&] {
                serializer.Int("status", request.status_code);
                serializer.String("statusText", "");
                serializer.String("httpVersion", "UNSPECIFIED");
                serializer.EmptyArray("cookies");
                output_headers(request.response_headers_offset,
                               request.response_headers_count);
                serializer.Object("content", [&] {
                  serializer.Int("size", 0);
                  serializer.String("mimeType", "");
                });
                serializer.String("redirectURL", "");
                serializer.Int("headersSize", -1);
                serializer.Int("bodySize", -1);
                serializer.Int("_transferSize",
                               static_cast<int>(request.transfer_size));
              });

              // timings
              serializer.Object("timings", [&] {
                using Timings = Diagnostics::Timings;
                serializer.Double("blocked", duration(Timings::Queue));
                serializer.Double("dns", duration(Timings::NameLookup));
                serializer.Double("connect", duration(Timings::Connect));
                serializer.Double("ssl", duration(Timings::SSL_Handshake));
                serializer.Double("send", duration(Timings::Send, 0.0));
                serializer.Double("wait", duration(Timings::Wait, 0.0));
                serializer.Double("receive",
                                  duration(Timings::Receive, total_time));
              });
            });
          }
        });
      });
    });

    file.close();

    OLP_SDK_LOG_INFO("HarCaptureAdapter::SaveSession",
                     "Session is saved to: " << har_out_path_);
  }

  std::mutex mutex_;
  std::unordered_map<size_t, std::string> cache_{};
  std::deque<std::pair<size_t, size_t> > headers_{};
  std::deque<RequestEntry> requests_{};
  std::deque<Diagnostics> diagnostics_{};

  std::shared_ptr<Network> network_;
  std::string har_out_path_;
};

HarCaptureAdapter::HarCaptureAdapter(std::shared_ptr<Network> network,
                                     std::string har_out_path)
    : impl_(std::make_shared<HarCaptureAdapterImpl>(std::move(network),
                                                    std::move(har_out_path))) {}

HarCaptureAdapter::~HarCaptureAdapter() = default;

SendOutcome HarCaptureAdapter::Send(NetworkRequest request, Payload payload,
                                    Callback callback,
                                    HeaderCallback header_callback,
                                    DataCallback data_callback) {
  return impl_->Send(std::move(request), std::move(payload),
                     std::move(callback), std::move(header_callback),
                     std::move(data_callback));
}

void HarCaptureAdapter::Cancel(const RequestId id) { impl_->Cancel(id); }

}  // namespace http
}  // namespace olp
