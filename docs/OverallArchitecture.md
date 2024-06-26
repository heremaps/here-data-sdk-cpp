# Overall architecture of HERE Data SDK for C++

## Outline

This document describes the overall architecture of HERE Data SDK for C++ (here also referred to as SDK). For an overview of the scope and the features of the SDK, see [README.md](../README.md#why-use).

## Component overview

HERE Data SDK for C++ consists of three main independent modules:

* **olp-cpp-sdk-authentication**
* **olp-cpp-sdk-dataservice-read**
* **olp-cpp-sdk-dataservice-write**

Each of these modules has a dependency on the **olp-cpp-sdk-core** module that contains a set of utilities shared among all modules.

In the following sections, you find description of each of these modules.

### Component detail

The following diagram shows an overview of the SDK components and their relationship with the HERE Cloud.

![component_overview](diagrams/sdk-module-overview.png "HERE Data SDK for C++ Component Overview")

#### olp-cpp-sdk-core

The core module offers the following platform-independent functionality:

* **cache** - disk and memory cache
* **client** - generic HTTP client that performs communication with the HERE platform
* **geo** - geo utilities
* **logging** - logging to file, console or any customer-defined logger
* **math** - geo math utilities
* **network** - wraps network implementation on the supported platforms
* **thread** - utility classes commonly used in concurrent programming
* **utils** - some utilities such as base64 or LRU cache

For more information on how to use cache, see the [cache example](dataservice-cache-example.md).

#### olp-cpp-sdk-authentication

The authentication module wraps HERE Authentication and Authorization REST API. It's an OAuth 2.0 compliant REST API that lets you obtain short-lived access tokens that are used to authenticate requests to the HERE platform Services. Tokens expire after 24 hours.

For more information on how to use this module, see the [read and write examples](../docs).

#### olp-cpp-sdk-dataservice-read

The dataservice-read module wraps a subset of the Data REST API related to reading data from platform catalogs. It allows reading of data from the following layer types and with the listed Data APIs:

* Versioned layer. Used Data APIs:
  * [Config API](https://www.here.com/docs/bundle/data-api-config-v1-api-reference/page/index.html)
  * [Metadata API](https://www.here.com/docs/bundle/data-api-metadata-v1-api-reference/page/index.html)
  * [Query API](https://www.here.com/docs/bundle/data-api-query-v1-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)
* Volatile layer. Used Data APIs:
  * [Config API](https://www.here.com/docs/bundle/data-api-config-v1-api-reference/page/index.html)
  * [Metadata API](https://www.here.com/docs/bundle/data-api-metadata-v1-api-reference/page/index.html)
  * [Query API](https://www.here.com/docs/bundle/data-api-query-v1-api-reference/page/index.html)
  * [Volatile API](https://www.here.com/docs/bundle/data-api-volatile-blob-v1-api-reference/page/index.html)
* Index layer (not supported yet). Used Data APIs:
  * [Index API](https://www.here.com/docs/bundle/data-api-index-v1-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)
* Stream layer. Used Data APIs:
  * [Stream API](https://www.here.com/docs/bundle/data-api-stream-v1-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)

For more information on how to use this module, see the [read example](dataservice-read-catalog-example.md).

#### olp-cpp-sdk-dataservice-write

The dataservice-write module wraps a subset of the Data REST API related to writing data to platform catalogs. It allows writing of data to the following layer types and with the listed Data APIs:

* Versioned layer. Used Data APIs:
  * [Publish API](https://www.here.com/docs/bundle/data-api-publish-v2-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)
* Volatile layer. Used Data APIs:
  * [Publish API](https://www.here.com/docs/bundle/data-api-publish-v2-api-reference/page/index.html)
  * [Volatile API](https://www.here.com/docs/bundle/data-api-volatile-blob-v1-api-reference/page/index.html)
* Index layer. Used Data APIs:
  * [Index API](https://www.here.com/docs/bundle/data-api-index-v1-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)
* Stream layer. Used Data APIs:
  * [Ingest API](https://here.com/docs/bundle/data-api-ingest-v1-api-reference/page/index.html)
  * [Publish API](https://www.here.com/docs/bundle/data-api-publish-v2-api-reference/page/index.html)
  * [Blob API](https://www.here.com/docs/bundle/data-api-blob-v1-api-reference/page/index.html)

For more information on how to use this module, see the [write example](dataservice-write-example.md).

### External dependencies and depending systems

See [README.md](../README.md#dependencies).

## Requirements overview

The requirements break down into feature and nonfunctional requirements.

### Feature requirements

#### Authentication

Feature                          |  Status
---------------------------------|--------------
Sign in with client credentials  | Implemented
Sign in with email and password  | Implemented
Sign in with Facebook            | Implemented
Sign in with Google              | Implemented
Sign in with ArcGIS              | Implemented
Sign in with refresh token       | Implemented
Sign out                         | Implemented
Accept terms                     | Implemented

#### Dataservice-read

Feature                          |  Status
---------------------------------|--------------
Reading from versioned layer     | Implemented
Reading from volatile layer      | Implemented
Reading from stream layer        | Implemented
Reading from indexed layer       | Planned
API Lookup Service               | Implemented
Get catalog and layer configuration | Implemented
Get partition metadata for all partitions | Implemented
Get partition metadata for a subset of partitions | Implemented
Statistics Service  | Planned

#### Dataservice-write

Feature                          |  Status
---------------------------------|--------------
Writing to versioned layer       | Implemented
Writing to volatile layer        | Implemented
Writing to stream layer          | Implemented
Writing to indexed layer         | Implemented
API Lookup Service               | Implemented

#### Additional features

Feature                          |  Status
---------------------------------|--------------
LRU caching                      | Implemented
Prefetching data strategies      | Planned
Off-line capabilities            | Planned

### Nonfunctional requirements

The nonfunctional requirements are being finalized. After that this section is to be updated.

## Security model

The user of HERE Data SDK for C++ is responsible to take care of any security and privacy requirements of the target system.
