/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include <functional>
#include <type_traits>
#include <vector>

#include <olp/core/client/ApiError.h>
#include <olp/core/porting/make_unique.h>

namespace olp {
namespace thread {
namespace internal {

/// A helper class removes type qualifiers and provides a type.
template <typename Type>
struct RemoveRefAndConstImpl {
  using type =
      typename std::remove_cv<typename std::remove_reference<Type>::type>::type;
};

/// A template for a type without type qualifiers
template <typename Type>
using RemoveRefAndConst = typename RemoveRefAndConstImpl<Type>::type;

/// Helper class which provides a type only
/// if `Type` and std::vector<void> name the same
/// type (taking into account const/volatile qualifications).
template <typename Type>
struct ReduceVoidVector {
  using type =
      typename std::conditional<std::is_same<Type, std::vector<void>>::value,
                                void, Type>::type;
};

/// A class for wrapping a type with std::function.
template <typename Type>
struct TypeToFunctionInputImpl {
  using type = std::function<void(Type)>;
};

/// A class for wrapping a void type with std::function.
template <>
struct TypeToFunctionInputImpl<void> {
  using type = std::function<void()>;
};

template <typename Type>
/// An alias for a std::function wrapper around the `Type`.
using TypeToFunctionInput = typename TypeToFunctionInputImpl<Type>::type;

template <typename Class, typename ExecutionContext, typename Arg>
/// A const function pointer with two parameters used to declare a type.
Arg SecondArgType(void (Class::*)(ExecutionContext, Arg) const);

template <typename Class, typename ExecutionContext, typename Arg>
/// A function pointer with two parameters used to declare a type.
Arg SecondArgType(void (Class::*)(ExecutionContext, Arg));

template <typename Class, typename Some, typename ExecutionContext,
          typename Arg>
/// A const function pointer with three parameters used to declare a type.
Arg ThirdArgType(void (Class::*)(Some, ExecutionContext, Arg) const);

template <typename Class, typename Some, typename ExecutionContext,
          typename Arg>
/// A function pointer with three parameters used to declare a type.
Arg ThirdArgType(void (Class::*)(Some, ExecutionContext, Arg));

/// A function declaration with std::function<void> parameter.
void FuncArgType(std::function<void()>);

/// A template function declaration with std::function<Arg> parameter.
template <typename Arg>
Arg FuncArgType(std::function<void(Arg)>);

template <typename Callable>
/// Declares the type of a callable with two parameters.
struct AsyncResultTypeImpl {
  using FirstArgType =
      RemoveRefAndConst<decltype(SecondArgType(&Callable::operator()))>;
  using type = RemoveRefAndConst<decltype(FuncArgType(FirstArgType()))>;
};

template <typename Callable>
/// An alias for the `Type` from callable object with two parameters.
using AsyncResultType = typename AsyncResultTypeImpl<Callable>::type;

template <typename Callable>
/// Declares the type of a callable with three parameters.
struct DeducedTypeImpl {
  using ThirdArgTypeParam =
      RemoveRefAndConst<decltype(ThirdArgType(&Callable::operator()))>;
  using type = RemoveRefAndConst<decltype(FuncArgType(ThirdArgTypeParam()))>;
};

template <typename Callable>
/// An alias for the `Type` from callable object with three parameters.
using DeducedType = typename DeducedTypeImpl<Callable>::type;

/// An interface for untyped pointer which allows
/// to convert any data type to a void pointer.
struct UntypedSmartPointer {
  virtual void* Get() const = 0;
  virtual ~UntypedSmartPointer() = default;
};

/// An implementation of `UntypedSmartPointer` interface.
template <typename SmartPointer>
struct TypedSmartPointer : UntypedSmartPointer {
  /// A move contructor.
  explicit TypedSmartPointer(SmartPointer&& pointer)
      : pointer_(std::move(pointer)) {}

  /// Method converts data to a void pointer.
  void* Get() const override { return static_cast<void*>(pointer_.get()); }

  SmartPointer pointer_;
};

/// A function wraps a void pointer with a unique pointer.
template <typename SmartPointer>
inline std::unique_ptr<UntypedSmartPointer> MakeUntypedUnique(
    SmartPointer&& ptr) {
  return std::make_unique<TypedSmartPointer<SmartPointer>>(std::move(ptr));
}

}  // namespace internal
}  // namespace thread
}  // namespace olp
