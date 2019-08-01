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

#import <Foundation/Foundation.h>

#import "../NetworkUtils.h"
#import "HttpClient.h"
#import "HttpTask.h"
#import "NetworkProtocolIosImpl.h"
#import "olp/core/logging/Log.h"
#import "olp/core/network/Network.h"
#import "olp/core/network/NetworkResponse.h"

#import <memory>
#import <sstream>
#import <string>
#import <vector>

@interface HttpTaskContext : NSObject

@property(nonatomic) std::shared_ptr<std::ostream> payload;
@property(nonatomic) olp::network::Network::Callback callback;
@property(nonatomic) std::shared_ptr<std::mutex> callbackMutex;
@property(nonatomic) std::string etag;
@property(nonatomic) std::string contentType;
@property(nonatomic) int id;
@property(nonatomic) int status;
@property(nonatomic) int maxAge;
@property(nonatomic) time_t expires;
@property(nonatomic) std::uint64_t count;
@property(nonatomic) std::uint64_t offset;
@property(nonatomic) bool resume;
@property(nonatomic) bool rangeOut;
@property(nonatomic) bool ignoreOffset;
@property(nonatomic) std::string date;

@end

@implementation HttpTaskContext

- (instancetype)init {
  self = [super init];
  if (self) {
    self.payload = nullptr;
    self.callback = nullptr;
    self.callbackMutex = std::make_shared<std::mutex>();
    self.etag = "";
    self.contentType = "";
    self.id = 0;
    self.status = 0;
    self.maxAge = -1;
    self.expires = -1;
    self.count = 0;
    self.offset = 0;
    self.resume = false;
    self.rangeOut = false;
    self.ignoreOffset = false;
    self.date = "";
  }
  return self;
}
@end

namespace olp {
namespace network {
#define LOGTAG "NETWORKIOS"

namespace {
constexpr int MaxRequestCount = 32;
static NSDate* lastHttpTimeStamp = nil;
static NSString* const NETWORKHTTPTIMESTAMPUPDATE = @"NETWORKHttpTimestampUpdate";

void sendHttpTimestampUpdateNotification(NSString* timeStamp) {
  @autoreleasepool {
    NSDateFormatter* dateFormatter = [NSDateFormatter new];
    dateFormatter.dateFormat = @"EEE, dd MMM yyyy HH:mm:ss z";
    NSDate* date = [dateFormatter dateFromString:timeStamp];
    // Send notification when time stamp update for more than 60 seonds
    if (date != nil &&
        ([date timeIntervalSince1970] - [lastHttpTimeStamp timeIntervalSince1970]) > 60) {
      lastHttpTimeStamp = [[NSDate alloc] initWithTimeInterval:0 sinceDate:date];
      dispatch_async(dispatch_get_main_queue(), ^(void) {
        [[NSNotificationCenter defaultCenter] postNotificationName:NETWORKHTTPTIMESTAMPUPDATE
                                                            object:lastHttpTimeStamp];
      });
    }
  }
}
}

NetworkProtocolIosImpl::NetworkProtocolIosImpl() : m_httpClient(nil) {}

NetworkProtocolIosImpl::~NetworkProtocolIosImpl() { Deinitialize(); }

bool NetworkProtocolIosImpl::Initialize() {
  @autoreleasepool {
    m_httpClient = [[HttpClient alloc] init];
  }

  return true;
}

void NetworkProtocolIosImpl::Deinitialize() {
  if (!m_httpClient) {
    return;
  }

  std::vector<std::pair<Network::RequestId, Network::Callback> > completedMessages;

  // Adding lock since accessing m_httpClient.activeTasks from another method in different thread
  // might result in crash. It happens because m_httpClient is being destroyed here, whereas
  // another thread might start waiting for locked activeTasks.
  // Each call to m_httpClient.activeTasks should be surrounded with m_httpClient lock guard
  // to prevent race condition and possible crash.
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    NSArray* tasks = m_httpClient.activeTasks;
    for (HttpTask* task in tasks) {
      std::lock_guard<std::mutex> callbackLock(*task.context.callbackMutex.get());
      if (task.context.callback) {
        completedMessages.emplace_back(task.context.id, task.context.callback);
        task.context.callback = nullptr;
      }
    }
    @autoreleasepool {
      [m_httpClient cleanup];
    }

    m_httpClient = nil;
  }

