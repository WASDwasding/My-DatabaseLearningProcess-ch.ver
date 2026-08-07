#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace p7 {

/**
 * Minimal in-memory extendible hash table (P7 stage 1).
 *
 * - key/value are int for teaching clarity
 * - Hash(key) = key (identity), so low bits are easy to hand-simulate
 * - Directory stores pointers to MemBucket
 * - Split + directory doubling follow P7-2
 *
 * Invariants:
 *  I1: directory.size() == 2^global_depth
 *  I2: for each bucket, #directory refs == 2^(global_depth - local_depth)
 *  I3: every key is in the bucket selected by low_bits(hash(key), global_depth)
 *  I4: bucket.size <= capacity
 *  I5: local_depth <= global_depth
 */
class ExtendibleHashTable {
 public:
  explicit ExtendibleHashTable(size_t bucket_capacity = 2);

  ~ExtendibleHashTable();

  ExtendibleHashTable(const ExtendibleHashTable &) = delete;
  auto operator=(const ExtendibleHashTable &) -> ExtendibleHashTable & = delete;

  /** Insert or update. Returns true. */
  auto Insert(int key, int value) -> bool;

  /** Lookup. Returns nullopt if missing. */
  auto Get(int key) const -> std::optional<int>;

  /** Remove. Returns false if key not found. */
  auto Remove(int key) -> bool;

  auto GetGlobalDepth() const -> int { return global_depth_; }
  auto GetBucketCapacity() const -> size_t { return bucket_capacity_; }
  auto DirectorySize() const -> size_t { return directory_.size(); }
  auto UniqueBucketCount() const -> size_t;

  /** Empty string if OK. */
  auto CheckInvariants() const -> std::string;

  /** Debug dump for failed tests. */
  auto DebugString() const -> std::string;

 private:
  struct MemBucket {
    int local_depth{0};
    std::vector<std::pair<int, int>> entries;  // (key, value)
  };

  static auto Hash(int key) -> uint32_t { return static_cast<uint32_t>(key); }

  auto LowBits(uint32_t h, int depth) const -> size_t {
    if (depth <= 0) {
      return 0;
    }
    return static_cast<size_t>(h & ((1u << depth) - 1u));
  }

  auto IndexOf(int key) const -> size_t { return LowBits(Hash(key), global_depth_); }

  auto FindBucket(int key) -> MemBucket *;
  auto FindBucket(int key) const -> const MemBucket *;

  void Split(MemBucket *bucket);
  void DoubleDirectory();

  int global_depth_{0};
  size_t bucket_capacity_;
  std::vector<MemBucket *> directory_;
};

}  // namespace p7
