# p4_buffer_pool 设计说明

## 目标

实现讲义阶段 1：单线程 BufferPoolManager。

- `NewPage` / `FetchPage` / `UnpinPage` / `FlushPage` / `DeletePage`
- 替换策略：简单 LRU（只管理 pin=0 的 frame）
- DiskManager：内存模拟磁盘（便于测试 flush/evict）

## 组件

```text
上层
  -> BufferPoolManager
       -> page_table: page_id -> frame_id
       -> frames/pages_[pool_size]
       -> free_list
       -> LruReplacer
  -> DiskManager::ReadPage / WritePage / AllocatePage
```

## 失败条件

| API | 失败 |
|-----|------|
| New/Fetch | 无 free frame 且无 evictable victim（全 pinned） |
| Unpin | page 不在池中，或 pin 已是 0 |
| Flush | page 不在池中 |
| Delete | page 仍被 pin |

## 不变量（CheckInvariants）

1. page_table 与 frame.page_id 双向一致  
2. pin_count >= 0  
3. free_list 中的 frame 必须 empty  
4. occupied frame 不在 free_list  

## 与 P3 的边界

- BPM 只搬运 `kPageSize` 字节，不解析 tuple。  
- 之后 TableHeap 应：`FetchPage` → 把 bytes 当 TablePage 用 → `UnpinPage(dirty?)`。

## 对照 bustub

| 本实现 | bustub |
|--------|--------|
| 单线程 | 后续有 latch / PageGuard |
| LRU | LRU-K / ARC 等 |
| 内存 DiskManager | 真实文件 + DiskScheduler |
