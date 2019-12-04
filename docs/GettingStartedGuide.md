# Getting Started Guide

## Get Credentials

To create catalog or service requests to the [HERE Open Location Platform](https://platform.here.com), you need to have the OLP authentication and authorization credentials.

The following two authentication approaches are available within your application:

1. Using your **platform credentials** that are available on the platform portal by means of the HERE OLP SDK for C++ authentication library.
2. Using the **access key ID** and **access key secret** from the platform credentials as described in the [Authentication and Authorization Developer's Guide](https://developer.here.com/olp/documentation/access_control/topics/introduction.html) to create your own authentication and authorization client for retrieving HERE tokens that you can then provide to the HERE OLP SDK for C++ libraries.

To get your platform credentials, on the [Apps and Keys](https://platform.here.com/profile/apps-and-keys) page, create a new application. Then, to download the credentials, click **Create a key**. For more information on how to get platform credentials, see the [Manage Apps](https://developer.here.com/olp/documentation/access-control/user-guide/topics/manage-apps.html) section of the [Teams and Permissions User Guide](https://developer.here.com/olp/documentation/access-control/user-guide/index.html).

## OLP Prerequisite Knowledge

To use the HERE Open Location Platform (OLP) SDK for C++, you need to understand the following concepts related to OLP:

* [Catalogs](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/catalogs.html)
* [Layers](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/layers.html)
* [Partitions](https://developer.here.com/olp/documentation/data-user-guide/portal/layers/partitions.html)
* [HERE Resource Names (HRNs)](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/hrn.html)

For more information, see the [HERE OLP Data User Guide](https://developer.here.com/olp/documentation/data-user-guide/index.html).

## Available Components

The HERE OLP SDK for C++ contains separate libraries, each of which has a distinct functionality. For more information about the components, see the [architectural overview](OverallArchitecture.md).

## HERE OLP SDK for C++ in CMake Projects

Once the libraries are installed, you can find them using the `find_package()` function within your project. For more information on how to install libraries, see [the related section](../README.md#Install) in the README.md file.

```CMake
find_package(olp-cpp-sdk-core REQUIRED)
find_package(olp-cpp-sdk-authentication REQUIRED)
find_package(olp-cpp-sdk-dataservice-read REQUIRED)
find_package(olp-cpp-sdk-dataservice-write REQUIRED)
```

Once the necessary targets are imported, you can simply link them to your library:

```CMake
target_link_libraries(example_app
    olp-cpp-sdk-core
    olp-cpp-sdk-authentication
    olp-cpp-sdk-dataservice-read
    olp-cpp-sdk-dataservice-write
)
```

## Reference Documentation

The API reference documentation for the HERE OLP SDK for C++ is available [here](https://heremaps.github.io/here-olp-sdk-cpp/).


## Examples

* [Read example](dataservice-read-catalog-example.md) shows how to read metadata from an OLP catalog and download a specific partition data from a catalog layer.
* [Write example](dataservice-write-example.md) shows how to write data to a specific stream layer partition.
