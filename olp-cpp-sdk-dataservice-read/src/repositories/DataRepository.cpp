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

#include "DataRepository.h"

#include <algorithm>
#include <sstream>

#include "ApiRepository.h"
#include "CatalogRepository.h"
#include "DataCacheRepository.h"
#include "ExecuteOrSchedule.inl"
#include "PartitionsCacheRepository.h"
#include "PartitionsRepository.h"
#include "generated/api/BlobApi.h"
#include "generated/api/VolatileBlobApi.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;

namespace {
constexpr auto kDataInlinePrefix = "data:";
constexpr auto kLogTag = "DataRepository";

void GetDataInternal(std::shared_ptr<CancellationContext> cancellationContext,
                     std::shared_ptr<ApiRepository> apiRepo,
                     const std::string& layerType,
                     const read::DataRequest& request,
                     const read::DataResponseCallback& callback,
                     DataCacheRepository& cache) {
  std::string service;
  std::function<CancellationToken(const OlpClient&)> dataFunc;
  auto key = request.CreateKey();
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetDataInternal '%s'", key.c_str());
  auto cancel_callback = [callback, key]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", key.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  /* Cache put intercept */
  auto cacheDataResponseCallback = [=, &cache](DataResponse response) {
    if (response.IsSuccessful()) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "put '%s' to cache", key.c_str());
      cache.Put(response.GetResult(), request.GetLayerId(),
                request.GetDataHandle().value_or(std::string()));
    } else {
      if (403 == response.GetError().GetHttpStatusCode()) {
        EDGE_SDK_LOG_INFO_F(kLogTag, "clear '%s' cache", key.c_str());
        cache.Clear(request.GetLayerId(),
                    request.GetDataHandle().value_or(std::string()));
      }
    }
    callback(response);
  };

  if (layerType == "versioned") {
    service = "blob";
    dataFunc = [=](const OlpClient& client) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "getBlob '%s", key.c_str());
      return BlobApi::GetBlob(client, request.GetLayerId(),
                              *request.GetDataHandle(), request.GetBillingTag(),
                              boost::none, cacheDataResponseCallback);
    };
  } else if (layerType == "volatile") {
    service = "volatile-blob";
    dataFunc = [=](const OlpClient& client) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "getVolatileBlob '%s", key.c_str());
      return VolatileBlobApi::GetVolatileBlob(
          client, request.GetLayerId(), *request.GetDataHandle(),
          request.GetBillingTag(), cacheDataResponseCallback);
    };
  } else {
    // TODO handle stream api
    EDGE_SDK_LOG_INFO_F(kLogTag, "service unavailable '%s'", key.c_str());
    callback(ApiError(client::ErrorCode::ServiceUnavailable,
                      "Stream layers are not supported yet."));
    return;
  }

  cancellationContext->ExecuteOrCancelled(
      [=, &cache]() {
        /* Check the cache */
        if (OnlineOnly != request.GetFetchOption()) {
          auto cachedData =
              cache.Get(request.GetLayerId(),
                        request.GetDataHandle().value_or(std::string()));
          if (cachedData) {
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(),
              [=] {
                EDGE_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                                    key.c_str());
                callback(*cachedData);
              }
            );
            return CancellationToken();
          } else if (CacheOnly == request.GetFetchOption()) {
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(),
              [=] {
                EDGE_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                                    key.c_str());
                callback(
                    ApiError(ErrorCode::NotFound,
                            "Cache only resource not found in cache (data)."));
             }
            );
            return CancellationToken();
          }
        }

        return apiRepo->getApiClient(
            service, "v1", [=](ApiClientResponse response) {
              if (!response.IsSuccessful()) {
                EDGE_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' unsuccessful",
                                    key.c_str());
                callback(response.GetError());
                return;
              }

              cancellationContext->ExecuteOrCancelled(
                  [=]() {
                    EDGE_SDK_LOG_INFO_F(kLogTag,
                                        "getApiClient '%s' getting catalog",
                                        key.c_str());
                    return dataFunc(response.GetResult());
                  },
                  cancel_callback);
            });
      },
      cancel_callback);
}

}  // namespace

