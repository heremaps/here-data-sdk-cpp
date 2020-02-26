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

#define URL_LOOKUP_CONFIG \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis/config/v1)"

#define URL_CONFIG                                                        \
  R"(https://config.data.api.platform.in.here.com/config/v1/catalogs/)" + \
      GetTestCatalog()

#define URL_LOOKUP_METADATA \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)"+GetTestCatalog()+R"(/apis/metadata/v1)"

#define URL_LATEST_CATALOG_VERSION \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest?startVersion=-1)"

#define URL_LAYER_VERSIONS \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layerVersions?version=4)"

#define URL_LAYER_VERSIONS_V2 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layerVersions?version=2)"

#define URL_LAYER_VERSIONS_V10 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layerVersions?version=10)"

#define URL_LAYER_VERSIONS_VN1 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layerVersions?version=-1)"

#define URL_PARTITIONS_INVALID_LAYER \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/somewhat_not_okay/partitions?version=4)"

#define URL_PARTITIONS \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?version=4)"

#define URL_PARTITIONS_V2 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?version=2)"

#define URL_PARTITIONS_V10 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?version=10)"

#define URL_PARTITIONS_VN1 \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?version=-1)"

#define URL_PARTITIONS_VOLATILE \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions)"

#define URL_PARTITIONS_VOLATILE_INVALID_LAYER \
  R"(https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2/layers/somewhat_not_okay/partitions)"

#define URL_LOOKUP_QUERY \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)"+GetTestCatalog()+R"(/apis/query/v1)"

#define URL_QUERY_PARTITION_269 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269&version=4)"

#define URL_QUERY_PARTITION_269_V2 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269&version=2)"

#define URL_QUERY_PARTITION_269_V10 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269&version=10)"

#define URL_QUERY_PARTITION_269_VN1 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269&version=-1)"

#define URL_LOOKUP_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)"+GetTestCatalog()+R"(/apis/blob/v1)"

#define URL_BLOB_DATA_269 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)"

#define URL_BLOB_DATA_269_V2 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256410)"

#define URL_PARTITION_3 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=3&version=4)"

#define URL_STREAM_SUBSCRIBE_SERIAL \
  R"(https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial)"

#define URL_STREAM_SUBSCRIBE_PARALLEL \
  R"(https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2/layers/testlayer/subscribe?mode=parallel)"

#define URL_STREAM_SUBSCRIBE_SUBSCRIPTION_ID \
  R"(https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial&subscriptionId=subscribe_id_12345)"

#define URL_STREAM_SUBSCRIBE_CONSUMER_ID \
  R"(https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2/layers/testlayer/subscribe?consumerId=consumer_id_1234&mode=serial)"

#define URL_STREAM_SUBSCRIBE_ALL_PARAMETERS \
  R"(https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2/layers/testlayer/subscribe?consumerId=consumer_id_1234&mode=parallel&subscriptionId=subscribe_id_12345)"

#define URL_STREAM_UNSUBSCRIBE_SERIAL \
  R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial&subscriptionId=subscribe_id_12345)"

#define URL_STREAM_UNSUBSCRIBE_PARALLEL \
  R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=parallel&subscriptionId=subscribe_id_12345)"

#define URL_LOOKUP_VOLATILE_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)"+GetTestCatalog()+R"(/apis/volatile-blob/v1)"

#define URL_LOOKUP_STREAM \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/)"+GetTestCatalog()+R"(/apis/stream/v2)"

#define URL_SEEK_STREAM \
  R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/seek?mode=serial&subscriptionId=subscribe_id_12345)"

#define CONFIG_BASE_URL "https://config.data.api.platform.in.here.com/config/v1"

