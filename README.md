# HERE Data SDK for C++

HERE Data SDK for C++ is a C++ client for the <a href="https://platform.here.com" target="_blank">HERE platform</a>.

## Health check

### Build and test

| Linux GCC | Linux CLang | MacOS |
| :-------- | :---------- | :---- |
| [![Linux gcc build status][1]][2] | [![Linux Clang build status][3]][4] | [![MacOS build status][5]][4] |

| iOS | Windows | Android |
| :-- | :------ | :------ |
| [![iOS build status][6]][4] | [![Windows build status][7]][4] | [![Android build status][8]][4] |

[1]: https://github.com/heremaps/here-data-sdk-cpp/workflows/CI/badge.svg?branch=master
[2]: https://github.com/heremaps/here-data-sdk-cpp/actions?query=workflow%3ACI+branch%3Amaster
[3]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Linux_build_clang
[4]: https://dev.azure.com/heremaps/here-data-sdk/_build/latest?definitionId=3&branchName=master
[5]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=MacOS_build
[6]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=iOS_build
[7]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Windows_build
[8]: https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Android_build

### Test coverage

| Platform | Status                                                                                                                                                                                              |
| :------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Linux    | <a href="https://codecov.io/gh/heremaps/here-data-sdk-cpp/" target="_blank"><img src="https://codecov.io/gh/heremaps/here-data-sdk-cpp/branch/master/graph/badge.svg" alt="Linux code coverage"/></a> |

## Why use

Data SDK for C++ provides support for the core HERE platform use cases through a set of native C++ interfaces. The Data SDK is intended to save your time and effort on using HERE REST APIs. It provides a set of stable APIs that simplify complex platform operations and keeps up to date with the latest HERE REST API changes.

Data SDK for C++ is a modern (C++11), lightweight, and modular SDK with minimal dependencies targeted towards a wide range of hardware platforms from embedded devices to desktops.

This SDK lets you:

- Authenticate to HERE platform.
- Read catalog and partition metadata.
- Retrieve data from versioned, volatile, and stream layers of the platform catalogs.
- Upload data to the platform.

Additionally, the Data SDK includes classes for work with geospatial tiling schemes that are used by most platform catalog layers.

## Backward compatibility

We try to develop and maintain our API in a way that preserves its compatibility with the existing applications. Changes in Data SDK for C++ are greatly influenced by the Data API development. Data API introduces breaking changes 6 months in advance. Therefore, you may need to migrate to a new version of Data SDK for C++ every half a year.

For more information on Data API, see its <a href="https://developer.here.com/documentation/data-api/data_dev_guide/index.html" target="_blank">Developer Guide</a> and <a href="https://developer.here.com/documentation/data-api/api-reference.html" target="_blank">API Reference</a>.

When new API is introduced in Data SDK for C++, the old one is not deleted straight away. The standard API deprecation time is 6 months. It gives you time to switch to new code. However, we do not provide ABI backward compatibility.

All of the deprecated methods, functions, and parameters are documented in the Data SDK for C++ <a href="https://developer.here.com/documentation/sdk-cpp/api_reference/index.html" target="_blank">API Reference</a> and <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CHANGELOG.md" target="_blank">changelog</a>.

For more information on Data SDK for C++, see our <a href="https://developer.here.com/documentation/sdk-cpp/dev_guide/index.html" target="blank">Developer Guide</a>.

## Supported platforms

The table below lists the platforms on which the Data SDK has been tested.

| Platform                   | Minimum requirement     |
| :------------------------- | :---------------------- |
| Ubuntu Linux               | GCC 7.5 and Clang 7.0   |
| Embedded Linux (32 bit)    | GCC 7.4 armhf           |
| Windows                    | MSVC++ 2017             |
| macOS                      | Apple Clang 11.0.0      |
| iOS                        | Xcode 11.1, Swift 5.0   |
| Android                    | API level 21            |

<h6 id="dependencies"></h6>

## Dependencies

The table below lists the dependencies of the Data SDK.

| Library              | Minimum version |
| :------------------- | :-------------- |
| OpenSSL              | 1.1.1           |
| Boost (headers only) | 1.69.0          |
| LevelDB              | 1.21            |
| Snappy               | 1.1.7           |
| RapidJSON            | latest          |

<h6 id="additional-linux-dependencies"></h6>

### Additional Linux dependencies

To build the Data SDK on Linux, additionally to the dependencies listed in the previous section, you also need to have <a href="https://curl.haxx.se/download.html" target="_blank">libcurl</a> 7.47.0 or later.

To install the dependencies on Linux, run the following command:

```bash
sudo apt-get update && sudo apt-get --yes install git g++ make cmake libssl-dev libcurl4-openssl-dev libboost-all-dev
```

## Install the SDK

By default, the Data SDK downloads and compiles its dependencies. The versions of the downloaded dependencies may conflict with the versions that are already installed on your system. Therefore, the downloaded dependencies are not added to the install targets.

You can use the Data SDK in your CMake project or install it on your system.

Тo use the Data SDK directly in your CMake project, add the Data SDK via `add_subdirectory()`.

**To install the Data SDK on your system:**

