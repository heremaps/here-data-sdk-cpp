/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#pragma once

#define URL_LOOKUP_INGEST \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/ingest/v1)"
#define HTTP_RESPONSE_LOOKUP_INGEST \
  R"jsonString([{"api":"ingest","version":"v1","baseURL":"https://ingest.data.api.platform.here.com/ingest/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_CONFIG \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis/config/v1)"
#define HTTP_RESPONSE_LOOKUP_CONFIG \
  R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.here.com/config/v1","parameters":{}}])jsonString"

#define URL_LOOKUP_PUBLISH_V2 \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/publish/v2)"
#define HTTP_RESPONSE_LOOKUP_PUBLISH_V2 \
  R"jsonString([{"api":"publish","version":"v2","baseURL":"https://publish.data.api.platform.here.com/publish/v2/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/blob/v1)"
#define HTTP_RESPONSE_LOOKUP_BLOB \
  R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_VOLATILE_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/volatile-blob/v1)"
#define HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB \
  R"jsonString([{"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_INDEX \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/index/v1)"
#define HTTP_RESPONSE_LOOKUP_INDEX \
  R"jsonString([{"api":"index","version":"v1","baseURL":"https://index.data.api.platform.here.com/index/v1/catalogs/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_METADATA \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/metadata/v1)"
#define HTTP_RESPONSE_LOOKUP_METADATA \
  R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://sab.metadata.data.api.platform.here.com/metadata/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_LOOKUP_QUERY \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/apis/query/v1)"
#define HTTP_RESPONSE_LOOKUP_QUERY \
  R"jsonString([{"api":"query","version":"v1","baseURL":"https://sab.query.data.api.platform.here.com/query/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog","parameters":{}}])jsonString"

#define URL_GET_CATALOG \
  R"(https://config.data.api.platform.here.com/config/v1/catalogs/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog)"
#define URL_GET_CATALOG_BILLING_TAG \
  URL_GET_CATALOG R"(?billingTag=OlpCppSdkTest)"
