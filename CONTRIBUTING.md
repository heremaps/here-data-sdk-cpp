# HERE OLP SDK for C++ Contributor Guide

## Contributing to HERE OLP SDK for C++

First off, thanks for taking the time to contribute! The team behind the [HERE OLP SDK for C++](https://github.com/heremaps/here-data-sdk-cpp) gratefully accepts contributions via [pull requests](https://help.github.com/articles/about-pull-requests/) filed against the [GitHub project](https://github.com/heremaps/here-data-sdk-cpp/pulls). As part of filing a pull request, we ask you to sign off the [Developer Certificate of Origin](https://developercertificate.org/) (DCO).

## Development

### Code Style

This SDK follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). In case we differ from the Google C++ Style Guide in any way or are more specific on a given point, the following document reflects such differences.

#### Differences from the Google C++ Style Guide

##### File Names

- Name files in CamelCase. For example, `MyClassName.cpp`.
- End C++ files with `.cpp` and header files with `.h`.
- You can move inline functions into a separate `.inl` file to neatly arrange `.h` files.
- End files that contain a test code with `Test`. For example, `MyClassNameTest.cpp`.

##### Enumerator Names

- Use CamelCase for enumeration names, as an enumeration name is a type.
- Use CamelCase for individual enumerators.
- Use a type-safe scoped `enum class` instead of `enum`.

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

Every `.cpp`, `.h`, and `.inl` file must contain the following header at the very top:

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

The SDK uses `clang-format` to automatically format all source code before submission. The SDK follows the standard Google `clang-format` style, as seen in the [.clang-format](./.clang-format) file at the root of this project.

If you need to format one or more files, you can run `clang-format` manually:

```bash
clang-format -i --style=file <file>....
```

Alternatively, you can format all changes in your most recent commit as follows:

```bash
git clang-format HEAD^ --style=file
git commit -a --amend --no-edit
```

