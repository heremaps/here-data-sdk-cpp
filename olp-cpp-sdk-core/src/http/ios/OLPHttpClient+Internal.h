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

#import "OLPHttpClient.h"

/**
 * @brief Internal category, which extends OLPHttpClient with internal methods,
 * which shouldn't be exposed as public API.
 */
@interface OLPHttpClient (Internal)

@property(nonatomic, readonly) NSArray* activeTasks;

- (NSURLSession*)urlSessionWithProxy:
                     (const olp::http::NetworkProxySettings&)proxySettings
                          andHeaders:(NSDictionary*)headers;

- (void)registerDataTask:(NSURLSessionDataTask*)dataTask
             forHttpTask:(OLPHttpTask*)httpTask;

@end
