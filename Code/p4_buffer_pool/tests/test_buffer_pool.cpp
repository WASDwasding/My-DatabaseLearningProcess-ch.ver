#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "../src/buffer_pool_manager.h"

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

void WriteMarker(p4::Page *page, char c) { std::memset(page->GetData(), c, p4::kPageSize); }

auto ReadMarker(const p4::Page *page) -> char { return page->GetData()[0]; }

void TestNewFetchUnpinRoundTrip() {
  p4::DiskManager disk;
  p4::BufferPoolManager bpm(2, &disk);

  p4::page_id_t pid = p4::INVALID_PAGE_ID;
  p4::Page *p = bpm.NewPage(&pid);
  Expect(p != nullptr && pid == 0, "NewPage succeeds");
  WriteMarker(p, 'A');
  Expect(bpm.UnpinPage(pid, /*is_dirty=*/true), "Unpin dirty");

  p4::Page *again = bpm.FetchPage(pid);
  Expect(again != nullptr && ReadMarker(again) == 'A', "Fetch hit keeps content");
  Expect(bpm.UnpinPage(pid, false), "Unpin clean");
  Expect(bpm.CheckInvariants().empty(), "invariants ok");
}

void TestEvictFlushesDirty() {
  p4::DiskManager disk;
  p4::BufferPoolManager bpm(1, &disk);

  p4::page_id_t p0 = p4::INVALID_PAGE_ID;
  p4::Page *a = bpm.NewPage(&p0);
  Expect(a != nullptr, "create page0");
  WriteMarker(a, 'X');
  Expect(bpm.UnpinPage(p0, true), "unpin page0 dirty");

  p4::page_id_t p1 = p4::INVALID_PAGE_ID;
  p4::Page *b = bpm.NewPage(&p1);
  Expect(b != nullptr && p1 == 1, "create page1 evicts page0");
  Expect(bpm.UnpinPage(p1, false), "unpin page1");

  p4::Page *back = bpm.FetchPage(p0);
  Expect(back != nullptr && ReadMarker(back) == 'X', "evict flushed dirty page0");
  Expect(bpm.UnpinPage(p0, false), "unpin restored page0");
  Expect(bpm.CheckInvariants().empty(), "invariants after evict");
}

void TestAllPinnedCannotFetch() {
  p4::DiskManager disk;
  p4::BufferPoolManager bpm(1, &disk);

  p4::page_id_t p0 = p4::INVALID_PAGE_ID;
  Expect(bpm.NewPage(&p0) != nullptr, "new page0 pinned");

  p4::page_id_t p1 = p4::INVALID_PAGE_ID;
  Expect(bpm.NewPage(&p1) == nullptr, "NewPage fails when only frame pinned");
  Expect(bpm.FetchPage(99) == nullptr, "FetchPage fails when no victim");

  Expect(bpm.UnpinPage(p0, false), "unpin page0");
  Expect(bpm.NewPage(&p1) != nullptr, "NewPage works after unpin");
  Expect(bpm.UnpinPage(p1, false), "cleanup");
}

void TestDoublePinNeedsDoubleUnpin() {
  p4::DiskManager disk;
  p4::BufferPoolManager bpm(1, &disk);

  p4::page_id_t pid = p4::INVALID_PAGE_ID;
  Expect(bpm.NewPage(&pid) != nullptr, "new");
  Expect(bpm.FetchPage(pid) != nullptr, "second pin");
  Expect(bpm.UnpinPage(pid, false), "unpin #1 still pinned");
  Expect(bpm.NewPage(nullptr) == nullptr, "still no victim (pin=1)");
  Expect(bpm.UnpinPage(pid, false), "unpin #2 now evictable");

  p4::page_id_t other = p4::INVALID_PAGE_ID;
  Expect(bpm.NewPage(&other) != nullptr, "victim available");
  Expect(bpm.UnpinPage(other, false), "cleanup");
}

void TestFlushAndDelete() {
  p4::DiskManager disk;
  p4::BufferPoolManager bpm(2, &disk);

  p4::page_id_t pid = p4::INVALID_PAGE_ID;
  p4::Page *p = bpm.NewPage(&pid);
  WriteMarker(p, 'Z');
  Expect(bpm.UnpinPage(pid, true), "unpin dirty");
  Expect(bpm.FlushPage(pid), "flush");
  Expect(bpm.DeletePage(pid), "delete");

  p4::page_id_t other = p4::INVALID_PAGE_ID;
  Expect(bpm.NewPage(&other) != nullptr, "new after delete");
  Expect(bpm.UnpinPage(other, false), "cleanup");
  Expect(bpm.CheckInvariants().empty(), "invariants after delete");
}

}  // namespace

int main() {
  TestNewFetchUnpinRoundTrip();
  TestEvictFlushesDirty();
  TestAllPinnedCannotFetch();
  TestDoublePinNeedsDoubleUnpin();
  TestFlushAndDelete();

  if (g_failed > 0) {
    std::cerr << "\n" << g_failed << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "\nAll p4_buffer_pool tests passed.\n";
  return EXIT_SUCCESS;
}
