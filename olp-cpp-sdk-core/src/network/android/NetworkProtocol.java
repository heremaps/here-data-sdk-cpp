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
import android.net.SSLCertificateSocketFactory;
import android.net.TrafficStats;
import android.os.AsyncTask;
import android.os.OperationCanceledException;
import android.util.Log;

import org.apache.http.conn.ssl.AllowAllHostnameVerifier;

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
import java.text.SimpleDateFormat;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.Date;
import java.util.Locale;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;

public class NetworkProtocol {
  private final String LOGTAG = "NetworkProtocol";

  private int m_clientId = (int) (System.currentTimeMillis() % 10000);

  // Callback for completed request
  private native void completeRequest(int clientId, int requestId, int status, String error,
      int maxAge, int expires, String etag, String contentType);
  // Callback for data received
  private native void dataCallback(int clientId, int requestId, byte[] data, int len);
  // Callback set date and offset
  private native void dateAndOffsetCallback(int clientId, int requestId, long date, long offset);
  // Callback set date and offset
  private native void headersCallback(int clientId, int requestId, String[] headers);
  // Reset request for retry
  private native void resetRequest(int clientId, int requestId);

  public enum HttpVerb { GET, POST, HEAD, PUT, DELETE, PATCH }
  ;

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

  public class Request {
    public Request(String url, HttpVerb verb, int clientId, int requestId, int connectTimeout,
        int requestTimeout, String[] headers, byte[] postData, boolean ignoreCertificate,
        String proxyServer, int proxyPort, int proxyType, String certificatePath, int maxRetries) {
      m_url = url;
      m_verb = verb;
      m_clientId = clientId;
      m_requestId = requestId;
      m_connectTimeout = connectTimeout;
      m_requestTimeout = requestTimeout;
      m_headers = headers;
      m_postData = postData;
      m_proxyServer = proxyServer;
      m_proxyPort = proxyPort;
      m_certificatePath = certificatePath;
      switch (proxyType) {
        case 0:
          m_proxyType = Proxy.Type.HTTP;
          break;
        case 4:
          m_proxyType = Proxy.Type.SOCKS;
          break;
        case 5:
        case 6:
        case 7:
          m_proxyType = Proxy.Type.SOCKS;
          Log.e(LOGTAG,
              "NetworkProtocol::Request(): Unsupported proxy version (" + proxyType
                  + "). Falling back to SOCKS4(4)");
          break;
        default:
          Log.e(LOGTAG,
              "NetworkProtocol::Request(): Unsupported proxy version (" + proxyType
                  + "). Falling back to HTTP(0)");
          m_proxyType = Proxy.Type.HTTP;
          break;
      }
      m_maxRetries = maxRetries;
    }

    public final String url() {
      return m_url;
    }

    public final HttpVerb verb() {
      return m_verb;
    }

    public final int clientId() {
      return m_clientId;
    }

    public final int requestId() {
      return m_requestId;
    }

    public final int connectTimeout() {
      return m_connectTimeout;
    }

    public final int requestTimeout() {
      return m_requestTimeout;
    }

    public final String[] headers() {
      return m_headers;
    }

    public final byte[] postData() {
      return m_postData;
    }

    public final String proxyServer() {
      return m_proxyServer;
    }

    public final int proxyPort() {
      return m_proxyPort;
    }

    public final Proxy.Type proxyType() {
      return m_proxyType;
    }

    public final boolean hasProxy() {
      return (m_proxyServer != null) && !m_proxyServer.isEmpty();
    }

    public final boolean noProxy() {
      return hasProxy() && m_proxyServer.equals("No");
    }

    public final String certificatePath() {
      return m_certificatePath;
    }

    public final int maxRetries() {
      return m_maxRetries;
    }

    private final String m_url;
    private final int m_clientId;
    private final int m_requestId;
    private final int m_connectTimeout;
    private final int m_requestTimeout;
    private final String[] m_headers;
    private final byte[] m_postData;
    private final HttpVerb m_verb;
    private final String m_proxyServer;
    private final int m_proxyPort;
    private final Proxy.Type m_proxyType;
    private final String m_certificatePath;
    private final int m_maxRetries;
  }

  // Transmission worker
  @SuppressLint("StaticFieldLeak")
  private class GetTask extends AsyncTask<Request, Void, Void> {
    private AtomicBoolean m_cancel = new AtomicBoolean(false);

