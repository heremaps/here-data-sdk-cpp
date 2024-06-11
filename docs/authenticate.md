# Authenticate to the HERE platform

To authenticate to the HERE platform and start working with HERE Data SDK for C++, you need to get an access token. You can receive it using a [default token provider](#authenticate-using-a-default-token-provider), [project authentication](#authenticate-using-project-authentication), or [federated credentials](#authenticate-using-federated-credentials).

For instructions, see the [OAuth tokens](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/dev-token.html) section in the Identity & Access Management Developer Guide.

> #### Note
> Keep your credentials secure and do not disclose them. Make sure that your credentials are not stored in a way that enables others to access them.

## Authenticate using a default token provider

1. Get your platform credentials.

   For instructions, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

   You get the `credentials.properties` file.

2. Initialize the authentication settings using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   > #### Note
   > You can also retrieve your credentials from the `credentials.properties` file using the `ReadFromFile` method. For more information, see the [related API documentation](https://www.here.com/docs/bundle/data-sdk-for-cpp-api-reference/page/namespaceolp_1_1authentication.html#).

   ```cpp
   olp::authentication::Settings settings({kKeyId, kKeySecret});
   settings.task_scheduler = task_scheduler;
   settings.network_request_handler = http_client;
   ```

3. Set up the `AuthenticationSettings` object with a default token provider.

   ```cpp
   olp::client::AuthenticationSettings auth_settings;
   auth_settings.token_provider =
     olp::authentication::TokenProviderDefault(std::move(settings));
   ```

You get an access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object. For more information, see the [related section](create-platform-client-settings.md) in the Developer Guide.

## Authenticate using project authentication

1. Get your platform credentials.

   For instructions, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

2. Initialize the `AuthenticationCredentials` class using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   > #### Note
   > You can also retrieve your credentials from the `credentials.properties` file using the `ReadFromFile` method. For more information, see the [related API documentation](https://www.here.com/docs/bundle/data-sdk-for-cpp-api-reference/page/namespaceolp_1_1authentication.html#).

   ```cpp
   olp::authentication::AuthenticationCredentials credentials(kKeyId, kKeySecret);
   ```

3. Create an authentication client.

   ```cpp
   olp::authentication::AuthenticationSettings auth_client_settings;
   auth_client_settings.task_scheduler = task_scheduler;
   auth_client_settings.network_request_handler = http_client;
   olp::authentication::AuthenticationClient client(auth_client_settings);
   ```

4. Create the `SignInProperties` object with your project ID.

   ```cpp
   olp::authentication::AuthenticationClient::SignInProperties signin_properties;
   signin_properties.scope = "<project ID>";
   ```

5. Call the `SignInClient` API on the previously created `client` object.

   ```cpp
   client.SignInClient(
       credentials, signin_properties,
       [](olp::authentication::Response<olp::authentication::SignInResult>
              response) {
         // Handle the response
       });
   ```

You get an access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object. For more information, see the [related section](create-platform-client-settings.md) in the Developer Guide.

## Authenticate using federated credentials

1. Get your platform credentials.

   For instructions, see the [Register your application](https://www.here.com/docs/bundle/identity-and-access-management-developer-guide/page/topics/plat-token.html#step-1-register-your-application-and-get-credentials) section in the Identity & Access Management Developer Guide.

   You get the `credentials.properties` file.

2. Initialize the `AuthenticationCredentials` class using the **here.access.key.іd** and **here.access.key.secret** from the `credentials.properties` file as `kKeyId` and `kKeySecret` respectively.

   > #### Note
   > You can also retrieve your credentials from the `credentials.properties` file using the `ReadFromFile` method. For more information, see the [related API documentation](https://www.here.com/docs/bundle/data-sdk-for-cpp-api-reference/page/namespaceolp_1_1authentication.html#).

   ```cpp
   olp::authentication::AuthenticationCredentials credentials(kKeyId, kKeySecret);
   ```

3. Create an authentication client's settings.

   ```cpp
   olp::authentication::AuthenticationSettings auth_client_settings;
   auth_client_settings.task_scheduler = task_scheduler;
   auth_client_settings.network_request_handler = http_client;
   ```

4. Get your federated (Facebook or ArcGIS) properties.

   You should have at least your federated access token. For the complete list of federated properties, see the [related documentation](https://www.here.com/docs/bundle/data-sdk-for-cpp-api-reference/page/structolp_1_1authentication_1_1AuthenticationClient_1_1FederatedProperties.html).

5. Initialize your federated properties.

   ```cpp
   olp::authentication::AuthenticationClient::FederatedProperties properties;
   properties.access_token = "your-access-token";
   ```

6. Create your own token provider using the authentication client's settings created in step 3, your federated credentials.

   > #### Note
   > You can call your custom token provider form different threads.

   ```cpp
   auto token = std::make_shared<olp::client::OauthToken>();

   olp::client::AuthenticationSettings auth_settings;
   auth_settings.token_provider =
       [token, auth_client_settings, credentials,
        properties](olp::client::CancellationContext context)
       -> olp::client::OauthTokenResponse {
     if (context.IsCancelled()) {
       return olp::client::ApiError::Cancelled();
     }

     if (!token->GetAccessToken().empty() &&
         std::chrono::system_clock::to_time_t(
             std::chrono::system_clock::now()) >= token->GetExpiryTime()) {
       return *token;
     }
 
     std::promise<olp::authentication::AuthenticationClient::SignInUserResponse>
         token_promise;
 
     auto callback =
         [&token_promise](
             olp::authentication::AuthenticationClient::SignInUserResponse
                 response) { token_promise.set_value(std::move(response)); };
 
     olp::authentication::AuthenticationClient client(auth_client_settings);
     client.SignInFacebook(credentials, properties, callback);
     auto response = token_promise.get_future().get();
     if (!response) {
       return response.GetError();
     }
 
     (*token) = olp::client::OauthToken(response.GetResult().GetAccessToken(),
                                        response.GetResult().GetExpiresIn());
 
     return *token;
   };
   ```

You get an access token. By default, it expires in 24 hours. To continue working with the HERE platform after your token expires, generate a new access token.

You can use the `AuthenticationSettings` object to create the `OlpClientSettings` object. For more information, see the [related section](create-platform-client-settings.md) in the Developer Guide.
