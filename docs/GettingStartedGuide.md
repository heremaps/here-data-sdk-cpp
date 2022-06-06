# Getting Started Guide

## Prerequisites

To start using HERE Data SDK for C++, you need a platform user account.

Working with the Data SDK requires knowledge of the following subjects:

- Basic understanding of the core [HERE platform concepts](#concepts).
- Basic proficiency with C++11 and CMake.

## Concepts

To use HERE Data SDK for C++, you need to understand the following concepts related to the HERE platform:

- [Catalogs](https://developer.here.com/documentation/data-user-guide/portal/layers/catalogs.html)
- [Layers](https://developer.here.com/documentation/data-user-guide/portal/layers/layers.html)
- [Partitions](https://developer.here.com/documentation/data-user-guide/portal/layers/partitions.html)
- [HERE Resource Names (HRNs)](https://developer.here.com/documentation/data-user-guide/shared_content/topics/concepts/hrn.html)

For more details, see the [Data User Guide](https://developer.here.com/documentation/data-user-guide/index.html).

## Get credentials

To work with catalog or service requests to the HERE platform, you need to get authentication and authorization credentials.

You can authenticate to the HERE platform within your application with the platform credentials available on the HERE Portal by means of Data SDK for C++ authentication library. For the available authentication options, see the [Identity & Access Management Developer Guide](https://developer.here.com/documentation/identity-access-management/dev_guide/index.html).

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

## Available components

Data SDK for C++ contains separate libraries, each of which has a distinct functionality. For more information about the components, see the [architectural overview](OverallArchitecture.md).

## HERE Data SDK for C++ in CMake projects

Once the libraries are installed, you can find them using the `find_package()` function within your project. For more information on how to install libraries, see [the related section](../README.md#install-the-sdk) in the README.md file.

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

The API reference documentation for Data SDK for C++ is available on our [GitHub Pages](https://heremaps.github.io/here-data-sdk-cpp/).

## Examples

HERE Data SDK for C++ contains several example programs that demonstrate some of the key use cases. These example programs can be found in the **examples** folder:

- [Read example](dataservice-read-catalog-example.md) – demonstrates how to get catalog and partition metadata, as well as partition data.
- [Read example for a stream layer](dataservice-read-from-stream-layer-example.md) – demonstrates how to get data from a stream layer.
- [Cache example](dataservice-cache-example.md) – demonstrates how to get partition data and work with a mutable and protected cache.
- [Write example](dataservice-write-example.md) – demonstrates how to publish data to OLP.

