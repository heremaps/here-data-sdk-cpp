/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#define PATH_MATCHER_LOOKUP_METADATA                                                 \
  R"(/lookup/v1/resources/)" + \
      GetTestCatalog() + R"(/apis/metadata/v1)"

#define PATH_MATCHER_LATEST_CATALOG_VERSION \
  R"(/metadata/v1/catalogs/hereos-internal-test-v2/versions/latest)"

#define PATH_MATCHER_LOOKUP_CONFIG \
  R"(/lookup/v1/platform/apis/config/v1)"

#define PATH_MATCHER_CONFIG                                                        \
  R"(/config/v1/catalogs/)" + \
      GetTestCatalog()

#define PATH_MATCHER_LOOKUP_QUERY                                                    \
  R"(/lookup/v1/resources/)" + \
      GetTestCatalog() + R"(/apis/query/v1)"

#define PATH_MATCHER_QUERY_PARTITION_269 \
  R"(/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions)"

#define PATH_MATCHER_LOOKUP_BLOB                                                     \
  R"(/lookup/v1/resources/)" + \
      GetTestCatalog() + R"(/apis/blob/v1)"

#define PATH_MATCHER_BLOB_DATA_269 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)"

#define PATH_MATCHER_QUADKEYS_1476147 \
  R"(/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/1476147/depths/0)"

#define PATH_MATCHER_QUADKEYS_5904591 \
  R"(/query/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/versions/4/quadkeys/5904591/depths/1)"

#define PATH_MATCHER_BLOB_DATA_PREFETCH_1 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/a7a1afdf-db7e-4833-9627-d38bee6e2f81)"

#define PATH_MATCHER_BLOB_DATA_PREFETCH_2 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/e119d20e-c7c6-4563-ae88-8aa5c6ca75c3)"

#define PATH_MATCHER_BLOB_DATA_PREFETCH_3 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)"

#define PATH_MATCHER_BLOB_DATA_PREFETCH_4 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/9d515348-afce-44e8-bc6f-3693cfbed104)"

#define PATH_MATCHER_BLOB_DATA_PREFETCH_5 \
  R"(/blobstore/v1/catalogs/hereos-internal-test-v2/layers/hype-test-prefetch/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)"

#define HTTP_RESPONSE_LOOKUP_METADATA \
  R"jsonString([{"api":"metadata","version":"v1","baseURL":"https://metadata.data.api.platform.here.com/metadata/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LATEST_CATALOG_VERSION \
  R"jsonString({"version":4})jsonString"

#define CONFIG_BASE_URL "https://config.data.api.platform.in.here.com/config/v1"

#define HTTP_RESPONSE_LOOKUP_CONFIG                                                    \
  R"jsonString([{"api":"config","version":"v1","baseURL":")jsonString" CONFIG_BASE_URL \
  R"jsonString(","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString"

#define HTTP_RESPONSE_CONFIG \
  R"jsonString({"id":"hereos-internal-test","hrn":"hrn:here-dev:data:::hereos-internal-test","name":"hereos-internal-test","summary":"Internal test for hereos","description":"Used for internal testing on the staging olp.","contacts":{},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"tags":[],"billingTags":[],"created":"2018-07-13T20:50:08.425Z","layers":[{"id":"hype-test-prefetch","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch","name":"Hype Test Prefetch","summary":"hype prefetch testing","description":"Layer for hype prefetch testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":[],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_res","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer_res","name":"Resource Test Layer","summary":"testlayer_res","description":"testlayer_res","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"testlayer_volatile","ttl":1000,"hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"volatile"},{"id":"testlayer_stream","hrn":"hrn:here-dev:data:::hereos-internal-test:testlayer","name":"Test Layer","summary":"A test layer","description":"A simple test layer","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"stream"},{"id":"multilevel_testlayer","hrn":"hrn:here-dev:data:::hereos-internal-test:multilevel_testlayer","name":"Multi Level Test Layer","summary":"Multi Level Test Layer","description":"A multi level test layer just for testing","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"generic","partitioning":{"scheme":"generic"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-13T20:56:19.181Z","layerType":"versioned"},{"id":"hype-test-prefetch-2","hrn":"hrn:here-dev:data:::hereos-internal-test:hype-test-prefetch-2","name":"Hype Test Prefetch2","summary":"Layer for testing hype2 prefetching","description":"Layer for testing hype2 prefetching","coverage":{"adminAreas":[]},"owner":{"creator":{"id":"HERE-88c95a7e-4123-4dcd-ae0e-4682aa5c3db4"},"organisation":{"id":"olp-here"}},"partitioningScheme":"heretile","partitioning":{"tileLevels":[],"scheme":"heretile"},"contentType":"application/x-protobuf","volume":{"volumeType":"durable"},"tags":["TEST"],"billingTags":[],"created":"2018-07-24T17:52:23.818Z","layerType":"versioned"}],"version":3})jsonString"

#define HTTP_RESPONSE_LOOKUP_QUERY \
  R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_PARTITION_269 \
  R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString"

#define HTTP_RESPONSE_LOOKUP_BLOB \
  R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_BLOB_DATA_269 R"jsonString(DT_2_0031)jsonString"

#define HTTP_RESPONSE_QUADKEYS_1476147 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"1","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString"

#define HTTP_RESPONSE_QUADKEYS_5904591 \
  R"jsonString({"subQuads": [{"version":4,"subQuadKey":"4","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"},{"version":4,"subQuadKey":"5","dataHandle":"e119d20e-c7c6-4563-ae88-8aa5c6ca75c3"},{"version":4,"subQuadKey":"6","dataHandle":"a7a1afdf-db7e-4833-9627-d38bee6e2f81"},{"version":4,"subQuadKey":"7","dataHandle":"9d515348-afce-44e8-bc6f-3693cfbed104"},{"version":4,"subQuadKey":"1","dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString"

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

// </PREFETCH URLs and RESPONSEs>
