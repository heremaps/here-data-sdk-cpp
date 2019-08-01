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

#include <olp/core/network/Network.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#include "Memory.h"
#include "NetworkEventImpl.h"
#include "NetworkRequestPriorityQueue.h"

#include "olp/core/logging/Log.h"
#include "olp/core/network/NetworkFactory.h"
#include "olp/core/network/NetworkProtocol.h"
#include "olp/core/network/NetworkResponse.h"
#include "olp/core/utils/Credentials.h"

namespace olp {
namespace network {
namespace {
#if defined(IGNORE_SIGPIPE)

// Block SIGPIPE signals for the current thread and all threads it creates.
int block_sigpipe() {
  sigset_t sigset;
  int err;

  err = sigemptyset(&sigset);
  assert(err == 0);
  err = sigaddset(&sigset, SIGPIPE);
  assert(err == 0);
  err = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
  assert(err == 0);

  return err;
}

// Curl7.35+OpenSSL can write into closed sockets sometimes which results
// in the process being terminated with SIGPIPE on Linux. Here's
// a workaround for that bug. It block SIGPIPE for the startup thread and
// hence for all other threads in the application. The variable itself is
// not used but can be examined.
int block_sigpipe_result = block_sigpipe();

#endif  // IGNORE_SIGPIPE

Network::RequestId NextRequestId() {
  static std::atomic<Network::RequestId> next_id{Network::NetworkRequestIdMin};
  Network::RequestId id = (next_id += 1);
  if (id == Network::NetworkRequestIdInvalid) {
    id = (next_id += 1);
  }
  return id;
}

ClientId NextClientId() {
  static std::atomic<ClientIdType> next_id{
      static_cast<ClientIdType>(ClientId::Min)};
  auto id = static_cast<ClientId>(next_id += 1);
  if (id == ClientId::Invalid) {
    id = static_cast<ClientId>(next_id += 1);
  }
  return id;
}
}  // anonymous namespace

#define LOGTAG "NETWORK"

/*
 * This is the Network interface implementation. There should be
 * maximum one instance of this class, which then is shared by
 * Network interface instances.
 */
class NetworkSingleton : public std::enable_shared_from_this<NetworkSingleton> {
 public:
  NetworkSingleton();
  ~NetworkSingleton();
  bool InitializeClient();
  void Send(const NetworkRequest& request, Network::RequestId requestId,
            const std::shared_ptr<std::ostream>& payload,
            const Network::Callback& callback,
            const Network::HeaderCallback& header_callback,
            const Network::DataCallback& data_callback,
            const std::shared_ptr<NetworkConfig>& config);
  bool Cancel(Network::RequestId request_id);
  bool CancelIfPending(Network::RequestId request_id);

  static void SetCertificateUpdater(std::function<void()> update_certificate);
  void Unblock();
  std::shared_ptr<NetworkProtocol> GetProtocol() const;

 private:
  std::shared_ptr<NetworkProtocol> protocol_;
  std::recursive_mutex send_lock_;
  std::atomic<bool> blocked_;
  // Please be careful when removing the following variable.
  // This creates a race condition that may block the network forever,
  // when certificate updates are triggered.
  std::atomic<bool> certificate_update_was_triggered_;
  static std::function<void()>* s_update_certificate_;

