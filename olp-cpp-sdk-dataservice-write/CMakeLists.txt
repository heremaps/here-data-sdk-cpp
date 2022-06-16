# Copyright (C) 2019-2021 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE

project(olp-cpp-sdk-dataservice-write VERSION 1.15.1)
set(DESCRIPTION "C++ API library for writing data to OLP")

set(OLP_SDK_DATASERVICE_WRITE_API_HEADERS
    ./include/olp/dataservice/write/DataServiceWriteApi.h
    ./include/olp/dataservice/write/IndexLayerClient.h
    ./include/olp/dataservice/write/StreamLayerClient.h
    ./include/olp/dataservice/write/StreamLayerClientSettings.h
    ./include/olp/dataservice/write/VersionedLayerClient.h
    ./include/olp/dataservice/write/VolatileLayerClient.h
)

set(OLP_SDK_DATASERVICE_WRITE_GENERATED_MODEL_HEADERS
    ./include/olp/dataservice/write/generated/model/Details.h
    ./include/olp/dataservice/write/generated/model/Index.h
    ./include/olp/dataservice/write/generated/model/Publication.h
    ./include/olp/dataservice/write/generated/model/ResponseOk.h
    ./include/olp/dataservice/write/generated/model/ResponseOkSingle.h
    ./include/olp/dataservice/write/generated/model/VersionDependency.h
)

set(OLP_SDK_DATASERVICE_WRITE_MODEL_HEADERS
    ./include/olp/dataservice/write/model/CheckDataExistsRequest.h
    ./include/olp/dataservice/write/model/DeleteIndexDataRequest.h
    ./include/olp/dataservice/write/model/FlushRequest.h
    ./include/olp/dataservice/write/model/PublishDataRequest.h
    ./include/olp/dataservice/write/model/PublishIndexRequest.h
    ./include/olp/dataservice/write/model/PublishPartitionDataRequest.h
    ./include/olp/dataservice/write/model/PublishSdiiRequest.h
    ./include/olp/dataservice/write/model/StartBatchRequest.h
    ./include/olp/dataservice/write/model/UpdateIndexRequest.h
    ./include/olp/dataservice/write/model/VersionResponse.h
)

set(OLP_SDK_DATASERVICE_WRITE_INCLUDES
    ${OLP_SDK_DATASERVICE_WRITE_API_HEADERS}
    ${OLP_SDK_DATASERVICE_WRITE_MODEL_HEADERS}
    ${OLP_SDK_DATASERVICE_WRITE_GENERATED_MODEL_HEADERS}
)

