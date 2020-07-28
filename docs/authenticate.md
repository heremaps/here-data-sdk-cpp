# Authenticate to the HERE Platform

To authenticate to the HERE platform and start working with HERE Data SDK for C++, you need to get an access token. You can receive it using a [default token provider](#authenticate-using-a-default-token-provider), [project authentication](#authenticate-using-project-authentication), or [federated credentials](#authenticate-using-federated-credentials)

## Authenticate using a default token provider

1. Get your platform credentials.

   For instructions, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

   You get the `credentials.properties` file.

2. Initialize the authentification settings using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   > Note: You can also retrieve your credentials from the `credentials.properties` file using the `ReadFromFile` method. For more information, see the [related API documentation](https://developer.here.com/documentation/sdk-cpp/api_reference/classolp_1_1authentication_1_1AuthenticationCredentials.html#a6bfd8347ebe89e45713b966e621dccdd).

   ```cpp
   olp::authentication::Settings settings({kKeyId, kKeySecret});
   settings.task_scheduler = task_scheduler;
   settings.network_request_handler = http_client;
   ```

3. Set up the `AuthenticationSettings` object with a default token provider.

   ```cpp
   olp::client::AuthenticationSettings auth_settings;
   auth_settings.provider =
     olp::authentication::TokenProviderDefault(std::move(settings));
   ```

You get an access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object. For more information, see the [related](https://developer.here.com/documentation/sdk-cpp/dev_guide/topics/create-olp-client-settings.html) section in the Developer Guide.

## Authenticate using project authentication

1. Create your application and get your API key.

   For instructions, see the [Manage Apps](https://developer.here.com/documentation/access-control/user_guide/topics/manage-apps.html) section in the Terms and Permissions User Guide.

2. Create a function that returns your API key.
3. Set up the `AuthenticationSettings` object with your function that returns the API key.

   > Note: Do not trigger any tasks on `TaskScheduler` as this might result in a deadlock.

   ```cpp
   olp::client::AuthenticationSettings auth_settings;
   auth_settings.api_key_provider =
   [](){ return "your-api-key"; };
   ```

You get an access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object. For more information, see the [related](https://developer.here.com/documentation/sdk-cpp/dev_guide/topics/create-olp-client-settings.html) section in the Developer Guide.

## Authenticate using federated credentials

1. Get your platform credentials.

   For instructions, see the [Get Credentials](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html) section in the Terms and Permissions User Guide.

   You get the `credentials.properties` file.

2. Initialize the `AuthenticationCredentials` class using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   > Note: You can also retrieve your credentials from the `credentials.properties` file using the `ReadFromFile` method. For more information, see the [related API documentation](https://developer.here.com/documentation/sdk-cpp/api_reference/classolp_1_1authentication_1_1AuthenticationCredentials.html#a6bfd8347ebe89e45713b966e621dccdd).

   ```cpp
   olp::authentication::AuthenticationCredentials credentials(kKeyId, kKeySecret);
   ```

3. Create an authentication client.

   ```cpp
   olp::authentication::AuthenticationSettings settings;
   settings.task_scheduler = task_scheduler;
   settings.network_request_handler = http_client;
   authentication::AutnhentucationClient client(settings);
   ```

4. Get your federated (Facebook or ArcGIS) properties.

   You should have at least your federated access token. For the complete list of federated properties, see the [related](https://developer.here.com/documentation/sdk-cpp/api_reference/structolp_1_1authentication_1_1AuthenticationClient_1_1FederatedProperties.html) documentation.

5. Initialize your federated properties.

   ```cpp
   olp::authentication::AunthenticationClient::FederatedProperties properties;
   properties.access_token = "your-access-token";
   ```

6. Create the `SignInUserCallback` class.

   For more information, see the [`AuthenticationClient` reference documentation](https://developer.here.com/documentation/sdk-cpp/api_reference/classolp_1_1authentication_1_1AuthenticationClient.html)

7. Create your own token provider using the authentication client created in step 3, your federated credentials, and the `SignInUserCallback` class.

   > Note: You can call your custom token provider form different threads.

   ```cpp
   auto token = std::make_shared<std::string>();

   settings.provider = [token](){
   if (token->empty() || isExpired(token)) {
   std::promise<AuthenticationClient::SignInUserResponse> token_promise;

   auto callback = [&token_promise](AuthenticationClient::SignInUserResponse response)
      { token_promise.set(response); };

   authentication::AutnhentucationClient client(settings);
   client.SignInFacebook(credentials, properties, callback);
   auto response = token_promise.get_future().get();
   (*token) = response.GetResult().GetAccessToken();
   }
   return *token;
   }
   ```

You get an access token. By default, it expires in 24 hours. To continue working with the HERE platform after your token expires, generate a new access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object.
