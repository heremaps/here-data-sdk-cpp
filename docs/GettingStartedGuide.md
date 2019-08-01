# Getting Started Guide

## Get Credentials

Prior to any catalog or service request to the [HERE Open Location Platform](https://platform.here.com) (OLP), you must obtain authentication and authorization credentials.

The following approaches are available for authenticating to the OLP within your application:

1. Using the _platform credentials_ available from the OLP portal in combination with the HERE OLP Edge SDK C++ authentication library.
2. Using the access key ID and access key secret in the platform credentials as described in the [Authentication and Authorization Developer's Guide](https://developer.here.com/olp/documentation/access_control/topics/introduction.html) to create your own authentication and authorization client for retrieving HERE tokens, which you can then provide to the HERE OLP Edge SDK C++ libraries.

To obtain your platform credentials, create a new application via the [Apps and Keys](https://platform.here.com/profile/apps-and-keys) page. Then click **Create a key** to download these credentials. For more information on obtaining platform credentials, see the [Manage Apps](https://developer.here.com/olp/documentation/access-control/user-guide/topics/manage-apps.html) section of the [Teams and Permissions User Guide](https://developer.here.com/olp/documentation/access-control/user-guide/index.html).

## OLP Prerequisite Knowledge

The use of the HERE OLP Edge SDK C++ requires a basic understanding of the following concepts related to the OLP:

* [OLP catalog](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/catalogs.html)
* [OLP layers](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/layers.html)
* [OLP partitions](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/partitions.html)
* [HRN (HERE Resource Name)](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/hrn.html)

For more information, see the [HERE OLP Data User Guide](https://developer.here.com/olp/documentation/data-user-guide/index.html).

## Available Components

The HERE OLP Edge SDK C++ is composed of separate libraries with their own distinct functionality. For a component overview, see the [architectural overview](OverallArchitecture.md)

## HERE OLP Edge SDK C++ in CMake Projects

Once the libraries are installed, you can find them using `find_package()` within your project:

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

## Examples

* [Read example](dataservice-read-catalog-example.md) shows how to read metadata from an OLP catalog and download a specific partition data from a catalog layer.
* [Write example](dataservice-write-example.md) shows how to write data to a specific stream layer partition.