#define HTTP_RESPONSE_GET_CATALOG \
  R"jsonString({"id":"olp-cpp-sdk-ingestion-test-catalog","hrn":"hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog","name":"OLP CPP SDK Ingestion Test Catalog","summary":"Test Catalog for the OLP CPP SDK Ingestion Component","description":"Test Catalog for the OLP CPP SDK Ingestion Component.","contacts":{},"owner":{"creator":{"id":"HERE-6b18d678-cde1-41fb-b1a6-9969ef253144"},"organisation":{"id":"olp-here-test"}},"tags":["test","olp-cpp-sdk"],"billingTags":[],"created":"2019-02-04T22:20:24.262635Z","replication":{"regions":[{"id":"eu-ireland","role":"primary"}]},"layers":[{"id":"olp-cpp-sdk-ingestion-test-stream-layer","hrn":"hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-stream-layer","name":"OLP CPP SDK Ingestion Test Stream Layer","summary":"Stream Layer for OLP CPP SDK Ingestion Component Testing","description":"Stream Layer for OLP CPP SDK Ingestion Component Testing.","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-6b18d678-cde1-41fb-b1a6-9969ef253144"},"organisation":{"id":"olp-here-test"}},"contentType":"text/plain","ttlHours":1,"ttl":600000,"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"volume":{"volumeType":"durable"},"streamProperties":{"dataInThroughputMbps":1.0,"dataOutThroughputMbps":1.0,"parallelization":1},"tags":["test","olp-cpp-sdk"],"billingTags":[],"created":"2019-02-04T23:12:35.707254Z","layerType":"stream"},{"id":"olp-cpp-sdk-ingestion-test-stream-layer-2","hrn":"hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-stream-layer-2","name":"OLP CPP SDK Ingestion Test Stream Layer 2","summary":"Second Stream Layer for OLP CPP SDK Ingestion Component Testing","description":"Second Stream Layer for OLP CPP SDK Ingestion Component Testing. Content-Type differs from the first.","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-6b18d678-cde1-41fb-b1a6-9969ef253144"},"organisation":{"id":"olp-here-test"}},"contentType":"application/json","ttlHours":1,"ttl":600000,"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"volume":{"volumeType":"durable"},"streamProperties":{"dataInThroughputMbps":1.0,"dataOutThroughputMbps":1.0,"parallelization":1},"tags":["test","olp-cpp-sdk"],"billingTags":[],"created":"2019-02-05T22:11:54.412241Z","layerType":"stream"},{"id":"olp-cpp-sdk-ingestion-test-stream-layer-sdii","hrn":"hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-stream-layer-sdii","name":"OLP CPP SDK Ingestion Test Stream Layer SDII","summary":"SDII Stream Layer for OLP CPP SDK Ingestion Component Testing","description":"SDII Stream Layer for OLP CPP SDK Ingestion Component Testing.","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-6b18d678-cde1-41fb-b1a6-9969ef253144"},"organisation":{"id":"olp-here-test"}},"contentType":"application/x-protobuf","ttlHours":1,"ttl":600000,"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"volume":{"volumeType":"durable"},"streamProperties":{"dataInThroughputMbps":1.0,"dataOutThroughputMbps":1.0,"parallelization":1},"tags":["test","olp-cpp-sdk"],"billingTags":[],"created":"2019-02-07T20:15:46.920639Z","layerType":"stream"},{"id": "olp-cpp-sdk-ingestion-test-volatile-layer","hrn": "hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-volatile-layer","name": "OLP CPP SDK INGESTION TEST VOLATILE LAYER","summary": "olp-cpp-sdk-ingestion-test-volatile-layer","description": "olp-cpp-sdk-ingestion-test-volatile-layer","coverage": {"adminAreas": []},"owner": {"creator": {"id": "HERE-82b5fbf8-f968-41a3-9f18-df949c5e33cf"},"organisation": {"id": "olp-here-test"}},"ttlHours": 1,"ttl": 3600000,"partitioningScheme": "generic","partitioning": {"scheme": "generic"},"contentType": "application/json","volume": {"maxMemoryPolicy": "failOnWrite","packageType": "experimental","volumeType": "volatile"},"tags": [],"billingTags": [],"created": "2019-04-11T23:38:27.647440Z","layerType": "volatile"},{"id": "olp-cpp-sdk-ingestion-test-versioned-layer","hrn": "hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-versioned-layer","name": "OLP CPP SDK INGESTION TEST VERSIONED LAYER","summary": "olp-cpp-sdk-ingestion-test-versioned-layer","description": "olp-cpp-sdk-ingestion-test-versioned-layer","coverage": {"adminAreas": []},"owner": {"creator": {"id": "HERE-8c3f71bd-3fd5-4b48-9a32-1e347c319e93"},"organisation": {"id": "olp-here-test"}},"partitioningScheme": "generic","partitioning": {"scheme": "generic"},"contentType": "text/plain","volume": {"volumeType": "durable"},"tags": [],"billingTags": [],"created":"2019-04-12T22:23:36.220816Z","layerType": "versioned"},{"id": "olp-cpp-sdk-ingestion-test-index-layer","hrn": "hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog:olp-cpp-sdk-ingestion-test-index-layer","name": "OLP CPP SDK INGESTION TEST INDEX LAYER","summary": "olp-cpp-sdk-ingestion-test-index-layer","description": "olp-cpp-sdk-ingestion-test-index-layer","coverage": {"adminAreas": []},"owner": {"creator": {"id": "HERE-82b5fbf8-f968-41a3-9f18-df949c5e33cf"},"organisation": {"id": "olp-here-test"}},"contentType": "application/json","volume": {"volumeType": "durable"},"indexProperties": {"ttl": "7.days","indexDefinitions": [{"name": "testIndexLayer","duration": 600000,"type": "timewindow"},{"name": "Place","type": "string"},{"name": "Temperature","type": "int"},{"name": "Rain","type": "bool"}]},"tags": [],"billingTags": [],"created":"2019-04-25T19:13:46.811413Z","layerType":"index"}],"marketplaceReady":false,"version":3})jsonString"

