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

#include "NetworkAndroid.h"

#include <ctime>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "olp/core/context/Context.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"

#include "utils/JNIScopedLocalReference.h"
#include "utils/JNIThreadBinder.h"

namespace {
constexpr auto kLogTag = "NetworkAndroid";
}
namespace olp {
namespace http {

namespace {

struct StaticInitializer {
  StaticInitializer() {
    olp::context::Context::addInitializeCallbacks(
        []() {
          olp::http::NetworkAndroid::SetJavaVM(
              olp::context::Context::getJavaVM(),
              olp::context::Context::getAndroidContext());
        },
        []() {});
  }
};

static StaticInitializer s_initializer;

JavaVM* gJavaVM = nullptr;
jclass gStringClass = nullptr;
jobject gClassLoader = nullptr;
jmethodID gFindClassMethod = 0;
jfieldID gJniNativePtrField = 0;

olp::http::NetworkAndroid* GetNetworkAndroidNativePtr(JNIEnv* env,
                                                      jobject http_client) {
  jlong jnative_ptr = env->GetLongField(http_client, gJniNativePtrField);
  return reinterpret_cast<olp::http::NetworkAndroid*>(jnative_ptr);
}

}  // namespace

#ifndef __clang__
#define OLP_SDK_NETWORK_ANDROID_EXPORT \
  __attribute__((externally_visible)) JNIEXPORT
#else
#define OLP_SDK_NETWORK_ANDROID_EXPORT JNIEXPORT
#endif
/*
 * Callback to be called when response headers have been received
 */
extern "C" OLP_SDK_NETWORK_ANDROID_EXPORT void JNICALL
Java_com_here_olp_network_HttpClient_headersCallback(JNIEnv* env, jobject obj,
                                                     jlong request_id,
                                                     jobjectArray headers) {
  auto network = olp::http::GetNetworkAndroidNativePtr(env, obj);
  if (!network) {
    OLP_SDK_LOG_WARNING(
        kLogTag, "HeadersCallback failed - network is invalid, request_id="
                     << request_id);
    return;
  }
  network->HeadersCallback(env, request_id, headers);
}

/*
 * Callback to be called when a date header is received
 */
extern "C" OLP_SDK_NETWORK_ANDROID_EXPORT void JNICALL
Java_com_here_olp_network_HttpClient_dateAndOffsetCallback(
    JNIEnv* env, jobject obj, jlong request_id, jlong date, jlong offset) {
  auto network = olp::http::GetNetworkAndroidNativePtr(env, obj);
  if (!network) {
    OLP_SDK_LOG_WARNING(
        kLogTag,
        "DateAndOffsetCallback failed - network is invalid, request_id="
            << request_id);
    return;
  }
  network->DateAndOffsetCallback(env, request_id, date, offset);
}

/*
 * Callback to be called when a chunk of data is received
 */
extern "C" OLP_SDK_NETWORK_ANDROID_EXPORT void JNICALL
Java_com_here_olp_network_HttpClient_dataCallback(JNIEnv* env, jobject obj,
                                                  jlong request_id,
                                                  jbyteArray data, jint len) {
  auto network = olp::http::GetNetworkAndroidNativePtr(env, obj);
  if (!network) {
    OLP_SDK_LOG_WARNING(
        kLogTag,
        "DataCallback failed - network is invalid, request_id=" << request_id);
    return;
  }
  network->DataReceived(env, request_id, data, len);
}

/*
 * Callback to be called when a request is completed
 */
extern "C" OLP_SDK_NETWORK_ANDROID_EXPORT void JNICALL
Java_com_here_olp_network_HttpClient_completeRequest(
    JNIEnv* env, jobject obj, jlong request_id, jint status,
    jint uploaded_bytes, jint downloaded_bytes, jstring error,
    jstring content_type) {
  auto network = olp::http::GetNetworkAndroidNativePtr(env, obj);
  if (!network) {
    OLP_SDK_LOG_WARNING(
        kLogTag, "CompleteRequest failed - network is invalid, request_id="
                     << request_id);
    return;
  }
  network->CompleteRequest(env, request_id, status, uploaded_bytes,
                           downloaded_bytes, error, content_type);
}

/*
 * Reset request upon retry
 */
extern "C" OLP_SDK_NETWORK_ANDROID_EXPORT void JNICALL
Java_com_here_olp_network_HttpClient_resetRequest(JNIEnv* env, jobject obj,
                                                  jlong request_id) {
  auto network = olp::http::GetNetworkAndroidNativePtr(env, obj);
  if (!network) {
    OLP_SDK_LOG_WARNING(
        kLogTag,
        "ResetRequest failed - network is invalid, request_id=" << request_id);
    return;
  }
  network->ResetRequest(env, request_id);
}

NetworkAndroid::NetworkAndroid(size_t max_requests_count)
    : java_self_class_(nullptr),
      jni_send_method_(nullptr),
      java_shutdown_method_(nullptr),
      obj_(nullptr),
      started_(false),
      initialized_(false),
      max_requests_count_(max_requests_count) {
  OLP_SDK_LOG_TRACE(kLogTag, "Created NetworkAndroid, this="
                                 << this
                                 << ", requests_count=" << max_requests_count);
}

NetworkAndroid::~NetworkAndroid() {
  OLP_SDK_LOG_TRACE(kLogTag, "Destroyed NetworkAndroid, this=" << this);
  Deinitialize();
}

void NetworkAndroid::SetJavaVM(JavaVM* vm, jobject application) {
  if (gJavaVM != nullptr) {
    OLP_SDK_LOG_DEBUG(kLogTag, "SetJavaVM is already set");
    return;
  }

  gJavaVM = vm;

  JNIEnv* env;
  if (gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) !=
      JNI_OK) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to get JNIEnv");
    return;
  }

