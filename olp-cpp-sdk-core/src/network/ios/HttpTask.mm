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

#import <olp/core/logging/Log.h>

#import "HttpClient.h"
#import "HttpTask.h"

// Local constants
NSString* const kHTTPTaskHttpMethodGet = @"GET";
NSString* const kHTTPTaskHttpMethodPost = @"POST";
NSString* const kHTTPTaskHttpMethodHead = @"HEAD";
NSString* const kHTTPTaskHttpMethodPut = @"PUT";
NSString* const kHTTPTaskHttpMethodDelete = @"DELETE";
NSString* const kHTTPTaskHttpMethodPatch = @"PATCH";
NSString* const kHTTPTaskAppId = @"app_id";
NSString* const kHTTPTaskAppCode = @"app_code";
NSString* const kHTTPTaskQuestionMark = @"?";
NSString* const kHTTPTaskAmpersand = @"&";
NSString* const kHTTPTaskEqual = @"=";
NSString* const kHTTPTaskErrorDomain = @"HttpTask";

#define LOGTAG "HTTPTASK"

@implementation HttpTask {
  NSMutableString* _requestUrl;
  __weak HttpClient* _httpClient;
}

- (instancetype)initWithId:(int)identifier httpClient:(HttpClient*)client {
  self = [super init];
  if (self) {
    EDGE_SDK_LOG_TRACE(LOGTAG,
                       "HttpTask::initWithId [ " << (__bridge void*)self << " ] " << identifier);
    _httpClient = client;
    _uniqueId = identifier;
    _requestUrl = [NSMutableString new];
    _httpMethod = kHTTPTaskHttpMethodGet;
  }
  return self;
}

- (void)dealloc {
  EDGE_SDK_LOG_TRACE(LOGTAG,
                     "HttpTask::dealloc [ " << (__bridge void*)self << " ] " << self.uniqueId);
}

- (NSString*)url {
  return [NSString stringWithString:_requestUrl];
}

- (void)appendToUrl:(NSString*)value {
  [_requestUrl appendString:value];
}

- (HttpTaskStatus)run {
  if (!_requestUrl || self.dataTask) {
    return HttpTaskStatusNotReady;
  }

  NSString* paramValue = nil;
  for (NSString* param in self.urlParameters.allKeys) {
    if (!paramValue) {
      [_requestUrl appendString:kHTTPTaskQuestionMark];
    } else {
      [_requestUrl appendString:kHTTPTaskAmpersand];
    }
    paramValue = _urlParameters[param];
    [_requestUrl appendString:[self requestParameterWithKey:param stringValue:paramValue]];
  }

  NSMutableURLRequest* request =
      [NSMutableURLRequest requestWithURL:[NSURL URLWithString:_requestUrl]];
  request.timeoutInterval = self.transferTimeout;
  request.HTTPMethod = self.httpMethod;
  if (self.body.length) {
    request.HTTPBody = self.body;
  }

#ifndef EDGE_SDK_LOGGING_DISABLED
  NSMutableString* debugHeaders = [NSMutableString new];
#endif

  for (NSString* key in self.headers.allKeys) {
    NSString* value = self.headers[key];
    [request setValue:value forHTTPHeaderField:key];
#ifndef EDGE_SDK_LOGGING_DISABLED
    [debugHeaders appendFormat:@"%@\"%@:%@\"", @" --header ", key, value];
#endif
  }

  _dataTask = [self.urlSession dataTaskWithRequest:request];
  if (!self.dataTask) {
    return HttpTaskStatusNotReady;
  }

  // Cache the task id for fast retrieve later on
  _httpClient.idTaskMap[@(self.dataTask.taskIdentifier)] = self;

#ifndef EDGE_SDK_LOGGING_DISABLED
  NSMutableString* debugCurlString = [NSMutableString stringWithString:@"curl -vvv "];
  [debugCurlString appendFormat:@"\"%@\"%@", _requestUrl, debugHeaders];
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpTask::run [ " << (__bridge void*)self << " ] "
                                                << self.dataTask.taskIdentifier << "\t"
                                                << request.HTTPMethod << "\t"
                                                << [debugCurlString UTF8String]);
#endif

  [_dataTask resume];
  return HttpTaskStatusNone;
}

- (BOOL)cancel {
  EDGE_SDK_LOG_TRACE(LOGTAG,
                     "HttpTask::cancel [ " << (__bridge void*)self << " ] " << self.uniqueId);

  BOOL ret = NO;
  @synchronized(self) {
    self.responseHandler = nil;
    self.dataHandler = nil;
  }

  if (self.dataTask) {
    [self.dataTask cancel];
    ret = YES;
  }
  return ret;
}

- (void)didCompleteWithError:(NSError*)error {
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpTask::didComplete [ " << (__bridge void*)self << " ] "
                                                        << self.dataTask.taskIdentifier << "\t"
                                                        << error.code);
  HttpTaskCompletionHandler completionHandler = nil;
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

  [self resetTask];
}

- (void)didReceiveResponse:(NSURLResponse*)response {
#ifndef EDGE_SDK_LOGGING_DISABLED
  NSInteger httpResponseStatusCode = [(NSHTTPURLResponse*)response statusCode];
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpTask::didReceiveResponse [ " << (__bridge void*)self << " ] "
                                                               << self.dataTask.taskIdentifier
                                                               << "\t" << httpResponseStatusCode);
#endif

  HttpTaskReponseHandler responseHandler = nil;

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
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpTask::didReceiveData [ " << (__bridge void*)self << " ] "
                                                           << self.dataTask.taskIdentifier << "\t"
                                                           << (unsigned long)data.length);

  // Only print out the strData if debugging needed. It stucks the simulator when running code
  // coverage. NSString *strData = [[NSString alloc] initWithData:data
  // encoding:NSUTF8StringEncoding]; NSLog(@"HttpTask Response data: %@", strData);

  HttpTaskDataHandler dataHandler = nil;

  @synchronized(self) {
    if (self.dataHandler) {
      dataHandler = [self.dataHandler copy];
    }
  }

  if (dataHandler) {
    dataHandler(data);
  }
}

#pragma mark - Utilities

- (NSString*)requestParameterWithKey:(NSString*)key stringValue:(NSString*)value;
{
  NSString* formatString = @"%@=%@";
  NSString* formattedString =
      [NSString stringWithFormat:formatString, key, [self escapeURLQueryParam:value]];

  return formattedString;
}

#pragma mark - Private methods

- (void)resetTask {
  [_httpClient removeTaskWithId:self.uniqueId];
  _dataTask = nil;
}

- (NSError*)errorWithStatus:(HttpTaskStatus)status {
  return [NSError errorWithDomain:kHTTPTaskErrorDomain code:status userInfo:nil];
}

- (NSString*)escapeURLQueryParam:(NSString*)query {
  NSString* value;
  return
      [value stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                                    URLQueryAllowedCharacterSet]];
}

@end
