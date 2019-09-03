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
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(), [=] {
              EDGE_SDK_LOG_INFO_F(kLogTag, "cache data '%s' found!",
                                  key.c_str());
              callback(*cachedData);
            });
            return CancellationToken();
          } else if (CacheOnly == request.GetFetchOption()) {
            ExecuteOrSchedule(apiRepo->GetOlpClientSettings(), [=] {
              EDGE_SDK_LOG_INFO_F(kLogTag, "cache catalog '%s' not found!",
                                  key.c_str());
              callback(
                  ApiError(ErrorCode::NotFound,
                           "Cache only resource not found in cache (data)."));
            });
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
      {static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
       "Operation cancelled."}};
  multiRequestContext_ =
      std::make_shared<MultiRequestContext<read::DataResponse>>(
          cancelledResponse);
}

bool DataRepository::IsInlineData(const std::string& dataHandle) {
  return (dataHandle.find(kDataInlinePrefix) == 0);
}

namespace {
class Condition {
 public:
  Condition(CancellationContext context, int timeout = 60)
      : context_{context}, timeout_{timeout}, signaled_{false} {}

  void Notify() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      signaled_ = true;
    }
    condition_.notify_one();
  }

  bool Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool triggered = condition_.wait_for(
        lock, std::chrono::seconds(timeout_),
        [&] { return signaled_ || context_.IsCancelled(); });

    signaled_ = false;
    return triggered;
  }

 private:
  CancellationContext context_;
  int timeout_;
  std::condition_variable condition_;
  std::mutex mutex_;
  bool signaled_;
};
}  // namespace

