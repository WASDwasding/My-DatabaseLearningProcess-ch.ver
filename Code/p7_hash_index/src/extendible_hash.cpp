#include "extendible_hash.h"

#include <sstream>
#include <unordered_set>

namespace p7 {

ExtendibleHashTable::ExtendibleHashTable(size_t bucket_capacity)
    : bucket_capacity_(bucket_capacity == 0 ? 1 : bucket_capacity) {
  // Start with global_depth=0: one directory entry, one bucket.
  auto *bucket = new MemBucket();
  bucket->local_depth = 0;
  directory_.push_back(bucket);
  global_depth_ = 0;
}

ExtendibleHashTable::~ExtendibleHashTable() {
  std::unordered_set<MemBucket *> seen;
  for (MemBucket *b : directory_) {
    if (b != nullptr && seen.insert(b).second) {
      delete b;
    }
  }
}

auto ExtendibleHashTable::UniqueBucketCount() const -> size_t {
  std::unordered_set<const MemBucket *> seen;
  for (const MemBucket *b : directory_) {
    seen.insert(b);
  }
  return seen.size();
}

auto ExtendibleHashTable::FindBucket(int key) -> MemBucket * {
  return directory_[IndexOf(key)];
}

auto ExtendibleHashTable::FindBucket(int key) const -> const MemBucket * {
  return directory_[IndexOf(key)];
}

void ExtendibleHashTable::DoubleDirectory() {
  const size_t old_size = directory_.size();
  directory_.resize(old_size * 2);
  for (size_t i = 0; i < old_size; ++i) {
    directory_[i + old_size] = directory_[i];
  }
  ++global_depth_;
}

void ExtendibleHashTable::Split(MemBucket *bucket) {
  // 5a: need more directory bits before refining this bucket further
  if (bucket->local_depth == global_depth_) {
    DoubleDirectory();
  }

  // 5b: both sibling buckets will have local_depth = old + 1
  const int old_local = bucket->local_depth;
  const int new_local = old_local + 1;
  bucket->local_depth = new_local;

  // 5c: create sibling
  auto *sibling = new MemBucket();
  sibling->local_depth = new_local;

  // 5d: redistribute by the new bit (bit at position old_local)
  const uint32_t mask = 1u << old_local;
  std::vector<std::pair<int, int>> stay;
  stay.reserve(bucket->entries.size());
  for (const auto &kv : bucket->entries) {
    if ((Hash(kv.first) & mask) == 0) {
      stay.push_back(kv);
    } else {
      sibling->entries.push_back(kv);
    }
  }
  bucket->entries.swap(stay);

  // 5e: update directory entries that previously pointed to this bucket.
  // After split, use new_local bits: those with the new bit 0 -> bucket,
  // those with the new bit 1 -> sibling.
  for (size_t i = 0; i < directory_.size(); ++i) {
    if (directory_[i] != bucket) {
      continue;
    }
    // Compare low old_local bits of index with bucket's image.
    // Any directory index that had pointed here must match on old_local bits.
    if ((i & mask) == 0) {
      directory_[i] = bucket;
    } else {
      directory_[i] = sibling;
    }
  }
}

auto ExtendibleHashTable::Insert(int key, int value) -> bool {
  // May need multiple splits if redistribution is skewed (bucket_capacity small).
  for (int guard = 0; guard < 64; ++guard) {
    MemBucket *bucket = FindBucket(key);

    // Update existing key in-place (no size growth).
    for (auto &kv : bucket->entries) {
      if (kv.first == key) {
        kv.second = value;
        return true;
      }
    }

    if (bucket->entries.size() < bucket_capacity_) {
      bucket->entries.emplace_back(key, value);
      return true;
    }

    // Full -> split, then retry insert.
    Split(bucket);
  }
  return false;  // pathological: could not insert after many splits
}

auto ExtendibleHashTable::Get(int key) const -> std::optional<int> {
  const MemBucket *bucket = FindBucket(key);
  for (const auto &kv : bucket->entries) {
    if (kv.first == key) {
      return kv.second;
    }
  }
  return std::nullopt;
}

auto ExtendibleHashTable::Remove(int key) -> bool {
  MemBucket *bucket = FindBucket(key);
  for (auto it = bucket->entries.begin(); it != bucket->entries.end(); ++it) {
    if (it->first == key) {
      bucket->entries.erase(it);
      return true;
    }
  }
  return false;
}

auto ExtendibleHashTable::CheckInvariants() const -> std::string {
  std::ostringstream err;

  if (directory_.size() != (1u << global_depth_)) {
    err << "I1: directory.size != 2^global_depth";
    return err.str();
  }

  std::unordered_set<const MemBucket *> seen;
  for (size_t i = 0; i < directory_.size(); ++i) {
    const MemBucket *b = directory_[i];
    if (b == nullptr) {
      err << "I2: null directory entry at " << i;
      return err.str();
    }
    if (b->local_depth > global_depth_) {
      err << "I5: local_depth > global_depth";
      return err.str();
    }
    if (b->entries.size() > bucket_capacity_) {
      err << "I4: bucket over capacity";
      return err.str();
    }
    seen.insert(b);
  }

  for (const MemBucket *b : seen) {
    size_t refs = 0;
    for (const MemBucket *p : directory_) {
      if (p == b) {
        ++refs;
      }
    }
    const size_t expected = 1u << (global_depth_ - b->local_depth);
    if (refs != expected) {
      err << "I2: bucket refs=" << refs << " expected=" << expected
          << " (G=" << global_depth_ << " L=" << b->local_depth << ")";
      return err.str();
    }

    // Every key in this bucket must hash into an index that points back here.
    for (const auto &kv : b->entries) {
      const size_t idx = LowBits(Hash(kv.first), global_depth_);
      if (directory_[idx] != b) {
        err << "I3: key " << kv.first << " in wrong bucket";
        return err.str();
      }
    }
  }

  return {};
}

auto ExtendibleHashTable::DebugString() const -> std::string {
  std::ostringstream os;
  os << "G=" << global_depth_ << " dir_size=" << directory_.size() << "\n";
  std::unordered_set<const MemBucket *> printed;
  for (size_t i = 0; i < directory_.size(); ++i) {
    os << "  dir[" << i << "] -> bucket@" << directory_[i];
    if (printed.insert(directory_[i]).second) {
      os << " L=" << directory_[i]->local_depth << " size=" << directory_[i]->entries.size()
         << " keys=[";
      for (size_t k = 0; k < directory_[i]->entries.size(); ++k) {
        if (k) {
          os << ",";
        }
        os << directory_[i]->entries[k].first;
      }
      os << "]";
    }
    os << "\n";
  }
  return os.str();
}

}  // namespace p7
