#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace p6 {

/**
 * Dictionary encoding for low-cardinality strings.
 * Stores dict + int32 id column (ids themselves can later be bit-packed).
 */
class DictionaryColumn {
 public:
  void Append(const std::string &value) {
    auto it = value_to_id_.find(value);
    if (it == value_to_id_.end()) {
      int32_t id = static_cast<int32_t>(dict_.size());
      dict_.push_back(value);
      value_to_id_[value] = id;
      ids_.push_back(id);
    } else {
      ids_.push_back(it->second);
    }
  }

  auto Size() const -> size_t { return ids_.size(); }
  auto DictSize() const -> size_t { return dict_.size(); }
  auto Get(size_t i) const -> const std::string & { return dict_[ids_[i]]; }
  auto IdAt(size_t i) const -> int32_t { return ids_[i]; }

  /** Rough encoded size: dict strings + compact ids (1B if dict<=256). */
  auto EncodedBytes() const -> size_t {
    size_t id_width = dict_.size() <= 256 ? 1 : 4;
    size_t n = ids_.size() * id_width;
    for (const auto &s : dict_) {
      n += s.size();
    }
    return n;
  }

  auto RawBytes() const -> size_t {
    size_t n = 0;
    for (int32_t id : ids_) {
      n += dict_[id].size();
    }
    return n;
  }

 private:
  std::vector<std::string> dict_;
  std::unordered_map<std::string, int32_t> value_to_id_;
  std::vector<int32_t> ids_;
};

/**
 * Bit-pack non-negative integers that fit in `bits_per_value` bits.
 * Teaching toy: pack into a bit stream stored as bytes.
 */
class BitPackedColumn {
 public:
  BitPackedColumn() = default;
  explicit BitPackedColumn(int bits_per_value) : bits_(bits_per_value) {}

  void Encode(const std::vector<int64_t> &values) {
    values_count_ = values.size();
    data_.clear();
    uint64_t acc = 0;
    int filled = 0;
    for (int64_t v : values) {
      acc |= (static_cast<uint64_t>(v) & ((1ULL << bits_) - 1)) << filled;
      filled += bits_;
      while (filled >= 8) {
        data_.push_back(static_cast<uint8_t>(acc & 0xFF));
        acc >>= 8;
        filled -= 8;
      }
    }
    if (filled > 0) {
      data_.push_back(static_cast<uint8_t>(acc & 0xFF));
    }
  }

  auto Decode() const -> std::vector<int64_t> {
    std::vector<int64_t> out;
    out.reserve(values_count_);
    size_t bit_pos = 0;
    for (size_t i = 0; i < values_count_; ++i) {
      uint64_t v = 0;
      for (int b = 0; b < bits_; ++b) {
        size_t byte_i = (bit_pos + b) / 8;
        int bit_i = static_cast<int>((bit_pos + b) % 8);
        if (byte_i < data_.size() && ((data_[byte_i] >> bit_i) & 1)) {
          v |= (1ULL << b);
        }
      }
      out.push_back(static_cast<int64_t>(v));
      bit_pos += static_cast<size_t>(bits_);
    }
    return out;
  }

  auto EncodedBytes() const -> size_t { return data_.size(); }
  auto RawBytes(size_t n) const -> size_t { return n * sizeof(int64_t); }
  auto BitsPerValue() const -> int { return bits_; }

 private:
  int bits_{10};
  size_t values_count_{0};
  std::vector<uint8_t> data_;
};

}  // namespace p6
