#pragma once

#include <list>
#include <optional>
#include <unordered_map>

#include "config.h"

namespace p4 {

/**
 * Simple LRU replacer for evictable frames only.
 * Victim() returns the least-recently used frame among those with SetEvictable(true).
 */
class LruReplacer {
 public:
  explicit LruReplacer(size_t capacity) : capacity_(capacity) {}

  void RecordAccess(frame_id_t frame_id) {
    if (!evictable_.count(frame_id) || !evictable_[frame_id]) {
      // Still refresh position if currently tracked as non-evictable pin holder? No:
      // only track frames that are currently in the LRU list (evictable).
      return;
    }
    Touch(frame_id);
  }

  void SetEvictable(frame_id_t frame_id, bool set_evictable) {
    if (set_evictable) {
      if (!evictable_[frame_id]) {
        evictable_[frame_id] = true;
        lru_list_.push_front(frame_id);
        table_[frame_id] = lru_list_.begin();
      }
    } else {
      if (evictable_[frame_id]) {
        lru_list_.erase(table_[frame_id]);
        table_.erase(frame_id);
      }
      evictable_[frame_id] = false;
    }
  }

  auto Victim() -> std::optional<frame_id_t> {
    if (lru_list_.empty()) {
      return std::nullopt;
    }
    frame_id_t frame = lru_list_.back();
    lru_list_.pop_back();
    table_.erase(frame);
    evictable_[frame] = false;
    return frame;
  }

  void Remove(frame_id_t frame_id) {
    if (evictable_[frame_id]) {
      lru_list_.erase(table_[frame_id]);
      table_.erase(frame_id);
    }
    evictable_.erase(frame_id);
  }

  auto Size() const -> size_t { return lru_list_.size(); }

 private:
  void Touch(frame_id_t frame_id) {
    lru_list_.erase(table_[frame_id]);
    lru_list_.push_front(frame_id);
    table_[frame_id] = lru_list_.begin();
  }

  size_t capacity_;
  std::list<frame_id_t> lru_list_;
  std::unordered_map<frame_id_t, std::list<frame_id_t>::iterator> table_;
  std::unordered_map<frame_id_t, bool> evictable_;
};

}  // namespace p4