DataRepository::DataRepository(
    const client::HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
    std::shared_ptr<CatalogRepository> catalogRepo,
    std::shared_ptr<PartitionsRepository> partitionsRepo,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      apiRepo_(apiRepo),
      catalogRepo_(catalogRepo),
      partitionsRepo_(partitionsRepo),
      cache_(std::make_shared<DataCacheRepository>(hrn, cache)),
      partitionsCache_(
          std::make_shared<PartitionsCacheRepository>(hrn, cache)) {
  read::DataResponse cancelledResponse{
      {olp::network::Network::ErrorCode::Cancelled, "Operation cancelled."}};
  multiRequestContext_ = std::make_shared<
      MultiRequestContext<read::DataResponse, read::DataResponseCallback>>(
      cancelledResponse);
}

bool DataRepository::IsInlineData(const std::string& dataHandle) {
  return (dataHandle.find(kDataInlinePrefix) == 0);
}

CancellationToken DataRepository::GetData(
    const read::DataRequest& request,
    const read::DataResponseCallback& callback) {
  auto key = request.CreateKey();
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetData '%s'", key.c_str());
  if (!request.GetDataHandle() && !request.GetPartitionId()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "getData for '%s' failed", key.c_str());
    callback(ApiError(client::ErrorCode::InvalidArgument,
                      "A data handle or a partition id must be defined."));
    return CancellationToken();
  }

  CatalogRequest catalogRequest;
  catalogRequest.WithBillingTag(request.GetBillingTag())
      .WithFetchOption(request.GetFetchOption());

  // local copy of repos to ensure they live the duration of the request
  auto apiRepo = apiRepo_;
  auto partitionsRepo = partitionsRepo_;
  auto& cache = *cache_;

  auto cancel_context = std::make_shared<CancellationContext>();
  auto cancel_callback = [callback, key]() {
    EDGE_SDK_LOG_INFO_F(kLogTag, "cancelled '%s'", key.c_str());
    callback({{ErrorCode::Cancelled, "Operation cancelled.", true}});
  };

  MultiRequestContext<read::DataResponse, read::DataResponseCallback>::ExecuteFn
      executeFn = [=, &cache](read::DataResponseCallback callback) {
        cancel_context->ExecuteOrCancelled(
            [=, &cache]() {
              return catalogRepo_->getCatalog(
                  catalogRequest,
                  [=, &cache](read::CatalogResponse catalogResponse) {
                    if (!catalogResponse.IsSuccessful()) {
                      EDGE_SDK_LOG_INFO_F(
                          kLogTag, "getCatalog '%s' unsuccessful", key.c_str());
                      callback(catalogResponse.GetError());
                      return;
                    }

                    auto& catalogLayers =
                        catalogResponse.GetResult().GetLayers();
                    auto itr = std::find_if(
                        catalogLayers.begin(), catalogLayers.end(),
                        [&](const model::Layer& layer) {
                          return layer.GetId() == request.GetLayerId();
                        });

                    if (itr == catalogLayers.end()) {
                      EDGE_SDK_LOG_INFO_F(kLogTag,
                                          "Layer for '%s' doesn't exiist",
                                          key.c_str());
                      callback(ApiError(client::ErrorCode::InvalidArgument,
                                        "Layer specified doesn't exist."));
                      return;
                    }

                    auto layerType = itr->GetLayerType();
                    if (request.GetDataHandle()) {
                      GetDataInternal(cancel_context, apiRepo, layerType,
                                      request, callback, cache);
                    } else {
                      auto getPartitionsCallback = [=, &cache](
                                                       PartitionsResponse
                                                           partitionsResponse) {
                        if (!partitionsResponse.IsSuccessful()) {
                          callback(partitionsResponse.GetError());
                          return;
                        }

                        if (partitionsResponse.GetResult()
                                .GetPartitions()
                                .size() == 1) {
                          auto& dataHandle = partitionsResponse.GetResult()
                                                 .GetPartitions()
                                                 .at(0)
                                                 .GetDataHandle();
                          if (IsInlineData(dataHandle)) {
                            callback(
                                std::make_shared<std::vector<unsigned char>>(
                                    dataHandle.begin(), dataHandle.end()));
                          } else {
                            DataRequest appendedRequest(request);
                            appendedRequest.WithDataHandle(
                                partitionsResponse.GetResult()
                                    .GetPartitions()
                                    .at(0)
                                    .GetDataHandle());
                            auto verifyResponseCallback = [=](DataResponse
                                                                  response) {
                              if (!response.IsSuccessful()) {
                                if (403 ==
                                    response.GetError().GetHttpStatusCode()) {
                                  PartitionsRequest partitionsRequest;
                                  partitionsRequest
                                      .WithBillingTag(
                                          appendedRequest.GetBillingTag())
                                      .WithLayerId(appendedRequest.GetLayerId())
                                      .WithVersion(
                                          appendedRequest.GetVersion());
                                  std::vector<std::string> partitions;
                                  partitions.push_back(
                                      *appendedRequest.GetPartitionId());
                                  partitionsCache_->ClearPartitions(
                                      partitionsRequest, partitions);
                                }
                              }
                              callback(response);
                            };
                            GetDataInternal(cancel_context, apiRepo, layerType,
                                            appendedRequest,
                                            verifyResponseCallback, cache);
                          }
                        } else {
                          // Backend returns an empty partition list if the
                          // partition doesn't exist in the layer. So return no
                          // data.
                          EDGE_SDK_LOG_INFO_F(kLogTag,
                                              "Empty partition for '%s'!",
                                              key.c_str());
                          callback(model::Data());
                        }
                      };

                      cancel_context->ExecuteOrCancelled(
                          [=]() {
                            PartitionsRequest partitionsRequest;
                            partitionsRequest
                                .WithBillingTag(request.GetBillingTag())
                                .WithFetchOption(request.GetFetchOption())
                                .WithLayerId(request.GetLayerId())
                                .WithVersion(request.GetVersion());
                            std::vector<std::string> partitions;
                            partitions.push_back(*request.GetPartitionId());
                            return partitionsRepo->GetPartitionsById(
                                partitionsRequest, partitions,
                                getPartitionsCallback);
                          },

                          cancel_callback);
                    }
                  });
            },

            cancel_callback);

        return CancellationToken(
            [cancel_context]() { cancel_context->CancelOperation(); });
      };
  return multiRequestContext_->ExecuteOrAssociate(key, executeFn, callback);
}

void DataRepository::GetData(
    std::shared_ptr<client::CancellationContext> cancellationContext,
    const std::string& layerType, const read::DataRequest& request,
    const read::DataResponseCallback& callback) {
  GetDataInternal(cancellationContext, apiRepo_, layerType, request, callback,
                  *cache_);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