  // Find the ClassLoader java class
  jclass application_class = env->GetObjectClass(application);
  if (!application_class || env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to get application's java class");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jmethodID java_get_class_loader_method = env->GetMethodID(
      application_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
  if (!java_get_class_loader_method || env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to get getClassLoader method");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jobject class_loader =
      env->CallObjectMethod(application, java_get_class_loader_method);
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed during call to getClassLoader");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }
  gClassLoader = (jobject)env->NewGlobalRef(class_loader);

  jclass class_loader_class = env->FindClass("java/lang/ClassLoader");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to find ClassLoader");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  // Find the loadClass method of ClassLoader
  gFindClassMethod = env->GetMethodID(class_loader_class, "loadClass",
                                      "(Ljava/lang/String;)Ljava/lang/Class;");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to get loadClass method");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  // Find the java.lang.String class
  jstring string_class_name = env->NewStringUTF("java/lang/String");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to create String class name");
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jclass string_class = env->GetObjectClass((jobject)string_class_name);
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to get String class");
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(string_class_name);
    env->ExceptionClear();
    return;
  }

  gStringClass = (jclass)env->NewGlobalRef(string_class);
  env->DeleteLocalRef(string_class_name);
}

bool NetworkAndroid::Initialize() {
  std::unique_lock<std::mutex> lock(responses_mutex_);
  if (initialized_) {
    return true;
  }

  if (!gJavaVM) {
    OLP_SDK_LOG_ERROR(kLogTag, "Initialize failed - no Java VM, this=" << this);
    return false;
  }

  if (!gClassLoader || !gFindClassMethod || !gStringClass) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - JNI methods are not initiliazed, this=" << this);
    return false;
  }

