# p3_page

- 对应讲义：`Plan/P_x/P_3/P3-3.md` 与 `P3-4.md`
- 主题：页面布局、Tuple、RID、slot array
- 状态：已完成阶段 1（定长/原始 bytes + 单页）最小闭环

## 目录

```text
p3_page/
├── CMakeLists.txt
├── src/
│   ├── rid.h
│   ├── tuple.h
│   ├── table_page.h
│   └── table_page.cpp
├── tests/
│   └── test_table_page.cpp
└── notes/
    └── design.md
```

## API

| 方法 | 行为 |
|------|------|
| `InsertTuple` | 写入 tuple；优先复用等长 deleted slot；成功写出 RID |
| `GetTuple` | 读存活记录；删除/越界/损坏返回 false |
| `MarkDelete` | 逻辑删除；不 compaction |
| `CheckInvariants` | 校验 I1–I6 |

## 构建与测试

```bash
cd Code/p3_page
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
# 或直接：
./build/test_table_page
```

## 完成标准对照

- [x] 最短主路径：Insert → Get 内容一致
- [x] 最小测试：满页、删除后再读、slot 复用、corrupt offset、OOB
- [x] 不变量与失败条件：见 `notes/design.md`
