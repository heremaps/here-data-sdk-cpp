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

#import <CommonCrypto/CommonDigest.h>
#import <Security/Security.h>

#import <olp/core/logging/Log.h>
#import <olp/core/network/Network.h>
#import <olp/core/network/NetworkSystemConfig.h>

#import "HttpClient.h"
#import "HttpTask.h"

#define LOGTAG "HTTPCLIENT"

@interface CABundleLoader : NSObject

@property(nonatomic) NSMutableArray* caCertificateRefs;  // Array of SecCertificateRef

+ (instancetype)sharedInstance;

@end

static NSString* const kMosCACertificateMd5 = @"497904b0eb8719ac47b0bc11519b74d0";

@implementation CABundleLoader

+ (instancetype)sharedInstance {
  static CABundleLoader* sharedInstance = nil;
  static dispatch_once_t _singletonPredicate = 0;

  dispatch_once(&_singletonPredicate, ^{
    sharedInstance = [[CABundleLoader alloc] initPrivate];
  });

  return sharedInstance;
}

- (instancetype)init {
  return nil;
}

- (void)dealloc {
  for (id certificateRef in self.caCertificateRefs) {
    SecCertificateRef certiRef = (__bridge SecCertificateRef)certificateRef;
    CFRelease(certiRef);
  }
  [self.caCertificateRefs removeAllObjects];
}

- (instancetype)initPrivate {
  if (self = [super init]) {
    _caCertificateRefs = [NSMutableArray new];

    // load certificates from disk
    // TODO: Any expert to write lambda here?
    olp::network::NetworkSystemConfig sysConfig =
        olp::network::Network::SystemConfig().lockedCopy();
    NSString* certificateDirectory =
        [NSString stringWithUTF8String:sysConfig.GetCertificatePath().c_str()];
    if (certificateDirectory.length == 0) {
      return nil;
    }

    NSError* err;
    NSArray* directoryContents =
        [[NSFileManager defaultManager] contentsOfDirectoryAtPath:certificateDirectory error:&err];
    if (err || directoryContents.count == 0) {
      return nil;
    }

    for (NSString* certFile in directoryContents) {
      err = nil;
      NSString* certFilePath = [NSString stringWithFormat:@"%@/%@", certificateDirectory, certFile];
      NSData* certData =
          [NSData dataWithContentsOfFile:certFilePath options:NSDataReadingMappedIfSafe error:&err];

      if (!certData || err) {
        continue;
      }

      // verify md5 hash to prevent cert file tempering
      if (![[certFile pathExtension] isEqualToString:@"0"]) {  // MOS certificates verification
        return nil;
      }

      // parse into certificate ( http://goo.gl/agBj2q )
      SecCertificateRef caCertificateRef =
          SecCertificateCreateWithData(NULL, (__bridge CFDataRef)certData);
      if (caCertificateRef) {
        [self.caCertificateRefs addObject:(__bridge id)caCertificateRef];
      }
    }
  }
  return self;
}

- (BOOL)verifyCertificateData:(NSData*)data {
  // REF http://goo.gl/atfXWd

  // Create byte array of unsigned chars
  unsigned char md5Buffer[CC_MD5_DIGEST_LENGTH];

  // Create 16 byte MD5 hash value, store in buffer
  CC_MD5(data.bytes, (unsigned int)data.length, md5Buffer);

  // Convert unsigned char buffer to NSString of hex values
  NSMutableString* output = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
  for (int i = 0; i < CC_MD5_DIGEST_LENGTH; i++) {
    [output appendFormat:@"%02x", md5Buffer[i]];
  }

  return [kMosCACertificateMd5 isEqualToString:output];
}

@end

@interface HttpClient ()<NSURLSessionDataDelegate>

@property(nonatomic, readonly) NSURLSession* sharedUrlSession;  // shared URL session
@property(nonatomic) NSMutableDictionary* urlSessions;          // request id : NSURLSession object

