#include "record.h"

#include <cstring>

namespace p5 {

namespace {

void AppendU32(std::vector<char> *buf, uint32_t v) {
  char raw[4];
  std::memcpy(raw, &v, 4);
  buf->insert(buf->end(), raw, raw + 4);
}

void AppendU64(std::vector<char> *buf, uint64_t v) {
  char raw[8];
  std::memcpy(raw, &v, 8);
  buf->insert(buf->end(), raw, raw + 8);
}

auto ReadU32(const char *data, size_t size, size_t offset, uint32_t *out) -> bool {
  if (offset + 4 > size) {
    return false;
  }
  std::memcpy(out, data + offset, 4);
  return true;
}

auto ReadU64(const char *data, size_t size, size_t offset, uint64_t *out) -> bool {
  if (offset + 8 > size) {
    return false;
  }
  std::memcpy(out, data + offset, 8);
  return true;
}

}  // namespace

auto EncodeRecord(const Record &rec) -> std::vector<char> {
  std::vector<char> buf;
  AppendU32(&buf, static_cast<uint32_t>(rec.key.size()));
  buf.insert(buf.end(), rec.key.begin(), rec.key.end());
  AppendU32(&buf, static_cast<uint32_t>(rec.value.size()));
  buf.insert(buf.end(), rec.value.begin(), rec.value.end());
  buf.push_back(static_cast<char>(rec.type));
  AppendU64(&buf, rec.seq);
  return buf;
}

auto DecodeRecord(const char *data, size_t size, size_t offset, Record *out, size_t *bytes_read)
    -> bool {
  size_t cursor = offset;
  uint32_t key_len = 0;
  if (!ReadU32(data, size, cursor, &key_len)) {
    return false;
  }
  cursor += 4;
  if (cursor + key_len > size) {
    return false;
  }
  std::string key(data + cursor, key_len);
  cursor += key_len;

  uint32_t val_len = 0;
  if (!ReadU32(data, size, cursor, &val_len)) {
    return false;
  }
  cursor += 4;
  if (cursor + val_len > size) {
    return false;
  }
  std::string value(data + cursor, val_len);
  cursor += val_len;

  if (cursor + 1 > size) {
    return false;
  }
  auto type = static_cast<RecordType>(static_cast<uint8_t>(data[cursor]));
  if (type != RecordType::kPut && type != RecordType::kDelete) {
    return false;
  }
  cursor += 1;

  uint64_t seq = 0;
  if (!ReadU64(data, size, cursor, &seq)) {
    return false;
  }
  cursor += 8;

  out->key = std::move(key);
  out->value = std::move(value);
  out->type = type;
  out->seq = seq;
  if (bytes_read != nullptr) {
    *bytes_read = cursor - offset;
  }
  return true;
}

}  // namespace p5
