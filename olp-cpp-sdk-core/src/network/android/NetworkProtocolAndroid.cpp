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

#include "NetworkProtocolAndroid.h"

#include <olp/core/context/Context.h>
#include <olp/core/logging/Log.h>
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/network/NetworkResponse.h>
#include <olp/core/porting/make_unique.h>

#include <cassert>
#include <ctime>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace olp {
namespace network {
namespace {
struct StaticInitializer {
  StaticInitializer() {
    context::Context::addInitializeCallbacks(
        []() {
          network::NetworkProtocolAndroid::setJavaVm(
              olp::context::Context::getJavaVM(),
              context::Context::getAndroidContext());
        },
        []() {});
  }
};

static StaticInitializer s_initializer;

std::string doDateString(long timeStamp) {
  static const char* dateStr[] = {"Sun, ", "Mon, ", "Tue, ", "Wed, ",
                                  "Thu, ", "Fri, ", "Sat, "};
  static const char* monthStr[] = {" Jan ", " Feb ", " Mar ", " Apr ",
                                   " May ", " Jun ", " Jul ", " Aug ",
                                   " Sep ", " Oct ", " Nov ", " Dec "};

  time_t t = time_t(timeStamp);
  std::tm* ptm = gmtime(&t);
  std::ostringstream sstream;
  sstream << dateStr[ptm->tm_wday];
  sstream << std::dec << ptm->tm_mday;
  sstream << monthStr[ptm->tm_mon];
  sstream << std::dec << (1900 + ptm->tm_year);
  sstream << " ";
  sstream << std::dec << ptm->tm_hour;
  sstream << ":";
  sstream << std::dec << ptm->tm_min;
  sstream << ":";
  sstream << std::dec << ptm->tm_sec;
  sstream << " GMT";

  return sstream.str();
}

JavaVM* gVm = nullptr;
jclass gStringClass = nullptr;
jobject gClassLoader = nullptr;
jmethodID gFindClassMethod = 0;

std::mutex& getProtocolsMutex() {
  static std::mutex sProtocolsMutex;
  return sProtocolsMutex;
}

std::map<int, std::shared_ptr<NetworkProtocolAndroid> >& getProtocols() {
  static std::map<int, std::shared_ptr<NetworkProtocolAndroid> > sProtocols;
  return sProtocols;
}

struct ScopedLocalRef {
  ScopedLocalRef(JNIEnv* env, jstring string) : m_env(env), m_obj(string) {}

  ScopedLocalRef(JNIEnv* env, jobjectArray objArray)
      : m_env(env), m_obj(objArray) {}

  ScopedLocalRef(JNIEnv* env, jobject obj) : m_env(env), m_obj(obj) {}

  ~ScopedLocalRef() {
    if (m_obj) m_env->DeleteLocalRef(m_obj);
  }

  ScopedLocalRef(const ScopedLocalRef& other) = delete;
  ScopedLocalRef(ScopedLocalRef&& other) = delete;
  ScopedLocalRef& operator=(const ScopedLocalRef& other) = delete;
  ScopedLocalRef& operator=(ScopedLocalRef&& other) = delete;

  JNIEnv* m_env;
  jobject m_obj;
};

std::shared_ptr<NetworkProtocolAndroid> getProtocolForClient(int clientId) {
  std::shared_ptr<NetworkProtocolAndroid> protocol;
  std::lock_guard<std::mutex> lock(getProtocolsMutex());
  auto& protocols = getProtocols();
  auto it = protocols.find(clientId);
  if (it != protocols.end()) {
    protocol = it->second;
  }

  return protocol;
}

}  // namespace

#define LOGTAG "NETWORKANDROID"

void NetworkProtocolAndroid::setJavaVm(JavaVM* vm, jobject application) {
  if (gVm != nullptr) {
    EDGE_SDK_LOG_DEBUG(LOGTAG,
                       "setJavaVM previously called, no need to set it now");
    return;
  }

  gVm = vm;

  JNIEnv* env;
  if (gVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to get Java Env");
    return;
  }

  /* First get the class loader */
  jclass acl = env->GetObjectClass(application);
  if (!acl || env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "setJavaVm failed to get class for application object");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jmethodID getClassLoader =
      env->GetMethodID(acl, "getClassLoader", "()Ljava/lang/ClassLoader;");
  if (!getClassLoader || env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to get getClassLoader method");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jobject obj = env->CallObjectMethod(application, getClassLoader);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to get getClassLoader");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }
  gClassLoader = (jobject)env->NewGlobalRef(obj);

