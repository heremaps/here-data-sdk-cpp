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

#include <olp/core/context/Context.h>

#include <assert.h>
#include <atomic>
#include <cassert>
#include <memory>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#undef min
#undef max
#elif defined(ANDROID)
#include <jni.h>
#endif

namespace {
std::atomic_flag gDeInitializing = ATOMIC_FLAG_INIT;
}

namespace olp {
namespace context {
// Because of static initializer orders, we need to have this on the heap.
struct ContextData {
  std::vector<olp::context::Context::InitializedCallback> initVector;
  std::vector<olp::context::Context::DeinitializedCallback> deinitVector;

  std::mutex contextMutex;
  size_t contextInstanceCounter = 0;
#ifdef ANDROID
  JavaVM* javaVM = nullptr;
  jobject context = nullptr;
#endif
};

#define LOGTAG "Context"

std::shared_ptr<ContextData> instance() {
  // Static initialization is thread safe.
  // Shared pointer allows to extend the life of ContextData
  // until all Scope objects are destroyed.
  static std::shared_ptr<ContextData> gSetupVectors =
      std::make_shared<ContextData>();
  return gSetupVectors;
}

void initialize() {
  const auto data = instance();

  // Fire all initializations.
  for (const olp::context::Context::InitializedCallback& cb : data->initVector)
    cb();
}

void deinitialize() {
  if (!atomic_flag_test_and_set(&gDeInitializing)) {
    const auto data = instance();

    // Fire all deinitializeds.
    for (const olp::context::Context::DeinitializedCallback& cb :
         data->deinitVector)
      cb();

    atomic_flag_clear(&gDeInitializing);
  }
}

void Context::addInitializeCallbacks(InitializedCallback initCallback,
                                     DeinitializedCallback deinitCalback) {
  auto cd = instance();

  cd->initVector.emplace_back(std::move(initCallback));
  cd->deinitVector.emplace_back(std::move(deinitCalback));
}

void Context::init() {
  auto cd = instance();
#ifdef ANDROID
  cd->javaVM = nullptr;
#else
  (void)cd;
#endif
  initialize();
}

void Context::deinit() {
#ifdef ANDROID
  auto cd = instance();
  // Release the global reference of Context.
  // Technically not needed if we took the application context,
  // but good to release just in case.
  if (cd->javaVM) {
    JNIEnv* env = nullptr;
    bool attached = false;
    if (cd->javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) !=
        JNI_OK) {
      if (cd->javaVM->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        env = nullptr;
      } else {
        attached = true;
      }
    }
    if (env) {
      env->DeleteGlobalRef(cd->context);
      if (attached) {
        cd->javaVM->DetachCurrentThread();
      }
    }
  }

  cd->javaVM = nullptr;
#endif
  deinitialize();
}

#if defined(ANDROID)
void Context::init(JavaVM* vm, jobject context) {
  auto cd = instance();
  cd->javaVM = vm;

  JNIEnv* env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
    // Current thread trying to init thread is not bound to JVM.
    // Context cannot be valid.
    assert(0);
  }
  cd->context = env->NewGlobalRef(context);
  initialize();
}

JavaVM* Context::getJavaVM() {
  auto cd = instance();
  if (!cd->javaVM) {
    assert(false);
  }

  return cd->javaVM;
}

jobject Context::getAndroidContext() {
  auto cd = instance();
  if (!cd->context) {
    assert(false);
  }
  return cd->context;
}
#endif

Context::Scope::Scope() : m_cd(instance()) {
  std::lock_guard<std::mutex> lock(m_cd->contextMutex);
  assert(m_cd->contextInstanceCounter != std::numeric_limits<size_t>::max());
  if (m_cd->contextInstanceCounter++ == 0) {
    Context::init();
  }
}
#ifdef ANDROID
/// see Context::init()
Context::Scope::Scope(JavaVM* vm, jobject application) : m_cd(instance()) {
  std::lock_guard<std::mutex> lock(m_cd->contextMutex);
  assert(m_cd->contextInstanceCounter != std::numeric_limits<size_t>::max());
  if (m_cd->contextInstanceCounter++ == 0) {
    Context::init(vm, application);
  }
}
#endif

/// see Context::deinit()
Context::Scope::~Scope() {
  std::lock_guard<std::mutex> lock(m_cd->contextMutex);
  assert(m_cd->contextInstanceCounter != std::numeric_limits<size_t>::min());
  if (--m_cd->contextInstanceCounter == 0) {
    Context::deinit();
  }
}

}  // namespace context
}  // namespace olp
