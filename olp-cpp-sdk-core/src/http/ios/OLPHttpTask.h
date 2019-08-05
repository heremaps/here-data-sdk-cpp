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

#include <memory>
#include <mutex>

#include <olp/core/http/Network.h>

@class OLPHttpClient;

/**
 * @brief Enum, which represents the execution status of OLPHttpTask
 */
typedef NS_ENUM(NSUInteger, OLPHttpTaskStatus) {
  OLPHttpTaskStatusOk,       ///< OLPHttpTask is correctly setup
  OLPHttpTaskStatusNotReady  ///< request not ready
};

typedef void (^OLPHttpTaskReponseHandler)(NSHTTPURLResponse* response);
typedef void (^OLPHttpTaskDataHandler)(NSData* data);
typedef void (^OLPHttpTaskCompletionHandler)(NSError* error);

/**
 * @brief This class holds the response data from OLPHttpTask request.
 */
@interface OLPHttpTaskResponseData : NSObject

@property(nonatomic) int status;

@property(nonatomic) std::uint64_t count;

@property(nonatomic) std::uint64_t offset;

@property(nonatomic) bool rangeOut;

@end

/**
 * @brief HTTP task, which is a wrapper around NSURLSession task.
 * Performs HTTP request and return data in completion block.
 * It supports customized HTTP header and can be cancelled.
 */
@interface OLPHttpTask : NSObject

/// Initialize task with specific client, session and identifier
- (instancetype)initWithHttpClient:(OLPHttpClient*)client
                     andURLSession:(NSURLSession*)session
                             andId:(olp::http::RequestId)identifier;

// Data used for customizing the request
@property(nonatomic, copy) NSString* url;

@property(nonatomic, copy) NSString* HTTPMethod;

@property(nonatomic) NSData* body;

@property(nonatomic) NSDictionary* headers;

@property(nonatomic) NSUInteger connectionTimeout;

@property(nonatomic) std::shared_ptr<std::ostream> payload;

// Readonly
@property(nonatomic, readonly) olp::http::RequestId requestId;

@property(nonatomic, readonly) NSURLSessionDataTask* dataTask;

// Callbacks
@property(nonatomic) olp::http::Network::Callback callback;

@property(nonatomic) std::shared_ptr<std::mutex> callbackMutex;

@property(nonatomic, copy) OLPHttpTaskReponseHandler responseHandler;

@property(nonatomic, copy) OLPHttpTaskDataHandler dataHandler;

@property(nonatomic, copy) OLPHttpTaskCompletionHandler completionHandler;

// Execution flow of task
- (OLPHttpTaskStatus)run;

- (BOOL)cancel;

// Inquiry
- (BOOL)isCancelled;

- (BOOL)isValid;

/**
 * Property, which holds response data of this task. If no data is received,
 * or task is not valid anymore, it will store nil.
 */
@property(atomic) OLPHttpTaskResponseData* responseData;

@end
