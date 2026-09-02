#include "../src/b_plus_tree.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using p8::BPlusTree;

static int g_failed = 0;

#define EXPECT_TRUE(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " #cond "\n"; \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

#define EXPECT_EQ(a, b)                                                        \
  do {                                                                         \
    auto _a = (a);                                                             \
    auto _b = (b);                                                             \
    if (!(_a == _b)) {                                                         \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " (" << _a        \
                << " vs " << _b << ")\n";                                      \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

static void RequireOk(const BPlusTree &t, const char *where) {
  const std::string err = t.CheckInvariants();
  if (!err.empty()) {
    std::cerr << "FAIL invariant at " << where << ": " << err << "\n"
              << t.DebugString();
    ++g_failed;
  }
  const std::string chain = t.CheckLeafChain();
  if (!chain.empty()) {
    std::cerr << "FAIL leaf chain at " << where << ": " << chain << "\n"
              << t.DebugString();
    ++g_failed;
  }
}

void TestEmpty() {
  BPlusTree t(3);
  EXPECT_TRUE(t.Empty());
  EXPECT_EQ(t.Height(), 0);
  EXPECT_TRUE(!t.Get(1).has_value());
  EXPECT_TRUE(!t.Remove(1));
  RequireOk(t, "empty");
}

void TestBasicInsertGetUpdate() {
  BPlusTree t(3);
  EXPECT_TRUE(t.Insert(10, 100));
  EXPECT_TRUE(t.Insert(20, 200));
  EXPECT_EQ(*t.Get(10), 100);
  EXPECT_EQ(*t.Get(20), 200);
  EXPECT_TRUE(t.Insert(10, 111));
  EXPECT_EQ(*t.Get(10), 111);
  EXPECT_EQ(t.Size(), 2u);
  RequireOk(t, "basic");
}

void TestSequentialInsertSplits() {
  BPlusTree t(2);  // small fanout forces splits
  for (int i = 1; i <= 20; ++i) {
    EXPECT_TRUE(t.Insert(i, i * 10));
    RequireOk(t, "seq-insert");
  }
  for (int i = 1; i <= 20; ++i) {
    EXPECT_EQ(*t.Get(i), i * 10);
  }
  EXPECT_TRUE(t.Height() >= 2);
  RequireOk(t, "seq-end");
}

void TestReverseInsert() {
  BPlusTree t(2);
  for (int i = 30; i >= 1; --i) {
    t.Insert(i, i);
    RequireOk(t, "rev-insert");
  }
  for (int i = 1; i <= 30; ++i) {
    EXPECT_EQ(*t.Get(i), i);
  }
}

void TestIteratorOrdered() {
  BPlusTree t(3);
  std::vector<int> keys = {5, 1, 9, 3, 7, 2, 8, 4, 6};
  for (int k : keys) {
    t.Insert(k, k * 100);
  }
  std::sort(keys.begin(), keys.end());
  std::vector<int> got;
  for (auto it = t.Begin(); !it.IsEnd(); ++it) {
    got.push_back(it.Key());
    EXPECT_EQ(it.Value(), it.Key() * 100);
  }
  EXPECT_TRUE(got == keys);

  // Begin(key)
  auto it = t.Begin(4);
  EXPECT_TRUE(!it.IsEnd());
  EXPECT_EQ(it.Key(), 4);
  RequireOk(t, "iter");
}

void TestDeleteSequential() {
  BPlusTree t(2);
  for (int i = 1; i <= 40; ++i) {
    t.Insert(i, i);
  }
  RequireOk(t, "before-del");
  for (int i = 1; i <= 40; ++i) {
    EXPECT_TRUE(t.Remove(i));
    RequireOk(t, "del-seq");
  }
  EXPECT_TRUE(t.Empty());
  EXPECT_TRUE(t.Insert(1, 1));
  EXPECT_EQ(*t.Get(1), 1);
}

void TestDeleteReverse() {
  BPlusTree t(2);
  for (int i = 1; i <= 40; ++i) {
    t.Insert(i, i);
  }
  for (int i = 40; i >= 1; --i) {
    EXPECT_TRUE(t.Remove(i));
    RequireOk(t, "del-rev");
  }
  EXPECT_TRUE(t.Empty());
}

void TestRootShrink() {
  BPlusTree t(2);
  for (int i = 1; i <= 8; ++i) {
    t.Insert(i, i);
  }
  const int h0 = t.Height();
  EXPECT_TRUE(h0 >= 2);
  // Delete most keys until height may shrink
  for (int i = 1; i <= 7; ++i) {
    t.Remove(i);
    RequireOk(t, "shrink");
  }
  EXPECT_EQ(*t.Get(8), 8);
  EXPECT_TRUE(t.Height() <= h0);
}

void TestMapFuzz() {
  BPlusTree t(3);
  std::map<int, int> ref;
  std::srand(42);
  for (int step = 0; step < 2000; ++step) {
    const int op = std::rand() % 3;
    const int k = std::rand() % 100;
    const int v = std::rand() % 1000;
    if (op == 0) {
      t.Insert(k, v);
      ref[k] = v;
    } else if (op == 1) {
      EXPECT_EQ(t.Remove(k), ref.erase(k) > 0);
    } else {
      auto a = t.Get(k);
      auto it = ref.find(k);
      if (it == ref.end()) {
        EXPECT_TRUE(!a.has_value());
      } else {
        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(*a, it->second);
      }
    }
    if (step % 50 == 0) {
      RequireOk(t, "fuzz");
      // iterator vs map
      auto it = t.Begin();
      for (const auto &kv : ref) {
        EXPECT_TRUE(!it.IsEnd());
        EXPECT_EQ(it.Key(), kv.first);
        EXPECT_EQ(it.Value(), kv.second);
        ++it;
      }
      EXPECT_TRUE(it.IsEnd());
    }
  }
  RequireOk(t, "fuzz-end");
}

int main() {
  TestEmpty();
  TestBasicInsertGetUpdate();
  TestSequentialInsertSplits();
  TestReverseInsert();
  TestIteratorOrdered();
  TestDeleteSequential();
  TestDeleteReverse();
  TestRootShrink();
  TestMapFuzz();

  if (g_failed == 0) {
    std::cout << "All p8 B+Tree tests passed.\n";
    return 0;
  }
  std::cerr << g_failed << " assertion(s) failed.\n";
  return 1;
}