#define HTTP_RESPONSE_LOOKUP_CONFIG                                                    \
  R"jsonString([{"api":"config","version":"v1","baseURL":")jsonString" CONFIG_BASE_URL \
  R"jsonString(","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_METADATA \
  R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_QUERY \
  R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_BLOB \
  R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_STREAM \
  R"jsonString([{"api":"stream","version":"v1","baseURL":"https://stream-ireland.data.api.platform.here.com/stream/v2/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB \
  R"jsonString([{"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_CONFIG \
  R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString"

#define HTTP_RESPONSE_403 \
  R"jsonString("Forbidden - A catalog with the specified HRN doesn't exist or access to this catalog is forbidden)jsonString"

#define HTTP_RESPONSE_SUBSCRIBE_403 \
  R"jsonString({ "error": "Forbidden Error", "error_description": "Error description" })jsonString"

#define HTTP_RESPONSE_SUBSCRIBE_404 \
  R"jsonString({ "title": "Stream layer not found", "status": 404, "code": "E213016", "cause": "Stream layer for catalog=hrn:here:data::olp-here-test:hereos-internal-test-v2","action": "Verify stream layer name and existence, and retry", "correlationId": "correlationId_12345"})jsonString"

#define HTTP_RESPONSE_UNSUBSCRIBE_404 \
  R"jsonString({"title": "Subscription not found","status": 404,"code": "E213003","cause": "SubscriptionId subscribe_id_12345 not found","action": "Subscribe again","correlationId": "123"})jsonString"

#define HTTP_RESPONSE_EMPTY R"jsonString()jsonString"

#define HTTP_RESPONSE_LATEST_CATALOG_VERSION \
  R"jsonString({"version":4})jsonString"

#define HTTP_RESPONSE_LAYER_VERSIONS \
  R"jsonString({"version":4,"layerVersions":[{"layer":"hype-test-prefetch","version":4,"timestamp":1547159598712},{"layer":"hype-test-prefetch-2","version":4,"timestamp":1547159598712},{"layer":"testlayer_res","version":4,"timestamp":1547159598712},{"layer":"testlayer","version":4,"timestamp":1547159598712},{"layer":"testlayer_gzip","version":4,"timestamp":1547159598712},{"layer":"multilevel_testlayer","version":4,"timestamp":1547159598712}]})jsonString"

#define HTTP_RESPONSE_LAYER_VERSIONS_V2 \
  R"jsonString({"version":2,"layerVersions":[{"layer":"testlayer","version":2,"timestamp":1547159598712}]})jsonString"

#define HTTP_RESPONSE_PARTITIONS \
  R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"},{"version":4,"partition":"270","layer":"testlayer","dataHandle":"30640762-b429-47b9-9ed6-7a4af6086e8e"},{"version":4,"partition":"3","layer":"testlayer","dataHandle":"data:SomethingBaH!"},{"version":4,"partition":"here_van_wc2018_pool","layer":"testlayer","dataHandle":"bcde4cc0-2678-40e9-b791-c630faee14c3"}]})jsonString"

#define HTTP_RESPONSE_PARTITIONS_V2 \
  R"jsonString({ "partitions": [{"version":2,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256410"}]})jsonString"

#define HTTP_RESPONSE_BLOB_DATA_269 R"jsonString(DT_2_0031)jsonString"

#define HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE \
  R"(iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC)"

#define HTTP_RESPONSE_BLOB_DATA_269_V2 R"jsonString(DT_2_0031_V2)jsonString"

#define HTTP_RESPONSE_PARTITION_269 \
  R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString"

#define HTTP_RESPONSE_PARTITION_269_V2 \
  R"jsonString({ "partitions": [{"version":2,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256410"}]})jsonString"

#define HTTP_RESPONSE_PARTITION_3 \
  R"jsonString({ "partitions": [{"version":4,"partition":"3","layer":"testlayer","dataHandle":"data:something!"}]})jsonString"

#define HTTP_RESPONSE_EMPTY_PARTITIONS \
  R"jsonString({ "partitions": []})jsonString"

#define HTTP_RESPONSE_INVALID_VERSION_VN1 \
  R"jsonString({"title":"Bad Request","detail":[{"name":"version","error":"Invalid version: minimum version for this catalog is 0"}],"status":400})jsonString"

#define HTTP_RESPONSE_INVALID_VERSION_V10 \
  R"jsonString({"title":"Bad Request","detail":[{"name":"version","error":"Invalid version: latest known version is 4"}],"status":400})jsonString"

#define HTTP_RESPONSE_INVALID_LAYER \
  R"jsonString({"title":"Bad Request","detail":[{"name":"layer","error":"Layer 'somewhat_not_okay' is missing in the catalog configuration."}],"status":400})jsonString"

#define HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION \
  R"jsonString({ "nodeBaseURL": "https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2", "subscriptionId": "subscribe_id_12345" })jsonString"