  mutable std::mutex mutex_;
};

static int s_listener_id_count_ = 1;
static std::atomic<Network::ConnectionStatus> s_connection_status_{
    Network::ConnectionStatus::Valid};
static std::mutex s_listener_mutex_;

using ListenerPair =
    std::pair<Network::NetworkStatusChangedCallback, mem::MemoryScopeTracker>;
using ListenerMap = mem::map<int, ListenerPair>;

static ListenerMap& GetListeners() {
  static ListenerMap s_listeners_(
      MEMORY_GET_TAG_ALLOCATOR("network", ListenerMap));
  return s_listeners_;
}

std::function<void()>* NetworkSingleton::s_update_certificate_ =
    new std::function<void()>([] {});

namespace {
std::weak_ptr<NetworkSingleton> g_singleton;
thread::Atomic<NetworkSystemConfig> g_system_config;

// We use a ptr for this mutex and never delete it to ensure it is always
// available for use. During program exit the deletion order of global objects
// is undefined and on iOS gSingletonMutex gets destroyed before gSingleton
// which leads to an exception when gSingleton attempts to lock the mutex during
// exit.
std::mutex* g_singleton_mutex = new std::mutex();

std::shared_ptr<NetworkSingleton> SingletonInstance() {
  std::lock_guard<std::mutex> lock(*g_singleton_mutex);
  std::shared_ptr<NetworkSingleton> singleton = g_singleton.lock();
  if (!singleton) {
    // Make the network allocator in scope prior to constructing the instance.
    MEMORY_SCOPED_TAG("network");
    singleton = std::make_shared<NetworkSingleton>();
    g_singleton = singleton;
  }
  return singleton;
}

void NotifyListeners(int status) {
  Network::ConnectionStatus networkError =
      (status == Network::Offline || status == Network::IOError
           ? Network::ConnectionStatus::NoConnection
           : Network::ConnectionStatus::Valid);
  if (s_connection_status_ == networkError) return;

  if (s_connection_status_ == Network::ConnectionStatus::NoConnection &&
      networkError == Network::ConnectionStatus::Valid) {
    networkError = Network::ConnectionStatus::ConnectionReestablished;
    s_connection_status_ = Network::ConnectionStatus::Valid;
  } else {
    s_connection_status_ = networkError;
  }

  ListenerMap listeners;
  {
    std::lock_guard<std::mutex> lock(s_listener_mutex_);
    listeners = GetListeners();
  }
  for (auto listener : listeners) {
    MEMORY_TRACKER_SCOPE(listener.second.second);
    listener.second.first(networkError);
  }
}

}  // anonymous namespace

NetworkSingleton::NetworkSingleton()
    : blocked_(false), certificate_update_was_triggered_(false) {}

NetworkSingleton::~NetworkSingleton() {
  if (protocol_) {
    protocol_->Deinitialize();
  }
}

bool NetworkSingleton::InitializeClient() {
  MEMORY_SCOPED_TAG("network");

  std::lock_guard<std::mutex> lock(mutex_);
  if (!protocol_) {
    protocol_ = NetworkFactory::CreateNetworkProtocol();
    if (!protocol_ || !protocol_->Initialize()) {
      protocol_.reset();
      return false;
    }
  }
  return true;
}

void NetworkSingleton::Send(const NetworkRequest& request,
                            Network::RequestId requestId,
                            const std::shared_ptr<std::ostream>& payload,
                            const Network::Callback& callback,
                            const Network::HeaderCallback& header_callback,
                            const Network::DataCallback& data_callback,
                            const std::shared_ptr<NetworkConfig>& config) {
  if (blocked_) {
    callback(NetworkResponse(requestId, Network::AuthorizationError,
                             "Waiting for certificates."));
    return;
  }

  std::shared_ptr<NetworkProtocol> protocol = GetProtocol();
  if (!protocol) {
    callback(NetworkResponse(requestId, Network::Offline, "Offline"));
    return;
  }

  std::weak_ptr<NetworkSingleton> singleton_weak = shared_from_this();
  const auto err = protocol->Send(
      request, requestId, payload, config, header_callback, data_callback,
      [singleton_weak, callback](const NetworkResponse& response) {
        auto singleton = singleton_weak.lock();

        if (response.Status() == Network::AuthenticationError && singleton &&
            !singleton->certificate_update_was_triggered_.exchange(true)) {
          singleton->blocked_ = true;
          EDGE_SDK_LOG_WARNING(LOGTAG,
                               "Certificate outdated. Blocking network traffic "
                               "until new certificate is  downloaded.");
          std::function<void()> updateCertificate;
          {
            std::lock_guard<std::mutex> lock(*g_singleton_mutex);
            updateCertificate = *s_update_certificate_;
          }
          updateCertificate();
        }

        if (!singleton) {
          EDGE_SDK_LOG_ERROR(LOGTAG, "singleton is destroyed");
        }

        callback(response);
      });
  if (err != NetworkProtocol::ErrorNone)
    HandleSynchronousNetworkErrors(err, requestId, callback);
}

bool NetworkSingleton::Cancel(Network::RequestId request_id) {
  std::shared_ptr<NetworkProtocol> protocol = GetProtocol();
  if (!protocol) return false;

  MEMORY_SCOPED_TAG("network");
  return protocol->Cancel(request_id);
}

bool NetworkSingleton::CancelIfPending(Network::RequestId request_id) {
  std::shared_ptr<NetworkProtocol> protocol = GetProtocol();
  if (!protocol) return false;

  MEMORY_SCOPED_TAG("network");
  return protocol->CancelIfPending(request_id);
}

void NetworkSingleton::Unblock() {
  blocked_ = false;
  EDGE_SDK_LOG_INFO(LOGTAG, "Unblocking network traffic.");
}

std::shared_ptr<NetworkProtocol> NetworkSingleton::GetProtocol() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return protocol_;
}

void NetworkSingleton::SetCertificateUpdater(
    std::function<void()> update_certificate) {
  std::lock_guard<std::mutex> lock(*g_singleton_mutex);
  if (!update_certificate) update_certificate = [] {};
  *s_update_certificate_ = std::move(update_certificate);
}

Network::Network()
    : id_(ClientId::Invalid), config_(std::make_shared<NetworkConfig>()) {}

Network::~Network() { Stop(); }

bool Network::Start(const NetworkConfig& config) {
  std::lock_guard<std::mutex> nwLock(mutex_);
  if (Started()) return false;

  EDGE_SDK_LOG_TRACE(LOGTAG, "start");
  singleton_ = SingletonInstance();
  if (!singleton_->InitializeClient()) {
    singleton_.reset();
    return false;
  }

  *config_ = config;
  id_ = NextClientId();
  return true;
}

bool Network::Stop() {
  std::shared_ptr<NetworkSingleton> singleton;
  std::vector<Network::RequestId> requestIds;
  {
    std::lock_guard<std::mutex> nwLock(mutex_);
    if (!Started()) return false;

    //        EDGE_SDK_LOG_TRACE( LOGTAG, "Stop" );
    id_ = ClientId::Invalid;

    requestIds = request_ids_->Clear();
    singleton = std::move(singleton_);
    singleton_.reset();
  }

  // Shut down any remaining tasks outside of the mutex lock.
  std::for_each(begin(requestIds), end(requestIds),
                [singleton](RequestId id) { singleton->Cancel(id); });
  return true;
}

bool Network::Restart(const NetworkConfig& config) {
  Stop();
  return Start(config);
}

bool Network::Started() const { return id_ != ClientId::Invalid; }

Network::RequestId Network::Send(NetworkRequest request,
                                 const std::shared_ptr<std::ostream>& payload,
                                 const Network::Callback& callback,
                                 const HeaderCallback& header_callback,
                                 const DataCallback& data_callback) {
  std::shared_ptr<NetworkSingleton> singleton = GetSingleton();
  if (!Started() || !singleton) {
    NetworkResponse resp(0, false, Offline, "Offline", 0, -1, "", "", 0, 0,
                         payload, NetworkProtocol::StatisticsData());
    callback(resp);
    return 0;
  }

  std::shared_ptr<NetworkConfig> config;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    config = config_;
  }

