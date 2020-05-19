#pragma once

#include <mutex>
#include <string>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class NamedLock {
 public:
  std::unique_lock<std::mutex> AquireLock(const std::string& resource) {
    std::unique_lock<std::mutex> lock(mutex_);
    std::unique_lock<std::mutex> catalog_lock(mutexes_[resource]);
    return catalog_lock;
  }

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, std::mutex> mutexes_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp