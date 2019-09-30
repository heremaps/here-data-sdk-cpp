# HERE OLP SDK for C++ Contributor Guide

## Development

### Code Style

This SDK follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). In case we differ from the Google C++ Style Guide in any way or are more specific on a given point, the following document reflects such differences.

#### Differences from the Google C++ Style Guide

##### Naming

###### File Names

* Name files in CamelCase, for example `MyClassName.cpp`.
* End C++ files with a `.cpp` and header files with an `.h`.
* You can move the inline functions into a separate `.inl` file to neatly arrange the `.h` file.
* End files, which contain the test code with a `Test`, for example `MyClassNameTest.cpp`.

###### Enumerator Names

* The enumeration name is a type, thus use CamelCase for naming.
* For Individual enumerators also use CamelCase.
* Use Type-safe scoped `enum class` instead of `enum`.

```cpp
enum class FetchOption {
    LatestVersion,
    OnlineIfNotFound,
    OnlineOnly,
    CacheOnly,
    CacheWithUpdate,
};
```

### File Headers

Every `.cpp`, `.h` and `.inl` file must contain the following header at the very top:

```cpp
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
```

CMake files or bash scripts must contain the following header at the very top:

```cmake
# Copyright (C) 2019 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE
```

### Code Formatting

The SDK uses `clang-format` to automatically format all source code before submission. The SDK follows the standard `Google` `clang-format` style, as seen in the [.clang-format](./.clang-format) file at the root of this project.

If you need to format one or more files, you can run `clang-format` manually:

```bash
clang-format -i --style=file <file>....
```

Alternatively, you can format all changes in your most recent commit as follows:

```bash
git clang-format HEAD^ --style=file
git commit -a --amend --no-edit
```