  utils::JNIThreadBinder binder(gJavaVM);
  JNIEnv* env = binder.GetEnv();
  if (env == nullptr) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - can't retrieve JNIEnv ojbect, this=" << this);
    return false;
  }

  // Get corresponding HttpClient class
  jstring network_class_name =
      env->NewStringUTF("com/here/olp/network/HttpClient");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - couldn't create HttpClient string, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jclass network_class = (jclass)(env->CallObjectMethod(
      gClassLoader, gFindClassMethod, network_class_name));
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - can't get HttpClient class, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  java_self_class_ = (jclass)env->NewGlobalRef(network_class);
  env->DeleteLocalRef(network_class);
  env->DeleteLocalRef(network_class_name);

  // Get shutdown method
  java_shutdown_method_ = env->GetMethodID(java_self_class_, "shutdown", "()V");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - can't get HttpClient.shutdown, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jmethodID java_init_method =
      env->GetMethodID(java_self_class_, "<init>", "()V");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - can't get HttpClient ctor, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  jobject obj = env->NewObject(java_self_class_, java_init_method);
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - failed to create HttpClient, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }
  obj_ = env->NewGlobalRef(obj);
  env->DeleteLocalRef(obj);

  // Get send method
  jni_send_method_ = env->GetMethodID(
      java_self_class_, "send",
      "(Ljava/lang/String;IJII[Ljava/lang/String;[BLjava/lang/String;III)Lcom/"
      "here/olp/network/HttpClient$HttpTask;");

  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Initialize failed - can't get HttpClient.send, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
  }

  // Get the 'nativePtr 'field and initialize it with current pointer
  if (!gJniNativePtrField) {
    gJniNativePtrField = env->GetFieldID(java_self_class_, "nativePtr", "J");
    if (env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(
          kLogTag,
          "Initialize failed - can't get HttpClient.nativePtr, this=" << this);
      env->ExceptionDescribe();
      env->ExceptionClear();
      return false;
    }
  }
  env->SetLongField(obj_, gJniNativePtrField, (jlong)this);

  static bool natives_registered = false;
  if (!natives_registered) {
    JNINativeMethod methods[] = {
        {"headersCallback", "(J[Ljava/lang/String;)V",
         (void*)&Java_com_here_olp_network_HttpClient_headersCallback},
        {"dateAndOffsetCallback", "(JJJ)V",
         (void*)&Java_com_here_olp_network_HttpClient_dateAndOffsetCallback},
        {"dataCallback", "(J[BI)V",
         (void*)&Java_com_here_olp_network_HttpClient_dataCallback},
        {"completeRequest", "(JIIILjava/lang/String;Ljava/lang/String;)V",
         (void*)&Java_com_here_olp_network_HttpClient_completeRequest},
        {"resetRequest", "(J)V",
         (void*)&Java_com_here_olp_network_HttpClient_resetRequest}};

    env->RegisterNatives(java_self_class_, methods,
                         sizeof(methods) / sizeof(methods[0]));
    if (env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(
          kLogTag,
          "Initialize failed - can't register native methods, this=" << this);
      env->ExceptionDescribe();
      env->ExceptionClear();
      return false;
    }

    natives_registered = true;
  }

  run_thread_ = std::make_unique<std::thread>(NetworkAndroid::Run, this);

  initialized_ = true;

  {
    if (!started_) {
      run_thread_ready_cv_.wait(lock);
    }
  }

  return true;
}

void NetworkAndroid::Deinitialize() {
  {
    std::lock_guard<std::mutex> lock(responses_mutex_);
    if (!initialized_ || !started_) {
      return;
    }

    started_ = false;
    initialized_ = false;
  }

  OLP_SDK_LOG_TRACE(kLogTag, "Deinitialize network, this=" << this);

  run_thread_ready_cv_.notify_all();

  // Finish run thread:
  if (run_thread_) {
    run_thread_->join();
    run_thread_.reset();
  }

  // Cancel all pending requests
  utils::JNIThreadBinder env(gJavaVM);
  if (env.GetEnv() == nullptr) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Deinitialize failed - can't get Java Env, this=" << this);
    return;
  }

  std::shared_ptr<RequestCompletion> completion;
  std::vector<std::pair<RequestId, Network::Callback> > completed_messages;
  {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    if (!requests_.empty()) {
      completion = std::make_shared<RequestCompletion>(requests_.size());
      for (auto req : requests_) {
        completed_messages.emplace_back(req.first, req.second->callback);
        req.second->completion = completion;
        DoCancel(env.GetEnv(), req.second->obj);
      }
    }
  }

  // Empty reponses queue
  while (!responses_.empty()) {
    completed_messages.emplace_back(responses_.front().id,
                                    responses_.front().callback);
    responses_.pop();
  }

  env.GetEnv()->CallVoidMethod(obj_, java_shutdown_method_);
  if (env.GetEnv()->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Failed to call shutdown");
    env.GetEnv()->ExceptionDescribe();
    env.GetEnv()->ExceptionClear();
  }

  if (obj_) {
    env.GetEnv()->DeleteGlobalRef(obj_);
    obj_ = 0;
  }

  if (java_self_class_) {
    env.GetEnv()->DeleteGlobalRef(java_self_class_);
    java_self_class_ = nullptr;
  }

  for (auto& pair : completed_messages) {
    if (auto callback = pair.second) {
      callback(NetworkResponse()
                   .WithRequestId(pair.first)
                   .WithStatus(static_cast<int>(ErrorCode::OFFLINE_ERROR))
                   .WithError("Offline: network client is destroyed"));
    }
  }

  // TODO: replace without explicit waiting for specific amount of seconds
  if (completion) {
    if (completion->ready.get_future().wait_for(std::chrono::seconds(2)) !=
        std::future_status::ready) {
      OLP_SDK_LOG_WARNING(kLogTag,
                          "Requests are not ready in 2 seconds, this=" << this);
    }
  }
}

