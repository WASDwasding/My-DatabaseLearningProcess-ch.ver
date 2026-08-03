# My-DatabaseLearningProcess-ch.ver

个人学习仓库：一边用教学模式讲义理解概念，一边把抽象落成可运行的最小实现代码，并持续对照 `bustub` 理解真实数据库内核模块。

## 仓库结构

```text
.../
├── Plan/            # 25 讲教学模式学习计划
│   ├── plan.md      # 课程目录总览
│   └── P_x/         # P1–P25 讲义
├── Code/            # 按讲义节奏推进的最小实现代码
└── bustub/          # bustub 源码与参考实现
```

| 目录 | 作用 |
|------|------|
| `Plan/` | 导论课拆成可跟学的中文讲义，并映射到 BusTub 模块 |
| `Code/` | 按讲义同步推进的最小实现练习场，重点是自己把抽象落成代码 |
| `bustub/` | 源码参考，用来对照真实模块边界、接口和工程写法 |

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

`Code/` 不是 `bustub/` 的拷贝，也不是把官方作业再抄一遍。

它更适合承担两类任务：

1. **讲义同步的最小实现**
   - 跟着 `Plan/P_x/P_n/Pn-3.md` 里的“最小实现视角”去写
   - 目标不是一次做完整数据库，而是每学完一讲，就补一个最小闭环
2. **纸面抽象到代码的过渡层**
   - 先在 `Code/` 里把 page、buffer pool、B+Tree、executor、lock manager、WAL/recovery 这些核心对象写到能跑
   - 再去对照 `bustub/`，看工业化教学代码是怎么处理边界、测试和接口拆分的

你可以把三部分理解成：

- `Plan/`：回答“这一讲到底在解决什么问题”
- `Code/`：回答“我能不能自己先做一个最小版本”
- `bustub/`：回答“成熟教学型实现是怎样组织这些模块的”

### 当前目录节奏

`Code/` 现在已经按讲义节奏补出了一套骨架目录。你可以直接在对应目录下继续写代码，而不是从零再想怎么组织：

```text
Code/
├── README.md
├── p3_page/
├── p4_buffer_pool/
├── p5_append_only/
├── p6_column_store/
├── p7_hash_index/
├── p8_bplustree/
├── p9_aux_indexes/
├── p10_latch_lab/
├── p11_sort_agg/
├── p12_join/
├── p13_executor/
├── p14_parallel_lab/
├── p15_optimizer_rules/
├── p16_optimizer_dp/
├── p17_serializability_lab/
├── p18_lock_manager/
├── p19_timestamp_cc/
├── p20_mvcc/
├── p21_wal/
├── p22_recovery/
├── p23_dist_design/
├── p24_consensus_2pc/
└── p25_system_design/
```

每个目录里都预留了 `src/`、`tests/`、`notes/` 和一个对应说明文件；具体起手顺序可以直接看 `Code/README.md`。

### 推荐推进方式

1. **先学一讲，再补一个最小实现**
   - 例如学完 `P3`，就在 `Code/` 里先实现最小页面布局和 `InsertTuple / GetTuple`
2. **一个目录只解决一个核心问题**
   - 不要还没把 page 写稳，就提前把 buffer pool、B+Tree、WAL 全揉在一起
3. **每个小模块都写最小测试**
   - 比如页满、空页、重复插入、删除后读取、pin 泄漏、split 后查找
4. **写完再回头对照 bustub**
   - 看自己漏掉了哪些状态、不变量和接口层次

### 与讲义的对应关系

| 讲义 | `Code/` 里的建议动作 |
|------|----------------------|
| `P1–P2` | 不急着写大模块，先写最小逻辑节点、表达式或查询树草图也可以 |
| `P3–P4` | 先做页布局、RID、buffer pool，是最重要的存储地基 |
| `P5–P6` | 可以先做 toy 级 append-only / column layout，小而清楚即可 |
| `P7–P10` | 开始做索引与并发数据结构，先单线程再考虑 latch |
| `P11–P14` | 做执行器骨架、排序/聚合/join，小心 blocking operator |
| `P15–P16` | 可先做规则重写 toy optimizer，不必一开始追完整 cost model |
| `P17–P22` | 做事务状态机、2PL、MVCC、WAL、recovery 时要优先保证状态正确 |
| `P23–P25` | 更适合画模块图、写设计说明，不一定急着上分布式代码 |

### 一个最实用的原则

`Code/` 的目标不是“比 bustub 更全”，而是“让你自己真正把这节课写通一次”。

如果一讲最后你能在 `Code/` 里留下：

- 一个最小实现
- 一组边界测试
- 一份自己写的模块说明

那这讲基本就算真的进脑子了。

## Code 目录与讲义联动规则

从现在开始，讲义里凡是进入下面这些场景：

- `最小实现视角`
- `最小实现顺序建议`
- `推荐最小实验`
- `推荐练习清单`

都会明确指向一个 `Code/...` 目录。也就是说：

- 讲义负责告诉你“这一讲该实现什么、先后顺序是什么”
- `Code/` 负责承接真正的代码、测试与说明
- `bustub/` 负责让你回头对照工程化版本

这样你在阅读 `Px-n` 时，不会再出现“知道要写代码，但不知道该把代码放哪”的断层。

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

建议顺序通常是：

1. 先用 `Plan/` 建立概念和边界  
2. 再在 `Code/` 里做自己的最小实现  
3. 最后去 `bustub/` 对照工程化版本  

这样更容易分清：

- 哪些是课程抽象
- 哪些是你自己的实现取舍
- 哪些是 bustub 为了工程完整性引入的复杂度

## 学习节奏建议

1. **先跟讲义建框架**：每讲先读 `-1`，能复述“解决什么问题”再进 `-2`  
2. **再写 `Code/` 最小实现**：读 `-3` 时同步写一个最小版本，先让主路径跑通  
3. **再对照 BusTub**：打开对应源码，盯输入、输出、不变量和接口拆分  
4. **用 `-4` 验收**：做练习和小测试，过关后再进下一讲  
5. **最小闭环优先**：先跑通，再补边界、测试、并发与恢复


## 参考

- [CMU 15-445/645 Database Systems](https://15445.courses.cs.cmu.edu/)
- [CMU-DB BusTub](https://github.com/cmu-db/bustub)

---

从 [`Plan/P_x/P_1/P1-1.md`](Plan/P_x/P_1/P1-1.md) 开始即可；当讲义里进入“最小实现视角 / 推荐最小实验 / 推荐练习清单”时，直接按文中给出的 `Code/...` 路径去落代码。