  jclass classLoader = env->FindClass("java/lang/ClassLoader");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to find getClassLoader");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  gFindClassMethod = env->GetMethodID(classLoader, "loadClass",
                                      "(Ljava/lang/String;)Ljava/lang/Class;");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to get loadClass method");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  /* Now get the String class */
  jstring className = env->NewStringUTF("java/lang/String");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to create class name string");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jclass clazz = env->GetObjectClass((jobject)className);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "setJavaVm failed to get String class");
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(className);
    env->ExceptionClear();
    return;
  }

  gStringClass = (jclass)env->NewGlobalRef(clazz);
  env->DeleteLocalRef(className);
}

NetworkProtocolAndroid::NetworkProtocolAndroid()
    : m_class(nullptr),
      m_jmidSend(0),
      m_jmidShutdown(0),
      m_obj(0),
      m_id(-1),
      m_started(false),
      m_tracker(false) {}

NetworkProtocolAndroid::~NetworkProtocolAndroid() { Deinitialize(); }

bool NetworkProtocolAndroid::Initialize() {
  std::unique_lock<std::mutex> lock(m_resultMutex);
  if (!gVm) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "initialize no Java VM");
    return false;
  }

  if (!gClassLoader || !gFindClassMethod || !gStringClass) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "initialize: setup not completed");
    return false;
  }

  JNIThreadBinder binder(gVm);
  JNIEnv* env = binder.m_env;
  if (env == nullptr) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "initialize failed to get Java Env");
    return false;
  }

  /* Get the Network protocol class */
  jstring className = env->NewStringUTF("com/here/olp/network/NetworkProtocol");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "initialize failed to create class name string");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jclass clazz = (jclass)(
      env->CallObjectMethod(gClassLoader, gFindClassMethod, className));
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "initialize failed to get NetworkProtocol");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  m_class = (jclass)env->NewGlobalRef(clazz);
  env->DeleteLocalRef(clazz);
  env->DeleteLocalRef(className);

  // Get registerClient method
  jmethodID jmidRegister = env->GetMethodID(m_class, "registerClient", "()I");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(
        LOGTAG, "initialize failed to get NetworkProtocol::registerClient");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  // Get send method
  m_jmidSend = env->GetMethodID(
      m_class, "send",
      "(Ljava/lang/String;IIIII[Ljava/lang/String;[BZLjava/lang/"
      "String;IILjava/lang/String;I)Lcom/here/olp/network/"
      "NetworkProtocol$GetTask;");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "initialize failed to get NetworkProtocol::send");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  // Get shutdown method
  m_jmidShutdown = env->GetMethodID(m_class, "shutdown", "()V");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "initialize failed to get NetworkProtocol::shutdown");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jmethodID jmidInit = env->GetMethodID(m_class, "<init>", "()V");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "Failed to get NetworkProtocol::NetworkProtocol");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jobject obj = env->NewObject(m_class, jmidInit);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to create NetworkProtocol");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  m_obj = env->NewGlobalRef(obj);
  env->DeleteLocalRef(obj);

  // Register the client
  m_id = env->CallIntMethod(m_obj, jmidRegister);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to call registerClient");
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteGlobalRef(m_obj);
    env->DeleteGlobalRef(m_class);
    m_obj = 0;
    m_class = 0;
    return false;
  }

  {
    std::lock_guard<std::mutex> gpLock(getProtocolsMutex());
    getProtocols()[m_id] = shared_from_this();
  }

  // Store the tracker used when initializing.
  m_tracker.Capture();

  m_thread = std::make_unique<std::thread>(NetworkProtocolAndroid::run, this);
  {
    if (!m_started) m_resultCondition.wait(lock);
  }

  return true;
}

