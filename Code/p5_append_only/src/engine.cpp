#include "engine.h"

#include <sstream>

namespace p5 {

auto Engine::AppendRecord(const Record &rec) -> size_t {
  const size_t offset = log_.size();
  auto bytes = EncodeRecord(rec);
  log_.insert(log_.end(), bytes.begin(), bytes.end());
  return offset;
}

auto Engine::ReadAt(size_t offset, Record *out) const -> bool {
  size_t n = 0;
  return DecodeRecord(log_.data(), log_.size(), offset, out, &n);
}

auto Engine::Put(const std::string &key, const std::string &value) -> bool {
  if (key.empty()) {
    return false;
  }
  Record rec;
  rec.key = key;
  rec.value = value;
  rec.type = RecordType::kPut;
  rec.seq = next_seq_++;

  const size_t offset = AppendRecord(rec);
  index_[key] = IndexEntry{offset, rec.seq, RecordType::kPut};
  return true;
}

auto Engine::Delete(const std::string &key) -> bool {
  if (key.empty()) {
    return false;
  }
  Record rec;
  rec.key = key;
  rec.value.clear();
  rec.type = RecordType::kDelete;
  rec.seq = next_seq_++;

  const size_t offset = AppendRecord(rec);
  index_[key] = IndexEntry{offset, rec.seq, RecordType::kDelete};
  return true;
}

auto Engine::Get(const std::string &key, std::string *out) const -> bool {
  auto it = index_.find(key);
  if (it == index_.end()) {
    return false;
  }
  if (it->second.type == RecordType::kDelete) {
    return false;
  }
  Record rec;
  if (!ReadAt(it->second.offset, &rec)) {
    return false;
  }
  if (rec.type != RecordType::kPut || rec.key != key) {
    return false;
  }
  if (out != nullptr) {
    *out = rec.value;
  }
  return true;
}

auto Engine::LiveBytes() const -> size_t {
  size_t live = 0;
  for (const auto &[key, entry] : index_) {
    if (entry.type == RecordType::kDelete) {
      continue;
    }
    Record rec;
    if (!ReadAt(entry.offset, &rec)) {
      continue;
    }
    live += EncodeRecord(rec).size();
  }
  return live;
}

auto Engine::Amplification() const -> double {
  const size_t live = LiveBytes();
  if (live == 0) {
    return TotalBytes() == 0 ? 1.0 : static_cast<double>(TotalBytes());
  }
  return static_cast<double>(TotalBytes()) / static_cast<double>(live);
}

void Engine::Compact() {
  // Scan whole log; keep max-seq record per key.
  std::unordered_map<std::string, Record> latest;
  size_t offset = 0;
  while (offset < log_.size()) {
    Record rec;
    size_t n = 0;
    if (!DecodeRecord(log_.data(), log_.size(), offset, &rec, &n)) {
      // Truncate corrupt tail: stop scanning.
      break;
    }
    offset += n;
    auto it = latest.find(rec.key);
    if (it == latest.end() || rec.seq >= it->second.seq) {
      latest[rec.key] = rec;
    }
  }

  std::vector<char> new_log;
  std::unordered_map<std::string, IndexEntry> new_index;
  uint64_t max_seq = 0;

  for (auto &[key, rec] : latest) {
    if (rec.type == RecordType::kDelete) {
      // Drop tombstone and key entirely after full compaction.
      continue;
    }
    const size_t new_offset = new_log.size();
    auto bytes = EncodeRecord(rec);
    new_log.insert(new_log.end(), bytes.begin(), bytes.end());
    new_index[key] = IndexEntry{new_offset, rec.seq, RecordType::kPut};
    if (rec.seq > max_seq) {
      max_seq = rec.seq;
    }
  }

  // Atomic swap of in-memory segment.
  log_.swap(new_log);
  index_.swap(new_index);
  if (max_seq + 1 > next_seq_) {
    next_seq_ = max_seq + 1;
  }
}

auto Engine::GetByScan(const std::string &key, std::string *out, size_t *steps) const -> bool {
  Record best;
  bool found = false;
  size_t offset = 0;
  size_t count = 0;
  while (offset < log_.size()) {
    Record rec;
    size_t n = 0;
    if (!DecodeRecord(log_.data(), log_.size(), offset, &rec, &n)) {
      break;
    }
    offset += n;
    ++count;
    if (rec.key != key) {
      continue;
    }
    if (!found || rec.seq >= best.seq) {
      best = rec;
      found = true;
    }
  }
  if (steps != nullptr) {
    *steps = count;
  }
  if (!found || best.type == RecordType::kDelete) {
    return false;
  }
  if (out != nullptr) {
    *out = best.value;
  }
  return true;
}

void Engine::AppendCorruptTail() {
  log_.push_back('\xff');
  log_.push_back('\xfe');
}

auto Engine::CheckInvariants() const -> std::string {
  std::ostringstream err;
  for (const auto &[key, entry] : index_) {
    Record rec;
    if (!ReadAt(entry.offset, &rec)) {
      err << "index offset unreadable for key=" << key;
      return err.str();
    }
    if (rec.key != key) {
      err << "index key mismatch";
      return err.str();
    }
    if (rec.seq != entry.seq || rec.type != entry.type) {
      err << "index meta mismatch for key=" << key;
      return err.str();
    }
  }
  return {};
}

}  // namespace p5