@end

@implementation HttpClient {
  NSMutableDictionary* _tasks;
  NSOperationQueue* _delegateQueue;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    [self customInit];
  }
  return self;
}

- (void)dealloc {
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpClient::dealloc [ " << (__bridge void*)self << " ] ");
  if (self.sharedUrlSession) {
    [self cleanup];
  }
}

- (void)cleanup {
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpClient::cleanup [ " << (__bridge void*)self << " ] ");
  [self.sharedUrlSession finishTasksAndInvalidate];
  [self.urlSessions enumerateKeysAndObjectsUsingBlock:^(id key, id object, BOOL* stop) {
    NSURLSession* session = object;
    [session finishTasksAndInvalidate];
  }];
  [_delegateQueue cancelAllOperations];
  _sharedUrlSession = nil;
  [self.urlSessions removeAllObjects];
}

- (NSArray*)activeTasks {
  NSArray* tasks = nil;
  @synchronized(_tasks) {
    tasks = [NSArray arrayWithArray:_tasks.allValues];
  }
  return tasks;
}

- (void)customInit {
  _delegateQueue = [NSOperationQueue new];
  _delegateQueue.name = @"com.here.olp.networks.HttpClientSessionQueue";
  // TODO: Any expert to write lambda here?
  const olp::network::NetworkSystemConfig& sysConfig =
      olp::network::Network::SystemConfig().lockedCopy();
  _sharedUrlSession = [self urlSessionWithProxy:&(sysConfig.GetProxy()) headers:nil];
  _tasks = [NSMutableDictionary new];
  _idTaskMap = [NSMutableDictionary new];
  _urlSessions = [NSMutableDictionary new];
}

- (HttpTask*)createTaskWithId:(int)identifier {
  return [self createTaskWithId:identifier session:self.sharedUrlSession];
}

- (HttpTask*)createTaskWithId:(int)identifier session:(NSURLSession*)session {
  HttpTask* task = [[HttpTask alloc] initWithId:identifier httpClient:self];
  task.urlSession = session;
  if (session != self.sharedUrlSession) {
    self.urlSessions[@(identifier)] = session;
  }
  @synchronized(_tasks) {
    _tasks[@(identifier)] = task;
  }
  return task;
}

- (HttpTask*)taskWithId:(int)identifier {
  HttpTask* task;
  @synchronized(_tasks) {
    task = _tasks[@(identifier)];
  }
  return task;
}

- (HttpTask*)taskWithTaskIdentifier:(NSUInteger)taskId {
  HttpTask* task = nil;
  @synchronized(_tasks) {
    task = self.idTaskMap[@(taskId)];
  }
  return task;
}

- (void)cancelTaskWithId:(int)identifier {
  HttpTask* task = nil;
  @synchronized(_tasks) {
    task = _tasks[@(identifier)];

    if (!task) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "cancel unknown request " << identifier);
      return;
    }
  }

  if (task) [task cancel];
}

- (void)removeTaskWithId:(int)identifier {
  @synchronized(_tasks) {
    HttpTask* task = _tasks[@(identifier)];
    // cleanup idTaskMap
    if (task.dataTask) {
      [task.dataTask cancel];
      [self.idTaskMap removeObjectForKey:@(task.dataTask.taskIdentifier)];
    }
    [_tasks removeObjectForKey:@(identifier)];
    [self.urlSessions removeObjectForKey:@(identifier)];
  }
}