void NetworkProtocolAndroid::Deinitialize() {
  {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    if (!m_started) return;

    m_started = false;
  }
  m_resultCondition.notify_all();

  if (m_thread) {
    if (m_thread->get_id() != std::this_thread::get_id()) {
      m_thread->join();
      m_thread.reset();
    } else {
      m_thread->detach();
      m_thread.reset();
    }
  }

  // Cancel all pending requests
  JNIThreadBinder env(gVm);
  if (env.m_env == nullptr) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "deinitialize failed to get Java Env");
    return;
  }

  std::shared_ptr<RequestCompletion> completion;
  std::vector<std::pair<Network::RequestId, Network::Callback> >
      completedMessages;
  {
    std::lock_guard<std::mutex> lock(m_requestMutex);
    if (!m_requests.empty()) {
      completion = std::make_shared<RequestCompletion>(m_requests.size());
      for (auto req : m_requests) {
        completedMessages.emplace_back(req.first, req.second->m_callback);
        req.second->m_completion = completion;
        doCancel(env.m_env, req.second->m_obj);
      }
    }

    // Empty result queue
    while (!m_results.empty()) {
      completedMessages.emplace_back(m_results.front().m_id,
                                     m_results.front().m_callback);
      m_results.pop();
    }

    env.m_env->CallVoidMethod(m_obj, m_jmidShutdown);
    if (env.m_env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to call shutdown");
      env.m_env->ExceptionDescribe();
      env.m_env->ExceptionClear();
    }

    if (m_obj) {
      env.m_env->DeleteGlobalRef(m_obj);
      m_obj = 0;
    }

    if (m_class) {
      env.m_env->DeleteGlobalRef(m_class);
      m_class = 0;
    }
  }

  if (!completedMessages.empty()) {
    for (auto& pair : completedMessages) {
      pair.second(NetworkResponse(pair.first, Network::Offline, "Offline"));
    }
  }

  if (completion) {
    if (completion->m_ready.get_future().wait_for(std::chrono::seconds(2)) !=
        std::future_status::ready) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Pending requests not ready in 2 seconds");
    }
  }

  {
    std::lock_guard<std::mutex> lock(getProtocolsMutex());
    auto& protocols = getProtocols();
    auto protocol = protocols.find(m_id);
    if (protocol != protocols.end()) {
      protocols.erase(protocol);
    }
  }

  m_tracker.Clear();
}

bool NetworkProtocolAndroid::Initialized() const { return m_started; }

bool NetworkProtocolAndroid::Ready() { return (m_requests.size() < 32); }

size_t NetworkProtocolAndroid::AmountPending() { return m_requests.size(); }

