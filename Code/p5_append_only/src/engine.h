#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "record.h"

namespace p5 {

/**
 * Minimal append-only KV (P5 stage 0–1).
 *
 * - Log is an in-memory byte buffer (toy stand-in for a segment file).
 * - Index maps key -> latest record offset.
 * - Compact() does full compaction: keep newest non-tombstone PUT per key.
 *
 * Failure conditions:
 *  Put/Delete: empty key -> false
 *  Get: missing key / tombstone / corrupt offset -> false
 */
class Engine {
 public:
  auto Put(const std::string &key, const std::string &value) -> bool;
  auto Get(const std::string &key, std::string *out) const -> bool;
  auto Delete(const std::string &key) -> bool;

  /** Full compaction: rewrite log with only latest live PUTs. */
  void Compact();

  auto TotalBytes() const -> size_t { return log_.size(); }
  auto LiveBytes() const -> size_t;
  auto Amplification() const -> double;
  auto NextSeq() const -> uint64_t { return next_seq_; }
  auto IndexSize() const -> size_t { return index_.size(); }

  /** Debug: scan whole log without index; returns steps examined. */
  auto GetByScan(const std::string &key, std::string *out, size_t *steps) const -> bool;

  /** Intentionally append garbage for corrupt-tail tests. */
  void AppendCorruptTail();

  auto CheckInvariants() const -> std::string;

 private:
  struct IndexEntry {
    size_t offset{0};
    uint64_t seq{0};
    RecordType type{RecordType::kPut};
  };

  auto AppendRecord(const Record &rec) -> size_t;
  auto ReadAt(size_t offset, Record *out) const -> bool;

  std::vector<char> log_;
  std::unordered_map<std::string, IndexEntry> index_;
  uint64_t next_seq_{1};
};

}  // namespace p5
