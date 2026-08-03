# p5_append_only 设计说明

## 目标

实现讲义阶段 0–1：

1. 最小 append-only KV：`Put` / `Get` / `Delete`
2. full compaction：重写日志，丢掉过期版本与 tombstone

未做：多 segment seal、MemTable flush、L0 多层、真实 WAL（留待后续）。

## 日志记录格式

```text
key_len(u32) | key | val_len(u32) | value | type(u8) | seq(u64)
type: Put=1, Delete=2
```

- 存储介质：内存 `vector<char>` 模拟单个 active segment（便于测试）。
- 索引：`key -> {offset, seq, type}`，Get O(1) 定位最新记录。

## Compact 规则

1. 顺序扫描整段日志（遇损坏尾部则停止）
2. 每个 key 保留 seq 最大的记录
3. 若最大是 Delete，则丢弃该 key（教学版 full compact 后不再保留 tombstone）
4. 写出新日志并重建索引（内存中 atomic swap）

## 指标

- `total_bytes`：日志总大小
- `live_bytes`：索引中存活 PUT 编码后大小之和
- `amplification = total / live`

同一 key 覆盖 100 次不 compact，amplification 会很大；compact 后应接近 1。

## 失败条件

| API | 失败 |
|-----|------|
| Put/Delete | key 为空 |
| Get | 无此 key / 最新为 tombstone / offset 损坏 |

## 与页式 / WAL 边界

- 本引擎的日志是**主数据**，不是 WAL。
- bustub 页式：主数据是 page；WAL 另文件做恢复。
- 若以后加 MemTable，才可能再加一份 WAL 保护未 flush 状态。
