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

#import "OLPHttpTask+Internal.h"

#include "olp/core/logging/Log.h"

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
  }
  return self;
}

- (OLPHttpTaskStatus)run {
  if (!self.url || self.dataTask) {
    return OLPHttpTaskStatusNotReady;
  }

  NSMutableURLRequest* request =
      [NSMutableURLRequest requestWithURL:[NSURL URLWithString:self.url]];
  request.timeoutInterval = self.connectionTimeout;
  request.HTTPMethod = self.HTTPMethod;
  if (self.body.length) {
    request.HTTPBody = self.body;
  }

  for (NSString* key in self.headers.allKeys) {
    NSString* value = self.headers[key];
    [request setValue:value forHTTPHeaderField:key];
  }

  @synchronized(self) {
    if (!self.isCancelled) {
      _dataTask = [_urlSession dataTaskWithRequest:request];
    }
  }

  if (!self.dataTask) {
    return OLPHttpTaskStatusNotReady;
  }

  // Cache the task id for fast retrieve later on
  [_httpClient registerDataTask:_dataTask forHttpTask:self];

  EDGE_SDK_LOG_TRACE_F(
      kLogTag, "Run task: id=[%llu]; task_id=[%lu];\t httpMethod=[%s];\t [%s])",
      self.requestId, (unsigned long)self.dataTask.taskIdentifier,
      [request.HTTPMethod UTF8String], [[self getDebugCurlString] UTF8String]);

  [_dataTask resume];
  return OLPHttpTaskStatusOk;
}

- (BOOL)cancel {
  if (self.isCancelled) {
    return NO;
  }
  EDGE_SDK_LOG_TRACE_F(kLogTag, "Cancelled task: id=[%llu]", self.requestId);

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
  EDGE_SDK_LOG_TRACE_F(
      kLogTag, "task is completed: id=[%llu]; taskIdentifier=[%u]; error=[%i]",
      self.requestId, (unsigned int)self.dataTask.taskIdentifier,
      (int)error.code);

  OLPHttpTaskCompletionHandler completionHandler = nil;
  @synchronized(self) {
    if (self.completionHandler) {
      completionHandler = [self.completionHandler copy];
    }
  }

  if (completionHandler) {
    completionHandler(error);
  }

  @synchronized(self) {
    self.completionHandler = nil;
  }
}

- (void)didReceiveResponse:(NSURLResponse*)response {
  EDGE_SDK_LOG_TRACE_F(kLogTag,
                       "task receives response: id=[%llu]; taskIdentifier=[%u] "
                       "responseCode=[%i]",
                       self.requestId,
                       (unsigned int)self.dataTask.taskIdentifier,
                       (int)[(NSHTTPURLResponse*)response statusCode]);

  OLPHttpTaskReponseHandler responseHandler = nil;

  @synchronized(self) {
    if (self.responseHandler) {
      responseHandler = [self.responseHandler copy];
    }
  }

  if (responseHandler) {
    responseHandler((NSHTTPURLResponse*)response);
  }
}

- (void)didReceiveData:(NSData*)data {
  EDGE_SDK_LOG_TRACE_F(
      kLogTag,
      "task receives data: id=[%llu]; taskIdentifier=[%u] dataLength=[%lu]",
      self.requestId, (unsigned int)self.dataTask.taskIdentifier,
      (unsigned long)data.length);

  OLPHttpTaskDataHandler dataHandler = nil;

  @synchronized(self) {
    if (self.dataHandler) {
      dataHandler = [self.dataHandler copy];
    }
  }

  if (dataHandler) {
    dataHandler(data);
  }
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
  NSMutableString* debugHeaders = [[NSMutableString alloc] init];

  for (NSString* key in self.headers) {
    [debugHeaders appendFormat:@"%@\"%@:%@\"", @" --header ", key,
                               [self.headers objectForKey:key]];
  }

  return
      [NSString stringWithFormat:@"curl -vvv \"%@\"%@", self.url, debugHeaders];
}

@end

