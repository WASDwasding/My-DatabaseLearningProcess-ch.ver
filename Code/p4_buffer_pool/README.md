# p4_buffer_pool

- 对应讲义：`Plan/P_x/P_4/P4-3.md` 与 `P4-4.md`
- 主题：Buffer Pool、Page Table、Pin/Unpin、Replacer
- 状态：已完成阶段 1（单线程 Fetch/New/Unpin/Flush + LRU 驱逐）

## 目录

```text
p4_buffer_pool/
├── CMakeLists.txt
├── src/
│   ├── config.h
│   ├── page.h
│   ├── disk_manager.h
│   ├── lru_replacer.h
│   ├── buffer_pool_manager.h
│   └── buffer_pool_manager.cpp
├── tests/
│   └── test_buffer_pool.cpp
└── notes/
    └── design.md
```

## API

| 方法 | 行为 |
|------|------|
| `NewPage` | 分配 page_id，装入一个 frame，pin=1 |
| `FetchPage` | 命中则 pin++；未命中则读盘装入（可能 evict） |
| `UnpinPage` | pin--；若 pin=0 则进入 replacer |
| `FlushPage` | 把脏页写回 DiskManager，清 dirty |
| `DeletePage` | pin 必须为 0；释放 frame 并删磁盘页 |

## 构建与测试

```bash
cd Code/p4_buffer_pool
cmake -S . -B build && cmake --build build
./build/test_buffer_pool
```

## 完成标准对照

- [x] 最短主路径：New → 写 → Unpin(dirty) → Fetch → 内容一致
- [x] pool_size=1 触发 evict，且 dirty 页先 flush
- [x] 全 pinned 时 Fetch/New 失败
- [x] 双重 pin 需要双重 unpin