olp::network::NetworkProtocol::ErrorCode NetworkProtocolAndroid::Send(
    const olp::network::NetworkRequest& request, int id,
    const std::shared_ptr<std::ostream>& payload,
    std::shared_ptr<NetworkConfig> config,
    Network::HeaderCallback headerCallback, Network::DataCallback dataCallback,
    Network::Callback callback) {
  if (m_requests.size() >= 32)
    return olp::network::NetworkProtocol::ErrorNotReady;

  JNIThreadBinder env(gVm);
  if (env.m_env == nullptr) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Send failed to get Java Env");
    return olp::network::NetworkProtocol::ErrorIO;
  }

  if (!config->GetNetworkInterface().empty()) {
    return NetworkProtocol::ErrorNetworkInterfaceOptionNotImplemented;
  }

  if (!config->GetCaCert().empty()) {
    return NetworkProtocol::ErrorCaCertOptionNotImplemented;
  }

  std::string sysConfigCertificatePath;
  olp::network::NetworkProxy sysConfigProxy;
  bool sysDontVerifyCertificate;
  olp::network::Network::SystemConfig().locked(
      [&](const olp::network::NetworkSystemConfig& conf) {
        sysConfigCertificatePath = conf.GetCertificatePath();
        sysConfigProxy = conf.GetProxy();
        sysDontVerifyCertificate = conf.DontVerifyCertificate();
      });

  // Create a request that we keep in map until request has completed. Capture
  // the current tracker with the request so it can be restored when the request
  // is later fulfilled.
  std::shared_ptr<RequestData> req = std::make_shared<RequestData>(
      callback, headerCallback, dataCallback, request.Url(), payload);

  req->m_ignoreOffset = request.IgnoreOffset();

  // Convert the URL to jstring
  jstring url = env.m_env->NewStringUTF(request.Url().c_str());
  if (!url || env.m_env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create URI string");
    env.m_env->ExceptionDescribe();
    env.m_env->ExceptionClear();
    return olp::network::NetworkProtocol::ErrorIO;
  }
  ScopedLocalRef urlRef(env.m_env, url);

  std::vector<std::pair<std::string, std::string> > rangeHeaders;

  // Convert extra headers
  jobjectArray headers = createExtraHeaders(
      env.m_env, request.ExtraHeaders(), request.ModifiedSince(), rangeHeaders);
  if (env.m_env->ExceptionOccurred()) {
    env.m_env->ExceptionDescribe();
    env.m_env->ExceptionClear();
    return olp::network::NetworkProtocol::ErrorIO;
  }
  ScopedLocalRef headersRef(env.m_env, headers);

  // Get Post String (if any)
  jbyteArray postStr = 0;
  if (request.Verb() != olp::network::NetworkRequest::HttpVerb::GET &&
      request.Verb() != olp::network::NetworkRequest::HttpVerb::HEAD) {
    size_t size = 0;
    const unsigned char* postData = nullptr;
    auto content = request.Content();
    if (content && !content->empty()) {
      size = content->size();
      postData = &content->front();
    }
    postStr = env.m_env->NewByteArray(size);
    if (!postStr || env.m_env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create POST string");
      env.m_env->ExceptionDescribe();
      env.m_env->ExceptionClear();
      return olp::network::NetworkProtocol::ErrorIO;
    }
    env.m_env->SetByteArrayRegion(postStr, 0, size, (jbyte*)postData);
  }
  ScopedLocalRef postRef(env.m_env, postStr);

  jstring proxyStr = NULL;
  const NetworkProxy* proxy = &sysConfigProxy;
  if (config->Proxy().IsValid()) {
    proxy = &(config->Proxy());
  }
  if (proxy->IsValid()) {
    proxyStr = env.m_env->NewStringUTF(proxy->Name().c_str());
    if (!url || env.m_env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create proxy string");
      env.m_env->ExceptionDescribe();
      env.m_env->ExceptionClear();
      return olp::network::NetworkProtocol::ErrorIO;
    }
  }
  ScopedLocalRef proxyRef(env.m_env, proxyStr);
  int proxyPort = proxy->Port();
  int proxyType = static_cast<int>(proxy->ProxyType());

  // Convert the certificate path to jstring
  jstring certificatePath =
      env.m_env->NewStringUTF(sysConfigCertificatePath.c_str());
  if (!certificatePath || env.m_env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create certificate path string");
    env.m_env->ExceptionDescribe();
    env.m_env->ExceptionClear();
    return olp::network::NetworkProtocol::ErrorIO;
  }
  ScopedLocalRef certRef(env.m_env, certificatePath);

  // Add the request to request map
  {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    m_requests.insert(std::pair<int, std::shared_ptr<RequestData> >(id, req));
  }

  // Do sending
  jobject jobj = env.m_env->CallObjectMethod(
      m_obj, m_jmidSend, url, request.Verb(), m_id, id,
      config->ConnectTimeout(), config->TransferTimeout(), headers, postStr,
      sysDontVerifyCertificate, proxyStr, proxyPort, proxyType, certificatePath,
      config->GetRetries());
  if (env.m_env->ExceptionOccurred() || !jobj) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to call Send");
    env.m_env->ExceptionDescribe();
    env.m_env->ExceptionClear();
    {
      std::unique_lock<std::mutex> lock(m_requestMutex);
      auto request = m_requests.find(id);
      if (request != m_requests.end()) m_requests.erase(request);
    }
    return olp::network::NetworkProtocol::ErrorIO;
  }
  ScopedLocalRef objRef(env.m_env, jobj);
  req->m_obj = env.m_env->NewGlobalRef(jobj);

  return olp::network::NetworkProtocol::ErrorNone;
}

