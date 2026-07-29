#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "rid.h"
#include "tuple.h"

namespace p3 {

constexpr size_t kPageSize = 4096;

/**
 * Minimal slotted TablePage (single-page, in-memory).
 *
 * Layout:
 *  | HEADER | SLOT ARRAY (grows forward) | FREE SPACE | TUPLE DATA (grows backward) |
 *
 * Header:
 *  next_page_id (4) | slot_count (2) | deleted_count (2) | free_space_ptr (2) | pad (2)
 *
 * Slot entry (6 bytes):
 *  offset (2) | length (2) | flags (2)   flags bit0 = deleted
 *
 * Failure conditions (API returns false / nullopt):
 *  InsertTuple: empty tuple, too large, not enough free space
 *  GetTuple:    slot OOB, deleted, corrupt offset/length
 *  MarkDelete:  slot OOB, already deleted
 */
class TablePage {
 public:
  static constexpr uint16_t kFlagDeleted = 0x1;

  TablePage();

  void Init(page_id_t page_id = 0, page_id_t next_page_id = INVALID_PAGE_ID);

  auto GetPageId() const -> page_id_t;
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);

  auto GetSlotCount() const -> uint16_t;
  auto GetDeletedCount() const -> uint16_t;
  auto GetFreeSpace() const -> uint16_t;

  /**
   * Insert tuple bytes into this page.
   * On success, writes RID (page_id, slot_num) and returns true.
   * Prefer reusing a deleted slot of equal length; otherwise append.
   */
  auto InsertTuple(const Tuple &tuple, RID *rid_out) -> bool;

  /** Read a live tuple. Fails on OOB / deleted / corrupt slot. */
  auto GetTuple(const RID &rid, Tuple *tuple_out) const -> bool;

  /** Logical delete. Does not reclaim free space. */
  auto MarkDelete(const RID &rid) -> bool;

  /** Debug checker for page invariants I1–I6. Returns empty string if OK. */
  auto CheckInvariants() const -> std::string;

  /** Intentional corruption helper for tests. */
  void CorruptSlotOffset(uint32_t slot_num, uint16_t bad_offset);

 private:
#pragma pack(push, 1)
  struct Header {
    page_id_t next_page_id{INVALID_PAGE_ID};
    uint16_t slot_count{0};
    uint16_t deleted_count{0};
    uint16_t free_space_ptr{kPageSize};  // lowest offset occupied by tuple data
    uint16_t reserved{0};
  };

  struct SlotEntry {
    uint16_t offset{0};
    uint16_t length{0};
    uint16_t flags{0};
  };
#pragma pack(pop)

  static constexpr size_t kHeaderSize = sizeof(Header);
  static constexpr size_t kSlotEntrySize = sizeof(SlotEntry);

  static_assert(kHeaderSize == 12);
  static_assert(kSlotEntrySize == 6);

  auto HeaderPtr() -> Header *;
  auto HeaderPtr() const -> const Header *;
  auto SlotPtr(uint32_t slot_num) -> SlotEntry *;
  auto SlotPtr(uint32_t slot_num) const -> const SlotEntry *;

  auto SlotArrayEnd() const -> uint16_t;
  auto FindReusableSlot(uint16_t length) const -> std::optional<uint16_t>;
  auto AllocateNewSlot(uint16_t length) -> std::optional<uint16_t>;
  auto IsSlotReadable(const SlotEntry &slot) const -> bool;

  page_id_t page_id_{INVALID_PAGE_ID};
  std::array<char, kPageSize> data_{};
};

}  // namespace p3
