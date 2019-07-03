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

/**
 * @file
 * This header defines the RTTI used in geoviz. This consists of static and
 * runtime type information.
 *
 * ### Static type information
 *
 * Static means getting a type ID for a type. A type ID can be queried for each
 * type, nothing needs to be done for this. Use typeId() and typeName() to query
 * this information.
 *
 * ### Runtime type information
 *
 * Runtime means getting a type ID for an instantiated object. typeId() and
 * typeName() are also used for this. To enable this information the macro
 * `CORE_DEFINE_RTTI()` has to be defined in the class.
 *
 * ### Polymorphic runtime type information
 *
 * To be able to tell if a certain object is derived of a given type (and to
 * make a type-safe cast), the macros `CORE_DEFINE_RTTI_CASTABLE` and
 * `CORE_DEFINE_RTTI_CASTABLE_BASE()` have to be used instead.
 *
 * ### Dynamic casts
 *
 * Type-safe dynamic casts can be done for objects using dynamicCast() and
 * dynamicPointerCast().
 */
#pragma once

#include <olp/core/porting/warning_disable.h>
#include <olp/core/utils/Config.h>

#include <memory>
#include <string>
#include <type_traits>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include <boost/functional/hash.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <boost/type_index.hpp>

/**
 * Use this macro in a class declaration to be able to query the type of an
 * instantiated object.
 */
#define CORE_DEFINE_RTTI BOOST_TYPE_INDEX_REGISTER_CLASS

/**
 * Used this macro in a class declaration for base classes, for which
 * polymorphic RTTI shall be enabled.
 */
#define CORE_DEFINE_RTTI_CASTABLE_BASE(className)                           \
  CORE_DEFINE_RTTI                                                          \
 public:                                                                    \
  virtual const void* getPointerForType(const ::core::TypeIndex& typeIndex) \
      const {                                                               \
    if (typeIndex == ::core::typeId<className>())                           \
      return reinterpret_cast<const void*>(this);                           \
    return nullptr;                                                         \
  }                                                                         \
  virtual void* getPointerForType(const ::core::TypeIndex& typeIndex) {     \
    return (typeIndex == ::core::typeId<className>())                       \
               ? reinterpret_cast<void*>(this)                              \
               : nullptr;                                                   \
  }                                                                         \
  bool canConvertTo(const ::core::TypeIndex& typeIndex) const {             \
    return getPointerForType(typeIndex) != nullptr;                         \
  }

/**
 * Used this macro in a class declaration for sub classes, for which polymorphic
 * RTTI shall be enabled.
 *
 * In the base class the macro `CORE_DEFINE_RTTI_CASTABLE_BASE()` has to be
 * defined.
 */
#define CORE_DEFINE_RTTI_CASTABLE(className, baseClassName)                    \
  PORTING_CLANG_PUSH_WARNINGS()                                                \
  PORTING_CLANG_DISABLE_WARNING("-Winconsistent-missing-override")             \
  CORE_DEFINE_RTTI                                                             \
  PORTING_CLANG_POP_WARNINGS()                                                 \
                                                                               \
 public:                                                                       \
  const void* getPointerForType(const ::core::TypeIndex& typeIndex)            \
      const override {                                                         \
    static_assert(                                                             \
        std::is_base_of<baseClassName, className>::value,                      \
        "CORE_DEFINE_RTTI_CASTABLE used with invalid class/base class pair."); \
    if (typeIndex == ::core::typeId<className>())                              \
      return reinterpret_cast<const void*>(this);                              \
    else                                                                       \
      return baseClassName ::getPointerForType(typeIndex);                     \
  }                                                                            \
  void* getPointerForType(const ::core::TypeIndex& typeIndex) override {       \
    static_assert(                                                             \
        std::is_base_of<baseClassName, className>::value,                      \
        "CORE_DEFINE_RTTI_CASTABLE used with invalid class/base class pair."); \
    return (typeIndex == ::core::typeId<className>())                          \
               ? reinterpret_cast<void*>(this)                                 \
               : baseClassName ::getPointerForType(typeIndex);                 \
  }

/**
 * Used this macro in a class declaration for sub classes, for which polymorphic
 * RTTI shall be enabled.
 *
 * In the base class the macro `CORE_DEFINE_RTTI_CASTABLE_BASE()` has to be
 * defined.
 */
#define CORE_DEFINE_RTTI_CASTABLE2(className, baseClass1Name, baseClass2Name)  \
  PORTING_CLANG_PUSH_WARNINGS()                                                \
  PORTING_CLANG_DISABLE_WARNING("-Winconsistent-missing-override")             \
  CORE_DEFINE_RTTI                                                             \
  PORTING_CLANG_POP_WARNINGS()                                                 \
                                                                               \
 public:                                                                       \
  const void* getPointerForType(const ::core::TypeIndex& typeIndex)            \
      const override {                                                         \
    static_assert(                                                             \
        std::is_base_of<baseClass1Name, className>::value ||                   \
            std::is_base_of<baseClass2Name, className>::value,                 \
        "CORE_DEFINE_RTTI_CASTABLE used with invalid class/base class pair."); \
    if (typeIndex == ::core::typeId<className>())                              \
      return reinterpret_cast<const void*>(this);                              \
    const void* retVal = baseClass1Name ::getPointerForType(typeIndex);        \
    if (retVal) return retVal;                                                 \
    return baseClass2Name ::getPointerForType(typeIndex);                      \
  }                                                                            \
  void* getPointerForType(const ::core::TypeIndex& typeIndex) override {       \
    static_assert(                                                             \
        std::is_base_of<baseClass1Name, className>::value ||                   \
            std::is_base_of<baseClass2Name, className>::value,                 \
        "CORE_DEFINE_RTTI_CASTABLE used with invalid class/base class pair."); \
    if (typeIndex == ::core::typeId<className>())                              \
      return reinterpret_cast<void*>(this);                                    \
    void* retVal = baseClass1Name ::getPointerForType(typeIndex);              \
    if (retVal) return retVal;                                                 \
    return baseClass2Name ::getPointerForType(typeIndex);                      \
  }                                                                            \
  /* resolve ambiguity */                                                      \
  bool canConvertTo(const ::core::TypeIndex& typeIndex) const {                \
    return getPointerForType(typeIndex) != nullptr;                            \
  }

namespace core {
using TypeIndex = boost::typeindex::type_index;

/**
 * Get the type ID for a given type. Pass the type in question as template
 * argument.
 *
 * Example: `typeId<MyFunnyType>()`.
 */
template <typename Type>
inline TypeIndex typeId() {
  return boost::typeindex::type_id<Type>();
}

/**
 * Get the type ID for a given object.
 *
 * To use this version of typeId(), the class needs to define RTTI in it's
 * declaration using the `CORE_DEFINE_RTTI*()` macros above.
 *
 * Example:
 * ~~~{.c}
 * class MyFunnyType
 * {
 *     CORE_DEFINE_RTTI;
 * };
 *
 * MyFunnyType myInstance;
 *
 * auto myTypeId = typeId(myInstance);
 * ~~~
 */
template <typename Type>
inline TypeIndex typeId(const Type& runtimeValue) {
  return boost::typeindex::type_id_runtime(runtimeValue);
}

}  // namespace core
