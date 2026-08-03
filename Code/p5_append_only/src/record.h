#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace p5 {

enum class RecordType : uint8_t { kPut = 1, kDelete = 2 };

struct Record {
  std::string key;
  std::string value;  // empty for Delete
  RecordType type{RecordType::kPut};
  uint64_t seq{0};
};

/** Serialize record to bytes. Format:
 *  key_len(u32) | key | val_len(u32) | value | type(u8) | seq(u64)
 */
auto EncodeRecord(const Record &rec) -> std::vector<char>;

/**
 * Decode one record starting at data[offset].
 * On success, writes record and advances *bytes_read.
 * Returns false on truncated / corrupt input.
 */
auto DecodeRecord(const char *data, size_t size, size_t offset, Record *out, size_t *bytes_read)
    -> bool;

}  // namespace p5
