# P7 Design Notes — Extendible Hash（Stage 1）

## 与讲义的对应

| 讲义概念 | 代码 |
|---------|------|
| Global Depth `G` | `global_depth_`；`directory_.size() == 2^G` |
| Local Depth `L` | `MemBucket::local_depth` |
| Directory | `std::vector<MemBucket*> directory_` |
| Bucket / page of (key, RID) | `MemBucket::entries`；这里 value 用 `int` 代替 RID |
| `bucket_capacity` | 构造参数；满则 split |
| `index = low G bits of hash` | `IndexOf(key)` / `LowBits(Hash(key), G)` |
| Directory doubling | `DoubleDirectory()`：复制指针，`G++` |
| Split | `Split(bucket)`：新建 sibling，`L→L+1`，按新 bit 重分，改 directory 指针 |
| refs = `2^(G-L)` | `CheckInvariants()` I2 |

## Insert 流程（对齐 P7-2）

1. `idx = hash(key) & ((1<<G)-1)` → 找到 bucket
2. 若 key 已存在 → 原地更新 value（不占新槽）
3. 若未满 → append
4. 若满 → `Split`，然后重试（skew 时可能多次 split；`capacity=1` 必现）

## Split 细节（对齐讲义 5a–5e）

1. **5a** `L == G` → 先 `DoubleDirectory`
2. **5b** 两个桶的 `L` 都变成 `old_L + 1`
3. **5c** 新建 sibling bucket
4. **5d** 用 bit `old_L` 把 entries 分成两堆
5. **5e** 原指向该桶的 directory 槽：该 bit 为 0 → 旧桶，为 1 → sibling

## 为什么 Stage 1 用纯内存

讲义 P7-3：先把 **目录增长 + split + 不变式** 做对，再接到 page/BPM。  
本 lab 用 `new MemBucket` 代替 page，用 `int value` 代替 RID，方便单测和对照手算。

## 已知简化

- 无 merge / 目录收缩（Remove 只删 entry）
- 无磁盘、无 latch
- Hash 固定为 identity，便于调试；换成更好的 hash 只需改 `Hash()`