bool NetworkProtocolAndroid::Cancel(int id) {
  JNIThreadBinder env(gVm);
  if (env.m_env == nullptr) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Cancel failed to get Java Env");
    return false;
  }

  {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    auto result = m_requests.find(id);
    if (result == m_requests.end()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Cancel to unknown request " << id);
      return false;
    }

    doCancel(env.m_env, result->second->m_obj);

    env.m_env->DeleteGlobalRef(result->second->m_obj);
    result->second->m_obj = 0;

    m_cancelledRequests.push_back(id);
  }

  return true;
}

void NetworkProtocolAndroid::doCancel(JNIEnv* env, jobject object) {
  if (object == nullptr) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "AsyncTask object null");
    return;
  }

  jclass cls = env->GetObjectClass(object);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to get AsyncTask");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jmethodID mid = env->GetMethodID(cls, "cancelTask", "()V");
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to get AsyncTask::cancel");
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(cls);
    return;
  }
  env->DeleteLocalRef(cls);

  env->CallVoidMethod(object, mid);
  if (env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "NetworkProtocol::Request::cancel failed");
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
}

void NetworkProtocolAndroid::headersCallback(JNIEnv* env, int id,
                                             jobjectArray headers) {
  Network::HeaderCallback headerCallback;

  mem::MemoryScopeTracker tracker(false);
  {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    if (!m_started) return;

    auto req = m_requests.find(id);
    if (req == m_requests.end()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Headers to unknown request " << id);
      return;
    }
    headerCallback = req->second->m_headerCallback;
    tracker = req->second->m_tracker;
  }

  int headersCount = env->GetArrayLength(headers);
  if (headerCallback && (headersCount > 0)) {
    for (int i = 0; (i + 1) < headersCount; i += 2) {
      jstring hdrKey = (jstring)env->GetObjectArrayElement(headers, i);
      if (env->ExceptionOccurred()) {
        EDGE_SDK_LOG_ERROR(LOGTAG,
                           "headersCallback failed to get key for header");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
      }

      jstring hdrValue = (jstring)env->GetObjectArrayElement(headers, i + 1);
      if (env->ExceptionOccurred()) {
        EDGE_SDK_LOG_ERROR(LOGTAG,
                           "headersCallback failed to get value for header");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
      }

      // Restore the tracker used by the request.
      MEMORY_TRACKER_SCOPE(tracker);

      const char* rawKey = env->GetStringUTFChars(hdrKey, 0);
      const char* rawValue = env->GetStringUTFChars(hdrValue, 0);

      std::string key(rawKey);
      std::string value(rawValue);

      env->ReleaseStringUTFChars(hdrKey, rawKey);
      env->ReleaseStringUTFChars(hdrValue, rawValue);

      headerCallback(key, value);
    }
  }
}

void NetworkProtocolAndroid::dateAndOffsetCallback(JNIEnv* env, int id,
                                                   jlong date, jlong offset) {
  std::unique_lock<std::mutex> lock(m_requestMutex);
  if (!m_started) return;

  auto request = m_requests.find(id);
  if (request == m_requests.end()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Date and offset to unknown request " << id);
    return;
  }
  request->second->m_date = date;
  if (request->second->m_resume)
    request->second->m_count = offset - request->second->m_offset;
  else
    request->second->m_offset = offset;
}

void NetworkProtocolAndroid::dataReceived(JNIEnv* env, int id, jbyteArray data,
                                          int len) {
  std::shared_ptr<RequestData> request;

  {
    std::unique_lock<std::mutex> lock(m_requestMutex);
    if (!m_started) return;

    auto req = m_requests.find(id);
    if (req == m_requests.end()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Data to unknown request " << id);
      return;
    }
    request = req->second;
  }

  // Restore the tracker associated with the request.
  MEMORY_TRACKER_SCOPE(request->m_tracker);

  jbyte* dataBytes = env->GetByteArrayElements(data, NULL);
  if (request->m_payload) {
    if (!request->m_ignoreOffset) {
      if (request->m_payload->tellp() != std::streampos(request->m_count)) {
        request->m_payload->seekp(request->m_count);
        if (request->m_payload->fail()) {
          EDGE_SDK_LOG_WARNING(
              LOGTAG, "Reception stream doesn't support setting write point");
          request->m_payload->clear();
        }
      }
    }

    request->m_payload->write((const char*)dataBytes, len);
  }

  if (request->m_dataCallback)
    request->m_dataCallback(request->m_offset + request->m_count,
                            reinterpret_cast<uint8_t*>(dataBytes), len);

  env->ReleaseByteArrayElements(data, dataBytes, 0);

  request->m_count += len;
}

