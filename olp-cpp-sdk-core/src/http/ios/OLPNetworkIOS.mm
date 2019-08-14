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

#import "OLPNetworkIOS.h"

#import <Foundation/Foundation.h>

#include "olp/core/logging/Log.h"
#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkTypes.h"
#include "../NetworkUtils.h"

#import "OLPHttpClient+Internal.h"
#import "OLPHttpTask.h"
#import "OLPNetworkConstants.h"

namespace olp {
namespace http {

namespace {

constexpr auto kInvalidRequestId =
    static_cast<RequestId>(RequestIdConstants::RequestIdInvalid);

constexpr auto kLogTag = "OLPNetworkIOS";

NSMutableDictionary* ParseHeadersDictionaryFromRequest(
    const olp::http::NetworkRequest& request) {
  NSMutableDictionary* headers = [[NSMutableDictionary alloc] init];

  const auto& request_headers = request.GetHeaders();
  for (const auto& header : request_headers) {
    NSString* key = [NSString stringWithUTF8String:header.first.c_str()];
    NSString* value = [NSString stringWithUTF8String:header.second.c_str()];
    headers[key] = value;
  }
  return headers;
}

NSData* ParseBodyDataFromRequest(const olp::http::NetworkRequest& request) {
  const auto verb = request.GetVerb();
  if (verb == NetworkRequest::HttpVerb::GET ||
      verb == NetworkRequest::HttpVerb::HEAD) {
    return nil;
  }

  const auto body = request.GetBody();
  if (!body || body->empty()) {
    return nil;
  }
  return [NSData dataWithBytes:static_cast<const void*>(body->data())
                        length:body->size()];
}

NSString* ParseHttpMethodFromRequest(
    const olp::http::NetworkRequest& request) {
  switch (request.GetVerb()) {
    case NetworkRequest::HttpVerb::GET:
      return OLPHttpMethodGet;
    case NetworkRequest::HttpVerb::POST:
      return OLPHttpMethodPost;
    case NetworkRequest::HttpVerb::PUT:
      return OLPHttpMethodPut;
    case NetworkRequest::HttpVerb::DEL:
      return OLPHttpMethodDelete;
    case NetworkRequest::HttpVerb::PATCH:
      return OLPHttpMethodPatch;
    case NetworkRequest::HttpVerb::HEAD:
      return OLPHttpMethodHead;
    default:
      return nil;
  }
}

olp::http::ErrorCode ConvertNSURLErrorToNetworkErrorCode(
    NSInteger error_code) {
  switch (error_code) {
    case NSURLErrorUnsupportedURL:
    case NSURLErrorCannotFindHost:
      return ErrorCode::INVALID_URL_ERROR;
    case NSURLErrorNotConnectedToInternet:
      return ErrorCode::OFFLINE_ERROR;
    case NSURLErrorTimedOut:
      return ErrorCode::TIMEOUT_ERROR;
    default:
      if (error_code >= NSURLErrorClientCertificateRequired &&
          error_code <= NSURLErrorSecureConnectionFailed) {
        return ErrorCode::AUTHORIZATION_ERROR;
      } else {
        return ErrorCode::UNKNOWN_ERROR;
      }
      break;
  }
}
}  // namespace

#pragma mark - OLPNetworkIOS implementation

OLPNetworkIOS::OLPNetworkIOS(size_t max_requests_count)
    : max_requests_count_(max_requests_count) {
  @autoreleasepool {
    http_client_ = [[OLPHttpClient alloc] init];
  }
}

OLPNetworkIOS::~OLPNetworkIOS() { Cleanup(); }

olp::http::SendOutcome OLPNetworkIOS::Send(
    olp::http::NetworkRequest request,
    std::shared_ptr<std::ostream> payload,
    olp::http::Network::Callback callback,
    olp::http::Network::HeaderCallback header_callback,
    olp::http::Network::DataCallback data_callback) {
  @autoreleasepool {
    OLPHttpTask* task = nil;
    NSString* url = [NSString stringWithUTF8String:request.GetUrl().c_str()];
    if (url.length == 0) {
      EDGE_SDK_LOG_WARNING(kLogTag, "Invalid request URL");
      return SendOutcome(ErrorCode::INVALID_URL_ERROR);
    }

    // create new OLPHttpTask, if possible
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (http_client_.activeTasks.count >= max_requests_count_) {
        EDGE_SDK_LOG_WARNING_F(kLogTag,
                               "Can't send request - reached max requests "
                               "count:[%lu / %lu], URL:[%s]",
                               http_client_.activeTasks.count,
                               max_requests_count_, request.GetUrl().c_str());
        return SendOutcome(ErrorCode::NETWORK_OVERLOAD_ERROR);
      }

      olp::http::RequestId request_id = GenerateNextRequestId();
      const NetworkProxySettings& proxy_settings =
          request.GetSettings().GetProxySettings();
      task = [http_client_ createTaskWithProxy:proxy_settings andId:request_id];
    }
    if (!task) {
      EDGE_SDK_LOG_WARNING_F(kLogTag, "Can't create task for request URL=[%s]",
                             request.GetUrl().c_str());
      return SendOutcome(ErrorCode::UNKNOWN_ERROR);
    }

    // Setup task request data:
    const NetworkSettings& settings = request.GetSettings();

    task.url = url;
    task.connectionTimeout = settings.GetConnectionTimeout();
    task.payload = payload;
    task.callback = callback;
    task.callbackMutex = std::make_shared<std::mutex>();

    task.HTTPMethod = ParseHttpMethodFromRequest(request);
    task.body = ParseBodyDataFromRequest(request);
    task.headers = ParseHeadersDictionaryFromRequest(request);
    task.responseData = [[OLPHttpTaskResponseData alloc] init];

    __weak OLPHttpTask* weak_task = task;
    auto requestId = task.requestId;

    // setup handler for NSURLSessionDataTask::didReceiveResponse
    task.responseHandler = ^(NSHTTPURLResponse* response) {
      if (!weak_task) {
        EDGE_SDK_LOG_WARNING_F(
            kLogTag, "Response received after task=[%llu] was deleted",
            requestId);
        return;
      }
      __strong OLPHttpTask* strong_task = weak_task;
      strong_task.responseData.status = (int)response.statusCode;

      NSDictionary* response_headers = response.allHeaderFields;
      if (header_callback) {
        for (NSString* key in response_headers) {
          // Response header NSString interpret every byte like unichar
          // (unsigned short) and
          //    enlarge it by one zero byte.
          // f.e 0xc2 -> 0x00c2, 0xa9 -> 0x00a9, so trim them back.
          std::string skey;
          for (int i = 0; i < key.length; ++i) {
            unichar uc = [key characterAtIndex:i];
            if ((uc >> 8))  // uc > 0xff
              skey += (char)(uc >> 8);
            skey += (char)uc;
          }

          NSString* value = response_headers[key];
          std::string svalue;
          for (int i = 0; i < value.length; ++i) {
            unichar uc = [value characterAtIndex:i];
            if ((uc >> 8))  // uc > 0xff
              svalue += (char)(uc >> 8);
            svalue += (char)uc;
          }
          header_callback(skey, svalue);
        }
      }
    };

    // setup handler for NSURLSessionDataTask::didReceiveData callback
    task.dataHandler = ^(NSData* data) {
      if (!weak_task) {
        EDGE_SDK_LOG_WARNING_F(
            kLogTag, "Data received after task=[%llu] was deleted", requestId);
        return;
      }
      __strong OLPHttpTask* strong_task = weak_task;
      OLPHttpTaskResponseData* response_data = strong_task.responseData;
      if (response_data.rangeOut) {
        EDGE_SDK_LOG_TRACE(kLogTag, "Datacallback out of range");
        return;
      }

      const size_t len = data.length;
      if (data_callback) {
        data_callback(reinterpret_cast<uint8_t*>(const_cast<void*>(data.bytes)),
                      response_data.offset + response_data.count, len);
      }

      if (auto payload = strong_task.payload) {
        if (payload->tellp() != std::streampos(response_data.count)) {
          payload->seekp(response_data.count);
          if (payload->fail()) {
            EDGE_SDK_LOG_WARNING(
                kLogTag,
                "Reception stream doesn't support setting write point");
            payload->clear();
          }
        }

        const char* data_ptr = reinterpret_cast<const char*>(data.bytes);
        payload->write(data_ptr, len);
      }
      response_data.count += len;
    };

    // setup handler for NSURLSessionDataTask::didCompleteWithError callback
    task.completionHandler = ^(NSError* error) {
      if (!weak_task) {
        EDGE_SDK_LOG_WARNING_F(
            kLogTag, "Completion received after task=[%llu] was deleted",
            requestId);
        return;
      }

      __strong OLPHttpTask* strong_task = weak_task;
      olp::http::Network::Callback callback;
      {
        std::lock_guard<std::mutex> callback_lock(
            *strong_task.callbackMutex.get());
        if (strong_task.callback) {
          callback = strong_task.callback;
          strong_task.callback = nullptr;
        }
      }

      OLPHttpTaskResponseData* response_data = strong_task.responseData;

      // TODO: convert error code to string
      std::string error_str = "Success";
      int status = static_cast<int>(error.code);
      const bool cancelled = NSURLErrorCancelled == status;
      if (callback) {
        if (error && !cancelled) {
          error_str = status < 0
                          ? std::string(error.localizedDescription.UTF8String)
                          : "Failure";
          status =
              static_cast<int>(ConvertNSURLErrorToNetworkErrorCode(error.code));
        } else {
          status = cancelled ? static_cast<int>(ErrorCode::CANCELLED_ERROR)
                             : response_data.status;
          error_str = HttpErrorToString(status);
        }
        callback(olp::http::NetworkResponse()
                     .WithRequestId(strong_task.requestId)
                     .WithStatus(status)
                     .WithError(error_str)
                     .WithCancelled(cancelled));
      }

    };

    // Perform send request asycnrhonously in a NSURLSession's thread
    OLPHttpTaskStatus ret = [task run];
    if (OLPHttpTaskStatusOk != ret) {
      EDGE_SDK_LOG_WARNING_F(kLogTag, "Can't run task with id=[%llu]; url=[%s]",
                             task.requestId, request.GetUrl().c_str());
      return SendOutcome(kInvalidRequestId);
    }
    return SendOutcome(task.requestId);
  }  // autoreleasepool
}

void OLPNetworkIOS::Cleanup() {
  if (!http_client_) {
    return;
  }

  std::vector<std::pair<RequestId, Network::Callback> > completed_messages;

  // Adding lock since accessing http_client_.activeTasks from another
  // method in different thread might result in crash.
  // It happens because m_httpClient is being destroyed here, whereas
  // another thread might start waiting for locked activeTasks.
  // Each call to http_client_.activeTasks should be surrounded with
  // http_client_ lock guard to prevent race condition and possible crash.
  {
    std::lock_guard<std::mutex> lock(mutex_);
    NSArray* tasks = http_client_.activeTasks;
    for (OLPHttpTask* task in tasks) {
      std::lock_guard<std::mutex> callback_lock(*task.callbackMutex.get());
      if (task.callback) {
        completed_messages.emplace_back(task.requestId, task.callback);
        task.callback = nullptr;
      }
    }
    @autoreleasepool {
      [http_client_ cleanup];
    }

    http_client_ = nil;
  }

  for (auto& pair : completed_messages) {
    pair.second(NetworkResponse()
                    .WithRequestId(pair.first)
                    .WithStatus(static_cast<int>(ErrorCode::OFFLINE_ERROR))
                    .WithError("Offline: network is deinitialized"));
  }
}

void OLPNetworkIOS::Cancel(olp::http::RequestId identifier) {
  std::lock_guard<std::mutex> lock(mutex_);
  @autoreleasepool {
    [http_client_ cancelTaskWithId:identifier];
  }
}

olp::http::RequestId OLPNetworkIOS::GenerateNextRequestId() {
  auto request_id = request_id_counter_++;
  if (request_id_counter_ ==
      static_cast<RequestId>(RequestIdConstants::RequestIdMax)) {
    request_id_counter_ =
        static_cast<RequestId>(RequestIdConstants::RequestIdMin);
  }
  return request_id;
}

}  // http
}  // olp