  for (auto& pair : completedMessages) {
    pair.second(NetworkResponse(pair.first, Network::Offline, "Offline"));
  }
}

bool NetworkProtocolIosImpl::Initialized() const { return m_httpClient != nil; }

bool NetworkProtocolIosImpl::Ready() {
  bool ready;
  std::lock_guard<std::mutex> lock(m_mutex);
  @autoreleasepool {
    ready = m_httpClient.activeTasks.count < MaxRequestCount;
  }
  return ready && m_httpClient != nil;
}

size_t NetworkProtocolIosImpl::AmountPending() {
  size_t count;
  std::lock_guard<std::mutex> lock(m_mutex);
  @autoreleasepool {
    count = m_httpClient.activeTasks.count;
  }
  return count;
}

NetworkProtocol::ErrorCode NetworkProtocolIosImpl::Send(
    const NetworkRequest& request, int identifier, const std::shared_ptr<std::ostream>& payload,
    std::shared_ptr<NetworkConfig> config, Network::HeaderCallback headerCallback,
    Network::DataCallback dataCallback, Network::Callback callback) {
  @autoreleasepool {
    HttpTask* task = nil;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (m_httpClient.activeTasks.count >= MaxRequestCount) {
        return NetworkProtocol::ErrorNotReady;
      }

      // Create url session if proxy specified at network layer
      if (config->Proxy().IsValid()) {
        NSURLSession* session = [m_httpClient urlSessionWithProxy:&(config->Proxy())headers:nil];
        task = [m_httpClient createTaskWithId:identifier session:session];
      } else {  // use default session
        task = [m_httpClient createTaskWithId:identifier];
      }
    }
    if (!task) {
      return NetworkProtocol::ErrorNotReady;
    }

    // Create task context
    HttpTaskContext* taskContext = [HttpTaskContext new];
    if (!taskContext) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Network Out of memory");
      return NetworkProtocol::ErrorNotReady;
    }

    if (!config->GetNetworkInterface().empty()) {
      return NetworkProtocol::ErrorNetworkInterfaceOptionNotImplemented;
    }

    if (!config->GetCaCert().empty()) {
      return NetworkProtocol::ErrorCaCertOptionNotImplemented;
    }

    task.transferTimeout = config->TransferTimeout();
    task.connectionTimeout = config->ConnectTimeout();
    task.context = taskContext;
    taskContext.id = identifier;
    taskContext.payload = payload;
    taskContext.callback = callback;
    taskContext.ignoreOffset = request.IgnoreOffset();

    // Convert the URL to NSString
    NSString* url = [NSString stringWithUTF8String:request.Url().c_str()];
    if (url.length == 0) {
      return NetworkProtocol::ErrorIO;
    }
    [task appendToUrl:url];

    NSMutableDictionary* headers = [NSMutableDictionary new];

    if (request.ExtraHeaders().size()) {
      const std::vector<std::pair<std::string, std::string> >& extraHeaders =
          request.ExtraHeaders();
      for (auto header : extraHeaders) {
        std::string headerKey = header.first;
        if (taskContext.resume) {
          // Ignore Range and If-Range headers
          if (NetworkUtils::CaseInsensitiveCompare(headerKey, "Range") ||
              NetworkUtils::CaseInsensitiveCompare(headerKey, "If-Range"))
            continue;
        }

        NSString* key = [NSString stringWithUTF8String:headerKey.c_str()];
        NSString* value = [NSString stringWithUTF8String:header.second.c_str()];
        headers[key] = value;
      }
    }

    // Get Post data if any
    NetworkRequest::HttpVerb verb = request.Verb();
    if (verb == NetworkRequest::HttpVerb::GET || verb == NetworkRequest::HttpVerb::HEAD) {
      // GET or HEAD
      // Default to GET
      task.httpMethod =
          verb == NetworkRequest::HttpVerb::HEAD ? kHTTPTaskHttpMethodHead : kHTTPTaskHttpMethodGet;
      task.body = nil;
      if (request.ModifiedSince() > 0) {
        NSDate* date = [NSDate dateWithTimeIntervalSince1970:request.ModifiedSince()];
        NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
        [formatter setDateFormat:@"ddd, dd MMM yyyy hh:mm:ss"];
        // Optionally for time zone conversions
        [formatter setTimeZone:[NSTimeZone timeZoneWithName:@"GMT"]];
        NSString* stringFromDate = [formatter stringFromDate:date];
        headers[@"If-Modified-Since"] = [stringFromDate stringByAppendingString:@" GMT"];
      }
    } else {
      if (verb == NetworkRequest::HttpVerb::POST) {
        task.httpMethod = kHTTPTaskHttpMethodPost;
      } else if (verb == NetworkRequest::HttpVerb::PUT) {
        task.httpMethod = kHTTPTaskHttpMethodPut;
      } else if (verb == NetworkRequest::HttpVerb::DEL) {
        task.httpMethod = kHTTPTaskHttpMethodDelete;
      } else if (verb == NetworkRequest::HttpVerb::PATCH) {
        task.httpMethod = kHTTPTaskHttpMethodPatch;
      }

      auto content = request.Content();
      if (content && !content->empty()) {
        task.body = [NSData dataWithBytes:static_cast<const void*>(&content->front())
                                   length:content->size()];
      } else {
        task.body = [NSData dataWithBytes:nil length:0];
      }
    }

