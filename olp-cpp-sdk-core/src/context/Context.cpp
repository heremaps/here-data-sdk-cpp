/*
 * Copyright (C) 2019-2023 HERE Europe B.V.
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

#include "ContextInternal.h"

namespace {
std::atomic_flag gDeInitializing = ATOMIC_FLAG_INIT;
}

namespace olp {
namespace context {

void initialize() {
  const auto data = Instance();

  // Fire all initializations.
  for (const olp::context::Context::InitializedCallback& cb : data->init_vector)
    cb();
}

void deinitialize() {
  if (!atomic_flag_test_and_set(&gDeInitializing)) {
    const auto data = Instance();

    // Fire all deinitializeds.
    for (const olp::context::Context::DeinitializedCallback& cb :
         data->deinit_vector)
      cb();

    atomic_flag_clear(&gDeInitializing);
  }
}

void Context::addInitializeCallbacks(InitializedCallback initCallback,
                                     DeinitializedCallback deinitCalback) {
  auto cd = Instance();

  cd->init_vector.emplace_back(std::move(initCallback));
  cd->deinit_vector.emplace_back(std::move(deinitCalback));
}

void Context::init() {
  auto cd = Instance();
#ifdef ANDROID
  cd->java_vm = nullptr;
#else
  (void)cd;
#endif
  initialize();
}

void Context::deinit() {
#ifdef ANDROID
  auto cd = Instance();
  // Release the global reference of Context.
  // Technically not needed if we took the application context,
  // but good to release just in case.
  if (cd->java_vm) {
    JNIEnv* env = nullptr;
    bool attached = false;
    if (cd->java_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) !=
        JNI_OK) {
      if (cd->java_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        env = nullptr;
      } else {
        attached = true;
      }
    }
    if (env) {
      env->DeleteGlobalRef(cd->context);
      if (attached) {
        cd->java_vm->DetachCurrentThread();
      }
    }
  }

  cd->java_vm = nullptr;
#endif
  deinitialize();
}

#if defined(ANDROID)
void Context::init(JavaVM* vm, jobject context) {
  auto cd = Instance();
  cd->java_vm = vm;

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
  auto cd = Instance();
  if (!cd->java_vm) {
    assert(false);
  }

  return cd->java_vm;
}

jobject Context::getAndroidContext() {
  auto cd = Instance();
  if (!cd->context) {
    assert(false);
  }
  return cd->context;
}
#elif defined(__APPLE__)
void Context::EnterBackground() {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& clients = cd->enter_background_subscibers;
  std::for_each(clients.begin(), clients.end(),
                [&](std::weak_ptr<EnterBackgroundSubscriber> weak) {
                  auto strong = weak.lock();
                  if (strong) {
                    strong->OnEnterBackground();
                  }
                });
}

void Context::ExitBackground() {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& clients = cd->enter_background_subscibers;
  std::for_each(clients.begin(), clients.end(),
                [&](std::weak_ptr<EnterBackgroundSubscriber> weak) {
                  auto strong = weak.lock();
                  if (strong) {
                    strong->OnExitBackground();
                  }
                });
}

void Context::StoreBackgroundSessionCompletionHandler(
    const std::string& session_name,
    std::function<void(void)> completion_handler) {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& completion_handlers = cd->completion_handlers;
  completion_handlers.emplace(session_name, std::move(completion_handler));
}
#endif  // __APPLE__

Context::Scope::Scope() : m_cd(Instance()) {
  std::lock_guard<std::mutex> lock(m_cd->context_mutex);
  assert(m_cd->context_instance_counter != std::numeric_limits<size_t>::max());
  if (m_cd->context_instance_counter++ == 0) {
    Context::init();
  }
}
#ifdef ANDROID
/// see Context::init()
Context::Scope::Scope(JavaVM* vm, jobject application) : m_cd(Instance()) {
  std::lock_guard<std::mutex> lock(m_cd->context_mutex);
  assert(m_cd->context_instance_counter != std::numeric_limits<size_t>::max());
  if (m_cd->context_instance_counter++ == 0) {
    Context::init(vm, application);
  }
}
#endif

/// see Context::deinit()
Context::Scope::~Scope() {
  std::lock_guard<std::mutex> lock(m_cd->context_mutex);
  assert(m_cd->context_instance_counter != std::numeric_limits<size_t>::min());
  if (--m_cd->context_instance_counter == 0) {
    Context::deinit();
  }
}

}  // namespace context
}  // namespace olp
