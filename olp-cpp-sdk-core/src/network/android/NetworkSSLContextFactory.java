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

import android.net.SSLCertificateSocketFactory;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.security.cert.CertificateFactory;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.List;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;

class NetworkSSLContextFactory {
  private final String LOGTAG = "NetworkSSLContextFactory";
  private static NetworkSSLContextFactory s_instance = null;
  private CertificateFactory m_certificateFactory;
  private String m_certificatesPath;
  private SSLContext m_sslContext;
  private TrustManagerFactory m_trustManager;

  private NetworkSSLContextFactory() {
    m_sslContext = null;
    m_certificatesPath = null;
    m_certificateFactory = null;
    try {
      m_certificateFactory = CertificateFactory.getInstance("X.509");
    } catch (Exception e) {
      Log.e(LOGTAG, "X509 CertificateFactory failed to create" + e);
    }
  }

  private void generateSSlContext() {
    if (m_certificateFactory == null) {
      Log.w(LOGTAG, "generateSSlContext failed since certificateFactory is null");
      return;
    }

    // Create a KeyStore containing our trusted CAs
    try {
      String keyStoreType = KeyStore.getDefaultType();
      KeyStore keyStore = KeyStore.getInstance(keyStoreType);
      keyStore.load(null, null);

      // Add CAs into keystore
      List<File> certFiles = getListFiles(new File(m_certificatesPath));

      for (int i = 0; i < certFiles.size(); ++i) {
        File certFile = certFiles.get(i);
        Certificate ca = loadCertificate(certFile);
        if (ca != null) {
          keyStore.setCertificateEntry(certFile.getName(), ca);
        } else {
          Log.e(LOGTAG, "invalid certificate file " + certFile.getName());
        }
      }

      // Create a TrustManager that trusts the CAs in our KeyStore
      String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
      tmf.init(keyStore);

      // Create an SSLContext that uses our TrustManager
      m_sslContext = SSLContext.getInstance("TLS");
      m_sslContext.init(null, tmf.getTrustManagers(), null);
    } catch (Exception e) {
      Log.e(LOGTAG, "failed to generate ssl context " + e);
    }
  }

  private Certificate loadCertificate(File certFile) {
    Certificate ca = null;
    if (certFile.exists()) {
      try {
        InputStream caInput = new BufferedInputStream(new FileInputStream(certFile));
        ca = m_certificateFactory.generateCertificate(caInput);
        if (ca instanceof X509Certificate) {
          X509Certificate x509Cert = (X509Certificate) ca;
          // Log.d(LOGTAG, "ca=" + x509Cert.getSubjectDN() + certFile.getName());
        }
        caInput.close();
      } catch (Exception e) {
        ca = null;
        Log.e(LOGTAG, "Load certificate failed " + e);
      }
    } else {
      Log.e(LOGTAG, "certificate file " + certFile.getName() + "does not exist");
    }
    return ca;
  }

  private List<File> getListFiles(File parentDir) {
    ArrayList<File> inFiles = new ArrayList<File>();
    File[] files = parentDir.listFiles();
    for (File file : files) {
      if (file.isDirectory()) {
        inFiles.addAll(getListFiles(file));
      } else {
        // According to SDK implementation, size with 1 is the help file
        if (file.length() > 1)
          inFiles.add(file);
      }
    }
    return inFiles;
  }

  private static class LazyHolder {
    // Initialization-on-demand holder idiom
    private static final NetworkSSLContextFactory INSTANCE = new NetworkSSLContextFactory();
  }

  public static NetworkSSLContextFactory getInstance() {
    return LazyHolder.INSTANCE;
  }

  public SSLContext getSSLContext(String certificatesPath) {
    if (certificatesPath == null) {
      Log.w(LOGTAG, "getSSLContext certificatesPath is null");
      return null;
    }

    if (certificatesPath.isEmpty()) {
      Log.w(LOGTAG, "getSSLContext certificatesPath is empty");
      return null;
    }

    synchronized (this) {
      boolean generateContext = false;
      if (m_certificatesPath == null) {
        generateContext = true;
      } else if (m_certificatesPath.compareToIgnoreCase(certificatesPath) != 0) {
        generateContext = true;
      }

      if (generateContext) {
        m_certificatesPath = certificatesPath;
        generateSSlContext();
      }
    }

    return m_sslContext;
  }
}
