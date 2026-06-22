# Introduction

HERE Data SDK for C++ provides an interface to access the HERE platform. It is a modern (C++11), lightweight, and modular SDK with minimal dependencies targeted towards a wide range of hardware platforms from embedded devices to desktops.

The Data SDK for C++ package contains three independent modules that focus on different activities:

- `olp-cpp-sdk-authentication` – gets OAuth2 bearer tokens used to confirm platform requests.
- `olp-cpp-sdk-dataservice-read` – downloads and caches data from the platform.
- `olp-cpp-sdk-dataservice-write` – queues and uploads data to the platform layers.

For more information about the modules, see the [architectural overview](https://github.com/heremaps/here-olp-sdk-cpp/blob/master/docs/OverallArchitecture.md).

HERE is committed to respecting your privacy and to complying with applicable data protection and privacy laws. For more information, see the [HERE Privacy Charter](https://www.here.com/en-gb/here-privacy-charter).

For more information on HERE data security and durability best practices, see the [Data API](https://docs.here.com/data-api/docs/data-security).

## Why use

HERE Data SDK for C++ provides support for the core HERE platform use cases through a set of native C++ interfaces. The SDK is intended to save you time and effort on using HERE REST APIs. It provides a set of stable APIs that simplify complex platform operations and keeps up to date with the latest HERE REST API changes.

The Data SDK for C++ is a modern (C++11), lightweight, and modular SDK with minimal dependencies targeted towards a wide range of hardware platforms from embedded devices to desktops.

This SDK lets you:

- Authenticate to HERE platform.
- Read catalog and partition metadata.
- Retrieve data from versioned, volatile, and stream layers of the platform catalogs.
- Upload data to the platform.

Additionally, the SDK includes classes for work with geospatial tiling schemes that are used by most platform catalog layers.

## Backward Compatibility

We try to develop and maintain our API in a way that preserves its compatibility with the existing applications. Changes in the Data SDK for C++ are greatly influenced by the Data API development. Data API introduces breaking changes 6 months in advance. Therefore, you may need to migrate to a new version of the Data SDK for C++ every half a year.

For more information on Data API, see its [Developer Guide](https://docs.here.com/data-api/docs/data-api-intro) and [API Reference](https://docs.here.com/data-api/reference).

When new API is introduced in the Data SDK for C++, the old one is not deleted straight away. The standard API deprecation time is six months. It gives you time to switch to new code. However, we do not provide ABI backward compatibility. 

All of the deprecated methods, functions, and parameters are documented in the Data SDK for C++ [API Reference](https://heremaps.github.io/here-data-sdk-cpp/index.html) and [changelog](https://github.com/heremaps/here-olp-sdk-cpp/blob/master/CHANGELOG.md).