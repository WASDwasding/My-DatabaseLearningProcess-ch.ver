 # p6_column_store 设计说明

## 目标

对应讲义 Level A/B：

1. 同一数据的行存 / 列存布局
2. 用 `bytes_read` 量化少列扫描收益
3. early vs late materialization
4. dictionary + bit packing 压缩 toy
5. zone map 跳过 chunk

不做：真实向量化 SIMD、Parquet、UPDATE、接 bustub executor。

## 模块

### RowTable
- 行宽连续存放：`row[r*ncols + c]`
- `SumWhere`：每行计费 `ncols * 8` 字节（即使只用 2 列）
- 对应：行存物理上读整行

### ColumnTable
- 每列一个 `vector<int64_t>`
- `SumWhereEarly`：读满 filter 列 + sum 列
- `SumWhereLate`：先扫 filter 得 rowids，再 gather sum（只计选中行字节）
- `FilterWithZoneMaps`：chunk 级 min/max，不可能命中则跳过 I/O

### encoding.h
- `DictionaryColumn`：低基数 string → dict + id
- `BitPackedColumn`：小范围非负整数按 bit 打包

## 不变量 / 注意

- 各列 `size()` 必须相等（同一逻辑行对齐）
- late 的 rowid 必须来自同一 filter 扫描结果
- `bytes_read` 是教学模型，不是真实 OS I/O 计数

## 和概念分层的对应

```text
列存布局     -> ColumnTable
压缩         -> DictionaryColumn / BitPackedColumn
延迟物化     -> SumWhereLate
zone map     -> FilterWithZoneMaps
向量化执行   -> 本 toy 未实现（仍可用循环模拟批处理思想）
```