If you need to format all `.cpp` and `.h` files in a directory recursively, you can use this [one-liner](https://stackoverflow.com/a/36046965/275524) from Stack Overflow:

```bash
find foo/bar/ -iname *.h -o -iname *.cpp | xargs clang-format -i --style=file
```

### Style Guidelines for Commit Messages

We strive to maintain a high software quality standard and strongly encourage contributions to keep up with the the guidelines.
In general, we share the view on how a commit message should be written with the [Git project itself](https://github.com/git/git/blob/master/Documentation/SubmittingPatches).

When you create a commit message, follow these guidelines:

- [Make separate commits for logically separate changes](https://github.com/git/git/blob/e6932248fcb41fb94a0be484050881e03c7eb298/Documentation/SubmittingPatches#L43). For example, simple formatting changes that do not affect software behavior usually do not belong in the same commit as changes to program logic.
- [Describe your changes well](https://github.com/git/git/blob/e6932248fcb41fb94a0be484050881e03c7eb298/Documentation/SubmittingPatches#L101). Do not just repeat in prose what is "obvious" from the code, but provide a rationale explaining why you believe your change is necessary.
- [Describe your changes in the imperative mood and present tense](https://github.com/git/git/blob/e6932248fcb41fb94a0be484050881e03c7eb298/Documentation/SubmittingPatches#L133). Instead of writing "Fixes an issue with encoding" prefer "Fix an encoding issue". Think about it like the commit only does something if it is applied. Such approach usually results in more concise commit messages.
- [We are picky about whitespaces](https://github.com/git/git/blob/e6932248fcb41fb94a0be484050881e03c7eb298/Documentation/SubmittingPatches#L95). Trailing whitespace and duplicate blank lines are simply an annoyance, and most Git tools flag them red in the diff anyway.
- Limit the first line to 72 characters or less.
- If you ever wondered how a nearly "perfect" commit message looks like, have a look at one of [Jeff King's commits](https://github.com/git/git/commits?author=peff) in the Git project for references and ideas.
- When you address a review comments in a pull request, please fix the issue in the commit where it appears, not in a new commit on top of the pull request history. While this requires force-pushing of the new iteration of you pull request branch, it has several advantages:
  - Reviewers that go through (larger) pull requests commit by commit are always up-to-date with the latest fixes instead of coming across a commit that addresses one comment or possibly also their remarks only at the end.
  - It maintains a cleaner history without distracting commits like "Address review comments".
  - As a result, tools like [git-bisect](https://git-scm.com/docs/git-bisect) can operate in a more meaningful way.
  - When you fix up commits, you can also change your commit messages, which is not possible if you just add new commits.
- If you are unfamiliar with how to fix up the existing commits, please read about [rewriting history](https://git-scm.com/book/id/v2/Git-Tools-Rewriting-History) and `git rebase --interactive` in particular.
- To resolve conflicts, rebase pull request branches onto their target branch instead of merging the target branch into the pull request branch—such approach results in a cleaner history without "criss-cross" merges.

### Design Guidelines

- Follow the **SOLID** design principles:

  - **Single responsibility** – a class cannot have more than one reason for a change.
  - **Open/closed principle** – keep software open for extension by identifying customization points and closed for modification by making small parts well-defined and composable.
  - **Liskov substitution principle** – do not break the API contract of a class or interface with subtypes of a class or interface.
  - **Interface segregation** – many client-specific interfaces are better than one general-purpose interface.
  - **Dependency inversion** – one depends on abstractions, not concretions. Give dependencies to objects explicitly instead of pulling them implicitly from the inside.

- Avoid implementation inheritance. Use composition instead. Inherit not to reuse but to be reused.
- Avoid adding additional libraries. We want to keep the SDK as slim as possible and be able to pass the OSS.
- **DRY** – do not repeat yourself. Prefer to factor out common code and multiple very similar implementations to reduce testing and future maintenance costs.
- **YAGNI** – you won't need it. There must be a clear and well-defined requirement for a component before you add it.
- **KISS** – keep it simple, stupid. Make everything as simple as possible, but not simpler.

> "Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away." – Antoine de Saint-Exupery

### Implementation Guidelines

- Use the [C++11 standard](http://www.stroustrup.com/C++11FAQ.html).
- Use [RAII](https://en.cppreference.com/w/cpp/language/raii). Avoid explicit use of new and delete unless there is a good reason to do so.
- Always prefer a platform-independent implementation. If platform-specific code is required, prefer abstractions, which isolate the details to pre-processor macros.
- Avoid defining complex functions. Aim to reduce the number of parameters, lines of code, and several local variables for every function you write.
- Good code is not overcomplicated. It does things in straightforward, obvious ways. The code itself explains what it does. The need for comments should be minimal.
- Use meaningful naming conventions for all but the most transient of objects. The name of something informs when and how to use the object.
- Always keep performance and memory consumption in mind.
- Do not make changes to the code that is automatically generated. Change the generator template instead.
- Be [const correct](https://isocpp.org/wiki/faq/const-correctness).
- Be explicit when you need to [support rvalues](http://thbecker.net/articles/rvalue_references/section_01.html).
- Use `#pragma once` to include guards.
- Use `nullptr` instead of `NULL` or `0`.

### Testing Guidelines

- Use [Google Test](https://github.com/google/googletest) as a test framework.
- Each unit test endeavors to test one function only.
- Good tests serve as a good example of how to use the code under test.
- [Design](#design-guidelines) and [implementation](#implementation-guidelines) Guidelines apply to tests as well.

### Component Guidelines

#### Component Name

The SDK uses the following pattern for component names: **olp-cpp-sdk**-_component-name_. For example, `olp-cpp-sdk-dataservice-read`.

#### API Namespace

The SDK uses the following pattern for namespace names: **olp**::_component::subcomponent_. For example, `olp::authentication`,`olp::dataservice::write`.

#### API Class Name

The SDK uses the following pattern for API class names: \*Api**\*Client**. For example, `StreamLayerClient`.

#### API Method Signature

The SDK provides a set of common classes that are used to define the APIs.
These classes are located in the **olp-cpp-sdk-core/include/olp/core/client** folder.

| Base class          | Description                                                                                                                                   |
| ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| `ApiResponse`       | Template class that represents the request outcome. It contains a result or an error.                                                         |
| `ApiError`          | Error information container.                                                                                                                  |
| `ErrorCode`         | Common error codes holder.                                                                                                                    |
| `CancellationToken` | Control object that you can use to cancel asynchronous requests.                                                                              |
| `CancellableFuture` | Template class that you can use to retrieve a response to a synchronous call. This class has built-in support to cancel the synchronous call. |

The SDK recommends exposing both synchronous and asynchronous versions for a particular API method.
The SDK uses the following pattern to define API methods:

- The synchronous cancellable API:

  ```cpp
  olp::client::CancellableFuture<olp::client::ApiResponse> Api (const olp::client::ApiRequest& request);
  ```

- The asynchronous version of the same API:

  ```cpp
  using olp:client::ApiCallback = std::function<void(olp::client::ApiResponse response)>;
  olp::client::CancellationToken Api (const olp::client::ApiRequest& request, const olp:client::ApiCallback& callback);
  ```

The `ApiResponse` is defined in the following way:

```cpp
using ApiResponse = client::ApiResponse<ApiResult, client::ApiError>;
```

Both `ApiResult` and `ApiRequest` are custom classes that are defined by the API component.

Example:

```cpp
// Forwards declarations and aliases.
class CatalogRequest;
using CatalogResult = model::Catalog;
using CatalogResponse = client::ApiResponse<CatalogResult, client::ApiError>;
using CatalogResponseCallback = std::function<void(CatalogResponse response)>;

// The synchronous cancellable API.
olp::client::CancellableFuture<CatalogResponse> GetCatalog(const CatalogRequest& request);

// The asynchronous version of the same API.
olp::client::CancellationToken GetCatalog(const CatalogRequest& request, const CatalogResponseCallback& callback);
```

#### API Setters and Getters

The SDK uses the following pattern for getters and setters:

- `WithX()` functions that return a reference to this for setters and have explicit rvalue overloads.
- `GetX()` functions for getters.

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

The SDK uses `boost::optional` for optional members, parameters, or return types. Also, function comments should indicate whether the parameter is optional or required.

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

The SDK uses the `std::vector<unsigned char>` type to represent binary data.
Using `shared_ptr<std::vector<unsigned char>>` ensures that no data copy is required to transfer or process data.

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

The SDK provides a unified way of performing REST requests by exposing a set of utility classes in the **olp-cpp-sdk-core/include/olp/core/client** folder.

| Base class          | Description                                                                                                   |
| ------------------- | ------------------------------------------------------------------------------------------------------------- |
| `OlpClient`         | REST client class that provides all the functionality required to execute requests through the network stack. |
| `OlpClientSettings` | Settings class that consists of authentication, network proxy, and retry settings.                            |
| `OlpClientFactory`  | Factory class for the `OlpClient` class.                                                                      |

API classes that need to perform REST requests should use the `OlpClient` class.
API classes that use `OlpClient` should have `OlpClientSettings` as an input parameter to their constructor to enable client customization of the settings.

Example:

```cpp
/**
 * @brief Provides a high-level interface to access data hosted on `OLP` using
 * the HERE Open Location Platform (OLP) Data API.
 *
 * The behavior of the `CatalogClient` object can be defined via
 * `OlpClientSettings`.
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

The SDK uses CMake as a build tool and provides a set of utility scripts to build components, export component targets, as well as create and install component configuration files. These scripts are located in the **cmake** folder.
Make sure that each component has it's own CMake script or scripts that define the project and unit test targets. The component can use the **export_config()** method (from **cmake/common.cmake**) to export a component so that it can be used by a client application that uses **find_package()** to retrieve the component.

Sample component CMake files look as follows:

```cpp
project(olp-cpp-sdk-component VERSION 1.0.0 DESCRIPTION "Component descripton")

// Installs the component.
install (FILES ${COMPONENT_INC} DESTINATION ${INCLUDE_DIRECTORY}/olp/component)
export_config()
if(EOLP_SDK_ENABLE_TESTING)
  add_subdirectory(test/unit)
endif()
```

#### Component Documentation

The SDK uses [Doxygen](http://www.doxygen.nl/) to generate its documentation from the code.
Make sure to properly document each header file that is exposed by the component with the use of Doxygen format.
The SDK provides a documentation build script in the **doc** folder that can be used as a template to create a component level documentation build script.