void NetworkAndroid::DoCancel(JNIEnv* env, jobject object) {
  if (object == nullptr) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "DoCancel failed - HttpTask object is null, this=" << this);
    return;
  }

  jclass cls = env->GetObjectClass(object);
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "DoCancel failed - can't get HttpTask class, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    return;
  }

  jmethodID mid = env->GetMethodID(cls, "cancelTask", "()V");
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "DoCancel failed - can't get HttpTask.cancel method, this=" << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
    env->DeleteLocalRef(cls);
    return;
  }
  env->DeleteLocalRef(cls);

  env->CallVoidMethod(object, mid);
  if (env->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "DoCancel failed - HttpClient.Request.cancel call failed, this="
            << this);
    env->ExceptionDescribe();
    env->ExceptionClear();
  }
}

void NetworkAndroid::HeadersCallback(JNIEnv* env, RequestId request_id,
                                     jobjectArray headers) {
  Network::HeaderCallback header_callback;
  {
    std::unique_lock<std::mutex> lock(requests_mutex_);
    if (!started_) return;

    auto req = requests_.find(request_id);
    if (req == requests_.end()) {
      OLP_SDK_LOG_ERROR(kLogTag,
                        "Headers to unknown request with id=" << request_id);
      return;
    }
    header_callback = req->second->header_callback;
  }

  int header_count = env->GetArrayLength(headers);
  // Retrieve headers' key and value and call corresponding header callback
  if (header_callback && (header_count > 0)) {
    for (int i = 0; (i + 1) < header_count; i += 2) {
      jstring header_key = (jstring)env->GetObjectArrayElement(headers, i);
      if (env->ExceptionOccurred()) {
        OLP_SDK_LOG_ERROR(
            kLogTag,
            "Failed to get key of the header for request_id=" << request_id);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
      }

      jstring header_value =
          (jstring)env->GetObjectArrayElement(headers, i + 1);
      if (env->ExceptionOccurred()) {
        OLP_SDK_LOG_ERROR(
            kLogTag,
            "Failed to get value of the header for request_id=" << request_id);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
      }

      const char* raw_key = env->GetStringUTFChars(header_key, 0);
      const char* raw_value = env->GetStringUTFChars(header_value, 0);

      std::string key(raw_key);
      std::string value(raw_value);

      // Release temporary created c-strings:
      env->ReleaseStringUTFChars(header_key, raw_key);
      env->ReleaseStringUTFChars(header_value, raw_value);

      header_callback(key, value);
    }
  }
}

void NetworkAndroid::DateAndOffsetCallback(JNIEnv* env, RequestId request_id,
                                           jlong date, jlong offset) {
  std::unique_lock<std::mutex> lock(requests_mutex_);
  if (!started_) return;

  auto request = requests_.find(request_id);
  if (request == requests_.end()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Date and offset to unknown request with id=" << request_id);
    return;
  }
  request->second->offset = offset;
}

