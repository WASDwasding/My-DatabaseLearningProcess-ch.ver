#pragma once

#include <cstdint>
#include <sstream>
#include <string>

namespace p3 {

using page_id_t = int32_t;
constexpr page_id_t INVALID_PAGE_ID = -1;

/** Record Identifier: (page_id, slot_num). slot_num is a stable slot-array index. */
class RID {
 public:
  RID() = default;
  RID(page_id_t page_id, uint32_t slot_num) : page_id_(page_id), slot_num_(slot_num) {}

  auto GetPageId() const -> page_id_t { return page_id_; }
  auto GetSlotNum() const -> uint32_t { return slot_num_; }

  void Set(page_id_t page_id, uint32_t slot_num) {
    page_id_ = page_id;
    slot_num_ = slot_num;
  }

  auto ToString() const -> std::string {
    std::ostringstream os;
    os << "RID{page=" << page_id_ << ", slot=" << slot_num_ << "}";
    return os.str();
  }

  auto operator==(const RID &other) const -> bool {
    return page_id_ == other.page_id_ && slot_num_ == other.slot_num_;
  }

 private:
  page_id_t page_id_{INVALID_PAGE_ID};
  uint32_t slot_num_{0};
};

}  // namespace p3