#define URL_INGEST_DATA \
  R"(https://ingest.data.api.platform.here.com/ingest/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-stream-layer)"
#define URL_INGEST_DATA_BILLING_TAG \
  URL_INGEST_DATA R"(?billingTag=OlpCppSdkTest)"
#define URL_INGEST_DATA_LAYER_2 \
  R"(https://ingest.data.api.platform.here.com/ingest/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-stream-layer-2)"
#define HTTP_RESPONSE_INGEST_DATA \
  R"jsonString({"TraceID":"e69a5b02-b1b6-4d7f-83eb-38ec8f946fc8"})jsonString"
#define HTTP_RESPONSE_INGEST_DATA_LAYER_2 \
  R"jsonString({"TraceID":"037561ad-51b9-42ec-9d58-e8c70be1c73a"})jsonString"

#define URL_INIT_PUBLICATION \
  R"(https://publish.data.api.platform.here.com/publish/v2/catalogs/olp-cpp-sdk-ingestion-test-catalog/publications)"
#define HTTP_RESPONSE_INIT_PUBLICATION \
  R"jsonString({"catalogVersion":-1,"id":"v2-5f5f4eba-d304-4320-a1b1-def3399e8b45","catalogId":"olp-cpp-sdk-ingestion-test-catalog","details":{"expires":1554497597357,"state":"initialized","modified":1554238397357,"message":"Publication has been initialized","started":1554238397357},"layerIds":["olp-cpp-sdk-ingestion-test-stream-layer"]})jsonString"

#define URL_PUT_BLOB_PREFIX \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-stream-layer/data/)"

#define URL_PUT_BLOB_INDEX_PREFIX \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-index-layer/data/)"

#define URL_DELETE_BLOB_INDEX_PREFIX \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-index-layer/data/)"

#define URL_PUT_VOLATILE_BLOB_PREFIX \
  R"(https://volatile-blob.data.api.platform.here.com/blobstore/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-volatile-layer/data/)"

#define URL_UPLOAD_PARTITIONS \
  R"(https://publish.data.api.platform.here.com/publish/v2/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-stream-layer/publications/v2-5f5f4eba-d304-4320-a1b1-def3399e8b45/partitions)"

#define URL_SUBMIT_PUBLICATION \
  R"(https://publish.data.api.platform.here.com/publish/v2/catalogs/olp-cpp-sdk-ingestion-test-catalog/publications/v2-5f5f4eba-d304-4320-a1b1-def3399e8b45)"

#define URL_INGEST_SDII \
  R"(https://ingest.data.api.platform.here.com/ingest/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-stream-layer-sdii/sdiiMessageList)"
#define URL_INGEST_SDII_BILLING_TAG \
  URL_INGEST_SDII R"(?billingTag=OlpCppSdkTest)"
#define HTTP_RESPONSE_INGEST_SDII \
  R"jsonString({"TraceID":{"ParentID":"0dcaaa6d-fabc-482f-90ca-fb89c25fc08f","GeneratedIDs":["9081ab2a-ef39-4bdc-baf0-31a4c20751ce"]}})jsonString"

#define URL_INSERT_INDEX \
  R"(https://index.data.api.platform.here.com/index/v1/catalogs/hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-index-layer)"

#define URL_QUERY_PARTITION_1111 \
  R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/olp-cpp-sdk-ingestion-test-catalog/layers/olp-cpp-sdk-ingestion-test-volatile-layer/partitions?partition=1111)"
#define HTTP_RESPONSE_QUERY_DATA_HANDLE \
  R"jsonString({ "partitions": [{"version":4,"partition":"1111","layer":"olp-cpp-sdk-ingestion-test-volatile-layer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString"