void NetworkAndroid::DataReceived(JNIEnv* env, RequestId request_id,
                                  jbyteArray data, int len) {
  std::shared_ptr<RequestData> request;

  {
    std::unique_lock<std::mutex> lock(requests_mutex_);
    if (!started_) return;

    auto req = requests_.find(request_id);
    if (req == requests_.end()) {
      OLP_SDK_LOG_WARNING(
          kLogTag, "DataReceived failed - unknown request_id=" << request_id);
      return;
    }
    request = req->second;
  }

  OLP_SDK_LOG_TRACE(
      kLogTag, "Received " << len << " bytes for request_id=" << request_id);

  jbyte* jdata = env->GetByteArrayElements(data, NULL);
  if (auto payload = request->payload) {
    if (payload->tellp() != std::streampos(request->count)) {
      payload->seekp(request->count);
      if (payload->fail()) {
        OLP_SDK_LOG_WARNING(
            kLogTag, "Payload seekp() failed, request_id=" << request_id);
        payload->clear();
      }
    }

    payload->write(reinterpret_cast<const char*>(jdata), len);
  }

  if (request->data_callback)
    request->data_callback(reinterpret_cast<uint8_t*>(jdata),
                           request->offset + request->count, len);

  env->ReleaseByteArrayElements(data, jdata, 0);

  request->count += len;
}

void NetworkAndroid::CompleteRequest(JNIEnv* env, RequestId request_id,
                                     int status, int uploaded_bytes,
                                     int downloaded_bytes, jstring error,
                                     jstring jcontent_type) {
  std::unique_lock<std::mutex> lock(requests_mutex_);
  auto iter_request = requests_.find(request_id);
  if (iter_request == requests_.end()) {
    OLP_SDK_LOG_WARNING(
        kLogTag,
        "CompleteRequest is received with unknown request_id=" << request_id);
    return;
  }

  auto request_data = iter_request->second;
  OLP_SDK_LOG_DEBUG(
      kLogTag, "CompleteRequest, request_id="
                   << request_id << ",  uploaded_bytes=" << uploaded_bytes
                   << ",  downloaded_bytes=" << downloaded_bytes
                   << ", url=" << request_data->url << ", status=" << status);
  // We don't need the object anymore
  env->DeleteGlobalRef(request_data->obj);
  request_data->obj = nullptr;

  if (auto completion = request_data->completion) {
    if (--completion->count == 0) {
      completion->ready.set_value();
    }
    request_data->completion = nullptr;
    requests_.erase(iter_request);
    return;
  }

  // Partial response is OK if offset is 0
  if ((request_data->offset == 0) && (status == 206)) {
    status = 200;
  }

  // Copy the error string
  const char* error_data = env->GetStringUTFChars(error, NULL);
  // Copy the content type string
  const char* content_type_data = env->GetStringUTFChars(jcontent_type, NULL);
  // Create a response data
  ResponseData response_data(request_id, request_data->callback, status,
                             uploaded_bytes, downloaded_bytes, error_data,
                             content_type_data, request_data->count,
                             request_data->offset, request_data->payload);

  env->ReleaseStringUTFChars(error, error_data);
  env->ReleaseStringUTFChars(jcontent_type, content_type_data);

  // Remove the request from the map
  requests_.erase(iter_request);
  lock.unlock();

  // Complete the request
  {
    std::lock_guard<std::mutex> lock(responses_mutex_);
    responses_.push(response_data);
  }
  run_thread_ready_cv_.notify_all();
}

void NetworkAndroid::ResetRequest(JNIEnv* env, RequestId request_id) {
  std::lock_guard<std::mutex> lock(requests_mutex_);
  if (!started_) {
    return;
  }

  auto req = requests_.find(request_id);
  if (req == requests_.end()) {
    OLP_SDK_LOG_WARNING(kLogTag,
                        "ResetRequest of unknown request_id=" << request_id);
    return;
  }
  req->second->Reinitialize();
}