  EDGE_SDK_LOG_TRACE(
      LOGTAG, "send " << olp::utils::CensorCredentialsInUrl(request.Url()));
#if !defined(EDGE_SDK_LOGGING_DISABLED)
  for (const auto& header : request.ExtraHeaders()) {
    EDGE_SDK_LOG_TRACE(
        LOGTAG, "extra header: " << header.first << ": " << header.second);
  }
#endif

  const auto requestId = NextRequestId();

  request_ids_->Insert(requestId);
  auto requestIds = request_ids_;
  const auto url = olp::utils::CensorCredentialsInUrl(request.Url());
  singleton->Send(request, requestId, payload,
                  [requestIds, url, callback](const NetworkResponse& response) {
                    const auto status = response.Status();
                    requestIds->Remove(response.Id());

                    if (callback) {
                      callback(response);
                    }

                    NotifyListeners(status);
                  },
                  header_callback, data_callback, config);
  return requestId;
}

NetworkResponse Network::SendAndWait(
    NetworkRequest request, const std::shared_ptr<std::ostream>& payload,
    std::vector<std::pair<std::string, std::string> >& headers) {
  auto promise = std::make_shared<std::promise<NetworkResponse> >();

  Send(std::move(request), payload,
       [promise](const NetworkResponse& response) {
         promise->set_value(response);
       },
       [&headers](const std::string& key, const std::string& value) {
         headers.emplace_back(std::make_pair(key, value));
       });

  auto fut = promise->get_future();
  return fut.get();
}

