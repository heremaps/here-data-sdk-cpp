/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include <jni.h>

#include <olp/core/http/Network.h>

namespace olp {
namespace http {
/**
 * @brief Implementation of Network interface for Android based on the
 * java.net.HttpURLConnection
 */
class NetworkAndroid : public Network {
 public:
  /**
   * @brief NetworkAndroid constructor
   * @param max_requests_count Maximum requests the NetworkAndroid can process.
   */
  explicit NetworkAndroid(size_t max_requests_count);

  /**
   * @brief NetworkAndroid destructor
   */
  ~NetworkAndroid();

  SendOutcome Send(NetworkRequest request, Network::Payload payload,
                   Network::Callback callback,
                   Network::HeaderCallback header_callback = nullptr,
                   Network::DataCallback data_callback = nullptr) override;

  void Cancel(RequestId id) override;

  /**
   * @brief Headers received for the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param headers - List of headers as string array
   */
  void HeadersCallback(JNIEnv* env, RequestId request_id, jobjectArray headers);

  /**
   * @brief Date header received for the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param date - Date in milliseconds since January 1, 1970 GMT
   * @param offset - Offset of first byte from beginning of the whole content
   */
  void DateAndOffsetCallback(JNIEnv* env, RequestId request_id, jlong date,
                             jlong offset);

  /**
   * @brief Data received to the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param payload - Data
   * @param len - Length of the data
   * @param offset - Offset of the data
   */
  void DataReceived(JNIEnv* env, RequestId request_id, jbyteArray payload,
                    int len);

  /**
   * @brief Complete the given message
   * @param env - JNI environment for this thread
   * @param id - Unique Id of the message
   * @param uploaded_bytes - uploaded bytes associated with request
   * @param downloaded_bytes - downloaded bytes associated with request
   * @param status - HTTP status code of the completion
   * @param error - Error string
   * @param max_age - Maximum age of the data, from HTTP header
   * @param expires - expire time of response
   * @param etag - ETag from HTTP header
   * @param content_type - Content-Type from HTTP header
   */
  void CompleteRequest(JNIEnv* env, RequestId request_id, int status,
                       int uploaded_bytes, int downloaded_bytes, jstring error,
                       jstring content_type);

  /**
   * @brief Reset the given message if retry attempt invoked
   * @param id - Unique Id of the message
   */
  void ResetRequest(JNIEnv* env, RequestId request_id);

  /**
   * @brief Setup JavaVM pointer
   *        This must be done before calling initialize
   * @param vm - pointer to JavaVM
   * @param application - application object
   */
  static void SetJavaVM(JavaVM* vm, jobject application);

 private:
  /// Data, which is passed to the JNI as request's data
  struct RequestData {
    RequestData(Network::Callback callback,
                Network::HeaderCallback header_callback,
                Network::DataCallback data_callback, const std::string& url,
                Network::Payload payload);

    void Reinitialize() {
      obj = nullptr;
      count = 0;
      offset = 0;
    }
    Network::Callback callback;
    Network::HeaderCallback header_callback;
    Network::DataCallback data_callback;
    std::string url;
    Network::Payload payload;
    jobject obj;
    jlong count;
    jlong offset;
  };

  /// Response data, which is received from HttpClient.java
  struct ResponseData {
    ResponseData() = default;
    ResponseData(RequestId id, Network::Callback callback, int status,
                 int uploaded_bytes, int downloaded_bytes, const char* error,
                 const char* content_type, jlong count, jlong offset,
                 Network::Payload payload);
    bool IsValid() const { return (callback != nullptr); }

    RequestId id = static_cast<RequestId>(RequestIdConstants::RequestIdInvalid);
    Network::Callback callback;
    Network::Payload payload;
    std::string error;
    std::string content_type;
    int status = 0;
    int uploaded_bytes = 0;
    int downloaded_bytes = 0;
    jlong count = 0;
    jlong offset = 0;
  };

 private:
  static void Run(NetworkAndroid* self);

  static jobjectArray CreateExtraHeaders(
      JNIEnv* env,
      const std::vector<std::pair<std::string, std::string>>& extra_headers);

  void SelfRun();

  void DoCancel(JNIEnv* env, jobject object);

  RequestId GenerateNextRequestId();

  bool Initialize();

  void Deinitialize();

 private:
  std::unordered_map<RequestId, std::shared_ptr<RequestData>> requests_;
  std::queue<ResponseData> responses_;
  std::deque<RequestId> cancelled_requests_;

  std::mutex requests_mutex_;
  std::mutex responses_mutex_;

  jclass java_self_class_;
  jmethodID jni_send_method_;
  jmethodID java_shutdown_method_;
  jobject obj_;

  bool started_;
  bool initialized_;
  std::condition_variable run_thread_ready_cv_;

  const size_t max_requests_count_;

  RequestId request_id_counter_{
      static_cast<RequestId>(RequestIdConstants::RequestIdMin)};
  std::unique_ptr<std::thread> run_thread_;
};

}  // namespace http
}  // namespace olp

#endif
