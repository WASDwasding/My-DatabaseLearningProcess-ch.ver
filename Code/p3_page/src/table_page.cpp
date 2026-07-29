#include "table_page.h"

#include <algorithm>
#include <cstring>
#include <sstream>

namespace p3 {

TablePage::TablePage() { Init(); }

void TablePage::Init(page_id_t page_id, page_id_t next_page_id) {
  page_id_ = page_id;
  data_.fill(0);
  auto *hdr = HeaderPtr();
  hdr->next_page_id = next_page_id;
  hdr->slot_count = 0;
  hdr->deleted_count = 0;
  hdr->free_space_ptr = static_cast<uint16_t>(kPageSize);
  hdr->reserved = 0;
}

auto TablePage::HeaderPtr() -> Header * { return reinterpret_cast<Header *>(data_.data()); }

auto TablePage::HeaderPtr() const -> const Header * {
  return reinterpret_cast<const Header *>(data_.data());
}

auto TablePage::SlotPtr(uint32_t slot_num) -> SlotEntry * {
  return reinterpret_cast<SlotEntry *>(data_.data() + kHeaderSize) + slot_num;
}

auto TablePage::SlotPtr(uint32_t slot_num) const -> const SlotEntry * {
  return reinterpret_cast<const SlotEntry *>(data_.data() + kHeaderSize) + slot_num;
}

auto TablePage::GetPageId() const -> page_id_t { return page_id_; }

auto TablePage::GetNextPageId() const -> page_id_t { return HeaderPtr()->next_page_id; }

void TablePage::SetNextPageId(page_id_t next_page_id) { HeaderPtr()->next_page_id = next_page_id; }

auto TablePage::GetSlotCount() const -> uint16_t { return HeaderPtr()->slot_count; }

auto TablePage::GetDeletedCount() const -> uint16_t { return HeaderPtr()->deleted_count; }

auto TablePage::SlotArrayEnd() const -> uint16_t {
  return static_cast<uint16_t>(kHeaderSize + kSlotEntrySize * HeaderPtr()->slot_count);
}

auto TablePage::GetFreeSpace() const -> uint16_t {
  const auto free_ptr = HeaderPtr()->free_space_ptr;
  const auto slot_end = SlotArrayEnd();
  if (free_ptr < slot_end) {
    return 0;
  }
  return static_cast<uint16_t>(free_ptr - slot_end);
}

auto TablePage::FindReusableSlot(uint16_t length) const -> std::optional<uint16_t> {
  const auto *hdr = HeaderPtr();
  for (uint16_t i = 0; i < hdr->slot_count; ++i) {
    const auto *slot = SlotPtr(i);
    if ((slot->flags & kFlagDeleted) != 0 && slot->length == length) {
      return i;
    }
  }
  return std::nullopt;
}

auto TablePage::AllocateNewSlot(uint16_t length) -> std::optional<uint16_t> {
  auto *hdr = HeaderPtr();
  const uint16_t needed = static_cast<uint16_t>(kSlotEntrySize + length);
  if (GetFreeSpace() < needed) {
    return std::nullopt;
  }

  const uint16_t offset = static_cast<uint16_t>(hdr->free_space_ptr - length);
  const uint16_t slot_num = hdr->slot_count;

  auto *slot = SlotPtr(slot_num);
  slot->offset = offset;
  slot->length = length;
  slot->flags = 0;

  hdr->free_space_ptr = offset;
  hdr->slot_count = static_cast<uint16_t>(slot_num + 1);
  return slot_num;
}

auto TablePage::InsertTuple(const Tuple &tuple, RID *rid_out) -> bool {
  const uint16_t length = tuple.GetLength();
  if (length == 0) {
    return false;
  }
  // Tuple must leave room for at least header + one slot entry.
  if (length > kPageSize - kHeaderSize - kSlotEntrySize) {
    return false;
  }

  std::optional<uint16_t> slot_num = FindReusableSlot(length);
  if (slot_num.has_value()) {
    auto *slot = SlotPtr(*slot_num);
    std::memcpy(data_.data() + slot->offset, tuple.GetData(), length);
    slot->flags = 0;
    auto *hdr = HeaderPtr();
    if (hdr->deleted_count > 0) {
      hdr->deleted_count = static_cast<uint16_t>(hdr->deleted_count - 1);
    }
  } else {
    slot_num = AllocateNewSlot(length);
    if (!slot_num.has_value()) {
      return false;
    }
    auto *slot = SlotPtr(*slot_num);
    std::memcpy(data_.data() + slot->offset, tuple.GetData(), length);
  }

  if (rid_out != nullptr) {
    rid_out->Set(page_id_, *slot_num);
  }
  return true;
}

auto TablePage::IsSlotReadable(const SlotEntry &slot) const -> bool {
  if ((slot.flags & kFlagDeleted) != 0) {
    return false;
  }
  if (slot.length == 0) {
    return false;
  }
  const uint32_t end = static_cast<uint32_t>(slot.offset) + slot.length;
  if (end > kPageSize) {
    return false;
  }
  // Slot data must live in the data region (at/after free_space_ptr, before page end).
  if (slot.offset < HeaderPtr()->free_space_ptr) {
    return false;
  }
  // Must not overlap the header/slot-array region.
  if (slot.offset < SlotArrayEnd()) {
    return false;
  }
  return true;
}

auto TablePage::GetTuple(const RID &rid, Tuple *tuple_out) const -> bool {
  if (rid.GetPageId() != page_id_) {
    return false;
  }
  const auto slot_num = rid.GetSlotNum();
  if (slot_num >= HeaderPtr()->slot_count) {
    return false;
  }

  const auto *slot = SlotPtr(slot_num);
  if (!IsSlotReadable(*slot)) {
    return false;
  }

  if (tuple_out != nullptr) {
    *tuple_out = Tuple(data_.data() + slot->offset, slot->length);
    tuple_out->SetRid(rid);
  }
  return true;
}

auto TablePage::MarkDelete(const RID &rid) -> bool {
  if (rid.GetPageId() != page_id_) {
    return false;
  }
  const auto slot_num = rid.GetSlotNum();
  auto *hdr = HeaderPtr();
  if (slot_num >= hdr->slot_count) {
    return false;
  }

  auto *slot = SlotPtr(slot_num);
  if ((slot->flags & kFlagDeleted) != 0) {
    return false;
  }

  slot->flags = static_cast<uint16_t>(slot->flags | kFlagDeleted);
  hdr->deleted_count = static_cast<uint16_t>(hdr->deleted_count + 1);
  return true;
}

void TablePage::CorruptSlotOffset(uint32_t slot_num, uint16_t bad_offset) {
  if (slot_num >= HeaderPtr()->slot_count) {
    return;
  }
  SlotPtr(slot_num)->offset = bad_offset;
}

auto TablePage::CheckInvariants() const -> std::string {
  const auto *hdr = HeaderPtr();
  std::ostringstream err;

  if (hdr->free_space_ptr > kPageSize) {
    err << "I5: free_space_ptr out of page";
    return err.str();
  }
  if (SlotArrayEnd() > hdr->free_space_ptr) {
    err << "I3/I5: slot array overlaps free/data region";
    return err.str();
  }

  uint16_t deleted = 0;
  std::vector<std::pair<uint16_t, uint16_t>> live_ranges;

  for (uint16_t i = 0; i < hdr->slot_count; ++i) {
    const auto *slot = SlotPtr(i);
    if ((slot->flags & kFlagDeleted) != 0) {
      ++deleted;
      continue;
    }

    const uint32_t end = static_cast<uint32_t>(slot->offset) + slot->length;
    if (slot->length == 0 || end > kPageSize || slot->offset < hdr->free_space_ptr ||
        slot->offset < SlotArrayEnd()) {
      err << "I1: live slot " << i << " has invalid range";
      return err.str();
    }
    live_ranges.emplace_back(slot->offset, static_cast<uint16_t>(end));
  }

  if (deleted != hdr->deleted_count) {
    err << "I4: deleted_count mismatch";
    return err.str();
  }

  std::sort(live_ranges.begin(), live_ranges.end());
  for (size_t i = 1; i < live_ranges.size(); ++i) {
    if (live_ranges[i].first < live_ranges[i - 1].second) {
      err << "I2: overlapping live tuples";
      return err.str();
    }
  }

  return {};
}

}  // namespace p3
