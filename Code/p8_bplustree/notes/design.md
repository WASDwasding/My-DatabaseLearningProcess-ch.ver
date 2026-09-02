# P8 Design Notes — B+Tree（内存 Stage）

## 与讲义对应

| 讲义 | 代码 |
|------|------|
| 节点 = page | `LeafNode` / `InternalNode`（`Node*`） |
| 叶存 (key,rid) + next | `keys`/`values` + `next` |
| 内节点分隔 key + children | `keys` + `children`，`\|C\|=\|K\|+1` |
| 叶分裂复制分隔 key | `SplitLeaf`：`sep = right.min` 插入父 |
| 内分裂上推 | `SplitInternal`：中间 key 搬走进父 |
| 根分裂长高 | path 空或 path_index==0 时新建 root |
| redistribute / merge | `RedistributeLeaf/Internal`，`MergeLeaf/Internal` |
| Iterator 沿叶链 | `Begin` 找最左叶，`++` 跨 `next` |
| CheckLeafChain | `CheckLeafChain()` |

## 分隔 key 查找规则

内节点：找第一个满足 `search_key < keys[i]` 的 `i`，走 `children[i]`；否则走最后一个 child。

## 实现顺序（对照 P8-3）

1. Lookup + 空树 Insert  
2. Insert + Split（叶/内/根）  
3. Iterator  
4. Delete + redistribute/merge + 根降级  

## 已知简化

- value 用 `int` 代替 RID  
- 无磁盘 / BPM / latch  
- 父分隔 key 在删叶最小值时做了局部更新；Lookup 正确性主要靠范围不变量  
