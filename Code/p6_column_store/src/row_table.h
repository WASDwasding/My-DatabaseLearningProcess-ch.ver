#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace p6 {

struct ScanStats {
  size_t bytes_read{0};
  size_t rows_touched{0};
  int64_t result{0};
};

/**
 * Row-oriented table: each row stores all columns contiguously.
 * Scanning always pays ncols * sizeof(int64_t) per row (physical I/O model).
 */
class RowTable {
 public:
  explicit RowTable(size_t num_cols) : num_cols_(num_cols) {}

  void AppendRow(const std::vector<int64_t> &row) {
    data_.insert(data_.end(), row.begin(), row.end());
    ++num_rows_;
  }

  auto NumRows() const -> size_t { return num_rows_; }
  auto NumCols() const -> size_t { return num_cols_; }

  /** SELECT SUM(sum_col) WHERE filter_col == predicate */
  auto SumWhere(size_t filter_col, int64_t predicate, size_t sum_col) const -> ScanStats {
    ScanStats st;
    for (size_t r = 0; r < num_rows_; ++r) {
      st.bytes_read += num_cols_ * sizeof(int64_t);
      st.rows_touched += 1;
      const int64_t *row = &data_[r * num_cols_];
      if (row[filter_col] == predicate) {
        st.result += row[sum_col];
      }
    }
    return st;
  }

 private:
  size_t num_cols_{0};
  size_t num_rows_{0};
  std::vector<int64_t> data_;
};

}  // namespace p6
