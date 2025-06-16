/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#import "OLPHttpClient+Internal.h"

#import <CommonCrypto/CommonDigest.h>
#import <Security/Security.h>

#include <sstream>

#include "context/ContextInternal.h"
#include "olp/core/context/Context.h"
#include "olp/core/context/EnterBackgroundSubscriber.h"
#include "olp/core/http/Network.h"
#include "olp/core/logging/Log.h"

#import "OLPHttpTask+Internal.h"

namespace {
constexpr auto kLogTag = "OLPHttpClient";
constexpr auto kMaximumConnectionPerHost = 32;

using SessionId = std::uint64_t;
static SessionId sessionIdCounter_ = std::time(nullptr);

class EnterBackgroundSubscriberImpl
    : public olp::context::EnterBackgroundSubscriber {
 public:
  EnterBackgroundSubscriberImpl(OLPHttpClient* http_client)
      : http_client_(http_client) {}

  void OnEnterBackground() override {
    if (http_client_ && !http_client_.inBackground) {
      http_client_.inBackground = true;
      [http_client_ restartCurrentTasks];
    }
  }

  void OnExitBackground() override {
    if (http_client_ && http_client_.inBackground) {
      http_client_.inBackground = false;
    }
  }

 private:
  OLPHttpClient* http_client_{nullptr};
};

}  // namespace

@interface OLPHttpClient ()<NSURLSessionDataDelegate>

@property(nonatomic) NSMutableDictionary* tasks;

@property(nonatomic) NSMutableDictionary* urlSessions;

@property(nonatomic, readonly) NSURLSession* sharedUrlSession;

@property(nonatomic, readonly) NSURLSession* sharedUrlBackgroundSession;

@property(nonatomic, readonly) NSMutableDictionary* idTaskMap;

@end

@implementation OLPHttpClient {
  NSOperationQueue* _delegateQueue;

  olp::context::Context::Scope _scope;
  std::shared_ptr<EnterBackgroundSubscriberImpl> _enterBackgroundSubscriber;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Created client=%p ", (__bridge void*)self);
    _delegateQueue = [[NSOperationQueue alloc] init];
    _delegateQueue.name = @"com.here.olp.network.HttpClientSessionQueue";

    _sharedUrlSession =
        [self urlSessionWithProxy:nil andHeaders:nil andBackgroundId:nil];

    _sharedUrlBackgroundSession =
        [self urlSessionWithProxy:nil
                       andHeaders:nil
                  andBackgroundId:[self generateNextSessionId]];

    _tasks = [[NSMutableDictionary alloc] init];
    _idTaskMap = [[NSMutableDictionary alloc] init];
    _urlSessions = [[NSMutableDictionary alloc] init];
    _toIgnoreResponse = [[NSMutableDictionary alloc] init];

    self.inBackground = false;
#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
    _enterBackgroundSubscriber =
        std::make_shared<EnterBackgroundSubscriberImpl>(self);
    olp::context::ContextInternal::SubscribeEnterBackground(
        _enterBackgroundSubscriber);
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  }
  return self;
}

- (void)dealloc {
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Destroyed client=%p ", (__bridge void*)self);
  if (self.sharedUrlSession) {
    [self cleanup];
  }
}

- (void)cleanup {
  OLP_SDK_LOG_DEBUG_F(kLogTag, "Cleanup tasks for client=%p",
                      (__bridge void*)self);

  [self.sharedUrlSession finishTasksAndInvalidate];
  [self.urlSessions
      enumerateKeysAndObjectsUsingBlock:^(id key, id object, BOOL* stop) {
        NSURLSession* session = object;
        [session finishTasksAndInvalidate];
      }];
  [_delegateQueue cancelAllOperations];
  _sharedUrlSession = nil;
  [self.urlSessions removeAllObjects];
  [self.toIgnoreResponse removeAllObjects];
#ifdef OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
  olp::context::ContextInternal::UnsubscribeEnterBackground(
      _enterBackgroundSubscriber);
  _enterBackgroundSubscriber.reset();
#endif  // OLP_SDK_NETWORK_IOS_BACKGROUND_DOWNLOAD
}

- (NSArray*)activeTasks {
  NSArray* tasks = nil;
  @synchronized(_tasks) {
    tasks = [NSArray arrayWithArray:_tasks.allValues];
  }
  return tasks;
}

