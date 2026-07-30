#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config.h"
#include "disk_manager.h"
#include "lru_replacer.h"
#include "page.h"

namespace p4 {

/**
 * Single-threaded BufferPoolManager (P4 stage 1).
 *
 * APIs:
 *  NewPage / FetchPage / UnpinPage / FlushPage / DeletePage
 *
 * Failure conditions:
 *  Fetch/New: no free frame and no evictable victim (all pinned) -> nullptr / false
 *  Unpin: page not in pool, or pin_count already 0 -> false
 *  Flush/Delete: page not present / still pinned (Delete) -> false
 */
class BufferPoolManager {
 public:
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager);

  auto NewPage(page_id_t *page_id_out) -> Page *;
  auto FetchPage(page_id_t page_id) -> Page *;
  auto UnpinPage(page_id_t page_id, bool is_dirty) -> bool;
  auto FlushPage(page_id_t page_id) -> bool;
  auto DeletePage(page_id_t page_id) -> bool;
  void FlushAllPages();

  auto GetPoolSize() const -> size_t { return pool_size_; }
  auto CheckInvariants() const -> std::string;

 private:
  auto FindFreeFrame() -> std::optional<frame_id_t>;
  auto AllocateFrameForPage(page_id_t page_id, bool is_new) -> Page *;

  size_t pool_size_;
  DiskManager *disk_manager_;
  std::vector<Page> pages_;
  std::unordered_map<page_id_t, frame_id_t> page_table_;
  std::unordered_set<frame_id_t> free_list_;
  LruReplacer replacer_;
};

}  // namespace p4