void NetworkProtocolAndroid::completeRequest(JNIEnv* env, int id, int status,
                                             jstring error, int maxAge,
                                             int expires, jstring etag,
                                             jstring contentType) {
  std::unique_lock<std::mutex> lock(m_requestMutex);
  auto request = m_requests.find(id);
  if (request == m_requests.end()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Complete to unknown request " << id);
    return;
  }

  // We don't need the object anymore
  env->DeleteGlobalRef(request->second->m_obj);
  request->second->m_obj = 0;

  if (request->second->m_completion) {
    std::shared_ptr<RequestCompletion> completion =
        request->second->m_completion;
    if (--completion->m_count == 0) completion->m_ready.set_value();
    request->second->m_completion = nullptr;
    m_requests.erase(request);
    return;
  }

  // Restore the tracker associated with the request.
  MEMORY_TRACKER_SCOPE(request->second->m_tracker);

  // Partial response is OK if offset is 0
  if ((request->second->m_offset == 0) && (status == 206)) status = 200;

  // Copy the error string
  const char* errorBytes = env->GetStringUTFChars(error, NULL);
  // Copy the etag string
  const char* etagBytes = env->GetStringUTFChars(etag, NULL);
  // Copy the content type string
  const char* contentTypeBytes = env->GetStringUTFChars(contentType, NULL);
  // Create a result
  ResultData result(id, request->second->m_callback, status, maxAge, expires,
                    errorBytes, etagBytes, contentTypeBytes,
                    request->second->m_count, request->second->m_offset,
                    request->second->m_payload);
  env->ReleaseStringUTFChars(error, errorBytes);
  env->ReleaseStringUTFChars(etag, etagBytes);
  env->ReleaseStringUTFChars(contentType, contentTypeBytes);

  // Remove the request from the map
  m_requests.erase(request);
  lock.unlock();

  // Complete the request
  {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_results.push(result);
  }
  m_resultCondition.notify_all();
}

void NetworkProtocolAndroid::resetRequest(JNIEnv* env, int id) {
  std::lock_guard<std::mutex> lock(m_requestMutex);
  if (!m_started) return;

  auto req = m_requests.find(id);
  if (req == m_requests.end()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Reset of unknown request " << id);
    return;
  }
  req->second->reinitialize();
}

