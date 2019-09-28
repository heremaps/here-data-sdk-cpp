#include "olp/dataservice/read/VersionedLayerClient.h"

#include "olp/authentication/Settings.h"
#include "olp/authentication/TokenProvider.h"
#include "olp/core/client/OlpClientFactory.h"
#include "olp/core/client/OlpClientSettingsFactory.h"
#include "olp/dataservice/read/CatalogClient.h"

#include "testutils/CustomParameters.hpp"

#include <gtest/gtest.h>

TEST(VersionedLayerClient, GetData) {
  using namespace olp;
  using namespace dataservice::read;

  auto network =
      client::OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  authentication::Settings auth_settings;
  auth_settings.network_request_handler = network;

  authentication::TokenProviderDefault provider(
      CustomParameters::getArgument("dataservice_read_test_appid"),
      CustomParameters::getArgument("dataservice_read_test_secret"),
      auth_settings);

  client::AuthenticationSettings auth_client_settings;
  auth_client_settings.provider = provider;
  client::OlpClientSettings settings_;
  settings_.network_request_handler = network;
  settings_.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  settings_.authentication_settings = auth_client_settings;
  settings_.cache = CreateDefaultCache();

  auto catalog = client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_catalog"));

  VersionedLayerClient client(catalog, "testlayer", settings_);

  DataRequest request;
  request.WithPartitionId("269");

  std::promise<model::Data> promise;

  client.GetData(
      request,
      [&](VersionedLayerClient::CallbackResponse<model::Data> response) {
        if (response.IsSuccessful()) {
          promise.set_value(response.GetResult());
        }
      });

  auto future = promise.get_future();
  EXPECT_TRUE(future.wait_for(std::chrono::seconds(20)) !=
              std::future_status::timeout);
}