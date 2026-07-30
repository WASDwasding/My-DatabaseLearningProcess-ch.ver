#pragma once

#include <array>
#include <cstring>

#include "config.h"

namespace p4 {

/** One in-memory page frame payload + metadata used by BPM. */
class Page {
 public:
  Page() { Reset(); }

  auto GetData() -> char * { return data_.data(); }
  auto GetData() const -> const char * { return data_.data(); }

  auto GetPageId() const -> page_id_t { return page_id_; }
  void SetPageId(page_id_t page_id) { page_id_ = page_id; }

  auto GetPinCount() const -> int { return pin_count_; }
  void Pin() { ++pin_count_; }
  auto Unpin() -> bool {
    if (pin_count_ <= 0) {
      return false;
    }
    --pin_count_;
    return true;
  }

  auto IsDirty() const -> bool { return is_dirty_; }
  void SetDirty(bool dirty) { is_dirty_ = dirty; }

  void Reset() {
    page_id_ = INVALID_PAGE_ID;
    pin_count_ = 0;
    is_dirty_ = false;
    data_.fill(0);
  }

 private:
  page_id_t page_id_{INVALID_PAGE_ID};
  int pin_count_{0};
  bool is_dirty_{false};
  std::array<char, kPageSize> data_{};
};

}  // namespace p4