jobjectArray NetworkAndroid::CreateExtraHeaders(
    JNIEnv* env,
    const std::vector<std::pair<std::string, std::string> >& extra_headers) {
  if ((extra_headers.empty())) {
    return 0;
  }

  jstring jempty_string = env->NewStringUTF("");
  if (!jempty_string || env->ExceptionOccurred()) {
    OLP_SDK_LOG_WARNING(kLogTag, "Failed to create an empty string");
    return 0;
  }

  // Merge vector of pair strings into one JNI array
  const size_t header_count = extra_headers.size() * 2;
  jobjectArray headers = (jobjectArray)env->NewObjectArray(
      header_count, gStringClass, jempty_string);
  if (!headers || env->ExceptionOccurred()) {
    env->DeleteLocalRef(jempty_string);
    OLP_SDK_LOG_ERROR(
        kLogTag, "Failed to allocate strings array count=" << header_count);
    return 0;
  }
  env->DeleteLocalRef(jempty_string);

  for (size_t i = 0; i < extra_headers.size(); ++i) {
    jstring name = env->NewStringUTF(extra_headers[i].first.c_str());
    if (!name || env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(
          kLogTag, "Failed to create header name=" << extra_headers[i].first);
      return 0;
    }
    utils::JNIScopedLocalReference name_ref(env, name);

    jstring value = env->NewStringUTF(extra_headers[i].second.c_str());
    if (!value || env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(
          kLogTag, "Failed to create header value=" << extra_headers[i].second);
      return 0;
    }
    utils::JNIScopedLocalReference value_ref(env, value);

    env->SetObjectArrayElement(headers, i * 2, name);
    if (env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(kLogTag,
                        "Failed to set header name=" << extra_headers[i].first);
      return 0;
    }
    env->SetObjectArrayElement(headers, i * 2 + 1, value);
    if (env->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(
          kLogTag, "Failed to set header value=" << extra_headers[i].second);
      return 0;
    }
  }

  return headers;
}

void NetworkAndroid::Run(NetworkAndroid* self) {
  JNIEnv* env;
  bool attached = false;
  if (gJavaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) !=
      JNI_OK) {
    if (JNI_OK == gJavaVM->AttachCurrentThread(&env, nullptr)) {
      attached = true;
    }
  }

  self->SelfRun();

  if (attached) {
    gJavaVM->DetachCurrentThread();
  }
}

void NetworkAndroid::SelfRun() {
  OLP_SDK_LOG_TRACE(kLogTag, "Worker thread start, this=" << this);

  {
    std::lock_guard<std::mutex> lock(responses_mutex_);
    started_ = true;
    run_thread_ready_cv_.notify_all();
  }

  // Continously retrieves the recent completed response (in a separate thread)
  // and send the response back to user via callback
  while (started_) {
    ResponseData response_data;

    {
      std::unique_lock<std::mutex> lock(responses_mutex_);
      // TODO: add lambda to wait method
      while (started_ && responses_.empty()) {
        run_thread_ready_cv_.wait(lock);
      }

      if (!started_) {
        return;
      }

      if (!responses_.empty()) {
        response_data = responses_.front();
        responses_.pop();
      }
    }

    if (response_data.IsValid()) {
      bool cancelled = false;
      auto iter = this->cancelled_requests_.begin();
      while (iter != this->cancelled_requests_.end()) {
        if (*iter == response_data.id) {
          cancelled = true;
          this->cancelled_requests_.erase(iter);
          break;
        }
        ++iter;
      }

      // Notify user about response
      if (auto callback = response_data.callback) {
        callback(NetworkResponse()
                     .WithRequestId(response_data.id)
                     .WithStatus(response_data.status)
                     .WithError(response_data.error)
                     .WithBytesUploaded(response_data.uploaded_bytes)
                     .WithBytesDownloaded(response_data.downloaded_bytes));
      }
    }
  }

  OLP_SDK_LOG_TRACE(kLogTag, "Worker thread finish, this=" << this);
}

