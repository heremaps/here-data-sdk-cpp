[![Build Status](https://travis-ci.com/heremaps/here-olp-edge-sdk-cpp.svg?branch=master)](https://travis-ci.com/heremaps/here-olp-edge-sdk-cpp)
[![Build Status](https://dev.azure.com/heremaps/github/_apis/build/status/heremaps.here-olp-edge-sdk-cpp?branchName=master)](https://dev.azure.com/heremaps/github/_build/latest?definitionId=1&branchName=master)

# HERE OLP Edge SDK C++

The HERE OLP Edge SDK is a _work in progress_ c++ client for [HERE Open Location Platform](https://platform.here.com).

## Why Use the SDK

The HERE OLP Edge SDK provides support for core **HERE Open Location Platform (OLP)** use cases through a set of native C++ interfaces. The SDK should save its users time and effort using OLP REST APIs and provide a set of stable APIs simplifying complex OLP operations and removing the burden of keeping up to date with the latest OLP REST API changes.
It is a modern (C++11 or later), lightweight, and modular SDK with minimal dependencies targeted towards a wide range of hardware platforms from embedded devices to desktops.

This SDK will allow you to:

* Authenticate to HERE Open Location Platform (OLP) using client credentials.
* Read catalog and partitions metadata.
* Retrieve data from versioned and volatile layers of OLP catalogs.
* Upload your own data to OLP.

Additionally, there are convenient classes for working with the geospatial tiling schemes that are used by most OLP catalog layers.

## Building the SDK

The HERE OLP Edge SDK has been tested on Linux, Windows, iOS, and Android. CMake is the main build system.

### Building on Linux

#### Dependencies

The HERE OLP Edge SDK has been tested with the following compilers:

| Compiler | Minimum version |
| -------- | --------------- |
| GCC | 4.9 |
| Clang | 3.3 |

Library dependencies:

| Library   | Minimum version |
| --------- | --------------- |
| OpenSSL   | 1.0.2 |
| Curl      | 7.47.0 |
| Boost (headers only) | 1.69.0 |
| leveldb   | 1.21 |
| snappy    | 1.1.7 |
| rapidjson | latest |

To install above dependencies, run:

```bash
sudo apt-get update && sudo apt-get --yes install git g++ make cmake libssl-dev libcurl4-openssl-dev libboost-all-dev
```

Additionally CMake downloads [leveldb](https://github.com/google/leveldb), [snappy](https://github.com/google/snappy), [rapidjson](https://github.com/Tencent/rapidjson) and [Boost](https://www.boost.org/) (Windows only).
This can be disabled by setting `EDGE_SDK_BUILD_EXTERNAL_DEPS` to `OFF`.

Optionally, if you want to build the documentation, you will need to install [Doxygen](http://www.doxygen.nl/).

#### Build

To build the HERE OLP Edge SDK run the following commands in the root of the repository folder after cloning:

```bash
mkdir build && cd build
cmake ..
make
```

To build the [Doxygen](http://www.doxygen.nl/) documentation, you will need to have CMake version 3.9 or greater, and set `EDGE_SDK_BUILD_DOC=ON` when running CMake configuration:

```bash
mkdir build && cd build
cmake -DEDGE_SDK_BUILD_DOC=ON ..
make docs
```

#### Install

By default this SDK downloads and compiles its dependencies. The version of downloaded dependencies may conflict with versions already installed on your system, therefore they are not added to the install targets.
To install the HERE OLP Edge SDK you must first install all its dependencies.
Then configure the SDK with `EDGE_SDK_BUILD_EXTERNAL_DEPS` set to `OFF`, so that it will find required dependencies in the system.
Another option is to build the SDK as a shared library, setting `BUILD_SHARED_LIBS` option to `ON`.

The following command builds and installs the HERE OLP Edge SDK:

```bash
cmake --build . --target install
```

Alternatively, if you want to use the SDK directly in your CMake project, you can add it via `add_subdirectory()`.

### Building on Windows

Launch Visual Studio as Administrator and open the folder containing the SDK or a CMake based project using the SDK.
Make sure the target in Visual Studio does not contain "* (Default)". For example, select "x64-Debug" instead of "x64-Debug (Default)".
Using the provided CMake menu by the Visual C++ tools for CMake, generate the cmake files and build the entire project normally.

Note: Visual Studio uses a default build directory that has a long path name.
It is recommended that you edit the generated CMakeSettings.json and change the build directory to a short path name since dependencies for the SDK are installed within the build directory.
This will ensure the maximum length for each individual path is not greater than 260 characters.
[See here for details](https://docs.microsoft.com/en-us/windows/desktop/fileio/naming-a-file)

### CMake flags

#### BUILD_SHARED_LIBS

(Defaults to `OFF`) If enabled, this will cause all libraries to be built shared.

#### EDGE_SDK_BUILD_DOC

(Defaults to `OFF`) If enabled, the API reference is generated in your build directory. Note requires [Doxygen](http://www.doxygen.nl/) to be installed.

#### EDGE_SDK_ENABLE_TESTING

(Defaults to `ON`) If enabled, unit tests will be built for each library.

#### EDGE_SDK_BUILD_EXTERNAL_DEPS

(Defaults to `OFF`) If enabled, the CMake will download and compile dependencies.

#### EDGE_SDK_NO_EXCEPTION

(Defaults to `OFF`) If enabled, this will cause all libraries to be built without exceptions.

#### EDGE_SDK_MSVC_PARALLEL_BUILD_ENABLE (Windows only)

(Defaults to `ON`) If enabled, this will add /MP compilation flag to build SDK using multiple cores.

## SDK Usage

To learn how to use the HERE OLP Edge SDK, check the [Getting Started Guide](docs/GettingStartedGuide.md).

## Contribution

See the [CONTRIBUTING](CONTRIBUTING.md) file for details.

## LICENSE

Copyright (C) 2019 HERE Europe B.V.

See the [LICENSE](LICENSE) file in the root of this project for license details.

Note that this project has Open Source Software dependencies which will be downloaded and installed upon execution of the abovementioned installation commands. See the CMake configuration files included in the [external](/external) directory for further details.
