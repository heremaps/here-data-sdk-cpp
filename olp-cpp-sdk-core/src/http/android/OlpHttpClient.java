/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

package com.here.olp.network;

import android.os.AsyncTask;
import android.os.OperationCanceledException;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import okhttp3.Headers;
import okhttp3.OkHttpClient;
import okhttp3.Protocol;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class OlpHttpClient {
    // The replica of http/NetworkType.h error codes
    public static final int IO_ERROR = -1;
    public static final int AUTHORIZATION_ERROR = -2;
    public static final int INVALID_URL_ERROR = -3;
    public static final int OFFLINE_ERROR = -4;
    public static final int CANCELLED_ERROR = -5;
    public static final int TIMEOUT_ERROR = -7;
    public static final int THREAD_POOL_SIZE = 8;
    private static final String LOGTAG = "OlpHttpClient";
    // The raw pointer to the C++ NetworkAndroid class
    private long nativePtr;
    private ExecutorService executor;

    public OlpHttpClient() {
        this.executor = Executors.newFixedThreadPool(THREAD_POOL_SIZE);
    }

    public static HttpVerb toHttpVerb(int verb) {
        switch (verb) {
            case 0:
                return HttpVerb.GET;
            case 1:
                return HttpVerb.POST;
            case 2:
                return HttpVerb.HEAD;
            case 3:
                return HttpVerb.PUT;
            case 4:
                return HttpVerb.DELETE;
            case 5:
                return HttpVerb.PATCH;
            case 6:
                return HttpVerb.OPTIONS;
            default:
                return HttpVerb.GET;
        }
    }

    public void shutdown() {
        if (this.executor != null) {
            this.executor.shutdown();
            this.executor = null;
        }
        synchronized (this) {
            // deinitialize the nativePtr to stop receiving scheduled events from Java to C++
            this.nativePtr = 0;
        }
    }

    public HttpTask send(
            String url,
            int httpMethod,
            long requestId,
            int connTimeout,
            int reqTimeout,
            String[] headers,
            byte[] postData,
            String proxyServer,
            int proxyPort,
            int proxyType) {
        final OlpRequest request =
                new OlpRequest(
                        url,
                        OlpHttpClient.toHttpVerb(httpMethod),
                        requestId,
                        connTimeout,
                        reqTimeout,
                        headers,
                        postData,
                        proxyServer,
                        proxyPort,
                        proxyType);
        final HttpTask task = new HttpTask(this);
        task.executeOnExecutor(executor, request);
        return task;
    }

    // Native methods:
    // Synchronization is required in order to provide thread-safe access to `nativePtr`
    // Callback for completed request
    private synchronized native void completeRequest(
            long requestId, int status, int uploadedBytes, int downloadedBytes, String error, String contentType);

    // Callback for data received
    private synchronized native void dataCallback(long requestId, byte[] data, int len);

    // Callback set date and offset
    private synchronized native void dateAndOffsetCallback(long requestId, long date, long offset);

    // Callback set date and offset
    private synchronized native void headersCallback(long requestId, String[] headers);

    public enum HttpVerb {
        GET,
        POST,
        HEAD,
        PUT,
        DELETE,
        PATCH,
        OPTIONS
    }

    /**
     * Task class sends the request HttpUrlConnection and responsible for handling response as well.
     */
    private static class HttpTask extends AsyncTask<OlpRequest, Void, Void> {
        private final WeakReference<OlpHttpClient> weakReference;
        private final AtomicBoolean cancelled = new AtomicBoolean(false);

        private HttpTask(OlpHttpClient client) {
            weakReference = new WeakReference<>(client);
        }

        public synchronized void cancelTask() {
            this.cancelled.set(true);
        }

        private Proxy createProxy(OlpRequest olpRequest) {
            if (olpRequest.noProxy()) {
                return Proxy.NO_PROXY;
            }

            return new Proxy(
                    olpRequest.proxyType(),
                    new InetSocketAddress(olpRequest.proxyServer(), olpRequest.proxyPort()));
        }

        @Override
        protected Void doInBackground(OlpRequest... olpRequests) {
            for (OlpRequest olpRequest : olpRequests) {
                // TODO: calculate downloaded and uploaded bytes
                // TODO: check cancellation
                // TODO: handle exceptions

                final OlpHttpClient olpHttpClient = weakReference.get();
                if (olpHttpClient == null) {
                    return null;
                }

                OkHttpClient client = new OkHttpClient.Builder()
                        .protocols(Arrays.asList(Protocol.HTTP_2, Protocol.HTTP_1_1))
                        .proxy(createProxy(olpRequest))
                        .connectTimeout(olpRequest.connectTimeout(), TimeUnit.SECONDS)
                        .readTimeout(olpRequest.requestTimeout(), TimeUnit.SECONDS)
                        .build();

                try {
                    Request.Builder requestBuilder = new Request.Builder();
                    requestBuilder.url(olpRequest.url());

                    if (olpRequest.postData() != null) {
                        requestBuilder.method(olpRequest.verb().toString(), RequestBody.create(olpRequest.postData()));
                    } else {
                        requestBuilder.method(olpRequest.verb().toString(), null);
                    }

                    {
                        String[] headers = olpRequest.headers();
                        if (headers != null) {
                            Headers.Builder headersBuilder = new Headers.Builder();

                            for (int j = 0; (j + 1) < headers.length; j += 2) {
                                headersBuilder.add(headers[j], headers[j + 1]);
                            }

                            requestBuilder.headers(headersBuilder.build());
                        }
                    }

                    Request request = requestBuilder.build();

                    Log.i(LOGTAG, "Request: " + request);

                    try (Response response = client.newCall(request).execute()) {
                        if (!response.isSuccessful()) {
                            throw new IOException("Unexpected code " + response);
                        }

                        Log.i(LOGTAG, "Response: " + response);

                        try (InputStream inputStream = Objects.requireNonNull(response.body()).byteStream()) {
                            byte[] buffer = new byte[8192];
                            int bytesRead;
                            while ((bytesRead = inputStream.read(buffer)) != -1) {
                                olpHttpClient.dataCallback(olpRequest.requestId(), buffer, bytesRead);
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }

                        String contentType = response.header("Content-Type", "");
                        olpHttpClient.completeRequest(olpRequest.requestId(), response.code(), 0, 0, response.message(), contentType);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }


                } catch (Exception e) {
                    e.printStackTrace();
                }
            }

            return null;
        }

        private void completeErrorRequest(long requestId, int status, int uploadedBytes, int downloadedBytes, String error) {
            final OlpHttpClient olpHttpClient = weakReference.get();
            if (olpHttpClient != null) {
                olpHttpClient.completeRequest(requestId, status, uploadedBytes, downloadedBytes, error, "");
            }
        }

        private void checkCancelled() {
            if (this.cancelled.get()) {
                throw new OperationCanceledException();
            }
        }

        private int calculateHeadersSize(Map<String, List<String>> headers) throws IOException {
            int size = 0;
            for (Map.Entry<String, List<String>> entry : headers.entrySet()) {
                String header = entry.getKey();
                List<String> values = entry.getValue();
                if (header != null) {
                    size += header.length();
                }
                for (String value : values) {
                    if (value != null) {
                        size += value.length();
                    }
                }
            }
            return size;
        }
    }

    /**
     * Class to hold the request's data, which will be used by HttpUrlConnection object.
     */
    public final class OlpRequest {
        private final String url;
        private final long requestId;
        private final int connectionTimeout;
        private final int requestTimeout;
        private final String[] headers;
        private final byte[] postData;
        private final HttpVerb verb;
        private final String proxyServer;
        private final int proxyPort;
        private final Proxy.Type proxyType;

        public OlpRequest(
                String url,
                HttpVerb verb,
                long requestId,
                int connectionTimeout,
                int requestTimeout,
                String[] headers,
                byte[] postData,
                String proxyServer,
                int proxyPort,
                int proxyType) {
            this.url = url;
            this.verb = verb;
            this.requestId = requestId;
            this.connectionTimeout = connectionTimeout;
            this.requestTimeout = requestTimeout;
            this.headers = headers;
            this.postData = postData;
            this.proxyServer = proxyServer;
            this.proxyPort = proxyPort;

            switch (proxyType) {
                case 0:
                    this.proxyType = Proxy.Type.DIRECT;
                    break;
                case 1:
                    this.proxyType = Proxy.Type.HTTP;
                    break;
                case 2:
                    Log.w(LOGTAG, "OlpHttpClient::Request(): Unsupported proxy version (" + proxyType + "). Falling back to HTTP(1)");
                    this.proxyType = Proxy.Type.HTTP;
                    break;
                case 3:
                    this.proxyType = Proxy.Type.SOCKS;
                    break;
                case 4:
                case 5:
                case 6:
                    Log.w(LOGTAG, "OlpHttpClient::Request(): Unsupported proxy version (" + proxyType + "). Falling back to SOCKS4(3)");
                    this.proxyType = Proxy.Type.SOCKS;
                    break;
                default:
                    Log.w(LOGTAG, "OlpHttpClient::Request(): Unsupported proxy version (" + proxyType + "). Falling back to HTTP(1)");
                    this.proxyType = Proxy.Type.HTTP;
                    break;
            }
        }

        public String url() {
            return this.url;
        }

        public HttpVerb verb() {
            return this.verb;
        }

        public long requestId() {
            return this.requestId;
        }

        public int connectTimeout() {
            return this.connectionTimeout;
        }

        public int requestTimeout() {
            return this.requestTimeout;
        }

        public String[] headers() {
            return this.headers;
        }

        public byte[] postData() {
            return this.postData;
        }

        public String proxyServer() {
            return this.proxyServer;
        }

        public int proxyPort() {
            return this.proxyPort;
        }

        public Proxy.Type proxyType() {
            return this.proxyType;
        }

        public boolean hasProxy() {
            return (this.proxyServer != null) && !this.proxyServer.isEmpty();
        }

        public boolean noProxy() {
            return (hasProxy() && this.proxyServer.equals("No")) || this.proxyType == Proxy.Type.DIRECT;
        }
    }
}