CancellationToken DataRepository::GetData(
    const read::DataRequest& request,
    const read::DataResponseCallback& callback) {
  // Should not return any token, rather it should accept a context as input and
  // use it to guard against continuation while cancelled.
  // Also from the design perspective if everything below CatalogClientImpl is
  // already async then this should be sync returning the response itself.

  auto key = request.CreateKey();
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetData - '%s'", key.c_str());

  if (!request.GetDataHandle() && !request.GetPartitionId()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetData for '%s' failed", key.c_str());
    callback({{ErrorCode::InvalidArgument,
               "Data handle or partition ID missing", false}});
    return CancellationToken();
  }

  // Local copy of repos to ensure they outlive the duration of the request
  auto api_repo = apiRepo_;
  auto partitions_repo = partitionsRepo_;
  auto& cache = *cache_;

  int timeout = api_repo->GetOlpClientSettings()
                    ? api_repo->GetOlpClientSettings()->retry_settings.timeout
                    : 60;

  // TODO: Extend CancellationContext to also provide a ExecuteOrCancelled
  // method that returns the return value of the lambda and not always a
  // CancellationToken. This way we can hide the waiting inside the lambda.
  // auto ExecuteOrCancelled(Func& func, CancelFunc& cancel) -> decltype(func())
  CancellationContext context;
  CancellationToken token([=]() mutable { context.CancelOperation(); });
  Condition wait(context, timeout);
  auto wait_and_check = [&] {
    if (!wait.Wait() && !context.IsCancelled()) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "wait_and_check() failed");
      callback({{ErrorCode::RequestTimeout, "GetData timed out.", true}});
      return false;
    }
    return true;
  };

  // Get catalog first
  EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog #####");

  read::CatalogResponse catalog;
  context.ExecuteOrCancelled([&] {
    catalogRepo_->getCatalog(CatalogRequest()
                                 .WithBillingTag(request.GetBillingTag())
                                 .WithFetchOption(request.GetFetchOption()),
                             [&](read::CatalogResponse response) {
                               EDGE_SDK_LOG_INFO_F(kLogTag,
                                                   "getCatalog reponse rcvd");
                               catalog = std::move(response);
                               wait.Notify();
                             });
    return CancellationToken();
  });

  // Wait for the catalog answer
  EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog wait_and_check()");
  if (!catalog.IsSuccessful() && !wait_and_check()) {
    return token;
  }

  // TODO: This will dissapear once we split up CatalogClient into layers
  if (!catalog.IsSuccessful()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "getCatalog '%s' unsuccessful", key.c_str());
    callback(catalog.GetError());
    return token;
  }

  auto get_data = [&](const read::DataRequest& request) -> DataResponse {
    // Cache first
    EDGE_SDK_LOG_INFO_F(kLogTag, "get_data - cache #####");

    if (OnlineOnly != request.GetFetchOption()) {
      auto data = cache.Get(request.GetLayerId(),
                            request.GetDataHandle().value_or(std::string()));
      if (!data && CacheOnly == request.GetFetchOption()) {
        return {{ErrorCode::NotFound, "Data not found in cache", false}};
      } else if (data) {
        return *data;
      }
    }

    // Now try online
    // 1. Get Look-up API first
    EDGE_SDK_LOG_INFO_F(kLogTag, "get_data - lookup #####");

    ApiClientResponse api_client;
    api_repo->getApiClient("blob", "v1", [&](ApiClientResponse response) {
      api_client = std::move(response);
      wait.Notify();
    });

    if (!wait_and_check()) {
      return {{ErrorCode::RequestTimeout, "getApiClient timeout", true}};
    }

    if (!api_client.IsSuccessful()) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "getApiClient '%s' failed", key.c_str());
      return api_client.GetError();
    }

    // 2. Get data from partition
    EDGE_SDK_LOG_INFO_F(kLogTag, "get_data - blob #####");

    DataResponse data_response;
    BlobApi::GetBlob(
        api_client.GetResult(), request.GetLayerId(), *request.GetDataHandle(),
        request.GetBillingTag(), boost::none, [&](DataResponse response) {
          if (response.IsSuccessful()) {
            EDGE_SDK_LOG_INFO_F(kLogTag, "adding '%s' to cache", key.c_str());
            cache.Put(response.GetResult(), request.GetLayerId(),
                      request.GetDataHandle().value_or(std::string()));
          } else if (http::HttpStatusCode::FORBIDDEN ==
                     response.GetError().GetHttpStatusCode()) {
            EDGE_SDK_LOG_INFO_F(kLogTag, "clearing '%s' cache", key.c_str());
            cache.Clear(request.GetLayerId(),
                        request.GetDataHandle().value_or(std::string()));
          }

          data_response = std::move(response);
          wait.Notify();
        });

    return (!wait_and_check())
               ? ApiError(ErrorCode::RequestTimeout, "GetBlob timeout", true)
               : std::move(data_response);
  };

  auto& layers = catalog.GetResult().GetLayers();
  auto itr = std::find_if(layers.begin(), layers.end(),
                          [&](const model::Layer& layer) {
                            return layer.GetId() == request.GetLayerId();
                          });

  if (itr == layers.end()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "Layer for '%s' doesn't exist", key.c_str());
    callback({{ErrorCode::InvalidArgument, "Layer doesn't exist."}});
    return token;
  }

  auto layer_type = itr->GetLayerType();
  if (request.GetDataHandle()) {
    auto data_response = get_data(request);
    callback(std::move(data_response));
    return token;
  }

  // Get partition handle
  EDGE_SDK_LOG_INFO_F(kLogTag, "GetPartitionsById #####");

  PartitionsResponse partitions;
  context.ExecuteOrCancelled([&] {
    partitions_repo->GetPartitionsById(
        PartitionsRequest()
            .WithBillingTag(request.GetBillingTag())
            .WithFetchOption(request.GetFetchOption())
            .WithLayerId(request.GetLayerId())
            .WithVersion(request.GetVersion()),
        {*request.GetPartitionId()}, [&](PartitionsResponse response) {
          partitions = std::move(response);
          wait.Notify();
        });
    return CancellationToken();
  });

  // Wait for the partitions handle response
  if (!wait_and_check()) {
    return token;
  }

  if (!partitions.IsSuccessful()) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetPartitionsById '%s' unsuccessful",
                        key.c_str());
    callback(partitions.GetError());
    return token;
  }

  auto& partitions_data = partitions.GetResult().GetPartitions();
  if (partitions_data.size() == 1u) {
    auto& handle = partitions_data.front().GetDataHandle();
    if (IsInlineData(handle)) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "Inline Data #####");
      callback(std::make_shared<model::Data::element_type>(handle.begin(),
                                                           handle.end()));
    } else {
      // Request data with handle
      EDGE_SDK_LOG_INFO_F(kLogTag, "get_data call #####");

      auto data = get_data(DataRequest(request).WithDataHandle(handle));
      if (!data.IsSuccessful() && 403 == data.GetError().GetHttpStatusCode()) {
        // Remove partition from cache as it is invalid
        partitionsCache_->ClearPartitions(
            PartitionsRequest()
                .WithBillingTag(request.GetBillingTag())
                .WithLayerId(request.GetLayerId())
                .WithVersion(request.GetVersion()),
            {*request.GetPartitionId()});
      }

      callback(std::move(data));
    }
  } else {
    // Backend returns an empty partition list if the partition doesn't exist
    // in the layer. So return no data.
    EDGE_SDK_LOG_INFO_F(kLogTag, "Partiton invalid '%s'", key.c_str());
    callback(model::Data());
  }

  EDGE_SDK_LOG_INFO_F(kLogTag, "done #####");
  return token;
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