- (NSString*)generateNextSessionId {
  auto sessionId = sessionIdCounter_++;
  if (sessionIdCounter_ == std::numeric_limits<SessionId>::max()) {
    sessionIdCounter_ = std::numeric_limits<SessionId>::min() + 1;
  }

  return [NSString stringWithFormat:@"olp-sdk-cpp-core # %llu", sessionId];
}

- (NSURLSessionTask*)createSessionTask:(NSURLSession*)session
                           withRequest:(NSMutableURLRequest*)request
                    withBackgroundMode:(bool)backgroundMode {
  if (backgroundMode) {
    NSURLSessionTask* task = [session downloadTaskWithRequest:request];

    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "createSessionTask background session=%p, task=%p",
                        (__bridge void*)session, (__bridge void*)task);
    return task;
  }

  NSURLSessionTask* task = [session dataTaskWithRequest:request];
  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "createSessionTask foreground session=%p, task=%p",
                      (__bridge void*)session, (__bridge void*)task);

  return task;
}

- (NSURLSession*)pickSession:(NSDictionary*)proxyDict {
  NSURLSession* session = _sharedUrlSession;

  if (self.inBackground) {
    session = _sharedUrlBackgroundSession;
  }

  if (proxyDict) {
    NSString* newSessionId = [self generateNextSessionId];
    session = [self urlSessionWithProxy:proxyDict
                             andHeaders:nil
                        andBackgroundId:newSessionId];
  }

  return session;
}

- (OLPHttpTask*)createTaskWithProxy:
                    (const olp::http::NetworkProxySettings&)proxySettings
                              andId:(olp::http::RequestId)identifier {
  NSDictionary* proxyDict = [self toProxyDict:proxySettings];
  NSURLSession* session = [self pickSession:proxyDict];

  OLPHttpTask* task = [[OLPHttpTask alloc] initWithHttpClient:self
                                                andURLSession:session
                                                        andId:identifier];
  task.backgroundMode = self.inBackground;

  @synchronized(self.tasks) {
    self.urlSessions[@(identifier)] = session;
    self.tasks[@(identifier)] = task;
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "createTaskWithProxy, session=%p, request_id=%llu, "
                      "httpTask=%p, session_config_id=%s",
                      (__bridge void*)session, identifier, (__bridge void*)task,
                      (char*)[session.configuration.identifier UTF8String]);

  return task;
}

- (OLPHttpTask*)taskWithId:(olp::http::RequestId)identifier {
  OLPHttpTask* task;
  @synchronized(_tasks) {
    task = _tasks[@(identifier)];
  }
  return task;
}

- (OLPHttpTask*)taskWithTaskDescription:(NSString*)taskDescription {
  OLPHttpTask* task = nil;
  @synchronized(_tasks) {
    task = [self.idTaskMap objectForKey:taskDescription];
  }
  return task;
}

- (void)cancelTaskWithId:(olp::http::RequestId)identifier {
  OLP_SDK_LOG_DEBUG_F(kLogTag, "cancelTaskWithId, id=%llu", identifier);
  OLPHttpTask* task = nil;
  @synchronized(_tasks) {
    task = _tasks[@(identifier)];
  }
  if (!task) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "Cancelling unknown request, id=%llu",
                          identifier);
    return;
  }

  [task cancel];
}

- (void)removeTaskWithId:(olp::http::RequestId)identifier {
  @synchronized(_tasks) {
    OLPHttpTask* task = _tasks[@(identifier)];

    NSNumber* requestId = @(identifier);
    NSURLSession* session = self.urlSessions[requestId];

    if (task.dataTask) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "removeTaskWithId, request_id=%llu, httpTask=%p, "
                          "dataTask=%p, session=%p, session_config_id=%s",
                          identifier, (__bridge void*)task,
                          (__bridge void*)task.dataTask,
                          (__bridge void*)session,
                          (char*)[session.configuration.identifier UTF8String]);

      [task.dataTask cancel];
      [self.idTaskMap removeObjectForKey:task.dataTask.taskDescription];
    } else {
      OLP_SDK_LOG_DEBUG_F(
          kLogTag,
          "removeTaskWithId, no http task in _tasks for request_id=%llu",
          identifier);
    }

    [_tasks removeObjectForKey:@(identifier)];
    [self.urlSessions removeObjectForKey:requestId];
  }
}

