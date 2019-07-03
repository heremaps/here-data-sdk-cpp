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

#pragma once

#ifdef __ANDROID__

#include "../Memory.h"

#include <jni.h>
#include <olp/core/network/NetworkProtocol.h>
#include <condition_variable>
#include <future>
#include <map>
#include <mutex>
#include <queue>
#include <thread>

namespace olp {
namespace network {
/**
 * @brief Implementation of NetworkProtocol for Android
 */
class NetworkProtocolAndroid
    : public NetworkProtocol,
      public std::enable_shared_from_this<NetworkProtocolAndroid> {
 public:
  /**
   * @brief Setup JavaVM pointer
   *        This must be done before calling initialize
   * @param vm - pointer to JavaVM
   * @param application - application object
   */
  static void setJavaVm(JavaVM* vm, jobject application);

  /**
   * @brief NetworkProtocolAndroid constructor
   */
  NetworkProtocolAndroid();

  /**
   * @brief NetworkProtocolAndroid destructor
   */
  ~NetworkProtocolAndroid();

  /**
   * @brief Shutdown the protocol
   * @param env - JNI environment for this thread
   */
  void shutdown(JNIEnv* env);

  bool Initialize() override;
  void Deinitialize() override;
  bool Initialized() const override;
  bool Ready() override;
  ErrorCode Send(const NetworkRequest& request, int id,
                 const std::shared_ptr<std::ostream>& payload,
                 std::shared_ptr<NetworkConfig> config,
                 Network::HeaderCallback headerCallback,
                 Network::DataCallback dataCallback,
                 Network::Callback callback) override;
  bool Cancel(int id) override;
  size_t AmountPending() override;

  /**
   * @brief Headers received for the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param headers - List of headers as string array
   */
  void headersCallback(JNIEnv* env, int id, jobjectArray headers);

  /**
   * @brief Date header received for the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param date - Date in milliseconds since January 1, 1970 GMT
   * @param offset - Offset of first byte from beginning of the whole content
   */
  void dateAndOffsetCallback(JNIEnv* env, int id, jlong date, jlong offset);

  /**
   * @brief Data received to the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param payload - Data
   * @param len - Length of the data
   * @param offset - Offset of the data
   */
  void dataReceived(JNIEnv* env, int id, jbyteArray payload, int len);

  /**
   * @brief Complete the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param status - HTTP status code of the completion
   * @param error - Error string
   * @param maxAge - Maximum age of the data, from HTTP header
   * @param expires - expire time of response
   * @param etag - ETag from HTTP header
   * @param contentType - Content-Type from HTTP header
   */
  void completeRequest(JNIEnv* env, int id, int status, jstring error,
                       int maxAge, int expires, jstring etag,
                       jstring contentType);

  /**
   * @brief Reset the given message if retry attempt invoked
   * @param id - Unique Id of the message
   */
  void resetRequest(JNIEnv* env, int id);

 private:
  struct RequestCompletion {
    RequestCompletion(size_t count) : m_count(count) {}

    std::promise<void> m_ready;
    size_t m_count;
  };

  struct RequestData {
    RequestData(Network::Callback callback,
                Network::HeaderCallback headerCallback,
                Network::DataCallback dataCallback, const std::string& url,
                const std::shared_ptr<std::ostream>& payload);
    void reinitialize() {
      m_obj = nullptr;
      m_date = -1;
      m_count = 0;
      m_offset = 0;
      m_resume = false;
    }
    Network::Callback m_callback;
    Network::HeaderCallback m_headerCallback;
    Network::DataCallback m_dataCallback;
    std::string m_url;
    jobject m_obj;
    jlong m_date;
    jlong m_count;
    jlong m_offset;
    bool m_resume;
    bool m_ignoreOffset;
    std::shared_ptr<std::ostream> m_payload;
    std::shared_ptr<RequestCompletion> m_completion;
    mem::MemoryScopeTracker m_tracker;
  };

  struct ResultData {
    ResultData() : m_callback(nullptr) {}
    ResultData(Network::RequestId id, Network::Callback callback, int status,
               int maxAge, int expires, const char* error, const char* etag,
               const char* contentType, jlong count, jlong offset,
               std::shared_ptr<std::ostream> payload);
    bool isValid() const { return (m_callback != nullptr); }

    Network::RequestId m_id;
    Network::Callback m_callback;
    std::shared_ptr<std::ostream> m_payload;
    std::string m_error;
    std::string m_etag;
    std::string m_contentType;
    int m_status = 0;
    int m_maxAge = 0;
    int m_expires = 0;
    jlong m_count;
    jlong m_offset;
  };

  struct JNIThreadBinder {
    JNIThreadBinder(JavaVM* vm) : m_attached(false), m_env(nullptr), m_vm(vm) {
      if (m_vm->GetEnv(reinterpret_cast<void**>(&m_env), JNI_VERSION_1_6) !=
          JNI_OK) {
        if (m_vm->AttachCurrentThread(&m_env, nullptr) == JNI_OK) {
          m_attached = true;
        }
      }
    }

    ~JNIThreadBinder() {
      if (m_attached) m_vm->DetachCurrentThread();
    }

    /// assignment operator
    JNIThreadBinder& operator=(const JNIThreadBinder&) = delete;

    /// move assignment operator
    JNIThreadBinder& operator=(JNIThreadBinder&& other) {
      m_attached = std::move(other.m_attached);
      m_env = std::move(other.m_env);
      m_vm = std::move(other.m_vm);
      return *this;
    }
    bool m_attached;
    JNIEnv* m_env;
    JavaVM* m_vm;
  };

  static void run(NetworkProtocolAndroid* self);
  void selfRun();
  static jobjectArray createExtraHeaders(
      JNIEnv* env,
      const std::vector<std::pair<std::string, std::string> >& extraHeaders,
      std::uint64_t modifiedSince,
      const std::vector<std::pair<std::string, std::string> >& rangeHeaders);
  void doCancel(JNIEnv* env, jobject object);
  void doDeinitialize();

  jclass m_class;
  jmethodID m_jmidSend;
  jmethodID m_jmidShutdown;

  jobject m_obj;
  jint m_id;
  bool m_started;

  mem::map<int, std::shared_ptr<RequestData> > m_requests;
  mem::deque<int> m_cancelledRequests;
  std::mutex m_requestMutex;
  std::mutex m_resultMutex;
  std::condition_variable m_resultCondition;
  std::queue<ResultData, mem::deque<ResultData> > m_results;
  mem::MemoryScopeTracker m_tracker;

  std::unique_ptr<std::thread> m_thread;
};

}  // namespace network
}  // namespace olp

#endif
