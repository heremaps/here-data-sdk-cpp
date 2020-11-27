# Getting Started Guide

## Prerequisites

To start using HERE Data SDK for C++, you need a platform user account.

Working with the Data SDK requires knowledge of the following subjects:

- Basic understanding of the core [HERE platform concepts](#concepts).
- Basic proficiency with C++11 and CMake.

## Concepts

To use HERE Data SDK for C++, you need to understand the following concepts related to the HERE platform:

- [Catalogs](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/catalogs.html)
- [Layers](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html)
- [Partitions](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/partitions.html)
- [HERE Resource Names (HRNs)](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/hrn.html)

For more details, see the [Data User Guide](https://developer.here.com/olp/documentation/data-user-guide/index.html).

## Get credentials

To work with catalog or service requests to the HERE platform, you need to get authentication and authorization credentials.

You can authenticate to the HERE platform within your application with the platform credentials available on the HERE Portal by means of Data SDK for C++ authentication library. For the available authentication options, see the [Identity & Access Management Developer Guide](https://developer.here.com/documentation/identity-access-management/dev_guide/index.html).

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

