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

@class HttpTask;
namespace olp {
namespace network {
class NetworkProxy;
};
};

/**
 * \brief This class is wrapper around Apple's NSURLSession
 */
@interface HttpClient : NSObject

@property(nonatomic, readonly) NSArray* activeTasks;
@property(nonatomic, readonly) NSMutableDictionary* idTaskMap;  // Internal taskId : HttpTask

// create a task with shared url session
- (HttpTask*)createTaskWithId:(int)identifier;
// create a task with customized session
- (HttpTask*)createTaskWithId:(int)identifier session:(NSURLSession*)session;
- (HttpTask*)taskWithId:(int)identifier;
- (void)removeTaskWithId:(int)identifier;
- (void)cancelTaskWithId:(int)identifier;
- (void)cleanup;

// helper
- (NSURLSession*)urlSessionWithProxy:(const olp::network::NetworkProxy*)proxy
                             headers:(NSDictionary*)headers;

@end