    task.headers = headers;

    // Response headers
    task.responseHandler = ^(NSHTTPURLResponse* response) {
      processResponseHeaders(identifier, response, headerCallback);
    };

    __weak HttpTask* weakTask = task;
    task.dataHandler = ^(NSData* data) {
      if (!weakTask) {
        EDGE_SDK_LOG_WARNING(LOGTAG, "Data received after task deleted");
        return;
      }

      HttpTask* strongTask = weakTask;
      HttpTaskContext* taskContext = strongTask.context;
      if (taskContext.rangeOut) {
        EDGE_SDK_LOG_TRACE(LOGTAG, "Datacallback out of range");
        return;
      }
      size_t len = data.length;
      EDGE_SDK_LOG_TRACE(LOGTAG, "Received " << len << " bytes");

      if (dataCallback) {
        dataCallback(taskContext.offset + taskContext.count,
                     reinterpret_cast<uint8_t*>(const_cast<void*>(data.bytes)), len);
      }

      if (payload) {
        if (!taskContext.ignoreOffset) {
          if (payload->tellp() != std::streampos(taskContext.count)) {
            payload->seekp(taskContext.count);
            if (payload->fail()) {
              EDGE_SDK_LOG_WARNING(LOGTAG, "Reception stream doesn't support setting write point");
              payload->clear();
            }
          }
        }

        const char* dataPtr = reinterpret_cast<const char*>(data.bytes);
        payload->write(dataPtr, len);
      }
      taskContext.count += len;
    };

    task.completionHandler = ^(NSError* error) {
      if (!weakTask) {
        EDGE_SDK_LOG_WARNING(LOGTAG, "Data received after task deleted");
        return;
      }

      HttpTask* strongTask = weakTask;
      int status = (int)error.code;
      bool cancelled = NSURLErrorCancelled == status;
      std::string errorStr("Success");
      Network::Callback cb;
      HttpTaskContext* taskContext = strongTask.context;
      {
        std::lock_guard<std::mutex> callbackLock(*taskContext.callbackMutex.get());
        if (taskContext.callback) {
          cb = taskContext.callback;
          taskContext.callback = nullptr;
        }
      }
      if (cb) {
        if (error && !cancelled) {
          errorStr =
              status < 0
                  ? std::string(error.localizedDescription.UTF8String
                                    ?: "")                       // NSURL-related Error Codes
                  : NetworkProtocol::HttpErrorToString(status);  // Network Protocol Error Codes
          status = convertSystemError(status);
          cb(NetworkResponse(identifier, status, errorStr));
        } else {
          status = cancelled ? Network::Cancelled : taskContext.status;
          errorStr = NetworkProtocol::HttpErrorToString(status);
          auto response =
              NetworkResponse(identifier, cancelled, status, errorStr, taskContext.maxAge,
                              taskContext.expires, taskContext.etag, taskContext.contentType,
                              taskContext.count, taskContext.offset, taskContext.payload, {});
          cb(std::move(response));
        }
      }
    };

    // Do sending
    HttpTaskStatus ret = [task run];
    if (HttpTaskStatusNone != ret) {
      return NetworkProtocol::ErrorNotReady;
    }
  }  // autoreleasepool

  return NetworkProtocol::ErrorNone;
}

bool NetworkProtocolIosImpl::Cancel(int identifier) {
  std::lock_guard<std::mutex> lock(m_mutex);
  @autoreleasepool {
    [m_httpClient cancelTaskWithId:identifier];
  }
  return true;
}

