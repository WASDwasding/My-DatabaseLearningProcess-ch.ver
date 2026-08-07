# P7 Extendible Hash Index（Stage 1：纯内存）

对应讲义：`Plan/P_x/P_7/`（P7-1 术语、P7-2 Insert/Split、P7-3 Stage 1）。

## 做什么

实现 **可扩展哈希（Extendible Hashing）** 的内存版：

- Directory：大小 `2^G`，每格指向一个 bucket
- Bucket：固定容量 `bucket_capacity`，存 `(key, value)`，带 `local_depth L`
- `Insert` / `Get` / `Remove`
- Bucket 满了：`Split`；若 `L == G` 先 `DoubleDirectory`
- Identity hash：`hash(key) = key`（方便手算、对照讲义）
- `CheckInvariants()`：验证讲义里的不变式（refs = `2^(G-L)` 等）

**刻意不做**（留给后续 stage）：真实 page / Buffer Pool / 持久化 / 并发。

## 构建 & 测试

```bash
cd Code/p7_hash_index
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
# 或
./build/test_extendible_hash
```

## 目录

```
src/extendible_hash.{h,cpp}   # 核心实现
tests/test_extendible_hash.cpp
notes/design.md               # 与讲义的对应关系
```