SendOutcome NetworkAndroid::Send(NetworkRequest request,
                                 Network::Payload payload,
                                 Network::Callback callback,
                                 Network::HeaderCallback header_callback,
                                 Network::DataCallback data_callback) {
  if (!Initialize()) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Send failed - network is offline, url=" << request.GetUrl());
    return SendOutcome(ErrorCode::OFFLINE_ERROR);
  }

  utils::JNIThreadBinder env(gJavaVM);
  if (env.GetEnv() == nullptr) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Send failed - invalid Java Env, url=" << request.GetUrl());
    return SendOutcome(ErrorCode::IO_ERROR);
  }

  // Convert the URL to jstring
  jstring jurl = env.GetEnv()->NewStringUTF(request.GetUrl().c_str());
  if (!jurl || env.GetEnv()->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Send failed - can't create a JNI String, url="
                                   << request.GetUrl());
    env.GetEnv()->ExceptionDescribe();
    env.GetEnv()->ExceptionClear();
    return SendOutcome(ErrorCode::IO_ERROR);
  }
  utils::JNIScopedLocalReference url_ref(env.GetEnv(), jurl);

  // Convert extra headers
  jobjectArray jheaders =
      CreateExtraHeaders(env.GetEnv(), request.GetHeaders());
  if (env.GetEnv()->ExceptionOccurred()) {
    OLP_SDK_LOG_ERROR(kLogTag, "Send failed - can't create JNI Headers, url="
                                   << request.GetUrl());
    env.GetEnv()->ExceptionDescribe();
    env.GetEnv()->ExceptionClear();
    return SendOutcome(ErrorCode::IO_ERROR);
  }
  utils::JNIScopedLocalReference headers_ref(env.GetEnv(), jheaders);

  // Get body data (if any)
  jbyteArray jbody = nullptr;
  if (request.GetVerb() != NetworkRequest::HttpVerb::GET &&
      request.GetVerb() != NetworkRequest::HttpVerb::HEAD) {
    size_t size = 0;
    const uint8_t* body_data = nullptr;
    auto body = request.GetBody();
    if (body && !body->empty()) {
      body_data = body->data();
      size = body->size();
    }

    jbody = env.GetEnv()->NewByteArray(size);
    if (!jbody || env.GetEnv()->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - can't allocate a body, url="
                                     << request.GetUrl());
      env.GetEnv()->ExceptionDescribe();
      env.GetEnv()->ExceptionClear();
      return SendOutcome(ErrorCode::IO_ERROR);
    }
    env.GetEnv()->SetByteArrayRegion(jbody, 0, size, (jbyte*)body_data);
  }
  utils::JNIScopedLocalReference body_ref(env.GetEnv(), jbody);

  // Set proxy settings:
  jstring jproxy = nullptr;
  const auto& proxy_settings = request.GetSettings().GetProxySettings();
  const bool is_proxy_valid =
      proxy_settings.GetType() != NetworkProxySettings::Type::NONE &&
      !proxy_settings.GetHostname().empty();

  if (is_proxy_valid) {
    jproxy = env.GetEnv()->NewStringUTF(proxy_settings.GetHostname().c_str());
    if (!jproxy || env.GetEnv()->ExceptionOccurred()) {
      OLP_SDK_LOG_ERROR(kLogTag, "Send failed - can't create a proxy, url="
                                     << request.GetUrl() << ", proxy="
                                     << proxy_settings.GetHostname());
      env.GetEnv()->ExceptionDescribe();
      env.GetEnv()->ExceptionClear();
      return SendOutcome(ErrorCode::IO_ERROR);
    }
  }
  utils::JNIScopedLocalReference proxy_ref(env.GetEnv(), jproxy);

  // Create a request that we keep in map until request has completed. Capture
  // the current tracker with the request so it can be restored when the
  // request is later fulfilled.
  auto request_data = std::make_shared<RequestData>(
      callback, header_callback, data_callback, request.GetUrl(), payload);
  RequestId request_id =
      static_cast<RequestId>(RequestIdConstants::RequestIdMin);

  const jint jhttp_verb = static_cast<jint>(request.GetVerb());
  const jint jconnection_timeout =
      static_cast<jint>(request.GetSettings().GetConnectionTimeout());
  const jint jtransfer_timeout =
      static_cast<jint>(request.GetSettings().GetTransferTimeout());
  const jint jproxy_port = static_cast<jint>(proxy_settings.GetPort());
  const jint jproxy_type = static_cast<jint>(proxy_settings.GetType());
  const jint jmax_retries =
      static_cast<jint>(request.GetSettings().GetRetries());
  // Do sending
  {
    std::lock_guard<std::mutex> lock(requests_mutex_);

    if (requests_.size() >= max_requests_count_) {
      OLP_SDK_LOG_WARNING(
          kLogTag, "Send failed - network overload, url=" << request.GetUrl());
      return SendOutcome(ErrorCode::NETWORK_OVERLOAD_ERROR);
    }

    // Add the request to the request map
    request_id = GenerateNextRequestId();
    requests_.insert({request_id, request_data});

    const jlong jrequest_id = static_cast<jlong>(request_id);
    auto task_obj = env.GetEnv()->CallObjectMethod(
        obj_, jni_send_method_, jurl, jhttp_verb, jrequest_id,
        jconnection_timeout, jtransfer_timeout, jheaders, jbody, jproxy,
        jproxy_port, jproxy_type, jmax_retries);
    if (env.GetEnv()->ExceptionOccurred() || !task_obj) {
      OLP_SDK_LOG_ERROR(
          kLogTag, "Send failed - HttpClient error, url=" << request.GetUrl());
      env.GetEnv()->ExceptionDescribe();
      env.GetEnv()->ExceptionClear();
      requests_.erase(request_id);
      return SendOutcome(ErrorCode::IO_ERROR);
    }

    // Store the HttpTask object in order to grant possibility for cancelling
    // requests
    utils::JNIScopedLocalReference task_obj_ref(env.GetEnv(), task_obj);
    request_data->obj = env.GetEnv()->NewGlobalRef(task_obj);
  }

  return SendOutcome(request_id);
}