bool Network::Cancel(Network::RequestId id) {
  if (!Started()) return false;

  std::shared_ptr<NetworkSingleton> singleton = GetSingleton();

  if (!singleton) return false;

  request_ids_->Remove(id);
  return singleton->Cancel(id);
}

bool Network::CancelIfPending(RequestId id) {
  if (!Started()) return false;

  std::shared_ptr<NetworkSingleton> singleton = GetSingleton();

  if (!singleton || !singleton->CancelIfPending(id)) return false;

  request_ids_->Remove(id);
  return true;
}

void Network::ResetSystemConfig() {
  g_system_config.lockedAssign(NetworkSystemConfig());
}

thread::Atomic<NetworkSystemConfig>& Network::SystemConfig() {
  return g_system_config;
}

std::int32_t Network::AddListenerCallback(
    const NetworkStatusChangedCallback& callback) {
  if (!callback) return 0;

  std::lock_guard<std::mutex> lock(s_listener_mutex_);
  int id = s_listener_id_count_++;
  auto& listeners = GetListeners();
  listeners[id] = ListenerPair(callback, mem::MemoryScopeTracker());
  return id;
}

void Network::RemoveListenerCallback(std::int32_t callback_id) {
  std::lock_guard<std::mutex> lock(s_listener_mutex_);
  GetListeners().erase(callback_id);
}

void Network::SetCertificateUpdater(std::function<void()> update_certificate) {
  NetworkSingleton::SetCertificateUpdater(std::move(update_certificate));
}

bool Network::GetStatistics(Statistics& statistics) {
  size_t clens = 0;
  size_t reqs = 0;
  size_t errs = 0;
  NetworkEventImpl::GetStatistics(clens, reqs, errs);
  statistics.content_bytes_ = clens;
  statistics.requests_ = reqs;
  statistics.errors_ = errs;
  return true;
}

void Network::Unblock() {
  std::lock_guard<std::mutex> lock(*g_singleton_mutex);
  std::shared_ptr<NetworkSingleton> singleton = g_singleton.lock();
  if (singleton) singleton->Unblock();
}

std::shared_ptr<NetworkSingleton> Network::GetSingleton() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return singleton_;
}

void Network::RequestIds::Insert(Network::RequestId id) {
  std::lock_guard<std::mutex> lock(mutex_);
  data_.emplace_back(id);
}

void Network::RequestIds::Remove(Network::RequestId id) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto entry = find(begin(data_), end(data_), id);

  if (entry != end(data_)) {
    data_.erase(entry);
  }

  return;
}

std::vector<Network::RequestId> Network::RequestIds::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Network::RequestId> result;
  swap(result, data_);
  return result;
}

}  // namespace network
}  // namespace olp
