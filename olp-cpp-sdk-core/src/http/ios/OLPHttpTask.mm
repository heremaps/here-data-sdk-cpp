/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#import "OLPHttpTask+Internal.h"

#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/logging/Log.h>

#import "OLPHttpClient+Internal.h"
#import "OLPNetworkConstants.h"

namespace {
constexpr auto kLogTag = "OLPHttpTask";
}  // namespace

#pragma mark - OLPHttpTaskResponseData

@implementation OLPHttpTaskResponseData

- (instancetype)init {
  self = [super init];
  if (self) {
    _status = 0;
    _count = 0;
    _offset = 0;
    _rangeOut = false;
  }
  return self;
}
@end

#pragma mark - OLPHttpTask

@interface OLPHttpTask ()

@property(atomic) BOOL cancelled;

@end

@implementation OLPHttpTask {
  __weak NSURLSession* _urlSession;
  __weak OLPHttpClient* _httpClient;
  uint64_t _headersSizeReceived;
  uint64_t _headersSizeSent;
  uint64_t _contentLength;
}

- (instancetype)initWithHttpClient:(OLPHttpClient*)client
                     andURLSession:(NSURLSession*)session
                             andId:(olp::http::RequestId)identifier {
  self = [super init];
  if (self) {
    _httpClient = client;
    _HTTPMethod = OLPHttpMethodGet;
    _urlSession = session;
    _requestId = identifier;
    _headersSizeReceived = 0;
    _headersSizeSent = 0;
    _contentLength = 0;
    _backgroundMode = false;
  }
  return self;
}

- (OLPHttpTaskStatus)run {
  if (!self.url || self.dataTask) {
    return OLPHttpTaskStatusNotReady;
  }

  return [self restart];
}

- (OLPHttpTaskStatus)restartInBackground:(NSURLSession*)session {
  _urlSession = session;
  self.backgroundMode = true;
  return [self restart];
}

- (OLPHttpTaskStatus)restart {
  if (!self.url) {
    return OLPHttpTaskStatusNotReady;
  }

  if (_dataTask) {
    @synchronized(_httpClient.toIgnoreResponse) {
      _httpClient.toIgnoreResponse[_dataTask.taskDescription] = _dataTask;
    }
    [_dataTask cancel];
    _dataTask = nil;
  }

  NSMutableURLRequest* request =
      [NSMutableURLRequest requestWithURL:[NSURL URLWithString:self.url]];
  request.timeoutInterval = self.connectionTimeout;
#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  if (self.backgroundMode) {
    request.timeoutInterval = self.backgroundConnectionTimeout;
  }
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  request.HTTPMethod = self.HTTPMethod;
  if (self.body.length) {
    request.HTTPBody = self.body;
  }

  for (NSString* key in self.headers.allKeys) {
    NSString* value = self.headers[key];
    _headersSizeSent += key.length + value.length;
    [request setValue:value forHTTPHeaderField:key];
  }

  @synchronized(self) {
    if (!self.isCancelled) {
      _dataTask = [_httpClient createSessionTask:_urlSession
                                     withRequest:request
                              withBackgroundMode:self.backgroundMode];
      _dataTask.taskDescription = [self createTaskDescription];
    }
  }

  if (!self.dataTask) {
    return OLPHttpTaskStatusNotReady;
  }

  // Cache the task id for fast retrieve later on
  [_httpClient registerDataTask:_dataTask forHttpTask:self];

  OLP_SDK_LOG_DEBUG_F(
      kLogTag,
      "Run task, session=%p, dataTask=%p, request_id=%llu, task_id=%lu, "
      "task_description=%s, http_method=%s, url=%s)",
      (__bridge void*)_urlSession, (__bridge void*)self.dataTask,
      self.requestId, (unsigned long)self.dataTask.taskIdentifier,
      (char*)[self.dataTask.taskDescription UTF8String],
      [request.HTTPMethod UTF8String], [[self getDebugCurlString] UTF8String]);

  [_dataTask resume];
  return OLPHttpTaskStatusOk;
}

