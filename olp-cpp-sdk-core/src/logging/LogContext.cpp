/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include <olp/core/logging/LogContext.h>

#include <cassert>

namespace olp {
namespace logging {
namespace {
thread_local std::shared_ptr<const LogContext> tls_logContext;

LogContextSetter s_defaultLogContextSetter =
    [](std::shared_ptr<const LogContext> context) {
      tls_logContext = std::move(context);
    };

LogContextSetter s_logContextSetter = s_defaultLogContextSetter;

LogContextGetter s_defaultLogContextGetter = []() { return tls_logContext; };
LogContextGetter s_logContextGetter = s_defaultLogContextGetter;

}  // namespace

void SetLogContextGetterSetter(LogContextGetter getter,
                               LogContextSetter setter) {
  s_logContextGetter = getter ? std::move(getter) : s_defaultLogContextGetter;
  s_logContextSetter = setter ? std::move(setter) : s_defaultLogContextSetter;
}

std::shared_ptr<const LogContext> GetContext() {
  assert(s_logContextGetter && "Invalid LogContext getter");
  return s_logContextGetter();
}

const std::string& GetContextValue(const std::string& key) {
  static std::string s_empty = "";
  auto ctx = GetContext();
  if (!ctx || key.empty())
    return s_empty;

  auto it = ctx->find(key);
  if (it == ctx->end())
    return s_empty;

  return it->second;
}

ScopedLogContext::ScopedLogContext(
    const std::shared_ptr<const LogContext>& context)
    : context_(context) {
  prev_context_ = GetContext();
  assert(s_logContextSetter && "Invalid LogContext setter");
  s_logContextSetter(context_);
}

ScopedLogContext::~ScopedLogContext() {
  assert(s_logContextSetter && "Invalid LogContext setter");
  s_logContextSetter(std::move(prev_context_));
}

}  // namespace logging
}  // namespace olp
