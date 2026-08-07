#include "../src/extendible_hash.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using p7::ExtendibleHashTable;

static int g_failed = 0;

#define EXPECT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " #cond "\n";   \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

#define EXPECT_EQ(a, b)                                                        \
  do {                                                                         \
    auto _a = (a);                                                             \
    auto _b = (b);                                                             \
    if (!(_a == _b)) {                                                         \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " #a " == " #b  \
                << " (" << _a << " vs " << _b << ")\n";                        \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

static void RequireOk(const ExtendibleHashTable &t, const char *where) {
  const std::string err = t.CheckInvariants();
  if (!err.empty()) {
    std::cerr << "FAIL invariant at " << where << ": " << err << "\n"
              << t.DebugString();
    ++g_failed;
  }
}

void TestEmpty() {
  ExtendibleHashTable t(4);
  EXPECT_EQ(t.GetGlobalDepth(), 0);
  EXPECT_EQ(t.DirectorySize(), 1u);
  EXPECT_TRUE(!t.Get(1).has_value());
  EXPECT_TRUE(!t.Remove(1));
  RequireOk(t, "empty");
}

void TestBasicInsertGetUpdate() {
  ExtendibleHashTable t(4);
  EXPECT_TRUE(t.Insert(10, 100));
  EXPECT_TRUE(t.Insert(20, 200));
  EXPECT_EQ(*t.Get(10), 100);
  EXPECT_EQ(*t.Get(20), 200);

  // Update same key
  EXPECT_TRUE(t.Insert(10, 111));
  EXPECT_EQ(*t.Get(10), 111);
  RequireOk(t, "basic");
}

void TestRemove() {
  ExtendibleHashTable t(4);
  t.Insert(1, 10);
  t.Insert(2, 20);
  EXPECT_TRUE(t.Remove(1));
  EXPECT_TRUE(!t.Get(1).has_value());
  EXPECT_EQ(*t.Get(2), 20);
  EXPECT_TRUE(!t.Remove(1));
  // Re-insert after remove
  EXPECT_TRUE(t.Insert(1, 99));
  EXPECT_EQ(*t.Get(1), 99);
  RequireOk(t, "remove");
}

void TestSplitAndDirectoryGrowth() {
  // capacity=2 forces splits quickly
  ExtendibleHashTable t(2);
  for (int i = 0; i < 16; ++i) {
    EXPECT_TRUE(t.Insert(i, i * 10));
    RequireOk(t, "growth-loop");
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(*t.Get(i), i * 10);
  }
  EXPECT_TRUE(t.GetGlobalDepth() >= 1);
  EXPECT_TRUE(t.DirectorySize() == (1u << t.GetGlobalDepth()));
  RequireOk(t, "growth-end");
}

void TestCapacityOneSkews() {
  // bucket_size=1 + identity hash: many recursive splits on colliding low bits
  ExtendibleHashTable t(1);
  for (int i = 0; i < 32; ++i) {
    EXPECT_TRUE(t.Insert(i, i));
    RequireOk(t, "cap1");
  }
  for (int i = 0; i < 32; ++i) {
    EXPECT_EQ(*t.Get(i), i);
  }
  // With identity hash and capacity 1, each key needs its own leaf eventually
  // for distinct low-bit patterns; G should be reasonably large.
  EXPECT_TRUE(t.GetGlobalDepth() >= 5);
  RequireOk(t, "cap1-end");
}

void TestManyUpdatesNoGrowthFromDup() {
  ExtendibleHashTable t(4);
  t.Insert(7, 1);
  const int g0 = t.GetGlobalDepth();
  for (int v = 0; v < 100; ++v) {
    EXPECT_TRUE(t.Insert(7, v));
  }
  EXPECT_EQ(*t.Get(7), 99);
  EXPECT_EQ(t.GetGlobalDepth(), g0);  // update should not force growth
  RequireOk(t, "updates");
}

void TestMixedWorkload() {
  ExtendibleHashTable t(3);
  for (int i = 0; i < 50; ++i) {
    t.Insert(i, i);
  }
  for (int i = 0; i < 50; i += 2) {
    EXPECT_TRUE(t.Remove(i));
  }
  for (int i = 0; i < 50; ++i) {
    if (i % 2 == 0) {
      EXPECT_TRUE(!t.Get(i).has_value());
    } else {
      EXPECT_EQ(*t.Get(i), i);
    }
  }
  for (int i = 0; i < 50; i += 2) {
    t.Insert(i, i * 100);
  }
  for (int i = 0; i < 50; i += 2) {
    EXPECT_EQ(*t.Get(i), i * 100);
  }
  RequireOk(t, "mixed");
}

int main() {
  TestEmpty();
  TestBasicInsertGetUpdate();
  TestRemove();
  TestSplitAndDirectoryGrowth();
  TestCapacityOneSkews();
  TestManyUpdatesNoGrowthFromDup();
  TestMixedWorkload();

  if (g_failed == 0) {
    std::cout << "All p7 extendible hash tests passed.\n";
    return 0;
  }
  std::cerr << g_failed << " assertion(s) failed.\n";
  return 1;
}