# Some files (e.g. auto-flush related files) are temporarily removed from compilation stage
# until it will be decided whether we definitely need to keep those files
set(OLP_SDK_DATASERVICE_WRITE_SOURCES
    ./src/ApiClientLookup.cpp
    ./src/ApiClientLookup.h
    # ./src/AutoFlushController.cpp
    # ./src/AutoFlushController.h
    # ./src/AutoFlushSettings.h
    # ./src/BackgroundTaskCollection.cpp
    # ./src/BackgroundTaskCollection.h
    ./src/CancellationTokenList.cpp
    ./src/CancellationTokenList.h
    ./src/CatalogSettings.cpp
    ./src/CatalogSettings.h
    # ./src/DefaultFlushEventListener.cpp
    # ./src/DefaultFlushEventListener.h
    # ./src/FlushEventListener.h
    # ./src/FlushMetrics.h
    ./src/IndexLayerClient.cpp
    ./src/IndexLayerClientImpl.cpp
    ./src/IndexLayerClientImpl.h
    ./src/StreamLayerClient.cpp
    ./src/StreamLayerClientImpl.cpp
    ./src/StreamLayerClientImpl.h
    ./src/TimeUtils.cpp
    ./src/TimeUtils.h
    ./src/VersionedLayerClient.cpp
    ./src/VersionedLayerClientImpl.cpp
    ./src/VersionedLayerClientImpl.h
    ./src/VolatileLayerClient.cpp
    ./src/VolatileLayerClientImpl.cpp
    ./src/VolatileLayerClientImpl.h

    ./src/generated/BlobApi.cpp
    ./src/generated/BlobApi.h
    ./src/generated/ConfigApi.cpp
    ./src/generated/ConfigApi.h
    ./src/generated/IndexApi.cpp
    ./src/generated/IndexApi.h
    ./src/generated/IngestApi.cpp
    ./src/generated/IngestApi.h
    ./src/generated/MetadataApi.cpp
    ./src/generated/MetadataApi.h
    ./src/generated/PlatformApi.cpp
    ./src/generated/PlatformApi.h
    ./src/generated/PublishApi.cpp
    ./src/generated/PublishApi.h
    ./src/generated/QueryApi.cpp
    ./src/generated/QueryApi.h
    ./src/generated/ResourcesApi.cpp
    ./src/generated/ResourcesApi.h

    ./src/generated/model/Api.h
    ./src/generated/model/Catalog.h
    ./src/generated/model/LayerVersions.h
    ./src/generated/model/Partitions.h
    ./src/generated/model/PublishPartition.h
    ./src/generated/model/PublishPartitions.h

    ./src/generated/parser/ApiParser.cpp
    ./src/generated/parser/ApiParser.h
    ./src/generated/parser/CatalogParser.cpp
    ./src/generated/parser/CatalogParser.h
    ./src/generated/parser/DetailsParser.cpp
    ./src/generated/parser/DetailsParser.h
    ./src/generated/parser/LayerVersionsParser.cpp
    ./src/generated/parser/LayerVersionsParser.h
    ./src/generated/parser/PartitionParser.h
    ./src/generated/parser/PartitionsParser.cpp
    ./src/generated/parser/PartitionsParser.h
    ./src/generated/parser/PublicationParser.cpp
    ./src/generated/parser/PublicationParser.h
    ./src/generated/parser/PublishDataRequestParser.cpp
    ./src/generated/parser/PublishDataRequestParser.h
    ./src/generated/parser/PublishPartitionParser.cpp
    ./src/generated/parser/PublishPartitionParser.h
    ./src/generated/parser/PublishPartitionsParser.cpp
    ./src/generated/parser/PublishPartitionsParser.h
    ./src/generated/parser/ResponseOkParser.cpp
    ./src/generated/parser/ResponseOkParser.h
    ./src/generated/parser/ResponseOkSingleParser.cpp
    ./src/generated/parser/ResponseOkSingleParser.h
    ./src/generated/parser/VersionDependencyParser.cpp
    ./src/generated/parser/VersionDependencyParser.h
    ./src/generated/parser/VersionResponseParser.cpp
    ./src/generated/parser/VersionResponseParser.h

    ./src/generated/serializer/ApiSerializer.cpp
    ./src/generated/serializer/ApiSerializer.h
    ./src/generated/serializer/CatalogSerializer.cpp
    ./src/generated/serializer/CatalogSerializer.h
    ./src/generated/serializer/IndexInfoSerializer.cpp
    ./src/generated/serializer/IndexInfoSerializer.h
    ./src/generated/serializer/JsonSerializer.h
    ./src/generated/serializer/PublicationSerializer.cpp
    ./src/generated/serializer/PublicationSerializer.h
    ./src/generated/serializer/PublishDataRequestSerializer.cpp
    ./src/generated/serializer/PublishDataRequestSerializer.h
    ./src/generated/serializer/PublishPartitionSerializer.cpp
    ./src/generated/serializer/PublishPartitionSerializer.h
    ./src/generated/serializer/PublishPartitionsSerializer.cpp
    ./src/generated/serializer/PublishPartitionsSerializer.h
    ./src/generated/serializer/UpdateIndexRequestSerializer.cpp
    ./src/generated/serializer/UpdateIndexRequestSerializer.h
)

add_library(${PROJECT_NAME}
    ${OLP_SDK_DATASERVICE_WRITE_INCLUDES}
    ${OLP_SDK_DATASERVICE_WRITE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE DATASERVICE_WRITE_LIBRARY)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC DATASERVICE_WRITE_SHARED_LIBRARY)
endif()

# Used also in the package config file
set(PROJECT_LIBS olp-cpp-sdk-core)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_LIBS}
)

# install component
install (FILES ${OLP_SDK_DATASERVICE_WRITE_API_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write)
install (FILES ${OLP_SDK_DATASERVICE_WRITE_GENERATED_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write/generated/model)
install (FILES ${OLP_SDK_DATASERVICE_WRITE_MODEL_HEADERS} DESTINATION ${INCLUDE_DIRECTORY}/olp/dataservice/write/model)

export_config()

if(OLP_SDK_ENABLE_TESTING)
    add_subdirectory(tests)
endif()
