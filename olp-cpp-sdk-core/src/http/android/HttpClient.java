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

package com.here.olp.network;

import android.annotation.SuppressLint;
import android.os.AsyncTask;
import android.os.OperationCanceledException;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.ProtocolException;
import java.net.Proxy;
import java.net.URL;
import java.net.URLConnection;
import java.net.UnknownHostException;
import java.net.SocketTimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class HttpClient {
  private static final String LOGTAG = "HttpClient";

  // The replica of http/NetworkType.h error codes
  // TODO: retrieve them via native method
  public static final int IO_ERROR = -1;
  public static final int INVALID_URL_ERROR = -3;
  public static final int CANCELLED_ERROR = -5;
  public static final int TIMEOUT_ERROR = -7;
  public static final int THREAD_POOL_SIZE = 8;

  private int uniqueId = (int) (System.currentTimeMillis() % 10000);

  public enum HttpVerb {
    GET,
    POST,
    HEAD,
    PUT,
    DELETE,
    PATCH
  };

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
      default:
        return HttpVerb.GET;
    }
  }

  /** Class to hold the request's data, which will be used by HttpUrlConnection object. */
  public final class Request {
    public Request(
        String url,
        HttpVerb verb,
        int clientId,
        long requestId,
        int connectionTimeout,
        int requestTimeout,
        String[] headers,
        byte[] postData,
        String proxyServer,
        int proxyPort,
        int proxyType,
        int maxRetries) {
      this.url = url;
      this.verb = verb;
      this.clientId = clientId;
      this.requestId = requestId;
      this.connectionTimeout = connectionTimeout;
      this.requestTimeout = requestTimeout;
      this.headers = headers;
      this.postData = postData;
      this.proxyServer = proxyServer;
      this.proxyPort = proxyPort;

      switch (proxyType) {
        case 0:
          this.proxyType = Proxy.Type.HTTP;
          break;
        case 4:
          this.proxyType = Proxy.Type.SOCKS;
          break;
        case 5:
        case 6:
        case 7:
          this.proxyType = Proxy.Type.SOCKS;
          Log.e(
              LOGTAG,
              "HttpClient::Request(): Unsupported proxy version ("
                  + proxyType
                  + "). Falling back to SOCKS4(4)");
          break;
        default:
          Log.e(
              LOGTAG,
              "HttpClient::Request(): Unsupported proxy version ("
                  + proxyType
                  + "). Falling back to HTTP(0)");
          this.proxyType = Proxy.Type.HTTP;
          break;
      }
      this.maxRetries = maxRetries;
    }

    public final String url() {
      return this.url;
    }

    public final HttpVerb verb() {
      return this.verb;
    }

    public final int clientId() {
      return this.clientId;
    }

    public final long requestId() {
      return this.requestId;
    }

    public final int connectTimeout() {
      return this.connectionTimeout;
    }

    public final int requestTimeout() {
      return this.requestTimeout;
    }

    public final String[] headers() {
      return this.headers;
    }

    public final byte[] postData() {
      return this.postData;
    }

    public final String proxyServer() {
      return this.proxyServer;
    }

    public final int proxyPort() {
      return this.proxyPort;
    }

    public final Proxy.Type proxyType() {
      return this.proxyType;
    }

    public final boolean hasProxy() {
      return (this.proxyServer != null) && !this.proxyServer.isEmpty();
    }

    public final boolean noProxy() {
      return hasProxy() && this.proxyServer.equals("No");
    }

    public final int maxRetries() {
      return this.maxRetries;
    }

    private final String url;
    private final int clientId;
    private final long requestId;
    private final int connectionTimeout;
    private final int requestTimeout;
    private final String[] headers;
    private final byte[] postData;
    private final HttpVerb verb;
    private final String proxyServer;
    private final int proxyPort;
    private final Proxy.Type proxyType;
    private final int maxRetries;
  }

  /**
   * Task class sends the request HttpUrlConnection and responsible for handling response as well.
   */
  @SuppressLint("StaticFieldLeak")
  private class HttpTask extends AsyncTask<Request, Void, Void> {
    private AtomicBoolean cancelled = new AtomicBoolean(false);

    public synchronized void cancelTask() {
      this.cancelled.set(true);
    }

    @Override
    protected Void doInBackground(Request... requests) {
      for (Request request : requests) {
        HttpURLConnection httpConn = null;

        try {
          int retryCount = 0;
          boolean isDone = false;
          URL url = new URL(request.url());

          do {
            // TODO: replace with more elegant and controlled approach
            if (retryCount > 0) {
              Thread.sleep(100);

              cleanup(httpConn);
            }

            URLConnection conn;
            if (request.hasProxy()) {
              if (request.noProxy()) {
                conn = url.openConnection(Proxy.NO_PROXY);
              } else {
                conn =
                    url.openConnection(
                        new Proxy(
                            request.proxyType(),
                            new InetSocketAddress(request.proxyServer(), request.proxyPort())));
              }
            } else {
              conn = url.openConnection();
            }

            if (conn instanceof HttpURLConnection) {
              httpConn = (HttpURLConnection) conn;
            }

            if (httpConn != null) {
              switch (request.verb()) {
                case HEAD:
                  httpConn.setRequestMethod("HEAD");
                  break;
                case PUT:
                  httpConn.setRequestMethod("PUT");
                  break;
                case DELETE:
                  httpConn.setRequestMethod("DELETE");
                  break;
                case PATCH:
                  httpConn.setRequestMethod("PATCH");
                  break;
                default:
                  httpConn.setRequestMethod("GET");
                  break;
              }
            }

            String[] headers = request.headers();
            boolean useEtag = false;
            boolean userSetConnection = false;
            if (headers != null) {
              for (int j = 0; (j + 1) < headers.length; j += 2) {
                conn.addRequestProperty(headers[j], headers[j + 1]);
                if (headers[j].compareToIgnoreCase("If-None-Match") == 0) {
                  useEtag = true;
                }
                if (headers[j].compareToIgnoreCase("Connection") == 0) {
                  userSetConnection = true;
                }
              }
            }

            conn.setUseCaches(false);
            conn.setConnectTimeout(request.connectTimeout() * 1000);
            conn.setReadTimeout(request.requestTimeout() * 1000);

            if (request.verb() != HttpVerb.HEAD && httpConn != null) {
              if (request.postData() != null) {
                httpConn.setFixedLengthStreamingMode(request.postData().length);
              } else {
                httpConn.setChunkedStreamingMode(8 * 1024);
              }
            }

            // Android Issue 24672: workaround
            if (android.os.Build.VERSION.SDK_INT < 21) {
              if (request.verb() == HttpVerb.HEAD || useEtag)
                conn.setRequestProperty("Accept-Encoding", "");
            }

            // Connection-Close setting causes extensive ~3-4 times delay.
            // Keep user's connection setting if set explicitly.
            if (!userSetConnection) {
              // Fix too many open files issues on
              // JellyBean (Android <= 4.3) and Android 6.x devices JellyBean 18 -
              // Android 4.3, Marshmallow 23 - Android 6.x
              if (android.os.Build.VERSION.SDK_INT <= 23)
                conn.setRequestProperty("Connection", "Close");
            }

            conn.setDoInput(true);

            // Do POST if needed
            if (request.postData() != null) {
              conn.setDoOutput(true);
              conn.getOutputStream().write(request.postData());
            } else {
              conn.setDoOutput(false);
            }

            // Wait for status response
            int status = 0;
            String error = "";
            if (httpConn != null) {
              try {
                status = httpConn.getResponseCode();
                error = httpConn.getResponseMessage();

                if ((status > 0)
                    && ((status < HttpURLConnection.HTTP_OK)
                        || (status >= HttpURLConnection.HTTP_SERVER_ERROR))) {
                  if (retryCount++ < request.maxRetries()) {
                    continue;
                  }
                }
              } catch (SocketTimeoutException | UnknownHostException e) {
                if (retryCount++ < request.maxRetries()) {
                  resetRequest(request.clientId(), request.requestId());
                  continue;
                } else {
                  throw e;
                }
              }
            }

            checkCancelled();

            // Read headers
            String contentType = conn.getHeaderField("Content-Type");
            if (contentType == null) {
              contentType = "";
            }

            // Parse Content-Range if available
            long offset = 0;
            String httpHeader = conn.getHeaderField("Content-Range");
            if (httpHeader != null) {
              int index = httpHeader.indexOf("bytes ");
              if (index >= 0) {
                index += 6;
                int endIndex = httpHeader.indexOf('-', index);
                try {
                  String rangeStr;
                  if (endIndex > index) rangeStr = httpHeader.substring(index, endIndex);
                  else rangeStr = httpHeader.substring(index);
                  offset = Long.parseLong(rangeStr);
                } catch (Exception e) {
                  Log.d(LOGTAG, "parse offset: " + e);
                }
              }
            }

            // Get all the headers of the response
            int headersCount = 0;
            while (conn.getHeaderFieldKey(headersCount) != null) {
              headersCount++;
            }

            String[] headersArray = new String[2 * headersCount];
            for (int j = 0; j < headersCount; j++) {
              headersArray[2 * j] = conn.getHeaderFieldKey(j);
              headersArray[2 * j + 1] = conn.getHeaderField(j);
            }

            checkCancelled();

            headersCallback(request.clientId(), request.requestId(), headersArray);
            dateAndOffsetCallback(request.clientId(), request.requestId(), 0l, offset);

            // Do the input phase
            InputStream in = null;
            try {
              try {
                in = new BufferedInputStream(conn.getInputStream());
              } catch (FileNotFoundException e) {
                // error occurred, continuing with error stream
                if (httpConn != null) {
                  in = new BufferedInputStream(httpConn.getErrorStream());
                } else {
                  throw e;
                }
              }
              int len;
              byte[] buffer = new byte[8 * 1024];

              while ((len = in.read(buffer)) >= 0) {
                checkCancelled();
                dataCallback(request.clientId(), request.requestId(), buffer, len);
              }
            }
            // Error handling:
            catch (FileNotFoundException e) {
              // ensure that the status has been set, then complete the request
              if (status == 0) {
                throw e;
              }
            } catch (ProtocolException e) {
              if (status != HttpURLConnection.HTTP_NOT_MODIFIED
                  && status != HttpURLConnection.HTTP_NO_CONTENT) {
                throw e;
              }
            } catch (SocketTimeoutException e) {
              if (retryCount++ < request.maxRetries()) {
                resetRequest(request.clientId(), request.requestId());
                continue;
              } else {
                throw e;
              }
            }

            checkCancelled();

            // The request is completed, not cancelled or retried
            // Notifies the native (C++) side that request was completed
            isDone = true;
            completeRequest(request.clientId(), request.requestId(), status, error, contentType);
          } while (!isDone);
        } catch (OperationCanceledException e) {
          completeRequest(
              request.clientId(), request.requestId(), CANCELLED_ERROR, "Cancelled", "");
        } catch (SocketTimeoutException e) {
          completeRequest(request.clientId(), request.requestId(), TIMEOUT_ERROR, "Timed out", "");
        } catch (Exception e) {
          Log.e(LOGTAG, "HttpClient::HttpTask::run exception: " + e);
          e.printStackTrace();
          if (e instanceof UnknownHostException) {
            completeRequest(
                request.clientId(), request.requestId(), INVALID_URL_ERROR, "Unknown Host", "");
          } else {
            completeRequest(request.clientId(), request.requestId(), IO_ERROR, e.toString(), "");
          }
        } finally {
          cleanup(httpConn);
        }
      }

      return null;
    }

    private final void checkCancelled() {
      if (this.cancelled.get()) {
        throw new OperationCanceledException();
      }
    }

    private final void cleanup(HttpURLConnection httpConn) {
      if (httpConn == null) {
        return;
      }

      if (httpConn.getDoOutput()) {
        try {
          if (httpConn.getOutputStream() != null) {
            httpConn.getOutputStream().flush();
          }
        } catch (IOException e) {
          // no-op
        }
      }

      try {
        clearInputStream(httpConn.getInputStream());
      } catch (IOException e) {
        // no-op
      }

      try {
        clearInputStream(httpConn.getErrorStream());
      } catch (IOException e) {
        // no-op
      }

      if (httpConn.getDoOutput()) {
        try {
          if (httpConn.getOutputStream() != null) httpConn.getOutputStream().close();
        } catch (IOException e) {
          // no-op
        }
      }

      try {
        if (httpConn.getInputStream() != null) httpConn.getInputStream().close();
      } catch (IOException e) {
        // no-op
      }

      try {
        if (httpConn.getErrorStream() != null) httpConn.getErrorStream().close();
      } catch (IOException e) {
        // no-op
      }

      httpConn.disconnect();
    }

    private final void clearInputStream(InputStream stream) throws IOException {
      if (stream == null) {
        return;
      }

      final byte[] buffer = new byte[8 * 1024];
      while (stream.read(buffer) > 0) {
        // clear stream
      }
    }
  };

  private ExecutorService executor;

  public HttpClient() {
    this.executor = Executors.newFixedThreadPool(THREAD_POOL_SIZE);
  }

  public void shutdown() {
    if (this.executor != null) {
      this.executor.shutdown();
      this.executor = null;
    }
  }

  public int registerClient() {
    return this.uniqueId++;
  }

  public HttpTask send(
      String url,
      int httpMethod,
      int clientId,
      long requestId,
      int connTimeout,
      int reqTimeout,
      String[] headers,
      byte[] postData,
      String proxyServer,
      int proxyPort,
      int proxyType,
      int maxRetries) {
    Request request =
        new Request(
            url,
            HttpClient.toHttpVerb(httpMethod),
            clientId,
            requestId,
            connTimeout,
            reqTimeout,
            headers,
            postData,
            proxyServer,
            proxyPort,
            proxyType,
            maxRetries);
    HttpTask task = new HttpTask();
    task.executeOnExecutor(executor, request);
    return task;
  }

  // Native methods:
  // Callback for completed request
  private native void completeRequest(
      int clientId, long requestId, int status, String error, String contentType);
  // Callback for data received
  private native void dataCallback(int clientId, long requestId, byte[] data, int len);
  // Callback set date and offset
  private native void dateAndOffsetCallback(int clientId, long requestId, long date, long offset);
  // Callback set date and offset
  private native void headersCallback(int clientId, long requestId, String[] headers);
  // Reset request for retry
  private native void resetRequest(int clientId, long requestId);
}