    public synchronized void cancelTask() {
      m_cancel.set(true);
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
            if (retryCount > 0) {
              Thread.sleep(100);

              cleanup(httpConn);
            }

            URLConnection conn;
            if (request.hasProxy())
              if (request.noProxy())
                conn = url.openConnection(Proxy.NO_PROXY);
              else
                conn = url.openConnection(new Proxy(request.proxyType(),
                    new InetSocketAddress(request.proxyServer(), request.proxyPort())));
            else
              conn = url.openConnection();

            // Custom certificate
            if (conn instanceof HttpsURLConnection) {
              HttpsURLConnection httpsConn = (HttpsURLConnection) conn;
              SSLContext sslContext = null;
              if (!request.certificatePath().isEmpty()) {
                sslContext =
                    NetworkSSLContextFactory.getInstance().getSSLContext(request.certificatePath());
                if (sslContext == null)
                  Log.e(LOGTAG,
                      "NetworkProtocol::GetTask::run failed to create ssl context, certificate path is set to ? "
                          + request.certificatePath());
              }

              if (sslContext != null) {
                httpsConn.setSSLSocketFactory(sslContext.getSocketFactory());
              }
            }

            if (conn instanceof HttpURLConnection)
              httpConn = (HttpURLConnection) conn;

            if (httpConn != null) {
              if (request.verb() == HttpVerb.HEAD) {
                httpConn.setRequestMethod("HEAD");
              } else if (request.verb() == HttpVerb.PUT) {
                httpConn.setRequestMethod("PUT");
              } else if (request.verb() == HttpVerb.DELETE) {
                httpConn.setRequestMethod("DELETE");
              } else if (request.verb() == HttpVerb.POST) {
                httpConn.setRequestMethod("POST");
              } else if (request.verb() == HttpVerb.PATCH) {
                httpConn.setRequestMethod("PATCH");
              } else { // Default to get
                httpConn.setRequestMethod("GET");
              }
            }

            String[] headers = request.headers();
            boolean useEtag = false;
            boolean userSetConnection = false;
            if (headers != null) {
              for (int j = 0; (j + 1) < headers.length; j += 2) {
                conn.addRequestProperty(headers[j], headers[j + 1]);
                if (headers[j].compareToIgnoreCase("If-None-Match") == 0)
                  useEtag = true;
                if (headers[j].compareToIgnoreCase("Connection") == 0)
                  userSetConnection = true;
              }
            }

            TrafficStats.setThreadStatsTag(0);

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

            // Connection-Close setting causes extensive ~3-4 times delay
            // comparing to StarterSDK 3.6. Keep user's connection setting if set
            // explicitly.
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

                if ((status > 0) && ((status < 200) || (status >= 500))) {
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
            String etag = conn.getHeaderField("ETag");
            if (etag == null)
              etag = "";

            String contentType = conn.getHeaderField("Content-Type");
            if (contentType == null)
              contentType = "";

            long date = conn.getHeaderFieldDate("Date", 0);

            // Parse Cache-Control if available
            int maxAge = 0;
            String httpHeader = conn.getHeaderField("Cache-Control");
            if (httpHeader != null) {
              int index = httpHeader.indexOf("max-age=");
              if (index >= 0) {
                index += 8;
                int endIndex = httpHeader.indexOf(',', index);
                String maxAgeStr;
                try {
                  if (endIndex > index)
                    maxAgeStr = httpHeader.substring(index, endIndex);
                  else
                    maxAgeStr = httpHeader.substring(index);
                  maxAge = Integer.parseInt(maxAgeStr);
                } catch (Exception e) {
                  Log.d(LOGTAG, "parse maxAge: " + e);
                }
              }
            }
            // Parse expires
            int expires = -1;
            httpHeader = conn.getHeaderField("Expires");
            if (httpHeader != null) {
              if (httpHeader.contentEquals("-1"))
                expires = -1;
              else if (httpHeader.contentEquals("0"))
                expires = 0;
              else {
                try {
                  SimpleDateFormat format =
                      new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss zzz", Locale.US);
                  Date expiryDate = format.parse(httpHeader);
                  expires = (int) expiryDate.getTime();
                } catch (Exception e) {
                  Log.d(LOGTAG, "parse expires: " + e);
                }
              }
            }

            // Parse Content-Range if available
            long offset = 0;
            httpHeader = conn.getHeaderField("Content-Range");
            if (httpHeader != null) {
              int index = httpHeader.indexOf("bytes ");
              if (index >= 0) {
                index += 6;
                int endIndex = httpHeader.indexOf('-', index);
                try {
                  String rangeStr;
                  if (endIndex > index)
                    rangeStr = httpHeader.substring(index, endIndex);
                  else
                    rangeStr = httpHeader.substring(index);
                  offset = Long.parseLong(rangeStr);
                } catch (Exception e) {
                  Log.d(LOGTAG, "parse offset: " + e);
                }
              }
            }

            // Get all the headers of the response
            int hdrCount = 0;
            while (conn.getHeaderFieldKey(hdrCount) != null) hdrCount++;

            String[] hdrArray = new String[2 * hdrCount];
            for (int j = 0; j < hdrCount; j++) {
              hdrArray[2 * j] = conn.getHeaderFieldKey(j);
              hdrArray[2 * j + 1] = conn.getHeaderField(j);
            }

            checkCancelled();

            headersCallback(request.clientId(), request.requestId(), hdrArray);

            // Date and Offset Callback
            dateAndOffsetCallback(request.clientId(), request.requestId(), date, offset);

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
            } catch (FileNotFoundException e) {
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

            // Completed, not cancelled or retried
            isDone = true;
            completeRequest(request.clientId(), request.requestId(), status, error, maxAge, expires,
                etag, contentType);
          } while (!isDone);
        } catch (OperationCanceledException e) {
          completeRequest(request.clientId(), request.requestId(), -5, "Cancelled", 0, -1, "", "");
        } catch (SocketTimeoutException e) {
          completeRequest(request.clientId(), request.requestId(), -7, "Timed out", 0, -1, "", "");
        } catch (Exception e) {
          Log.e(LOGTAG, "NetworkProtocol::GetTask::run exception: " + e);
          e.printStackTrace();
          if (e instanceof UnknownHostException) {
            // -3 here matches hype::Network::InvalidUrlError
            completeRequest(
                request.clientId(), request.requestId(), -3, "Unknown Host", 0, -1, "", "");
          } else {
            // -1 here matches hype::Network::IOError
            completeRequest(
                request.clientId(), request.requestId(), -1, e.toString(), 0, -1, "", "");
          }
        } finally {
          cleanup(httpConn);
        }
      }