void NetworkAndroid::Cancel(RequestId request_id) {
  utils::JNIThreadBinder env(gJavaVM);
  if (env.GetEnv() == nullptr) {
    OLP_SDK_LOG_ERROR(
        kLogTag, "Cancel failed - invalid Java Env, request_id=" << request_id);
    return;
  }

  {
    std::unique_lock<std::mutex> lock(requests_mutex_);
    auto request_iter = requests_.find(request_id);
    if (request_iter == requests_.end()) {
      OLP_SDK_LOG_WARNING(kLogTag,
                          "Cancel failed - unknown request_id=" << request_id);
      return;
    }

    OLP_SDK_LOG_TRACE(kLogTag, "Cancel request with id=" << request_id);

    auto request = request_iter->second;
    DoCancel(env.GetEnv(), request->obj);

    env.GetEnv()->DeleteGlobalRef(request->obj);
    request->obj = nullptr;

    cancelled_requests_.push_back(request_id);
  }
}

RequestId NetworkAndroid::GenerateNextRequestId() {
  auto request_id = request_id_counter_++;
  if (request_id_counter_ ==
      static_cast<RequestId>(RequestIdConstants::RequestIdMax)) {
    request_id_counter_ =
        static_cast<RequestId>(RequestIdConstants::RequestIdMin);
  }
  return request_id;
}

NetworkAndroid::RequestData::RequestData(
    Network::Callback callback, Network::HeaderCallback header_callback,
    Network::DataCallback data_callback, const std::string& url,
    Network::Payload payload)
    : callback(callback),
      header_callback(header_callback),
      data_callback(data_callback),
      url(url),
      payload(payload),
      obj(nullptr),
      count(0),
      offset(0) {}

NetworkAndroid::ResponseData::ResponseData(
    RequestId id, Network::Callback callback, int status, int uploaded_bytes,
    int downloaded_bytes, const char* error, const char* content_type,
    jlong count, jlong offset, Network::Payload payload)
    : id(id),
      callback(callback),
      payload(payload),
      error(error),
      content_type(content_type),
      status(status),
      count(count),
      uploaded_bytes(uploaded_bytes),
      downloaded_bytes(downloaded_bytes),
      offset(offset) {}

}  // namespace http
}  // namespace olp