- (NSURLSession*)urlSessionWithProxy:(const olp::network::NetworkProxy*)proxy
                             headers:(NSDictionary*)headers {
  NSMutableDictionary* proxyDict = nil;
  if (proxy->IsValid()) {
    NSString* proxyName = [NSString stringWithUTF8String:proxy->Name().c_str()];
    if (proxyName.length) {
      proxyDict = [NSMutableDictionary new];
      NSUInteger port = (NSUInteger)proxy->Port();
      NSString* proxyType = (__bridge NSString*)kCFProxyTypeHTTPS;
      BOOL httpProxy = YES;
      if (olp::network::NetworkProxy::Type::Socks4 == proxy->ProxyType() ||
          olp::network::NetworkProxy::Type::Socks5 == proxy->ProxyType() ||
          olp::network::NetworkProxy::Type::Socks5Hostname == proxy->ProxyType()) {
        proxyType = (__bridge NSString*)kCFProxyTypeSOCKS;
        httpProxy = NO;
      }
      if (httpProxy) {
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPEnable] = @(1);
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPProxy] = proxyName;
        proxyDict[(__bridge NSString*)kCFNetworkProxiesHTTPPort] = @(port);

        proxyDict[@"HTTPSEnable"] = @(1);
        proxyDict[@"HTTPSProxy"] = proxyName;
        proxyDict[@"HTTPSPort"] = @(port);
      } else {
        proxyDict[(__bridge NSString*)kCFProxyTypeKey] = proxyType;
      }
      proxyDict[(__bridge NSString*)kCFProxyHostNameKey] = proxyName;
      proxyDict[(__bridge NSString*)kCFProxyPortNumberKey] = @(port);
      NSString* userName = [NSString stringWithUTF8String:proxy->UserName().c_str()];
      NSString* userPassword = [NSString stringWithUTF8String:proxy->UserPassword().c_str()];
      if (userName.length && userPassword.length) {
        proxyDict[(NSString*)kCFProxyUsernameKey] = userName;
        proxyDict[(NSString*)kCFProxyPasswordKey] = userPassword;
      }
    }
  }

  NSURLSessionConfiguration* config = [NSURLSessionConfiguration ephemeralSessionConfiguration];
  if (proxyDict) {
    config.connectionProxyDictionary = proxyDict;
  }
  if (headers.count) {
    config.HTTPAdditionalHeaders = headers;
  }

  return [NSURLSession sessionWithConfiguration:config delegate:self delegateQueue:_delegateQueue];
}

#pragma mark - NSURLSessionDataDelegate

- (void)URLSession:(NSURLSession*)session
                    task:(NSURLSessionTask*)task
    didCompleteWithError:(NSError*)error {
  if (!self.sharedUrlSession &&
      NSURLErrorCancelled != error.code) {  // Cleanup called and not cancelled
    return;
  }

  @autoreleasepool {
    HttpTask* httpTask = [self taskWithTaskIdentifier:task.taskIdentifier];
    if (httpTask && httpTask.urlSession) {
      [httpTask didCompleteWithError:error];
    }
  }
}

- (void)URLSession:(NSURLSession*)session
              dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveResponse:(NSURLResponse*)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    return;
  }

  @autoreleasepool {
    HttpTask* httpTask = [self taskWithTaskIdentifier:dataTask.taskIdentifier];
    if (httpTask && httpTask.urlSession) {
      [httpTask didReceiveResponse:response];
    }
    completionHandler(NSURLSessionResponseAllow);
  }
}

- (void)URLSession:(NSURLSession*)session
          dataTask:(NSURLSessionDataTask*)dataTask
    didReceiveData:(NSData*)data {
  if (!self.sharedUrlSession) {  // Cleanup called
    return;
  }

  @autoreleasepool {
    HttpTask* httpTask = [self taskWithTaskIdentifier:dataTask.taskIdentifier];
    if (httpTask && httpTask.urlSession) {
      [httpTask didReceiveData:data];
    }
  }
}