void NetworkProtocolIosImpl::processResponseHeaders(int identifier, NSHTTPURLResponse* response,
                                                    Network::HeaderCallback headerCallback) {
  HttpTask* task;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    task = [m_httpClient taskWithId:identifier];
  }

  HttpTaskContext* taskContext = task.context;
  if (!task || !taskContext) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Unexpected response headers");
    return;
  }

  // staus code
  taskContext.status = (int)response.statusCode;

  NSDictionary* responseHeaders = response.allHeaderFields;
  if (headerCallback) {
    for (NSString* key in responseHeaders) {
      // Response header NSString interpret every byte like unichar (unsigned short) and
      //    enlarge it by one zero byte.
      // f.e 0xc2 -> 0x00c2, 0xa9 -> 0x00a9, so trim them back.
      std::string skey;
      for (int i = 0; i < key.length; ++i) {
        unichar uc = [key characterAtIndex:i];
        if ((uc >> 8))  // uc > 0xff
          skey += (char)(uc >> 8);
        skey += (char)uc;
      }

      NSString* value = responseHeaders[key];
      std::string svalue;
      for (int i = 0; i < value.length; ++i) {
        unichar uc = [value characterAtIndex:i];
        if ((uc >> 8))  // uc > 0xff
          svalue += (char)(uc >> 8);
        svalue += (char)uc;
      }
      headerCallback(skey, svalue);
    }
  }

  // Date
  NSString* dateString = responseHeaders[@"Date"];
  if (dateString.length) {
    std::string dateStr = std::string([dateString UTF8String]);
    taskContext.date = dateStr.substr(6);
    sendHttpTimestampUpdateNotification(dateString);
  }

  // max Age
  NSString* cacheControlString = responseHeaders[@"Cache-Control"];
  NSString* maxAgeString = nil;
  taskContext.maxAge = -1;
  if (cacheControlString.length) {
    NSRange range = [cacheControlString rangeOfString:@"max-age="];
    if (range.location != NSNotFound) {
      NSString* subString = [[cacheControlString substringFromIndex:NSMaxRange(range)]
          stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
      NSRange commaRange = [subString rangeOfString:@","];
      if (commaRange.location != NSNotFound) {
        NSRange finalRange = NSMakeRange(0, commaRange.location);
        maxAgeString = [subString substringWithRange:finalRange];
      } else {
        maxAgeString = subString;
      }
    }
  }
  if (maxAgeString.length) {
    taskContext.maxAge = [maxAgeString intValue];
  }

  // Expires
  NSString* expiresString = responseHeaders[@"Expires"];
  if (expiresString.length) {
    // Special case
    if ([expiresString isEqualToString:@"0"])
      taskContext.expires = 0;
    else if ([expiresString isEqualToString:@"-1"])
      taskContext.expires = -1;
    else {
      // create dateFormatter with UTC time format
      NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
      // format: Thu, 01 Dec 1994 16:00:00 GMT
      [dateFormatter setDateFormat:@"EEE, dd MMM yyyy HH:mm:ss zzz"];
      [dateFormatter setTimeZone:[NSTimeZone timeZoneWithAbbreviation:@"UTC"]];
      NSDate* date = [dateFormatter dateFromString:expiresString];
      if (date) {
        taskContext.expires = [date timeIntervalSince1970];
      }
    }
  }

  // etag
  NSString* eTagString = responseHeaders[@"ETag"];
  taskContext.etag = "";
  if (eTagString.length) {
    taskContext.etag = std::string([eTagString UTF8String]);
  }

  // content type
  NSString* contentTypeString = responseHeaders[@"Content-Type"];
  taskContext.contentType = "";
  if (contentTypeString.length) {
    taskContext.contentType = [contentTypeString UTF8String];
  }

  // count
  NSString* countString = responseHeaders[@"Content-Range"];
  if (countString.length == 0) return;

  std::string countStr = [countString UTF8String];
  if (NetworkUtils::CaseInsensitiveStartsWith(countStr, "bytes ", 0)) {
    if ((countStr[6] == '*') && (countStr[7] == '/')) {
      // We have requested range over end of the file
      taskContext.rangeOut = true;
      if (taskContext.resume) {
        taskContext.count = std::stoll(countStr.substr(8)) - taskContext.offset;
      }
    } else if ((countStr[6] >= '0') && (countStr[6] <= '9')) {
      if (taskContext.resume) {
        // We must keep old offset and just adjust count
        taskContext.count = std::stoll(countStr.substr(6)) - taskContext.offset;
      } else {
        taskContext.offset = std::stoll(countStr.substr(6));
      }
    } else {
      EDGE_SDK_LOG_WARNING(LOGTAG, "Invalid Content-Range header: " << countStr);
    }
  } else {
    EDGE_SDK_LOG_WARNING(LOGTAG, "Invalid Content-Range header: " << countString);
  }
}

int NetworkProtocolIosImpl::convertSystemError(int errorCode) {
  int ret = Network::IOError;

  if (NSURLErrorUnsupportedURL == errorCode || NSURLErrorCannotFindHost == errorCode) {
    ret = Network::InvalidURLError;
  } else if (NSURLErrorNotConnectedToInternet == errorCode) {
    ret = Network::Offline;
  } else if (NSURLErrorTimedOut == errorCode) {
    ret = Network::TimedOut;
  } else if (errorCode >= NSURLErrorClientCertificateRequired &&
             errorCode <= NSURLErrorSecureConnectionFailed) {
    // certificate errors range
    ret = Network::AuthorizationError;
  }

  return ret;
}
}  // network
}  // olp