jobjectArray NetworkProtocolAndroid::createExtraHeaders(
    JNIEnv* env,
    const std::vector<std::pair<std::string, std::string> >& extraHeaders,
    std::uint64_t modifiedSince,
    const std::vector<std::pair<std::string, std::string> >& rangeHeaders) {
  if ((extraHeaders.size() == 0) && (modifiedSince == 0) &&
      (rangeHeaders.size() == 0))
    return 0;

  jstring jemptyString = env->NewStringUTF("");
  if (!jemptyString || env->ExceptionOccurred()) {
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to create an empty string");
    return 0;
  }

  size_t headerCount = extraHeaders.size() + rangeHeaders.size() +
                       ((modifiedSince != 0) ? 1 : 0);
  jobjectArray headers = (jobjectArray)env->NewObjectArray(
      headerCount * 2, gStringClass, jemptyString);
  if (!headers || env->ExceptionOccurred()) {
    env->DeleteLocalRef(jemptyString);
    EDGE_SDK_LOG_ERROR(LOGTAG, "Failed to create string array for headers");
    return 0;
  }
  env->DeleteLocalRef(jemptyString);

  for (size_t i = 0; i < extraHeaders.size(); i++) {
    jstring name = env->NewStringUTF(extraHeaders[i].first.c_str());
    if (!name || env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create extra header name string");
      return 0;
    }
    ScopedLocalRef nameRef(env, name);

    jstring value = env->NewStringUTF(extraHeaders[i].second.c_str());
    if (!value || env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create extra header value string");
      return 0;
    }
    ScopedLocalRef valueRef(env, value);

    env->SetObjectArrayElement(headers, i * 2, name);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set extra header value string");
      return 0;
    }
    env->SetObjectArrayElement(headers, i * 2 + 1, value);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set extra header value string");
      return 0;
    }
  }

  size_t index = extraHeaders.size() * 2;
  for (size_t i = 0; i < rangeHeaders.size(); i++) {
    jstring name = env->NewStringUTF(rangeHeaders[i].first.c_str());
    if (!name || env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create range header name string");
      return 0;
    }
    ScopedLocalRef nameRef(env, name);

    jstring value = env->NewStringUTF(rangeHeaders[i].second.c_str());
    if (!value || env->ExceptionOccurred()) {
      env->DeleteLocalRef(name);
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create range header value string");
      return 0;
    }
    ScopedLocalRef valueRef(env, value);

    env->SetObjectArrayElement(headers, index + (i * 2), name);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set range header value string");
      return 0;
    }
    env->SetObjectArrayElement(headers, index + (i * 2) + 1, value);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set range header value string");
      return 0;
    }
  }

  index = (extraHeaders.size() + rangeHeaders.size()) * 2;
  if (modifiedSince != 0) {
    jstring name = env->NewStringUTF("If-Modified-Since");
    if (!name || env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create extra header name string");
      return 0;
    }
    ScopedLocalRef nameRef(env, name);

    std::string dateStr = doDateString(modifiedSince);

    jstring value = env->NewStringUTF(dateStr.c_str());
    if (!value || env->ExceptionOccurred()) {
      env->DeleteLocalRef(name);
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to create extra header value string");
      return 0;
    }
    ScopedLocalRef valueRef(env, value);

    env->SetObjectArrayElement(headers, index, name);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set extra header value string");
      return 0;
    }
    env->SetObjectArrayElement(headers, index + 1, value);
    if (env->ExceptionOccurred()) {
      EDGE_SDK_LOG_ERROR(LOGTAG, "Send to set extra header value string");
      return 0;
    }
  }

  return headers;
}

void NetworkProtocolAndroid::run(NetworkProtocolAndroid* self) {
  JNIEnv* env;
  bool attached = false;
  if (gVm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    if (JNI_OK == gVm->AttachCurrentThread(&env, nullptr)) {
      attached = true;
    }
  }

  // Restore the tracker used to initialize the instance.
  MEMORY_TRACKER_SCOPE(self->m_tracker);
  self->selfRun();

  if (attached) {
    gVm->DetachCurrentThread();
  }
}

void NetworkProtocolAndroid::selfRun() {
  // Hold a shared pointer so that object is not destroyed while thread is
  // running
  std::shared_ptr<NetworkProtocolAndroid> that = shared_from_this();
  {
    std::lock_guard<std::mutex> lock(m_resultMutex);
    m_started = true;
    m_resultCondition.notify_all();
  }

  while (that->m_started) {
    ResultData result;
    {
      std::unique_lock<std::mutex> lock(m_resultMutex);
      while (m_started && m_results.empty()) m_resultCondition.wait(lock);

      if (!m_started) return;

      if (!m_results.empty()) {
        result = m_results.front();
        m_results.pop();
      }
    }
    if (result.isValid()) {
      bool cancelled = false;
      auto it = that->m_cancelledRequests.begin();
      while (it != that->m_cancelledRequests.end()) {
        if (*it == result.m_id) {
          cancelled = true;
          that->m_cancelledRequests.erase(it);
          break;
        }
        ++it;
      }

      auto response =
          NetworkResponse(result.m_id, cancelled, result.m_status,
                          result.m_error, result.m_maxAge, result.m_expires,
                          result.m_etag, result.m_contentType, result.m_count,
                          result.m_offset, result.m_payload, {});
      result.m_callback(std::move(response));
    }
  }
}