// <PREFETCH URLs and RESPONSEs>
#define URL_CONFIG_V2 \
  R"(https://config.data.api.platform.here.com/config/v1/catalogs/)"+GetTestCatalog()

#define URL_QUADKEYS_23618364 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/23618364/depths/0)"

#define URL_QUADKEYS_1476147 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/1476147/depths/0)"

#define URL_QUADKEYS_5904591 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/5904591/depths/1)"

#define URL_QUADKEYS_369036 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/369036/depths/0)"

#define URL_QUADKEYS_1 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/1/depths/0)"

#define URL_QUERY_PARTITION_23618365 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/)"+GetTestCatalog()+R"(/layers/hype-test-prefetch/partitions?partition=23618365&version=4)"

#define URL_QUERY_PARTITION_1476147 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/)"+GetTestCatalog()+R"(/layers/hype-test-prefetch/partitions?partition=1476147&version=4)"

#define URL_QUERY_VOLATILE_PARTITION_269 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer_volatile/partitions?partition=269)"

#define URL_BLOB_DATA_PREFETCH_1 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/a7a1afdf-db7e-4833-9627-d38bee6e2f81)"

#define URL_BLOB_DATA_PREFETCH_2 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/e119d20e-c7c6-4563-ae88-8aa5c6ca75c3)"

#define URL_BLOB_DATA_PREFETCH_3 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)"

#define URL_BLOB_DATA_PREFETCH_4 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/9d515348-afce-44e8-bc6f-3693cfbed104)"

#define URL_BLOB_DATA_PREFETCH_5 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)"

#define URL_BLOB_DATA_PREFETCH_6 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)"

#define URL_BLOB_DATA_PREFETCH_7 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/95c5c703-e00e-4c38-841e-e419367474f1)"

#define URL_VOLATILE_BLOB_DATA \
  R"(https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer_volatile/data/4eed6ed1-0d32-43b9-ae79-043cb4256410)"

#define HTTP_RESPONSE_CONFIG_V2 \
  R"jsonString({"id":"hereos-internal-test-v2","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2","name":"hereos-internal-test-v2","summary":"hereos-internal-test-v2","description":"Used for internal testing","contacts":{},"owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"tags":[],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","replication":{"regions":[{"id":"eu-ireland","role":"primary"}]},"layers":[{"id":"hype-test-prefetch","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:testlayer","name":"Test Layer","summary":"Test Layer","description":"A simple test layer","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"},{"id":"testlayer_gzip","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:testlayer_gzip","name":"Test Layer","summary":"Test Layer","description":"A simple compressed test layer","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","contentEncoding":"gzip","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"},{"id":"multilevel_testlayer","hrn":"hrn:here:data::olp-here-test:hereos-internal-test-v2:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","owner":{"creator":{"id":"9PBigz3zyXks0OlAQv13"},"organisation":{"id":"olp-here-test"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2019-01-10T22:30:12.917Z","layerType":"versioned"}],"marketplaceReady":false,"version":1})jsonString"

#define HTTP_RESPONSE_QUADKEYS_23618364 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"1","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}],"parentQuads": []})jsonString"

#define HTTP_RESPONSE_QUADKEYS_1476147 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"1","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString"

#define HTTP_RESPONSE_QUADKEYS_5904591 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"4","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"},{"version":4,"subQuadKey":"5","dataHandle":"e119d20e-c7c6-4563-ae88-8aa5c6ca75c3"},{"version":4,"subQuadKey":"6","dataHandle":"a7a1afdf-db7e-4833-9627-d38bee6e2f81"},{"version":4,"subQuadKey":"7","dataHandle":"9d515348-afce-44e8-bc6f-3693cfbed104"},{"version":4,"subQuadKey":"1","dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString"

#define HTTP_RESPONSE_QUADKEYS_369036 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"1","dataHandle":"data:Embedded Data for 369036"}],"parentQuads": []})jsonString"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_1 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_2 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_3 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_4 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_5 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_6 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

#define HTTP_RESPONSE_BLOB_DATA_PREFETCH_7 \
  R"(--644ff000-704a-4f45-9988-c628602b7064)"

// </PREFETCH URLs and RESPONSEs>