- (std::string)getInfoForTaskWithId:(olp::http::RequestId)identifier {
  @synchronized(_tasks) {
    OLPHttpTask* task = _tasks[@(identifier)];
    std::stringstream out;

    NSNumber* requestId = @(identifier);
    NSURLSession* session = self.urlSessions[requestId];

    if (!task.dataTask) {
      out << "no http task in _tasks for request_id=" << identifier;
      return out.str();
    }

    const auto get_session_config_id = [&]() {
      if (session && session.configuration &&
          session.configuration.identifier) {
        return [session.configuration.identifier UTF8String];
      }
      return "<not set>";
    };

    out << "request_id=" << identifier << ", httpTask=" << (__bridge void*)task
        << ", backgroundMode=" << task.backgroundMode
        << ", dataTask=" << (__bridge void*)task.dataTask
        << ", session=" << (__bridge void*)session
        << ", session_config_id=" << get_session_config_id();
    return out.str();
  }
}

#pragma mark - NSURLSessionDataDelegate

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(NSError*)error {
  if (!self.sharedUrlSession &&
      NSURLErrorCancelled != error.code) {  // Cleanup called and not cancelled
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "didCompleteWithError failed, "
                          "task_id=%u; error=%i",
                          (unsigned int)task.taskIdentifier, (int)error.code);
    return;
  }

  OLP_SDK_LOG_DEBUG_F(
      kLogTag,
      "didCompleteWithError, session=%p, task_id=%u, error=%i, dataTask=%p",
      (__bridge void*)session, (unsigned int)task.taskIdentifier,
      (int)error.code, (__bridge void*)task);

  @synchronized(_toIgnoreResponse) {
    NSURLSessionTask* taskToIgnore =
        [self.toIgnoreResponse objectForKey:task.taskDescription];
    if (taskToIgnore) {
      OLP_SDK_LOG_DEBUG_F(kLogTag,
                          "didCompleteWithError, session=%p, task_id=%u, "
                          "error=%i, dataTask=%p ignored",
                          (__bridge void*)session,
                          (unsigned int)task.taskIdentifier, (int)error.code,
                          (__bridge void*)task);
      [self.toIgnoreResponse removeObjectForKey:task.taskDescription];
      return;
    }
  }

  // This delegate is called when the session task is done. Background tasks
  // have separate `didFinishDownloadingToURL` when the file is ready so
  // we don't wan to trigger the callback second time.
  if ([task isKindOfClass:[NSURLSessionDownloadTask class]] &&
      (!error || error == 0)) {
    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "didCompleteWithError skipping in background mode, "
                        "session=%p, task_id=%u, error=%i, dataTask=%p",
                        (__bridge void*)session,
                        (unsigned int)task.taskIdentifier, (int)error.code,
                        (__bridge void*)task);
    return;
  }

  @autoreleasepool {
    OLPHttpTask* httpTask = [self taskWithTaskDescription:task.taskDescription];
    if ([httpTask isValid]) {
      [httpTask didCompleteWithError:error];
      [self removeTaskWithId:httpTask.requestId];
    } else {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "didCompleteWithError failed - can't find the task, "
          "task_id=%u, error=%i",
          (unsigned int)task.taskIdentifier, (int)error.code);
    }
  }
}

- (void)URLSession:(NSURLSession*)session
              dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))
                           completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "didReceiveResponse failed - invalid session, "
                          "task_id=%u, url=%s, status_code=%i",
                          (unsigned int)dataTask.taskIdentifier,
                          [response.URL.absoluteString UTF8String],
                          (int)[(NSHTTPURLResponse*)response statusCode]);
    return;
  }

  OLP_SDK_LOG_DEBUG_F(
      kLogTag, "didReceiveResponse, session=%p, task_id=%u, dataTask=%p",
      (__bridge void*)session, (unsigned int)dataTask.taskIdentifier,
      (__bridge void*)dataTask);

  @autoreleasepool {
    OLPHttpTask* httpTask =
        [self taskWithTaskDescription:dataTask.taskDescription];
    if ([httpTask isValid] && ![httpTask isCancelled]) {
      [httpTask didReceiveResponse:response];
    } else {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "didReceiveResponse failed - task can't be found or cancelled, "
          "task_id=%u, url=%s, status=%i",
          (unsigned int)dataTask.taskIdentifier,
          [response.URL.absoluteString UTF8String],
          (int)[(NSHTTPURLResponse*)response statusCode]);
    }
    completionHandler(NSURLSessionResponseAllow);
  }
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionTask*)dataTask
    didReceiveData:(NSData*)data {
  if (!self.sharedUrlSession) {  // Cleanup called
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "didReceiveData failed - invalid session, "
                          "task_id=%u",
                          (unsigned int)dataTask.taskIdentifier);
    return;
  }

  @autoreleasepool {
    OLPHttpTask* httpTask =
        [self taskWithTaskDescription:dataTask.taskDescription];
    if ([httpTask isValid] && ![httpTask isCancelled]) {
      [httpTask didReceiveData:data withWholeData:false];
    } else {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "didReceiveData failed - task can't be found or cancelled, "
          "task_id=%u",
          (unsigned int)dataTask.taskIdentifier);
    }
  }
}

