# P8 B+Tree（单线程 · 纯内存）

对应讲义：`Plan/P_x/P_8/`（P8-1 模型、P8-2 分裂/合并、P8-3/P8-4 实现与验收）。

## 做什么

内存版 **B+Tree**（一节点 ≈ 一页，无 Buffer Pool）：

- `Insert` / `Get` / `Remove`
- 叶分裂（复制分隔 key 进父 + 修 `next` 链表）
- 内节点分裂（中间 key **上推**）
- 根分裂 → 树高 +1；根只剩一个 child → 降高
- Delete underflow：先 redistribute，再 merge
- `Begin` / `Begin(key)` / `++` 沿叶子链表扫描
- `CheckInvariants` / `CheckLeafChain` / `DebugString`

**刻意不做**：真实 page 布局、BPM、PageGuard、并发 latch（P10）。

## 构建 & 测试

```bash
cmake -S Code/p8_bplustree -B Code/p8_bplustree/build
cmake --build Code/p8_bplustree/build
ctest --test-dir Code/p8_bplustree/build --output-on-failure
```

## 目录

```
src/b_plus_tree.{h,cpp}
tests/test_b_plus_tree.cpp
notes/design.md
```
