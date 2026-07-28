# Code

这个目录用于承接 `Plan/` 讲义中的代码练习。原则不是一次写完整数据库，而是按课程节奏把每一讲里最关键的抽象先落成一个最小闭环。

## 使用原则

1. 先学 `Plan/`，再在这里写最小实现。
2. 一个目录尽量只解决一个核心问题。
3. 每个目录至少包含：最小实现、边界测试、模块说明。
4. 写完后再回头对照 `bustub/`。

## 目录索引

- `Code/p3_page/`：P3 页面布局、Tuple、RID、slot array
- `Code/p4_buffer_pool/`：P4 Buffer Pool、Page Table、Pin/Unpin、Replacer
- `Code/p5_append_only/`：P5 append-only 存储与 compaction toy 实验
- `Code/p6_column_store/`：P6 列存布局、压缩、late materialization toy 实验
- `Code/p7_hash_index/`：P7 extendible hashing 与桶/目录页实验
- `Code/p8_bplustree/`：P8 单线程 B+Tree 与 iterator
- `Code/p9_aux_indexes/`：P9 倒排、Bloom、跳表、向量索引的 toy 实验
- `Code/p10_latch_lab/`：P10 latch 封装、页保护与并发结构实验
- `Code/p11_sort_agg/`：P11 排序、外部排序、聚合状态实验
- `Code/p12_join/`：P12 NLJ / Hash Join / SMJ 教学实现
- `Code/p13_executor/`：P13 Volcano 执行器骨架与表达式求值
- `Code/p14_parallel_lab/`：P14 exchange、分区、并行聚合 toy 实验
- `Code/p15_optimizer_rules/`：P15 规则重写与粗糙基数估计
- `Code/p16_optimizer_dp/`：P16 join order、DP 搜索与 access path 选择
- `Code/p17_serializability_lab/`：P17 冲突图、调度分析、隔离级别实验工具
- `Code/p18_lock_manager/`：P18 2PL、锁表、等待队列与 deadlock 实验
- `Code/p19_timestamp_cc/`：P19 时间戳排序与 Thomas Write Rule 实验
- `Code/p20_mvcc/`：P20 版本链、可见性、快照与 GC 实验
- `Code/p21_wal/`：P21 WAL、LSN、log buffer 与 flush 实验
- `Code/p22_recovery/`：P22 analysis / redo / undo / CLR 恢复实验
- `Code/p23_dist_design/`：P23 分片、复制、路由与 distributed design toy
- `Code/p24_consensus_2pc/`：P24 Raft / 2PC / leader 切换教学实验
- `Code/p25_system_design/`：P25 模块图、路线图与综合设计文档