- (void)URLSession:(NSURLSession*)session
                   task:(NSURLSessionTask*)dataTask
    didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge
      completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition,
                                  NSURLCredential*))completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "didReceiveChallenge failed - invalid session, "
                          "task_id=%u",
                          (unsigned int)dataTask.taskIdentifier);
    return;
  }

  @autoreleasepool {
    if ([challenge.protectionSpace.authenticationMethod
            isEqualToString:NSURLAuthenticationMethodServerTrust]) {
      if (dataTask) {
        OLPHttpTask* httpTask =
            [self taskWithTaskDescription:dataTask.taskDescription];
        if (![httpTask isValid]) {
          return;
        }
        // TODO: Don't verify certificate is not implemented
        if (![self shouldTrustProtectionSpace:challenge.protectionSpace]) {
          completionHandler(
              NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
          return;
        }
      }

      NSURLCredential* credential = [NSURLCredential
          credentialForTrust:challenge.protectionSpace.serverTrust];
      completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
      return;
    }

    completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
  }
}

- (void)URLSession:(NSURLSession*)session
                          task:(NSURLSessionTask*)task
    willPerformHTTPRedirection:(NSHTTPURLResponse*)response
                    newRequest:(NSURLRequest*)request
             completionHandler:
                 (void (^)(NSURLRequest* _Nullable))completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "willPerformHTTPRedirection failed - invalid session, "
        "task_id=%u, status=%i, origin_url=%s, new_url=%s",
        (unsigned int)task.taskIdentifier, (int)response.statusCode,
        task.originalRequest.URL.absoluteString.UTF8String,
        request.URL.absoluteString.UTF8String);
    return;
  }

  NSURLRequest* originalRequest = task.originalRequest;
  NSString* authorizationHeaderValue =
      originalRequest.allHTTPHeaderFields[@"Authorization"];
  OLP_SDK_LOG_TRACE_F(kLogTag,
                      "HTTPRedirection: self=%p, task_id=%u, "
                      "status=%i, origin_url=%s, new_url=%s",
                      (__bridge void*)self, (unsigned int)task.taskIdentifier,
                      (int)response.statusCode,
                      originalRequest.URL.absoluteString.UTF8String,
                      request.URL.absoluteString.UTF8String);
  NSMutableURLRequest* newRequest = [[NSMutableURLRequest alloc] init];
  newRequest.URL = request.URL;
  newRequest.timeoutInterval = request.timeoutInterval;
  newRequest.cachePolicy = request.cachePolicy;
  newRequest.networkServiceType = request.networkServiceType;
  newRequest.HTTPMethod = request.HTTPMethod;
  newRequest.HTTPBody = request.HTTPBody;
  [request.allHTTPHeaderFields
      enumerateKeysAndObjectsUsingBlock:^(id key, id object, BOOL* stop) {
        [newRequest addValue:object forHTTPHeaderField:key];
      }];

  // NOTE: It appears that most headers are maintained during a redirect with
  // the exception of the `Authorization` header.
  // It appears that Apple's strips the `Authorization` header from the
  // redirected URL request. If you need to maintain the `Authorization` header,
  // you need to manually append it to the redirected request.
  if (authorizationHeaderValue.length) {
    [newRequest addValue:authorizationHeaderValue
        forHTTPHeaderField:@"Authorization"];
  }
  completionHandler(newRequest);
}