NetworkProtocolAndroid::RequestData::RequestData(
    Network::Callback callback, Network::HeaderCallback headerCallback,
    Network::DataCallback dataCallback, const std::string& url,
    const std::shared_ptr<std::ostream>& payload)
    : m_callback(callback),
      m_headerCallback(headerCallback),
      m_dataCallback(dataCallback),
      m_url(url),
      m_obj(nullptr),
      m_date(-1),
      m_count(0),
      m_offset(0),
      m_resume(false),
      m_ignoreOffset(false),
      m_payload(payload),
      m_completion(nullptr) {}

NetworkProtocolAndroid::ResultData::ResultData(
    Network::RequestId id, Network::Callback callback, int status, int maxAge,
    int expires, const char* error, const char* etag, const char* contentType,
    jlong count, jlong offset, std::shared_ptr<std::ostream> payload)
    : m_id(id),
      m_callback(callback),
      m_payload(payload),
      m_error(error),
      m_etag(etag),
      m_contentType(contentType),
      m_status(status),
      m_maxAge(maxAge),
      m_expires(expires),
      m_count(count),
      m_offset(offset) {}

}  // namespace network
}  // namespace olp

#ifndef __clang__
#define EDGE_SDK_NETWORK_EXPORT __attribute__((externally_visible)) JNIEXPORT
#else
#define EDGE_SDK_NETWORK_EXPORT JNIEXPORT
#endif

/*
 * Callback to be called when response headers have been received
 */
extern "C" EDGE_SDK_NETWORK_EXPORT void JNICALL
Java_com_here_olp_network_NetworkProtocol_headersCallback(
    JNIEnv* env, jobject obj, jint clientId, jint requestId,
    jobjectArray headers) {
  auto protocol = olp::network::getProtocolForClient(clientId);
  if (!protocol) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "headersCallback to non-existing client: " << clientId);
    return;
  }
  protocol->headersCallback(env, requestId, headers);
}

/*
 * Callback to be called when a date header is received
 */
extern "C" EDGE_SDK_NETWORK_EXPORT void JNICALL
Java_com_here_olp_network_NetworkProtocol_dateAndOffsetCallback(
    JNIEnv* env, jobject obj, jint clientId, jint requestId, jlong date,
    jlong offset) {
  auto protocol = olp::network::getProtocolForClient(clientId);
  if (!protocol) {
    EDGE_SDK_LOG_ERROR(
        LOGTAG, "dateAndOffsetCallback to non-existing client: " << clientId);
    return;
  }
  protocol->dateAndOffsetCallback(env, requestId, date, offset);
}

/*
 * Callback to be called when a chunk of data is received
 */
extern "C" EDGE_SDK_NETWORK_EXPORT void JNICALL
Java_com_here_olp_network_NetworkProtocol_dataCallback(JNIEnv* env, jobject obj,
                                                       jint clientId,
                                                       jint requestId,
                                                       jbyteArray data,
                                                       jint len) {
  auto protocol = olp::network::getProtocolForClient(clientId);
  if (!protocol) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "dataCallback to non-existing client: " << clientId);
    return;
  }
  protocol->dataReceived(env, requestId, data, len);
}

/*
 * Callback to be called when a request is completed
 */
extern "C" EDGE_SDK_NETWORK_EXPORT void JNICALL
Java_com_here_olp_network_NetworkProtocol_completeRequest(
    JNIEnv* env, jobject obj, jint clientId, jint requestId, jint status,
    jstring error, jint maxAge, jint expires, jstring etag,
    jstring contentType) {
  auto protocol = olp::network::getProtocolForClient(clientId);
  if (!protocol) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "completeRequest to non-existing client: " << clientId);
    return;
  }
  protocol->completeRequest(env, requestId, status, error, maxAge, expires,
                            etag, contentType);
}

/*
 * Reset request upon retry
 */
extern "C" EDGE_SDK_NETWORK_EXPORT void JNICALL
Java_com_here_olp_network_NetworkProtocol_resetRequest(JNIEnv* env, jobject obj,
                                                       jint clientId,
                                                       jint requestId) {
  auto protocol = olp::network::getProtocolForClient(clientId);
  if (!protocol) {
    EDGE_SDK_LOG_ERROR(LOGTAG,
                       "resetRequest to non-existing client: " << clientId);
    return;
  }
  protocol->resetRequest(env, requestId);
}
