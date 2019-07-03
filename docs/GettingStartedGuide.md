# Getting Started Guide

## Get Credentials

Prior to any catalog or service request to the [HERE Open Location Platform](https://platform.here.com) you must obtain authentication and authorization credentials.

The following approaches are available for authenticating to OLP within your application:

1. Using the _platform credentials_ available from the platform portal in combination with HERE OLP Edge SDK authentication library.
2. Using the access key ID and access key secret in the platform credentials and the [Authentication and Authorization Developer's Guide](https://developer.here.com/olp/documentation/access_control/topics/introduction.html) to create your own authentication and authorization client for retrieving HERE tokens, which you can then provide to the OLP Edge SDK libraries.

To obtain your platform credentials, create a new application via the [Apps and Keys](https://platform.here.com/profile/apps-and-keys) page. When you have created your application, click _Create A Key_ to download these credentials. More information on obtaining platform credentials can be found in the [Manage Apps](https://developer.here.com/olp/documentation/access-control/user-guide/topics/manage-apps.html) section of the [Teams and Permissions User Guide](https://developer.here.com/olp/documentation/access-control/user-guide/index.html).

## OLP Prerequisite Knowledge

The use of the HERE OLP Edge C++ SDK requires a basic understanding of the following concepts related to OLP:

* [OLP Catalog](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/catalogs.html)
* [OLP Layers](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/layers.html)
* [OLP Partitions](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/partitions.html)
* [HRN (HERE Resource Name)](https://developer.here.com/olp/documentation/data-user-guide/shared_content/topics/olp/concepts/hrn.html)

For more details see the [HERE OLP Data User Guide](https://developer.here.com/olp/documentation/data-user-guide/index.html).

## Available components

The HERE OLP Edge SDK is composed of separate libraries with their own distinct functionality.

* olp-cpp-sdk-core  - General purpose library providing platform agnostic helpers (logging, network, geo spacial helpers, cache, etc.)
* olp-cpp-sdk-authentication - API library to get a HERE OAuth2 token to access OLP.
* olp-cpp-sdk-dataservice-read - API library to retrieve data from OLP.
* olp-cpp-sdk-dataservice-write - API library to upload data to OLP.

## HERE OLP Edge SDK in CMake projects

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
