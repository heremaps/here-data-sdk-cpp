/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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
  using JsonDoc = boost::json::object;
  using JsonValue = boost::json::value;

 public:
  template <typename TargetType>
  class BuilderHelper {
    // -fno-rtti friendly way to get classname
    const std::string kTargetTypeName =
        boost::core::demangled_name(BOOST_CORE_TYPEID(TargetType));
    const std::string kLogTag = "ResponseFromJsonBuilder";

   public:
    explicit BuilderHelper(const JsonDoc& json_value) : json_{json_value} {}

    template <typename ArgType, typename Projection = detail::Identity>
    BuilderHelper& Value(const char* name, void (TargetType::*set_fn)(ArgType),
                         Projection projection = {}) {
      using CompatibleType = decltype(GetCompatibleType<ArgType>());
      fields_.emplace(name, [=](TargetType& target_obj,
                                const JsonValue& json_value) {
        if (auto value = If<CompatibleType>(json_value)) {
          (target_obj.*set_fn)(projection(*value));
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
        if (value.is_array()) {
          const auto& array = value.get_array();
          ArrayType array_result;
          array_result.reserve(array.size());
          for (const auto& element : array) {
            if (auto element_value = If<CompatibleType>(element)) {
              array_result.push_back(*element_value);
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

      auto it = json_.cbegin();
      auto it_end = json_.cend();
      for (; it != it_end; ++it) {
        auto find_it = fields_.find(std::string{it->key_c_str()});
        if (find_it != fields_.end()) {
          find_it->second(result, it->value());
          // erasing already processed value
          fields_.erase(find_it);
          continue;
        }

        OLP_SDK_LOG_WARNING_F(kLogTag,
                              "Unexpected value, response=%s, field=%s",
                              kTargetTypeName.c_str(), it->key_c_str());
      }

      // in the ideal scenario all fields should be processed
      for (const auto& field : fields_) {
        OLP_SDK_LOG_WARNING_F(kLogTag, "Absent value, response=%s, field=%s",
                              kTargetTypeName.c_str(), field.first.c_str());
      }

      return result;
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

    template <typename T, typename = typename std::enable_if<
                              std::is_same<T, bool>::value>::type>
    const bool* If(const JsonValue& value) {
      return value.if_bool();
    }

    template <typename T, typename = typename std::enable_if<
                              std::is_same<T, int64_t>::value ||
                              std::is_same<T, int>::value>::type>
    const int64_t* If(const JsonValue& value) {
      return value.if_int64();
    }

    template <typename T, typename = typename std::enable_if<
                              std::is_same<T, uint64_t>::value>::type>
    const uint64_t* If(const JsonValue& value) {
      return value.if_uint64();
    }

    template <typename T, typename = typename std::enable_if<
                              std::is_same<T, const char*>::value>::type>
    boost::optional<const char*> If(const JsonValue& value) {
      if (auto* str = value.if_string()) {
        return str->c_str();
      }
      return nullptr;
    }

    using Fields =
        std::unordered_map<std::string,
                           std::function<void(TargetType&, const JsonValue&)>>;

    const JsonDoc& json_;
    Fields fields_;
  };

  template <typename JsonType>
  static BuilderHelper<ResponseType> Build(const JsonType& json) {
    return BuilderHelper<ResponseType>{json};
  }
};

}  // namespace authentication
}  // namespace olp
