#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "row_table.h"

namespace p6 {

struct ZoneMap {
  int64_t min_v{0};
  int64_t max_v{0};
};

/**
 * Column-oriented table: one vector per column.
 * Supports early vs late materialization scan costing for teaching.
 */
class ColumnTable {
 public:
  explicit ColumnTable(size_t num_cols) : cols_(num_cols) {}

  void AppendRow(const std::vector<int64_t> &row) {
    for (size_t c = 0; c < cols_.size(); ++c) {
      cols_[c].push_back(row[c]);
    }
    ++num_rows_;
  }

  auto NumRows() const -> size_t { return num_rows_; }
  auto NumCols() const -> size_t { return cols_.size(); }

  const std::vector<int64_t> &Column(size_t c) const { return cols_[c]; }

  /**
   * Early materialization style cost model (still column layout):
   * read filter_col and sum_col fully, then filter+sum in lockstep.
   */
  auto SumWhereEarly(size_t filter_col, int64_t predicate, size_t sum_col) const -> ScanStats {
    ScanStats st;
    st.bytes_read += num_rows_ * sizeof(int64_t);  // filter column
    st.bytes_read += num_rows_ * sizeof(int64_t);  // sum column
    st.rows_touched = num_rows_;
    for (size_t r = 0; r < num_rows_; ++r) {
      if (cols_[filter_col][r] == predicate) {
        st.result += cols_[sum_col][r];
      }
    }
    return st;
  }

  /**
   * Late materialization:
   * 1) scan filter column -> rowids
   * 2) gather sum column only at those rowids
   */
  auto SumWhereLate(size_t filter_col, int64_t predicate, size_t sum_col) const -> ScanStats {
    ScanStats st;
    st.bytes_read += num_rows_ * sizeof(int64_t);  // full filter column
    st.rows_touched = num_rows_;

    std::vector<size_t> rowids;
    for (size_t r = 0; r < num_rows_; ++r) {
      if (cols_[filter_col][r] == predicate) {
        rowids.push_back(r);
      }
    }

    st.bytes_read += rowids.size() * sizeof(int64_t);  // gather sum values
    for (size_t r : rowids) {
      st.result += cols_[sum_col][r];
    }
    return st;
  }

  /** Build per-chunk zone maps for one column (chunk_size rows each). */
  auto BuildZoneMaps(size_t col, size_t chunk_size) const -> std::vector<ZoneMap> {
    std::vector<ZoneMap> maps;
    if (chunk_size == 0 || num_rows_ == 0) {
      return maps;
    }
    for (size_t start = 0; start < num_rows_; start += chunk_size) {
      size_t end = std::min(start + chunk_size, num_rows_);
      int64_t mn = cols_[col][start];
      int64_t mx = cols_[col][start];
      for (size_t i = start + 1; i < end; ++i) {
        mn = std::min(mn, cols_[col][i]);
        mx = std::max(mx, cols_[col][i]);
      }
      maps.push_back(ZoneMap{mn, mx});
    }
    return maps;
  }

  /**
   * Filter using zone maps: skip chunks that cannot match predicate.
   * Returns matching rowids and counts bytes actually scanned in filter column.
   */
  auto FilterWithZoneMaps(size_t filter_col, int64_t predicate, size_t chunk_size,
                          size_t *bytes_scanned) const -> std::vector<size_t> {
    auto maps = BuildZoneMaps(filter_col, chunk_size);
    std::vector<size_t> rowids;
    size_t bytes = 0;
    for (size_t ci = 0; ci < maps.size(); ++ci) {
      const auto &zm = maps[ci];
      if (predicate < zm.min_v || predicate > zm.max_v) {
        continue;  // skip whole chunk I/O
      }
      size_t start = ci * chunk_size;
      size_t end = std::min(start + chunk_size, num_rows_);
      bytes += (end - start) * sizeof(int64_t);
      for (size_t r = start; r < end; ++r) {
        if (cols_[filter_col][r] == predicate) {
          rowids.push_back(r);
        }
      }
    }
    if (bytes_scanned != nullptr) {
      *bytes_scanned = bytes;
    }
    return rowids;
  }

 private:
  size_t num_rows_{0};
  std::vector<std::vector<int64_t>> cols_;
};

}  // namespace p6
