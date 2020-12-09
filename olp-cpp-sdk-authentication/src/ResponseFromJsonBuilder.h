/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <olp/core/logging/Log.h>
#include <boost/core/typeinfo.hpp>

namespace olp {
namespace authentication {
namespace detail {
struct Identity {
  template <typename T>
  T&& operator()(T&& t) const {
    return std::forward<T>(t);
  }
};
}  // namespace detail

template <typename ResponseType>
class ResponseFromJsonBuilder {
  using JsonValue = rapidjson::Document::ValueType;

 public:
  template <typename TargetType>
  class BuilderHelper {
    // -fno-rtti friendly way to get classname
    const std::string kTargetTypeName =
        boost::core::demangled_name(BOOST_CORE_TYPEID(TargetType));
    const std::string kLogTag = "ResponseFromJsonBuilder";

   public:
    explicit BuilderHelper(const JsonValue& json_value) : json_{json_value} {}

    template <typename ArgType, typename Projection = detail::Identity>
    BuilderHelper& Value(const char* name, void (TargetType::*set_fn)(ArgType),
                         Projection projection = {}) {
      using CompatibleType = decltype(GetCompatibleType<ArgType>());
      fields_.emplace(name, [=](TargetType& target_obj,
                                const JsonValue& value) {
        if (value.Is<CompatibleType>()) {
          (target_obj.*set_fn)(projection(value.Get<CompatibleType>()));
        } else {
          OLP_SDK_LOG_WARNING_F(kLogTag, "Wrong type, response=%s, field=%s",
                                kTargetTypeName.c_str(), name);
        }
      });
      return *this;
    }

    template <typename ArrayType>
    BuilderHelper& Array(const char* name,
                         void (TargetType::*set_fn)(ArrayType)) {
      using CompatibleType =
          decltype(GetCompatibleType<typename ArrayType::value_type>());
      fields_.emplace(name, [=](TargetType& target_obj,
                                const JsonValue& value) {
        if (value.IsArray()) {
          const auto& array = value.GetArray();
          ArrayType array_result;
          array_result.reserve(array.Size());
          for (const auto& element : array) {
            if (element.Is<CompatibleType>()) {
              array_result.push_back(element.Get<CompatibleType>());
            }
          }
          (target_obj.*set_fn)(std::move(array_result));
        } else {
          OLP_SDK_LOG_WARNING_F(kLogTag, "Wrong type, response=%s, field=%s",
                                kTargetTypeName.c_str(), name);
        }
      });
      return *this;
    }

    TargetType Finish() {
      TargetType result;
      auto it = json_.MemberBegin();
      auto it_end = json_.MemberEnd();
      for (; it != it_end; ++it) {
        auto find_it = fields_.find(std::string{it->name.GetString()});
        if (find_it != fields_.end()) {
          find_it->second(result, it->value);
          continue;
        }
        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Unexpected value, response=%s, field=%s",
                              kTargetTypeName.c_str(), it->name.GetString());
      }
      return std::move(result);
    }

   private:
    template <typename T,
              typename = typename std::enable_if<
                  std::is_integral<T>::value && !std::is_enum<T>::value &&
                  !std::is_same<T, long>::value>::type>
    T GetCompatibleType();

    template <typename T,
              typename = typename std::enable_if<!std::is_integral<T>::value ||
                                                 std::is_enum<T>::value>::type>
    const char* GetCompatibleType();

    template <typename T, typename = typename std::enable_if<
                              std::is_same<T, long>::value>::type>
    int64_t GetCompatibleType();

    using Fields =
        std::unordered_map<std::string,
                           std::function<void(TargetType&, const JsonValue&)>>;

    const JsonValue& json_;
    Fields fields_;
  };

  template <typename JsonType>
  static BuilderHelper<ResponseType> Build(const JsonType& json) {
    return BuilderHelper<ResponseType>{json};
  }
};

}  // namespace authentication
}  // namespace olp