      return null;
    }

    private final void checkCancelled() {
      if (m_cancel.get()) {
        throw new OperationCanceledException();
      }
    }

    private final void cleanup(HttpURLConnection httpConn) {
      if (httpConn == null)
        return;

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
          if (httpConn.getOutputStream() != null)
            httpConn.getOutputStream().close();
        } catch (IOException e) {
          // no-op
        }
      }

      try {
        if (httpConn.getInputStream() != null)
          httpConn.getInputStream().close();
      } catch (IOException e) {
        // no-op
      }

      try {
        if (httpConn.getErrorStream() != null)
          httpConn.getErrorStream().close();
      } catch (IOException e) {
        // no-op
      }

      httpConn.disconnect();
    }

    private final void clearInputStream(InputStream stream) throws IOException {
      if (stream == null)
        return;

      final byte[] buffer = new byte[8 * 1024];
      while (stream.read(buffer) > 0) {
        // clear stream
      }
    }
  };

  private ExecutorService m_executor;

  public NetworkProtocol() {
    m_executor = Executors.newFixedThreadPool(8);
  }

  public void shutdown() {
    if (m_executor != null) {
      m_executor.shutdown();
      m_executor = null;
    }
  }

  public int registerClient() {
    return m_clientId++;
  }

  public GetTask send(String url, int httpMethod, int clientId, int requestId, int connTimeout,
      int reqTimeout, String[] headers, byte[] postData, boolean ignoreCert, String proxyServer,
      int proxyPort, int proxyType, String certificatePath, int maxRetries) {
    Request request = new Request(url, NetworkProtocol.toHttpVerb(httpMethod), clientId, requestId,
        connTimeout, reqTimeout, headers, postData, ignoreCert, proxyServer, proxyPort, proxyType,
        certificatePath, maxRetries);
    GetTask task = new GetTask();
    task.executeOnExecutor(m_executor, request);
    return task;
  }
}
