# HERE OLP SDK C++ Contributors Guide

## Development

### Code Style

This SDK follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). If we differ from the Google C++ Style Guide in any way or are more specific on a given point, it will be documented here.

#### Differences from the Google C++ Style Guide

##### Naming

###### File Names

* Filenames should be CamelCase, e.g. `MyClassName.cpp`.
* C++ files should end in `.cpp` and header files should end in `.h`.
* Inline functions can be moved into a separate `.inl` file to make the `.h` file more neatly arranged.
* Files which contain test code should end with `Test`, e.g. `MyClassNameTest.cpp`.

###### Enumerator Names

* The enumeration name should be CamelCase as it is a type.
* Individual enumerators should also be CamelCase.
* Type-safe scoped `enum class` should be used instead of `enum`.

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

The SDK uses `clang-format` to automatically format all source code before submission. The SDK follows the standard `Google` `clang-format` style as can be seen in the [.clang-format](./.clang-format) file at the root of this project.

If you need to format one or more files, you can simply run `clang-format` manually:

```bash
clang-format -i --style=file <file>....
```

Or you can format all changes in your most recent commit as follows:

```bash
git clang-format HEAD^ --style=file
git commit -a --amend --no-edit
```

If you need to format all .cpp and .h files in a directory recursively, you can use this handy [one-liner](https://stackoverflow.com/a/36046965/275524) from Stack Overflow:

```bash
find foo/bar/ -iname *.h -o -iname *.cpp | xargs clang-format -i --style=file
```

### Design Guidelines

* Follow the **SOLID** design principles:
  <br />**Single responsibility** - a class should not have more than one reason to change.
  <br />**Open/closed principle** - software should be open for extension by identifying customization points and closed for modification by making small parts well-defined and composable.
  <br />**Liskov substitution principle** - subtypes of a class or interface should not break the API contract of that class or interface.
  <br />**Interface segregation** - many client-specific interfaces are better than one general-purpose interface.
  <br />**Dependency inversion** - one should depend up abstractions, not concretions. Give dependencies to objects explicitly instead of pulling them implicitly from the inside.
* Avoid implementation inheritance, use composition instead. Inherit not to reuse but to be reused.
* Avoid adding additional libraries, we want to keep the SDK as slim as possible and also have to pass OSS.
* **DRY** - don't repeat yourself. Prefer to factor out common code (and multiple, very similar implementations) to reduce testing and future maintenance costs.
* **YAGNI** - you aren't gonna need it. There should be a clear and well-defined requirement for a component before it is added.
* **KISS** - keep it simple, stupid. Everything should be made as simple as possible, but not simpler.
* “Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away.” – Antoine de Saint-Exupery

### Implementation Guidelines

* Use the [C++11 standard](http://www.stroustrup.com/C++11FAQ.html).
* Follow the [Rule of 5](https://cpppatterns.com/patterns/rule-of-five.html).
* Use [RAII](https://en.cppreference.com/w/cpp/language/raii). Explicit use of new and delete should be avoided unless there is a good reason to do so.
* Always prefer an implementation which is platform independent. If platform-specific code is required, prefer abstractions which isolate the details to pre-processor macros.
* Avoid defining complex functions. Aim to reduce the number of parameters, lines of code and number of local variables for each function you write.
* Good code is not "clever". It does things in straightforward, obvious ways. The code itself should explain what is done. The need for comments should be minimal.
* Use meaningful naming conventions for all but the most transient of objects. The name of something is informative about when and how to use the object.
* Always keep performance and memory consumption in mind.
* Do not make changes to the code which is automatically generated, change the generator template instead.
* Be [const correct](https://isocpp.org/wiki/faq/const-correctness).
* Be explicit when you need to [support r-values](http://thbecker.net/articles/rvalue_references/section_01.html).
* Use `#pragma once` for include guards.
* Use `nullptr` instead of NULL or 0.

### Testing Guidelines

* Use [googletest](https://github.com/google/googletest) as test framework.
* Each unit test should endeavor to test one function only.
* Good tests should serve as a good example of how to use the code under test.
* Design and Implementation Guidelines apply to tests as well.

### Component Guidelines

#### Component Name

The SDK uses the following pattern for component name:  **olp-cpp-sdk-***component-name*, e.g. `olp-cpp-sdk-dataservice-read`.

#### API Namespace

The SDK uses the following pattern for namespace name: **olp**::*component::subcomponent*, e.g. `olp::authentication`, `olp::dataservice::write`.

#### API Class Name

The SDK uses the following pattern for API class name : *Api***Client**, e.g. `StreamLayerClient`

#### API Method Signature

The SDK provides a set of common classes that are used when defining the APIs.
These classes are located in the *olp-cpp-sdk-core/include/olp/core/client* folder.

|Base class|Description|
|------|------|
|**ApiResponse**|Template class representing the outcome of making a request. It either contains the result or the error.|
|**ApiError**|Error information container.|
|**ErrorCode**|Common error codes holder.|
|**CancellationToken**|Control object for cancelling asynchronous requests.|
|**CancellableFuture**|Template class used for retrieving the response to a synchronous call. This class has built in support for cancelling the synchronous call.|

The SDK recommends exposing both synchronous and asynchronous versions for a particular API method.
The SDK uses the following pattern for defining API methods:

// Synchronous cancellable API
<br />**olp::client::CancellableFuture**<*olp::client::Api***Response**>  *Api* (const *olp::client::Api***Request**& request);

// Asynchronous version of the same API
<br />using *olp:client::Api***Callback** = std::function<void(*olp::client::Api***Response** response)>;
<br />**olp::client::CancellationToken**  *Api* (const *olp::client::Api***Request**& request, const *olp:client::Api***Callback**& callback);

where the *Api***Response** is defined like this:
using *Api***Response** = client::ApiResponse<*Api***Result**, client::ApiError>;

Both the *Api***Result** and the *Api***Request** are custom classes defined by the API component.

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

The  SDK uses **boost::optional** for optional members, parameters or return types. Also, function comments should indicate whether the parameter is optional or required.

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
Using a **shared_ptr<std::vector<unsigned char>>** will ensure that no copy of the data is required while transferring or processing it.

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
API classes that use **OlpClient** should have **OlpClientSettings** as an input parameter to their constructor in order to enable client customization of the settings.

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

The SDK uses CMake as a build tool and provides a set of utility scripts to build components, export component targets and create and install component configuration files. These scripts are located in the *cmake* folder.
Each component should have it's own CMake script(s) that define the project and unit test targets. The component can use **export_config()** method (from *cmake/common.cmake*) to export component so it can be used by a client application that is using **find_package()** to retrieve the component.

A sample component CMake files would look like this:

<br />**project**(*olp-cpp-sdk*-**component** VERSION 1.0.0 DESCRIPTION "Component descripton")
<br />...
<br />// install component
<br />**install** (FILES ${COMPONENT_INC} DESTINATION ${INCLUDE_DIRECTORY}/olp/component)
<br />**export_config()**
<br />  if(**OLP_SDK_ENABLE_TESTING**)
<br />add_subdirectory(**test/unit**)
<br />endif()

#### Component Documentation

The SDK uses [Doxygen](http://www.doxygen.nl/) for generating its documentation from the code.
Each header file that is exposed by the component should be properly documented using the doxygen format.
The SDK provides a documentation build script in the *docs* folder.
This can be used as a template on how to to create a component level documentation build script.