1. Install all the dependencies needed for the Data SDK.<br>For more information on dependencies, see the [Dependencies](#dependencies) and [Additional Linux dependencies](#additional-linux-dependencies) sections.

2. (Optional) To find the required dependencies in the system, set the `OLP_SDK_BUILD_EXTERNAL_DEPS` flag to `OFF`.

3. (Optional) To build the Data SDK as a shared library, set the `BUILD_SHARED_LIBS` flag to `ON`.

**Example**

The following command builds and installs the Data SDK:

```bash
cmake --build . --target install
```

## Build the SDK

<a href="https://cmake.org/download/" target="_blank">CMake</a> is the main build system. The minimal required version of CMake is 3.9.

CMake downloads <a href="https://github.com/google/leveldb" target="_blank">LevelDB</a>, <a href="https://github.com/google/snappy" target="_blank">Snappy</a>, <a href="https://github.com/Tencent/rapidjson" target="_blank">RapidJSON</a>, and <a href="https://www.boost.org/" target="_blank">Boost</a>. To disable downloading, set `OLP_SDK_BUILD_EXTERNAL_DEPS` to `OFF`. For details on CMake flags, see the [related](#cmake-flags) section.

**To build the Data SDK:**

1. Clone the repository folder.
2. In the root of the repository folder, run the following commands:

```bash
    mkdir build && cd build
    cmake ..
    cmake --build .
```

If you cannot build the Data SDK on Windows using this instruction, see [Build on Windows](#build-on-windows).

<h6 id="build-on-windows"></h6>

### Build on Windows

<a href="https://dev.azure.com/heremaps/here-data-sdk/_build/latest?definitionId=3&branchName=master" target="_blank"><img src="https://dev.azure.com/heremaps/here-data-sdk/_apis/build/status/heremaps.here-data-sdk-cpp?branchName=master&jobName=Windows_build" alt="Windows build status"/></a>

We assume that you have installed CMake, Microsoft Visual Studio 2017, and the Visual C++ tools for CMake component.

**To build the Data SDK on Windows:**

1. Launch Microsoft Visual Studio as administrator.

2. Open the folder containing the Data SDK or a CMake-based project that uses the Data SDK.

3. In Microsoft Visual Studio, check that the target does not contain "(Default)".<br>For example, select "x64-Debug" instead of "x64-Debug (Default)".

4. Using the CMake menu provided by the Visual C++ tools for CMake, generate the `.cmake` files, and build the entire project with default options.

> #### Note
> Microsoft Visual Studio uses a default build directory that has a long path name. Since dependencies for the Data SDK are installed within the build directory, it is recommended that you edit the generated `CMakeSettings.json` file and change the build directory path name to a shorter path name. This ensures that the maximum length of each path is not greater than 260 characters. For details, see the <a href="https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file" target="_blank">Naming Files, Paths, and Namespaces</a> section of the Windows Dev Center documentation.

### Generate documentation with Doxygen

If you want to build documentation from annotated source code, you need to have <a href="http://www.doxygen.nl/" target="_blank">Doxygen</a> and CMake version 3.9 or later.

To generate Doxygen documentation, set the `OLP_SDK_BUILD_DOC` flag to `ON` when running the CMake configuration:

```bash
mkdir build && cd build
cmake -DOLP_SDK_BUILD_DOC=ON ..
cmake --build . --target docs
```

<h6 id="cmake-flags"></h6>

### CMake flags

| Flag | Description |
| :- | :- |
| `BUILD_SHARED_LIBS` | Defaults to `OFF`. If enabled, all libraries are built as shared. |
| `OLP_SDK_BUILD_DOC` | Defaults to `OFF`. If enabled, the API reference is generated in your build directory.<br> **Note:** Before you download the API reference, install <a href="http://www.doxygen.nl/" target="_blank">Doxygen</a>. |
| `OLP_SDK_ENABLE_TESTING` | Defaults to `ON`. If enabled, unit tests are built for each library. |
| `OLP_SDK_BUILD_EXTERNAL_DEPS` | Defaults to `ON`. If enabled, CMake downloads and compiles dependencies. |
| `OLP_SDK_NO_EXCEPTION` | Defaults to `OFF`. If enabled, all libraries are built without exceptions. |
| `OLP_SDK_BOOST_THROW_EXCEPTION_EXTERNAL` | Defaults to `OFF`. When `OLP_SDK_NO_EXCEPTION` is `ON`, `boost` requires `boost::throw_exception()` to be defined. If enabled, the external definition of `boost::throw_exception()` is used. Otherwise, the library uses own definition. |
| `OLP_SDK_MSVC_PARALLEL_BUILD_ENABLE` (Windows Only) | Defaults to `ON`. If enabled, the `/MP` compilation flag is added to build the Data SDK using multiple cores. |
| `OLP_SDK_DISABLE_DEBUG_LOGGING`| Defaults to `OFF`. If enabled, The debug and trace level log messages will not be printed. |

## Use the SDK

To learn how to use the Data SDK, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/docs/GettingStartedGuide.md" target="_blank">Getting Started Guide</a> and the <a href="https://developer.here.com/documentation/sdk-cpp/dev_guide/index.html" target="blank">Developer Guide</a>.

## Contribution

For details, see <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/CONTRIBUTING.md" target="_blank">HERE Data SDK C++ Contributors Guide</a>.

## LICENSE

Copyright (C) 2019–2020 HERE Europe B.V.

For license details, see the <a href="https://github.com/heremaps/here-data-sdk-cpp/blob/master/LICENSE" target="_blank">LICENSE</a> file in the root of this project.

> #### Note
> This project has Open Source Software dependencies that are downloaded and installed upon execution of the abovementioned installation commands. For further details, see the CMake configuration files included in the <a href="https://github.com/heremaps/here-data-sdk-cpp/tree/master/external" target="_blank">external</a> directory.
