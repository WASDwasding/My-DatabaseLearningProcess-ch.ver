#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../src/column_table.h"
#include "../src/encoding.h"
#include "../src/row_table.h"

namespace {

int g_failed = 0;

void Expect(bool cond, const std::string &msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    ++g_failed;
  } else {
    std::cout << "[PASS] " << msg << "\n";
  }
}

void FillWideTable(p6::RowTable *row, p6::ColumnTable *col, size_t n, size_t ncols) {
  for (size_t i = 0; i < n; ++i) {
    std::vector<int64_t> r(ncols, 0);
    // col0 = year-like filter key: mostly 2023, some 2024
    r[0] = (i % 100 == 0) ? 2024 : 2023;
    // col1 = sales
    r[1] = static_cast<int64_t>(i % 1000);
    for (size_t c = 2; c < ncols; ++c) {
      r[c] = static_cast<int64_t>(c);
    }
    row->AppendRow(r);
    col->AppendRow(r);
  }
}

void TestRowVsColumnBytes() {
  constexpr size_t kN = 10000;
  constexpr size_t kCols = 20;
  p6::RowTable row(kCols);
  p6::ColumnTable col(kCols);
  FillWideTable(&row, &col, kN, kCols);

  auto row_st = row.SumWhere(/*filter*/ 0, 2024, /*sum*/ 1);
  auto early = col.SumWhereEarly(0, 2024, 1);
  auto late = col.SumWhereLate(0, 2024, 1);

  Expect(row_st.result == early.result && early.result == late.result, "same SUM result");
  Expect(row_st.bytes_read == kN * kCols * sizeof(int64_t), "row pays full width");
  Expect(early.bytes_read == kN * 2 * sizeof(int64_t), "early column reads 2 full cols");
  Expect(late.bytes_read < early.bytes_read, "late reads less than early when selective");
  Expect(late.bytes_read < row_st.bytes_read, "column late << row bytes_read");
  Expect(row_st.bytes_read / late.bytes_read >= 5, "order-of-magnitude saving on wide table");
}

void TestLateMaterializationSelectivity() {
  p6::ColumnTable col(2);
  // 100 rows, only 1 matches -> 1% selectivity
  for (int i = 0; i < 100; ++i) {
    col.AppendRow({i == 0 ? 1 : 0, 10});
  }
  auto early = col.SumWhereEarly(0, 1, 1);
  auto late = col.SumWhereLate(0, 1, 1);
  Expect(early.result == 10 && late.result == 10, "sum correct");
  // early: 100*8 + 100*8 = 1600
  // late:  100*8 + 1*8 = 808
  Expect(early.bytes_read == 1600, "early bytes");
  Expect(late.bytes_read == 808, "late bytes at 1% selectivity");
}

void TestDictionaryEncoding() {
  p6::DictionaryColumn d;
  for (int i = 0; i < 1000; ++i) {
    d.Append((i % 2 == 0) ? "Red" : "Blue");
  }
  Expect(d.DictSize() == 2, "dict has Red/Blue");
  Expect(d.Get(0) == "Red" && d.Get(1) == "Blue" && d.Get(2) == "Red", "decode ok");
  Expect(d.EncodedBytes() < d.RawBytes(), "dict smaller than raw repeated strings");
}

void TestBitPacking() {
  std::vector<int64_t> vals;
  for (int i = 0; i < 1000; ++i) {
    vals.push_back(i % 1001);  // 0..1000 fits in 10 bits
  }
  p6::BitPackedColumn bp(10);
  bp.Encode(vals);
  auto decoded = bp.Decode();
  Expect(decoded == vals, "bitpack roundtrip");
  Expect(bp.EncodedBytes() < bp.RawBytes(vals.size()), "bitpack smaller than int64 array");
}

void TestZoneMapSkip() {
  p6::ColumnTable col(1);
  // chunk0: 0..99, chunk1: 1000..1099 — predicate 50 should skip chunk1
  for (int i = 0; i < 100; ++i) {
    col.AppendRow({i});
  }
  for (int i = 0; i < 100; ++i) {
    col.AppendRow({1000 + i});
  }
  size_t bytes = 0;
  auto rowids = col.FilterWithZoneMaps(0, 50, /*chunk_size=*/100, &bytes);
  Expect(rowids.size() == 1 && rowids[0] == 50, "found row 50");
  Expect(bytes == 100 * sizeof(int64_t), "skipped second chunk via zone map");
}

}  // namespace

int main() {
  TestRowVsColumnBytes();
  TestLateMaterializationSelectivity();
  TestDictionaryEncoding();
  TestBitPacking();
  TestZoneMapSkip();

  if (g_failed > 0) {
    std::cerr << "\n" << g_failed << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "\nAll p6_column_store tests passed.\n";
  return EXIT_SUCCESS;
}
