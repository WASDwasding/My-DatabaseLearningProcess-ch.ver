#pragma once

#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "config.h"

namespace p4 {

/**
 * Minimal DiskManager: in-memory "disk" keyed by page_id.
 * Enough to teach Read/Write/Allocate without real filesystem noise.
 */
class DiskManager {
 public:
  auto AllocatePage() -> page_id_t {
    page_id_t id = next_page_id_++;
    store_[id].assign(kPageSize, 0);
    return id;
  }

  void ReadPage(page_id_t page_id, char *dest) {
    auto it = store_.find(page_id);
    if (it == store_.end()) {
      std::memset(dest, 0, kPageSize);
      return;
    }
    std::memcpy(dest, it->second.data(), kPageSize);
  }

  void WritePage(page_id_t page_id, const char *src) {
    auto &buf = store_[page_id];
    if (buf.size() != kPageSize) {
      buf.assign(kPageSize, 0);
    }
    std::memcpy(buf.data(), src, kPageSize);
  }

  auto DeallocatePage(page_id_t page_id) -> bool {
    return store_.erase(page_id) > 0;
  }

  auto HasPage(page_id_t page_id) const -> bool { return store_.count(page_id) > 0; }

 private:
  page_id_t next_page_id_{0};
  std::unordered_map<page_id_t, std::vector<char>> store_;
};

}  // namespace p4