If you need to format all `.cpp` and `.h` files in a directory recursively, you can use this handy [one-liner](https://stackoverflow.com/a/36046965/275524) from Stack Overflow:

```bash
find foo/bar/ -iname *.h -o -iname *.cpp | xargs clang-format -i --style=file
```

### Design Guidelines

* Follow the **SOLID** design principles:

  <br />**Single responsibility** - a class cannot have more than one reason for a change.
  <br />**Open/closed principle** - keep software open for extension by identifying customization points and closed for modification by making small parts well-defined and composable.
  <br />**Liskov substitution principle** - do not break the API contract of class or interface with subtypes of a class or interface.
  <br />**Interface segregation** - many client-specific interfaces are better than one general-purpose interface.
  <br />**Dependency inversion** - one depends up abstractions, not concretions. Give dependencies to objects explicitly instead of pulling them implicitly from the inside.
* Avoid implementation inheritance, use composition instead. Inherit not to reuse but to be reused.
* Avoid adding additional libraries. We want to keep the SDK as slim as possible and be able to pass the OSS.
* **DRY** - do not repeat yourself. Prefer to factor out common code, and multiple, very similar implementations to reduce testing and future maintenance costs.
* **YAGNI** - you won't need it. There must be a clear and well-defined requirement for a component before you add it.
* **KISS** - keep it simple, stupid. Make everything as simple as possible, but not simpler.
* "Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away." – Antoine de Saint-Exupery.

### Implementation Guidelines

* Use the [C++11 standard](http://www.stroustrup.com/C++11FAQ.html).
* Use [RAII](https://en.cppreference.com/w/cpp/language/raii). Avoid explicit use of new and delete unless there is a good reason to do so.
* Always prefer a platform-independent implementation. If platform-specific code is required, prefer abstractions, which isolate the details to pre-processor macros.
* Avoid defining complex functions. Aim to reduce the number of parameters, lines of code, and several local variables for every function you write.
* Good code is not overcomplicated. It does things in straightforward, obvious ways. The code itself explains what it does. The need for comments should be minimal.
* Use meaningful naming conventions for all but the most transient of objects. The name of something is informative about when and how to use the object.
* Always keep performance and memory consumption in mind.
* Do not make changes to the code, which is automatically generated. Change the generator template instead.
* Be [const correct](https://isocpp.org/wiki/faq/const-correctness).
* Be explicit when you need to [support r-values](http://thbecker.net/articles/rvalue_references/section_01.html).
* Use `#pragma once` for include guards.
* Use `nullptr` instead of NULL or 0.

### Testing Guidelines

* Use [googletest](https://github.com/google/googletest) as a test framework.
* Each unit test endeavor to test one function only.
* Good tests serve as a good example of how to use the code under test.
* Design and Implementation Guidelines apply to tests as well.

### Component Guidelines

#### Component Name

The SDK uses the following pattern for component name:  **olp-cpp-sdk-***component-name*, for example `olp-cpp-sdk-dataservice-read`.

#### API Namespace

The SDK uses the following pattern for namespace name: **olp**::*component::subcomponent*, for example `olp::authentication`, `olp::dataservice::write`.

#### API Class Name

The SDK uses the following pattern for API class name: *Api***Client**, for example `StreamLayerClient`

#### API Method Signature

The SDK provides a set of common classes that are used to define the APIs.
These classes are located in the *olp-cpp-sdk-core/include/olp/core/client* folder.

|Base class|Description|
|------|------|
|**ApiResponse**|Template class represents the outcome of making a request. It either contains the result or the error.|
|**ApiError**|Error information container.|
|**ErrorCode**|Common error codes holder.|
|**CancellationToken**|Control object to cancel asynchronous requests.|
|**CancellableFuture**|Use this template class to retrieve the response to the synchronous call. This class has built-in support to cancel the synchronous call.|

The SDK recommends to expose both synchronous and asynchronous versions for a particular API method.
The SDK uses the following pattern to define API methods:

// Synchronous cancelable API
<br />**olp::client::CancellableFuture**<*olp::client::Api***Response**>  *Api* (const *olp::client::Api***Request**& request);

// Asynchronous version of the same API
<br />using *olp:client::Api***Callback** = std::function<void(*olp::client::Api***Response** response)>;
<br />**olp::client::CancellationToken** *Api* (const *olp::client::Api***Request**& request, const *olp:client::Api***Callback**& callback);

where the *Api***Response** is defined like this:
using *Api***Response** = client::ApiResponse<*Api***Result**, client::ApiError>;

Both *Api***Result** and *Api***Request** are custom classes that are defined by the API component.

Example:

```cpp
// Forward declarations and aliases
class CatalogRequest;
using CatalogResult = model::Catalog;
using CatalogResponse = client::ApiResponse<CatalogResult, client::ApiError>;
using CatalogResponseCallback = std::function<void(CatalogResponse response)>;

// Synchronous cancellable API
olp::client::CancellableFuture<CatalogResponse> GetCatalog(const CatalogRequest& request);

// Asynchronous version of the same API
olp::client::CancellationToken GetCatalog(const CatalogRequest& request, const CatalogResponseCallback& callback);
```

#### API Setters and Getters

The SDK uses the following pattern for getters and setters:
**WithX()** functions returning a reference to this for setters, with explicit R-value overloads.
**GetX()** functions for getters.

Example:

```cpp
inline const std::string& GetLayerId() const { return layer_id_; }

inline PublishDataRequest& WithLayerId(const std::string& layer_id) {
  layer_id_ = layer_id;
  return *this;
}

inline PublishDataRequest& WithLayerId(std::string&& layer_id) {
  layer_id_ = std::move(layer_id);
  return *this;
}
```

#### API Optional Parameters

The SDK uses **boost::optional** for optional members, parameters or return types. Also, function comments should indicate whether the parameter is optional or required.

Example:

```cpp
inline const boost::optional<std::string>& GetBillingTag() const {
  return billing_tag_;
}

inline PublishDataRequest& WithBillingTag(const std::string& billing_tag) {
  billing_tag_ = billing_tag;
  return *this;
}

inline PublishDataRequest& WithBillingTag(std::string&& billing_tag) {
  billing_tag_ = std::move(billing_tag);
  return *this;
}
```

#### API Binary Data

The SDK uses **std::vector<unsigned char>** type for representing binary data.
Using a **shared_ptr<std::vector<unsigned char>>** ensures that no copy of the data is required while transferring or processing it.

Example:

```cpp
inline std::shared_ptr<std::vector<unsigned char>> GetData() const {
  return data_;
}

inline PublishDataRequest& WithData(const std::shared_ptr<std::vector<unsigned char>>& data) {
  data_ = data;
  return *this;
}

inline PublishDataRequest& WithData(std::shared_ptr<std::vector<unsigned char>>&& data) {
  data_ = std::move(data);
  return *this;
}
```

### REST Client

The SDK provides a unified way of performing REST requests by exposing a set of utility classes in the *olp-cpp-sdk-core/include/olp/core/client* folder.

|  Base class | Description  |
| ------ | ------ |
| **OlpClient** | REST client class that provides all the functionality required to execute requests through the network stack.|
| **OlpClientSettings** | Settings class consisting of authentication, network proxy and retry settings.  |
| **OlpClientFactory**   | Factory class for the class **OlpClient**. |

API classes that need to perform REST request should use the **OlpClient** class.
API classes that use **OlpClient** should have **OlpClientSettings** as an input parameter to their constructor to enable client customization of the settings.

Example:

```cpp
/**
 * @brief The CatalogClient class.
 */
class DATASERVICE_READ_API CatalogClient final {
 public:
  CatalogClient(
      const client::HRN& hrn,
      std::shared_ptr<client::OlpClientSettings> settings,
      std::shared_ptr<cache::KeyValueCache> cache = CreateDefaultCache());

  // Class declaration

}
```

#### Components Build Script

The SDK uses CMake as a build tool and provides a set of utility scripts to build components, export component targets, create and install component configuration files. These scripts are located in the *cmake* folder.
Make sure each component has it's own CMake script(s) that define the project and unit test targets. The component can use **export_config()** method (from *cmake/common.cmake*) to export component so it can be used by a client application that is using **find_package()** to retrieve the component.

A sample component CMake files looks as follows:

<br />**project**(*olp-cpp-sdk*-**component** VERSION 1.0.0 DESCRIPTION "Component descripton")
<br />...
<br />// install component
<br />**install** (FILES ${COMPONENT_INC} DESTINATION ${INCLUDE_DIRECTORY}/olp/component)
<br />**export_config()**
<br />  if(**EOLP_SDK_ENABLE_TESTING**)
<br />add_subdirectory(**test/unit**)
<br />endif()

#### Component Documentation

The SDK uses [Doxygen](http://www.doxygen.nl/) to generate its documentation from the code.
Make sure to properly document each header file that is exposed by the component with the use of Doxygen format.
The SDK provides a documentation build script in the *docs* folder, which can be used as a template to create a component level documentation build script.
