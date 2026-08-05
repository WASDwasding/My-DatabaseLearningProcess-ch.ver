# p6_column_store

- 对应讲义：`Plan/P_x/P_6/P6-3.md` 与 `P6-4.md`
- 主题：列存布局、压缩、late materialization toy 实验
- 状态：已完成 Level A–B 最小闭环（行/列 bytes 对比 + dictionary/bitpack + zone map）

## 目录

```text
p6_column_store/
├── CMakeLists.txt
├── src/
│   ├── row_table.h       # 行存 + bytes_read
│   ├── column_table.h    # 列存 early/late + zone map
│   └── encoding.h        # dictionary / bit packing
├── tests/
│   └── test_column_store.cpp
└── notes/
    └── design.md
```

## 实验在验证什么

| 实验 | 对应讲义 |
|------|----------|
| 宽表行存 vs 列存 `bytes_read` | 少列扫描的 I/O 优势 |
| early vs late materialization | 高选择性时 late 更省 |
| dictionary / bitpack | 存储编码压缩 |
| zone map skip | min/max 跳过无关 chunk |

## 构建与测试

```bash
cd Code/p6_column_store
cmake -S . -B build && cmake --build build
./build/test_column_store
```

上传时不要带 `build/`。