- (BOOL)cancel {
  if (self.isCancelled) {
    return NO;
  }
  OLP_SDK_LOG_TRACE_F(kLogTag, "Cancel task, request_id=%llu", self.requestId);

  self.cancelled = YES;
  @synchronized(self) {
    self.responseHandler = nil;
    self.dataHandler = nil;
  }

  if (self.dataTask) {
    [self.dataTask cancel];
    return YES;
  }

  return NO;
}

#pragma mark - Response handlers

- (void)didCompleteWithError:(NSError*)error {
  int status = 0;
  if (self.responseData) {
    status = self.responseData.status;
  }
  if (status == 0 && (!error || error == 0)) {
    status = olp::http::HttpStatusCode::OK;
    self.responseData.status = status;
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "Task is completed, request_id=%llu, url=%s, "
                      "task_id=%u, error=%i, status=%i",
                      self.requestId, [self.url UTF8String],
                      (unsigned int)self.dataTask.taskIdentifier,
                      (int)error.code, status);

  OLPHttpTaskCompletionHandler completionHandler = nil;
  @synchronized(self) {
    if (self.completionHandler) {
      completionHandler = [self.completionHandler copy];
    }
  }

  if (completionHandler) {
    completionHandler(
        error,
        _headersSizeReceived +
            (_contentLength ? _contentLength : _dataTask.countOfBytesReceived),
        _headersSizeSent + _dataTask.countOfBytesSent);
  }

  @synchronized(self) {
    self.completionHandler = nil;
  }
}

- (void)didReceiveResponse:(NSURLResponse*)response {
  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "didReceiveResponse, request_id=%llu, url=%s, "
                      "task_identifier=%u,"
                      "status_code=%i",
                      self.requestId, [response.URL.absoluteString UTF8String],
                      (unsigned int)self.dataTask.taskIdentifier,
                      (int)[(NSHTTPURLResponse*)response statusCode]);

  OLPHttpTaskReponseHandler responseHandler = nil;

  @synchronized(self) {
    if (self.responseHandler) {
      responseHandler = [self.responseHandler copy];
    }
  }

  if (responseHandler) {
    auto headers = ((NSHTTPURLResponse*)response).allHeaderFields;
    for (NSString* key in headers) {
      NSString* value = headers[key];
      if ([key isEqualToString:@"Content-Length"]) {
        auto longLongValue = [value longLongValue];
        _contentLength = longLongValue < 0 ? 0 : longLongValue;
      }
      _headersSizeReceived += key.length + value.length;
    }
    responseHandler((NSHTTPURLResponse*)response);
  }
}

- (void)didReceiveData:(NSData*)data withWholeData:(bool)wholeData {
  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "didReceiveData, request_id=%llu, url=%s, "
                      "task_id=%u, data_length=%lu",
                      self.requestId, [self.url UTF8String],
                      (unsigned int)self.dataTask.taskIdentifier,
                      (unsigned long)data.length);

  OLPHttpTaskDataHandler dataHandler = nil;

  @synchronized(self) {
    if (self.dataHandler) {
      dataHandler = [self.dataHandler copy];
    }
  }

  if (dataHandler) {
    dataHandler(data, wholeData);
  }
}

- (NSString*)createTaskDescription {
  return [NSString stringWithFormat:@"%llu", self.requestId];
}

#pragma mark - Inquiry methods

- (BOOL)isValid {
  return _urlSession != nil;
}

- (BOOL)isCancelled {
  return self.cancelled;
}

#pragma mark - Private methods

- (NSString*)getDebugCurlString {
  if (olp::logging::Log::getLevel() == olp::logging::Level::Trace) {
    NSMutableString* debugHeaders = [[NSMutableString alloc] init];

    for (NSString* key in self.headers) {
      [debugHeaders appendFormat:@"%@\"%@:%@\"", @" --header ", key,
                                 [self.headers objectForKey:key]];
    }

    return [NSString
        stringWithFormat:@"curl -vvv \"%@\"%@", self.url, debugHeaders];
  } else {
    return self.url;
  }
}

@end
