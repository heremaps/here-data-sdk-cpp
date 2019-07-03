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

typedef void (^HttpTaskReponseHandler)(NSHTTPURLResponse* response);
typedef void (^HttpTaskDataHandler)(NSData* data);
typedef void (^HttpTaskCompletionHandler)(NSError* error);

// Status code in NSError object of HttpClientCompletionBlock
typedef NS_ENUM(NSUInteger, HttpTaskStatus) {
  HttpTaskStatusNone,
  HttpTaskStatusNotReady  // request not ready
};

FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodGet;
FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodPost;
FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodHead;
FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodPut;
FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodDelete;
FOUNDATION_EXPORT NSString* const kHTTPTaskHttpMethodPatch;

@class HttpClient;
@class HttpTaskContext;

/**
 * @brief HTTP task: for example, GET, POST etc
 * it is used to make http request and return data in completion block.
 * It supports customized HTTP header & http request parameters. And the request can be cancelled.
 */
@interface HttpTask : NSObject

// Request
@property(nonatomic) HttpTaskContext* context;
@property(nonatomic)
    NSString* httpMethod;  // kHTTPTaskHttpMethodxxxx, Default to kHTTPTaskHttpMethodGet
@property(nonatomic, readonly) NSString* url;
@property(nonatomic) NSDictionary* headers;  // {key, value}
@property(nonatomic) NSDictionary* urlParameters;
@property(nonatomic) NSData* body;  // Data body
@property(nonatomic) NSUInteger transferTimeout;
@property(nonatomic) NSUInteger connectionTimeout;

// Response
@property(nonatomic, copy) HttpTaskReponseHandler responseHandler;
@property(nonatomic, copy) HttpTaskDataHandler dataHandler;
@property(nonatomic, copy) HttpTaskCompletionHandler completionHandler;

@property(nonatomic, weak) NSURLSession* urlSession;

// Internal data task
@property(nonatomic, readonly) NSURLSessionDataTask* dataTask;
@property(nonatomic, readonly) int uniqueId;  // Client id

- (instancetype)initWithId:(int)identifier httpClient:(HttpClient*)client;
- (void)appendToUrl:(NSString*)value;
- (HttpTaskStatus)run;
- (BOOL)cancel;

// Reponse from Http
- (void)didReceiveResponse:(NSURLResponse*)response;
- (void)didReceiveData:(NSData*)data;
- (void)didCompleteWithError:(NSError*)error;

@end