- (void)URLSession:(NSURLSession*)session
                   task:(NSURLSessionTask*)dataTask
    didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge
      completionHandler:
          (void (^)(NSURLSessionAuthChallengeDisposition, NSURLCredential*))completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    return;
  }

  @autoreleasepool {
    if ([challenge.protectionSpace.authenticationMethod
            isEqualToString:NSURLAuthenticationMethodServerTrust]) {
      if (dataTask) {
        HttpTask* httpTask = [self taskWithTaskIdentifier:dataTask.taskIdentifier];
        if (!httpTask.urlSession) {
          return;
        }
        // TODO: Any expert to write lambda here?
        const olp::network::NetworkSystemConfig& sysConfig =
            olp::network::Network::SystemConfig().lockedCopy();
        if (!sysConfig.DontVerifyCertificate()) {
          if (![self shouldTrustProtectionSpace:challenge.protectionSpace]) {
            completionHandler(NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
            return;
          }
        }
      }

      NSURLCredential* credential =
          [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
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
             completionHandler:(void (^)(NSURLRequest* _Nullable))completionHandler {
  if (!self.sharedUrlSession) {  // Cleanup called
    return;
  }

  NSURLRequest* originalRequest = task.originalRequest;
  NSString* authorizationHeaderValue = originalRequest.allHTTPHeaderFields[@"Authorization"];
  EDGE_SDK_LOG_TRACE(LOGTAG, "HttpClient::willPerformHTTPRedirection [ "
                                 << (__bridge void*)self << " ] status " << (int)response.statusCode
                                 << "\nOrigURL " << originalRequest.URL.absoluteString.UTF8String
                                 << "\nNewURL " << request.URL.absoluteString.UTF8String);
  NSMutableURLRequest* newRequest = [NSMutableURLRequest new];
  newRequest.URL = request.URL;
  newRequest.timeoutInterval = request.timeoutInterval;
  newRequest.cachePolicy = request.cachePolicy;
  newRequest.networkServiceType = request.networkServiceType;
  newRequest.HTTPMethod = request.HTTPMethod;
  newRequest.HTTPBody = request.HTTPBody;
  [request.allHTTPHeaderFields enumerateKeysAndObjectsUsingBlock:^(id key, id object, BOOL* stop) {
    [newRequest addValue:object forHTTPHeaderField:key];
  }];

  // NOTE: It appears that most headers are maintained during a redirect with the exception of the
  // `Authorization` header. It appears that Apple's strips the `Authorization` header from the
  // redirected URL request. If you need to maintain the `Authorization` header, you need to
  // manually append it to the redirected request.
  if (authorizationHeaderValue.length) {
    [newRequest addValue:authorizationHeaderValue forHTTPHeaderField:@"Authorization"];
  }
  completionHandler(newRequest);
}

#pragma mark - MOS certificate helper

// http://goo.gl/jmZ4Uv
- (BOOL)shouldTrustProtectionSpace:(NSURLProtectionSpace*)protectionSpace {
  if (!protectionSpace) {
    return NO;
  }

  const olp::network::NetworkSystemConfig& sysConfig =
      olp::network::Network::SystemConfig().lockedCopy();

  CABundleLoader* caLoader = nil;
  // If no path is provided, system CA storage is used in order to verify the serverTrust
  if (sysConfig.GetCertificatePath().length() > 0) {
    caLoader = [CABundleLoader sharedInstance];
    if (caLoader.caCertificateRefs.count == 0) {
      return NO;
    }
  }

  SecTrustRef serverTrust = protectionSpace.serverTrust;
  if (!serverTrust) {
    return NO;
  }

  // Anchor certificate to trust, if path provided
  if (sysConfig.GetCertificatePath().length() > 0) {
    SecTrustSetAnchorCertificates(serverTrust, (__bridge CFArrayRef)caLoader.caCertificateRefs);
  }

  // evaluate server trust against certificate
  SecTrustResultType trustResult = kSecTrustResultInvalid;
  OSStatus status = SecTrustEvaluate(serverTrust, &trustResult);

  if (errSecSuccess != status) {
    return NO;
  }

  return (trustResult == kSecTrustResultUnspecified || trustResult == kSecTrustResultProceed);
}

@end
