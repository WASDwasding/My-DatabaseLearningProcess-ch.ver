#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "rid.h"

namespace p3 {

/**
 * In-memory tuple: opaque bytes (+ optional RID).
 * Schema interpretation is left to the caller for this minimal lab.
 */
class Tuple {
 public:
  Tuple() = default;

  explicit Tuple(std::vector<char> data) : data_(std::move(data)) {}

  Tuple(const void *bytes, uint16_t length) {
    data_.resize(length);
    if (length > 0 && bytes != nullptr) {
      std::memcpy(data_.data(), bytes, length);
    }
  }

  static auto FromString(const std::string &s) -> Tuple {
    return Tuple(s.data(), static_cast<uint16_t>(s.size()));
  }

  auto GetData() const -> const char * { return data_.data(); }
  auto GetLength() const -> uint16_t { return static_cast<uint16_t>(data_.size()); }
  auto GetRid() const -> RID { return rid_; }
  void SetRid(RID rid) { rid_ = rid; }

  auto AsString() const -> std::string { return std::string(data_.begin(), data_.end()); }

  auto operator==(const Tuple &other) const -> bool { return data_ == other.data_; }

 private:
  std::vector<char> data_;
  RID rid_{};
};

}  // namespace p3
