#include <cstdlib>
#include <iostream>
#include <string>

#include "../src/table_page.h"

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

auto MakeFixed(char fill, uint16_t len) -> p3::Tuple {
  std::string s(len, fill);
  return p3::Tuple::FromString(s);
}

void TestInsertGetRoundTrip() {
  p3::TablePage page;
  page.Init(/*page_id=*/7);

  p3::RID rid;
  Expect(page.InsertTuple(p3::Tuple::FromString("hello"), &rid), "insert hello");
  Expect(rid.GetPageId() == 7 && rid.GetSlotNum() == 0, "rid is (7,0)");

  p3::Tuple out;
  Expect(page.GetTuple(rid, &out), "get hello");
  Expect(out.AsString() == "hello", "content matches");
  Expect(page.CheckInvariants().empty(), "invariants after insert/get");
}

void TestFillUntilFull() {
  p3::TablePage page;
  page.Init(1);

  constexpr uint16_t kTupleLen = 64;
  int inserted = 0;
  while (true) {
    p3::RID rid;
    if (!page.InsertTuple(MakeFixed(static_cast<char>('A' + (inserted % 26)), kTupleLen), &rid)) {
      break;
    }
    ++inserted;
  }

  Expect(inserted > 0, "filled page with some tuples");
  p3::RID rid;
  Expect(!page.InsertTuple(MakeFixed('Z', kTupleLen), &rid), "insert fails when full");
  Expect(page.CheckInvariants().empty(), "invariants still hold when full");
}

void TestMarkDeleteThenGetFails() {
  p3::TablePage page;
  page.Init(3);

  p3::RID rid;
  Expect(page.InsertTuple(p3::Tuple::FromString("alive"), &rid), "insert alive");
  Expect(page.MarkDelete(rid), "mark delete");
  Expect(!page.MarkDelete(rid), "second mark delete fails");

  p3::Tuple out;
  Expect(!page.GetTuple(rid, &out), "get deleted tuple fails (I6)");
  Expect(page.GetDeletedCount() == 1, "deleted_count == 1");
  Expect(page.CheckInvariants().empty(), "invariants after delete");
}

void TestReuseDeletedSlot() {
  p3::TablePage page;
  page.Init(4);

  p3::RID a;
  p3::RID b;
  Expect(page.InsertTuple(p3::Tuple::FromString("AAAA"), &a), "insert A");
  Expect(page.InsertTuple(p3::Tuple::FromString("BBBB"), &b), "insert B");
  Expect(page.MarkDelete(a), "delete A");

  const auto free_before = page.GetFreeSpace();
  p3::RID c;
  Expect(page.InsertTuple(p3::Tuple::FromString("CCCC"), &c), "insert C reuses deleted slot");
  Expect(c.GetSlotNum() == a.GetSlotNum(), "C reuses slot_num of A");
  Expect(page.GetFreeSpace() == free_before, "reuse does not consume new free space");

  p3::Tuple out;
  Expect(page.GetTuple(c, &out) && out.AsString() == "CCCC", "read C ok");
  Expect(a == c, "RID stable after reuse (same slot_num)");
  Expect(page.GetTuple(a, &out) && out.AsString() == "CCCC", "same RID reads new content");
  Expect(page.CheckInvariants().empty(), "invariants after reuse");
}

void TestCorruptSlotFailsCleanly() {
  p3::TablePage page;
  page.Init(5);

  p3::RID rid;
  Expect(page.InsertTuple(p3::Tuple::FromString("good"), &rid), "insert good");
  page.CorruptSlotOffset(rid.GetSlotNum(), /*bad_offset=*/0);

  p3::Tuple out;
  Expect(!page.GetTuple(rid, &out), "corrupt offset: GetTuple fails (no crash)");
  Expect(!page.CheckInvariants().empty(), "checker detects corrupt slot");
}

void TestEmptyAndOob() {
  p3::TablePage page;
  page.Init(6);

  p3::RID rid;
  Expect(!page.InsertTuple(p3::Tuple{}, &rid), "empty tuple insert fails");

  p3::Tuple out;
  Expect(!page.GetTuple(p3::RID(6, 0), &out), "get OOB slot fails");
  Expect(!page.MarkDelete(p3::RID(6, 99)), "mark delete OOB fails");
  Expect(!page.GetTuple(p3::RID(99, 0), &out), "wrong page_id fails");
}

}  // namespace

int main() {
  TestInsertGetRoundTrip();
  TestFillUntilFull();
  TestMarkDeleteThenGetFails();
  TestReuseDeletedSlot();
  TestCorruptSlotFailsCleanly();
  TestEmptyAndOob();

  if (g_failed > 0) {
    std::cerr << "\n" << g_failed << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "\nAll p3_page tests passed.\n";
  return EXIT_SUCCESS;
}
