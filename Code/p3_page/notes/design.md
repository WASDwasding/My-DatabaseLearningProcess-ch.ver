# p3_page 设计说明

## 目标

实现讲义阶段 1：定长/变长字节均可的 **单页 slotted page**，支持：

- `InsertTuple`
- `GetTuple`
- `MarkDelete`

不接 BufferPool，不用多页 TableHeap。

## 页布局

```text
+------------------+------------------------+-------------+------------------+
| Header (12B)     | Slot Array (6B * n)    | Free Space  | Tuple Data       |
|                  | grows forward -------> |             | <------- grows   |
+------------------+------------------------+-------------+------------------+
                                              ^
                                         free_space_ptr
```

Header 字段：

| 字段 | 含义 |
|------|------|
| `next_page_id` | 堆表下一页（本阶段仅占位） |
| `slot_count` | slot 项数（含已删除） |
| `deleted_count` | 已 MarkDelete 的数量 |
| `free_space_ptr` | tuple data 区最低偏移 |

Slot 项：`(offset, length, flags)`，`flags bit0 = deleted`。

RID = `(page_id, slot_num)`，`slot_num` 是下标，不是 byte offset。

## 失败条件

### InsertTuple → false
- tuple 长度为 0
- tuple 大到单页不可能放下（header + 1 slot + data）
- 没有可复用的等长 deleted slot，且 free space 不够

### GetTuple → false
- `page_id` 不匹配
- `slot_num` 越界
- slot 已删除（I6）
- offset/length 破坏 I1（越界 / 与 header、slot array 重叠）

### MarkDelete → false
- `page_id` 不匹配 / slot 越界
- slot 本已删除

失败路径保证：不修改 page（Insert 失败时），或仅在合法路径更新 metadata。

## 不变量（CheckInvariants）

- I1：有效 slot 的 `[offset, offset+len)` 在页内且落在 data 区
- I2：有效 tuple 区间不重叠
- I3：free space 不与有效 tuple 重叠（由 `free_space_ptr` 与 slot array end 保证）
- I4：`deleted_count` 与实际 deleted slot 数一致
- I5：`free_space_ptr` 合法，且 slot array 不侵入 data 区
- I6：deleted slot 的 GetTuple 必须失败

## 实现取舍

1. **MarkDelete 不 compaction**：删除后 free space 不立即合并；等长插入可复用 deleted slot。
2. **Tuple 是 opaque bytes**：不做 schema 解析，聚焦 page 布局。
3. **单页内存数组**：模拟 4KB page，为 P4 BufferPool 留接口边界。

## 对照 bustub

| 本实现 | bustub |
|--------|--------|
| `TablePage` | `storage/page/table_page.h` |
| `RID` | `common/rid.h` |
| `Tuple` 原始 bytes | `storage/table/tuple.h`（更完整，含 schema） |
| 无 TableHeap | `TableHeap` 管多页 + BPM |

下一步（P4）：把 `char[4096]` 换成 BufferPoolManager::FetchPage。