// http://goo.gl/jmZ4Uv
- (BOOL)shouldTrustProtectionSpace:(NSURLProtectionSpace*)protectionSpace {
  if (!protectionSpace) {
    return NO;
  }

  SecTrustRef serverTrust = protectionSpace.serverTrust;
  if (!serverTrust) {
    return NO;
  }

  // TODO - certificate paths are not supported!

  // evaluate server trust against certificate
  SecTrustResultType trustResult = kSecTrustResultInvalid;
  OSStatus status = SecTrustEvaluate(serverTrust, &trustResult);

  if (errSecSuccess != status) {
    return NO;
  }

  return (trustResult == kSecTrustResultUnspecified ||
          trustResult == kSecTrustResultProceed);
}

- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionTask*)dataTask
                 didWriteData:(int64_t)bytesWritten
            totalBytesWritten:(int64_t)totalBytesWritten
    totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite {
  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "didWriteData for session=%p, task_id=%u, bytes "
                      "written now=%lli, in total=%lli, expected=%lli",
                      (__bridge void*)session,
                      (unsigned int)dataTask.taskIdentifier, bytesWritten,
                      totalBytesWritten, totalBytesExpectedToWrite);
}

- (void)URLSession:(NSURLSession*)session
                 downloadTask:(NSURLSessionTask*)dataTask
    didFinishDownloadingToURL:(NSURL*)location {
  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "didFinishDownloadingToURL for session=%p, dataTask=%p, "
                      "session_config_id=%s, task_id=%u, location=%s",
                      (__bridge void*)session, (__bridge void*)dataTask,
                      (char*)[session.configuration.identifier UTF8String],
                      (unsigned int)dataTask.taskIdentifier,
                      (char*)[location.absoluteString UTF8String]);

  if (!self.sharedUrlSession) {  // Cleanup called // Revise sessions cleanup
    OLP_SDK_LOG_WARNING_F(kLogTag,
                          "didFinishDownloadingToURL failed - invalid session, "
                          "task_id=%u",
                          (unsigned int)dataTask.taskIdentifier);
    return;
  }

  @autoreleasepool {
    NSData* data =
        [[NSFileManager defaultManager] contentsAtPath:location.path];

    @synchronized(_urlSessions) {
      OLPHttpTask* httpTask =
          [self taskWithTaskDescription:dataTask.taskDescription];

      if (!httpTask) {
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "didFinishDownloadingToURL failed - task can't "
                              "be found, session=%p, dataTask=%p, task_id=%u, "
                              "task_description=%s",
                              (__bridge void*)session, (__bridge void*)dataTask,
                              (unsigned int)dataTask.taskIdentifier,
                              (char*)[dataTask.taskDescription UTF8String]);
        return;
      }

      if ([httpTask isValid] && ![httpTask isCancelled]) {
        [httpTask didReceiveData:data withWholeData:true];
        [httpTask didCompleteWithError:dataTask.error];
      } else {
        OLP_SDK_LOG_WARNING_F(
            kLogTag,
            "didFinishDownloadingToURL failed - task not valid or "
            "cancelled, session=%p, dataTask=%p, task_id=%u, "
            "task_description=%s",
            (__bridge void*)session, (__bridge void*)dataTask,
            (unsigned int)dataTask.taskIdentifier,
            (char*)[dataTask.taskDescription UTF8String]);
      }

      [self removeTaskWithId:httpTask.requestId];
    }
  }
}

- (void)URLSessionDidFinishEventsForBackgroundURLSession:
    (NSURLSession*)session {
  @autoreleasepool {
    NSString* sessionId = session.configuration.identifier;

    OLP_SDK_LOG_DEBUG_F(kLogTag,
                        "URLSessionDidFinishEventsForBackgroundURLSession for "
                        "session=%p, session_config_id=%s",
                        (__bridge void*)session, (char*)[sessionId UTF8String]);

    @synchronized(self.tasks) {
      olp::context::ContextInternal::CallBackgroundSessionCompletionHandler(
          [sessionId UTF8String]);
    }
  }
}

#pragma mark - Internal methods

