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

#pragma once

#include <olp/core/CoreApi.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief The LogContext object stores per-thread key/value pairs that can be
 * picked up in log messages using the
 * 'olp::logging::MessageFormatter::ElementType::ContextValue' logging element.
 */
namespace olp {
namespace logging {
/// A context key/value map.  The keys are required to be const and immutable.
using LogContext = std::unordered_map<std::string, std::string>;

/// A function that sets the current log context.
using LogContextSetter = std::function<void(std::shared_ptr<const LogContext>)>;

/// A function that gets the current log context.
using LogContextGetter = std::function<std::shared_ptr<const LogContext>()>;

/**
 * Sets the getter and setter for the log context.
 *
 * @param[in] getter The log context getter, or the default getter if
 * nullptr.
 * @param[in] setter The log context setter, or the default setter if
 * nullptr.
 */
CORE_API void SetLogContextGetterSetter(LogContextGetter getter,
                                        LogContextSetter setter);

/// Gets the current thread context.
CORE_API std::shared_ptr<const LogContext> GetContext();

/// Gets a value from the current thread context.
CORE_API const std::string& GetContextValue(const std::string& key);

/**
 * @brief The ScopedLogContext class takes ownership of a log context
 * and makes it active on construction and restores the previous context on
 * destruction.
 *
 * Passing 'nullptr' is the same as passing an empty LogContext.
 *
 * @see MessageFormatter::ElementType::Context
 */
class CORE_API ScopedLogContext final {
 public:
  ScopedLogContext(const std::shared_ptr<const LogContext>& context);
  ~ScopedLogContext();

 private:
  std::shared_ptr<const LogContext> context_;
  std::shared_ptr<const LogContext> prev_context_;
};
}  // namespace logging
}  // namespace olp
