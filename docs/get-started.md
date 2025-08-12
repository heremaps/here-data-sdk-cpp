# Get started

In this guide, learn how to authenticate to and start working with the HERE platform using the Data SDK:

- [Prerequisites](#prerequisites)
- [Concepts](#concepts)
- [Get credentials](#get-credentials)
- [Install the SDK](#install-the-sdk)
- [Build the SDK](#build-the-sdk)
  - [Build on Windows](#build-on-windows)
  - [Generate documentation with Doxygen](#generate-documentation-with-doxygen)
  - [CMake flags](#cmake-flags)
- [Available components](#available-components)
- [HERE Data SDK for C++ in CMake projects](#here-data-sdk-for-c-in-cmake-projects)
- [Reference documentation](#reference-documentation)
- [Examples](#examples)

## Prerequisites

To start using HERE Data SDK for C++, you need a platform user account.

Working with the Data SDK requires knowledge of the following subjects:

- Basic understanding of the core [HERE platform concepts](#concepts).
- Basic proficiency with C++11 and CMake.

For the terms and conditions covering this documentation, see the [HERE Documentation License](https://legal.here.com/en-gb/terms/documentation-license).

## Concepts

To use HERE Data SDK for C++, you need to understand the following concepts related to the HERE platform:

- [Catalogs](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/catalogs.html)
- [Layers](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/layers.html)
- [Partitions](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/partitions.html)
- [HERE Resource Names (HRNs)](https://www.here.com/docs/bundle/data-api-developer-guide/page/rest/hrn.html)

For more details, see the [Data API Developer Guide](https://www.here.com/docs/bundle/data-api-developer-guide/page/README.html).

## Get credentials

To work with catalog or service requests to the HERE platform, you need to get authentication and authorization credentials.

You can authenticate to the HERE platform within your application with the platform credentials available on the HERE Portal by means of Data SDK for C++ authentication library. For the available authentication options, see the [Identity and Access Management Developer Guide](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/README.html).

## Install the SDK

By default, the Data SDK downloads and compiles its dependencies. The versions of the downloaded dependencies may conflict with the versions that are already installed on your system. Therefore, the downloaded dependencies are not added to the install targets.

You can use the Data SDK in your CMake project or install it on your system.

Тo use the Data SDK directly in your CMake project, add the Data SDK via `add_subdirectory()`.

**To install the Data SDK on your system:**

1. Install all the dependencies needed for the Data SDK.<br>For more information on dependencies, see the [Dependencies](../README.md#dependencies) and [Linux dependencies](../README.md#linux-dependencies) sections in the README.md file.

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
| `OLP_SDK_USE_STD_OPTIONAL` | Defaults to `OFF`. If enabled, all libraries are built with std::optional type instead of boost::optional for C++17 and above. |
| `OLP_SDK_BOOST_THROW_EXCEPTION_EXTERNAL` | Defaults to `OFF`. When `OLP_SDK_NO_EXCEPTION` is `ON`, `boost` requires `boost::throw_exception()` to be defined. If enabled, the external definition of `boost::throw_exception()` is used. Otherwise, the library uses own definition. |
| `OLP_SDK_MSVC_PARALLEL_BUILD_ENABLE` (Windows Only) | Defaults to `ON`. If enabled, the `/MP` compilation flag is added to build the Data SDK using multiple cores. |
| `OLP_SDK_DISABLE_DEBUG_LOGGING`| Defaults to `OFF`. If enabled, the debug and trace level log messages are not printed. |
| `OLP_SDK_DISABLE_LOCATION_LOGGING`| Defaults to `OFF`. If enabled, the level messages locations are not generated by compiler.                                                                                                                                                |
| `OLP_SDK_ENABLE_DEFAULT_CACHE `| Defaults to `ON`. If enabled, the default cache implementation based on the LevelDB backend is enabled. |
| `OLP_SDK_ENABLE_DEFAULT_CACHE_LMDB `| Defaults to `OFF`. If enabled, the default cache implementation based on the LMDB backend is enabled. |
| `OLP_SDK_ENABLE_ANDROID_CURL`| Defaults to `OFF`. If enabled, libcurl will be used instead of the Android native HTTP client. |

## Available components

Data SDK for C++ contains separate libraries, each of which has a distinct functionality. For more information about the components, see the [architectural overview](OverallArchitecture.md).

## HERE Data SDK for C++ in CMake projects

When the libraries are installed, you can find them using the `find_package()` function within your project. For more information on how to install libraries, see [Install the SDK](#install-the-sdk).

```CMake
find_package(olp-cpp-sdk-core REQUIRED)
find_package(olp-cpp-sdk-authentication REQUIRED)
find_package(olp-cpp-sdk-dataservice-read REQUIRED)
find_package(olp-cpp-sdk-dataservice-write REQUIRED)
```

Once the necessary targets are imported, you can link them to your library:

```CMake
target_link_libraries(example_app
    olp-cpp-sdk-core
    olp-cpp-sdk-authentication
    olp-cpp-sdk-dataservice-read
    olp-cpp-sdk-dataservice-write
)
```

## Reference documentation

The API reference documentation for Data SDK for C++ is available on [GitHub Pages](https://heremaps.github.io/here-data-sdk-cpp/).

## Examples

HERE Data SDK for C++ contains several example programs that demonstrate some of the key use cases. These example programs can be found in the **examples** folder:

- [Read example](dataservice-read-catalog-example.md) – demonstrates how to get catalog and partition metadata, as well as partition data.
- [Read example for a stream layer](dataservice-read-from-stream-layer-example.md) – demonstrates how to get data from a stream layer.
- [Cache example](dataservice-cache-example.md) – demonstrates how to get partition data and work with a mutable and protected cache.
- [Write example](dataservice-write-example.md) – demonstrates how to publish data to the HERE platform.