- (NSDictionary*)toProxyDict:
    (const olp::http::NetworkProxySettings&)proxySettings {
  NSMutableDictionary* proxyDict = nil;
  BOOL isProxyValid =
      proxySettings.GetType() != olp::http::NetworkProxySettings::Type::NONE &&
      !proxySettings.GetHostname().empty();

  if (isProxyValid) {
    NSString* proxyName =
        [NSString stringWithUTF8String:proxySettings.GetHostname().c_str()];
    if (proxyName.length) {
      proxyDict = [[NSMutableDictionary alloc] init];
      NSUInteger port = (NSUInteger)proxySettings.GetPort();

      const auto requestedProxyType = proxySettings.GetType();

      using ProxyType = olp::http::NetworkProxySettings::Type;

      if (ProxyType::SOCKS4 == requestedProxyType ||
          ProxyType::SOCKS5 == requestedProxyType ||
          ProxyType::SOCKS5_HOSTNAME == requestedProxyType) {
        proxyDict[(__bridge NSString*)kCFProxyTypeKey] =
            (__bridge NSString*)kCFProxyTypeSOCKS;
      } else if (ProxyType::HTTP == requestedProxyType) {
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPEnable] = @(1);
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPProxy] = proxyName;
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPPort] = @(port);
      } else if (ProxyType::HTTPS == requestedProxyType) {
        proxyDict[@"HTTPSEnable"] = @(1);
        proxyDict[@"HTTPSProxy"] = proxyName;
        proxyDict[@"HTTPSPort"] = @(port);
      }

      proxyDict[(__bridge NSString*)kCFProxyHostNameKey] = proxyName;
      proxyDict[(__bridge NSString*)kCFProxyPortNumberKey] = @(port);
      NSString* userName =
          [NSString stringWithUTF8String:proxySettings.GetUsername().c_str()];
      NSString* userPassword =
          [NSString stringWithUTF8String:proxySettings.GetPassword().c_str()];
      if (userName.length && userPassword.length) {
        proxyDict[(NSString*)kCFProxyUsernameKey] = userName;
        proxyDict[(NSString*)kCFProxyPasswordKey] = userPassword;
      }
    }
  }

  return proxyDict;
}

- (NSURLSession*)urlSessionWithProxy:(NSDictionary*)proxyDict
                          andHeaders:(NSDictionary*)headers
                     andBackgroundId:(NSString*)sessionBackgroundId {
  NSURLSessionConfiguration* config;

  if (sessionBackgroundId) {
    config = [NSURLSessionConfiguration
        backgroundSessionConfigurationWithIdentifier:sessionBackgroundId];
  } else {
    config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
  }

  config.HTTPMaximumConnectionsPerHost = kMaximumConnectionPerHost;

  if (proxyDict) {
    config.connectionProxyDictionary = proxyDict;
  }
  if (headers.count) {
    config.HTTPAdditionalHeaders = headers;
  }

  return [NSURLSession sessionWithConfiguration:config
                                       delegate:self
                                  delegateQueue:_delegateQueue];
}

- (void)registerDataTask:(NSURLSessionTask*)dataTask
             forHttpTask:(OLPHttpTask*)httpTask {
  NSString* identifier = dataTask.taskDescription;

  OLP_SDK_LOG_DEBUG_F(
      kLogTag, "registerDataTask, httpTask=%p, taskDescription=%s, task_id=%u",
      (__bridge void*)httpTask, (char*)[dataTask.taskDescription UTF8String],
      (unsigned int)dataTask.taskIdentifier);

  @synchronized(_tasks) {
    [self.idTaskMap setValue:httpTask forKey:identifier];
  }
}

- (void)restartCurrentTasks {
  OLP_SDK_LOG_DEBUG_F(kLogTag, "restartCurrentTasks");

  @autoreleasepool {
    @synchronized(self.tasks) {
      for (id key in self.tasks) {
        olp::http::RequestId requestId = [key unsignedLongLongValue];
        OLPHttpTask* httpTask = [self.tasks objectForKey:key];
        NSURLSessionTask* dataTask = [httpTask dataTask];
        NSURLSession* session = self.urlSessions[@(requestId)];

        if ([dataTask isKindOfClass:[NSURLSessionDownloadTask class]]) {
          continue;
        }

        // It's possible the task is completed just at the switching to
        // background.
        if (dataTask.state == NSURLSessionTaskStateRunning) {
          [dataTask suspend];

          NSURLSession* newSession = [self
              pickSession:session.configuration.connectionProxyDictionary];

          OLPHttpTaskStatus ret = [httpTask restartInBackground:newSession];

          if (OLPHttpTaskStatusOk != ret) {
            OLP_SDK_LOG_DEBUG_F(kLogTag,
                                "Task restart failed, request_id=%llu, url=%s",
                                requestId, (char*)[httpTask.url UTF8String]);

            continue;
          }

          self.urlSessions[@(requestId)] = newSession;
        }
      }
    }
  }
}

@end
