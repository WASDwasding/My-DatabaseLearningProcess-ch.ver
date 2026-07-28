# My-DatabaseLearningProcess-ch.ver

个人学习仓库：一边用教学模式讲义把概念吃透，一边把抽象落成可运行的最小实现代码。

## 仓库结构

```text
.../
├── Plan/          # 25 讲教学模式学习计划
│   ├── plan.md         # 课程目录总览
│   ├── P_x/            # P1–P25 讲义
├── bustub/        # bustub 源码       
└── Code/          # 讲义同步最小实现代码
```

| 目录 | 作用 |
|------|------|
| `Plan/` | 导论课拆成可跟学的中文讲义，并映射到 BusTub 模块 |
| `Code/` | 实现缓冲池、索引、执行器、并发、恢复等内核组件的练习场 |
| `bustub/` | 源码参考 

## Plan 怎么用

每讲对应一个目录 `Plan/P_x/P_n/`：

```text
P_n/
├── Pn.md      # 入口（只说明从哪读起）
├── Pn-1.md    # 为什么存在 / 问题定义 / 术语入门
├── Pn-2.md    # 核心机制深讲
├── Pn-3.md    # 边界、陷阱、bustub 映射、最小实现
└── Pn-4.md    # 练习、验收、进入下一讲
```

建议顺序：

1. 打开 `Plan/plan.md` 看全课地图  
2. 从 `Plan/P_x/P_1/P1-1.md` 开始，按 `Pn-1 → Pn-4` 学完一讲再进下一讲  
3. 每段末尾有小练习与自测；过关后再继续，不要只“看完觉得懂了”

## Code 怎么用

讲义中会有进行最小实现的实践部分，可在这里参考

## 课程地图（P1–P25）

| 板块 | 讲次 | 主题 |
|------|------|------|
| 逻辑与语言 | P1–P2 | 关系模型与代数、现代 SQL |
| 存储引擎 | P3–P6 | 页与元组、缓冲池、日志结构存储、列存 |
| 索引与并发结构 | P7–P10 | 哈希表、B+树、扩展索引、Latch |
| 查询执行 | P11–P14 | 排序与聚合、Join、执行器模型、并行执行 |
| 查询优化 | P15–P16 | 规则重写与代价、Join Order / Access Path |
| 事务与恢复 | P17–P22 | 并发理论、2PL、时间戳、MVCC、WAL、ARIES |
| 分布式与收口 | P23–P25 | 分布式基础、共识与 2PC、全景回顾 |

完整标题列表见 [`Plan/plan.md`](Plan/plan.md)。

## 和 BusTub 的关系

讲义不是只背名词，而是把每节课接到 BusTub 的真实模块上，例如：

- P3 / P4 → `TableHeap`、`TablePage`、`BufferPoolManager`
- P7 / P8 / P10 → Extendible Hash、B+Tree、PageGuard / Latch
- P11–P13 → Aggregation / Join / Volcano 风格 Executor
- P15–P16 → Optimizer rules
- P18–P22 → LockManager、MVCC / UndoLog、WAL、Recovery

## 学习节奏建议

1. **先跟讲义建框架**：每讲先读 `-1`，能复述“解决什么问题”再进 `-2`  
2. **再对照 BusTub**：读 `-3` 时打开对应源码，盯输入、输出、不变量  
3. **用 `-4` 验收**：做练习和小测试，过关后再进下一讲  
4. **最小闭环优先**：先让一条主路径跑通，再补边界与并发 / 恢复


## 参考

- [CMU 15-445/645 Database Systems](https://15445.courses.cs.cmu.edu/)
- [CMU-DB BusTub](https://github.com/cmu-db/bustub)

---

从 [`Plan/P_x/P_1/P1-1.md`](Plan/P_x/P_1/P1-1.md) 开始即可。
