#include <cstdlib>
#include <iostream>
#include <string>

#include "../src/engine.h"

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

void TestPutGetRoundTrip() {
  p5::Engine eng;
  Expect(eng.Put("a", "1"), "put a");
  std::string out;
  Expect(eng.Get("a", &out) && out == "1", "get a == 1");
  Expect(eng.CheckInvariants().empty(), "invariants");
}

void TestOverwriteKeepsLatest() {
  p5::Engine eng;
  Expect(eng.Put("k", "v1"), "put v1");
  Expect(eng.Put("k", "v2"), "put v2");
  Expect(eng.Put("k", "v3"), "put v3");
  std::string out;
  Expect(eng.Get("k", &out) && out == "v3", "latest is v3");
  Expect(eng.TotalBytes() > eng.LiveBytes(), "space amplification before compact");
}

void TestDeleteTombstone() {
  p5::Engine eng;
  Expect(eng.Put("x", "1"), "put x");
  Expect(eng.Delete("x"), "delete x");
  std::string out;
  Expect(!eng.Get("x", &out), "get after delete fails");
  Expect(eng.CheckInvariants().empty(), "invariants after delete");
}

void TestCompactShrinksAndPreservesSemantics() {
  p5::Engine eng;
  Expect(eng.Put("a", "1"), "a1");
  Expect(eng.Put("a", "2"), "a2");
  Expect(eng.Put("b", "9"), "b");
  Expect(eng.Put("c", "1"), "c");
  Expect(eng.Delete("c"), "del c");
  Expect(eng.Put("a", "3"), "a3");

  const size_t before = eng.TotalBytes();
  const double amp_before = eng.Amplification();
  eng.Compact();
  const size_t after = eng.TotalBytes();

  Expect(after < before, "compact shrinks total bytes");
  Expect(eng.Amplification() <= amp_before, "amplification not worse");
  Expect(eng.Amplification() < 1.5, "amplification near 1 after compact");

  std::string out;
  Expect(eng.Get("a", &out) && out == "3", "a still 3");
  Expect(eng.Get("b", &out) && out == "9", "b still 9");
  Expect(!eng.Get("c", &out), "c still deleted");
  Expect(eng.CheckInvariants().empty(), "invariants after compact");
}

void TestAmplificationExperiment() {
  p5::Engine eng;
  for (int i = 0; i < 100; ++i) {
    eng.Put("same", "v" + std::to_string(i));
  }
  Expect(eng.Amplification() > 50.0, "100 overwrites -> high amplification");
  eng.Compact();
  Expect(eng.Amplification() < 1.5, "after compact amplification ~1");
  std::string out;
  Expect(eng.Get("same", &out) && out == "v99", "latest preserved");
}

void TestCorruptTailGraceful() {
  p5::Engine eng;
  Expect(eng.Put("ok", "yes"), "put ok");
  eng.AppendCorruptTail();

  // Indexed get still works for prior good record.
  std::string out;
  Expect(eng.Get("ok", &out) && out == "yes", "indexed get survives corrupt tail");

  // Compact should stop at corrupt tail and keep valid latest.
  eng.Compact();
  Expect(eng.Get("ok", &out) && out == "yes", "compact keeps valid data");
}

void TestScanVsIndexSteps() {
  p5::Engine eng;
  for (int i = 0; i < 50; ++i) {
    eng.Put("k" + std::to_string(i), "v");
  }
  eng.Put("target", "hit");

  size_t steps = 0;
  std::string out;
  Expect(eng.GetByScan("target", &out, &steps) && out == "hit", "scan finds target");
  Expect(steps == 51, "scan examines all records");
  Expect(eng.Get("target", &out) && out == "hit", "index get also works");
}

void TestEmptyKeyFails() {
  p5::Engine eng;
  Expect(!eng.Put("", "x"), "empty key put fails");
  Expect(!eng.Delete(""), "empty key delete fails");
}

}  // namespace

int main() {
  TestPutGetRoundTrip();
  TestOverwriteKeepsLatest();
  TestDeleteTombstone();
  TestCompactShrinksAndPreservesSemantics();
  TestAmplificationExperiment();
  TestCorruptTailGraceful();
  TestScanVsIndexSteps();
  TestEmptyKeyFails();

  if (g_failed > 0) {
    std::cerr << "\n" << g_failed << " assertion(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "\nAll p5_append_only tests passed.\n";
  return EXIT_SUCCESS;
}
