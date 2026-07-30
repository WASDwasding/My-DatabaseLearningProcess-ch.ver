# P1–P4 全局思维构造图

> 用途：把前四课从“一堆名词”压成一条可运行的故事线。  
> 读法：先看总图，再看每课一句话，最后看与代码的对应。

---

## 0. 一句话总故事

```text
用户用 SQL 说想要什么（P2）
  -> 系统用关系代数理解查询语义（P1）
  -> 真正的数据按 page/tuple 躺在磁盘上（P3）
  -> 读数据时先经 buffer pool 在内存里缓存与换页（P4）
```

如果你现在懵，通常是因为把这四层混在同一层了。  
**它们回答的问题完全不同。**

---

## 1. 四层总图（先建立坐标系）

```text
┌─────────────────────────────────────────────────────────────┐
│  P2  用户接口层：SQL（声明式：说“要什么”，不说“怎么做”）      │
└─────────────────────────────┬───────────────────────────────┘
                              │ 解析 / 绑定
┌─────────────────────────────▼───────────────────────────────┐
│  P1  逻辑层：关系 + 关系代数（查询语义可组合、可改写）         │
└─────────────────────────────┬───────────────────────────────┘
                              │ 执行时需要读真实记录
┌─────────────────────────────▼───────────────────────────────┐
│  P4  内存管理层：Buffer Pool（page <-> frame，pin/evict）    │
└─────────────────────────────┬───────────────────────────────┘
                              │ 缺页 / 刷脏
┌─────────────────────────────▼───────────────────────────────┐
│  P3  存储层：Disk Page / Tuple / RID / Slot Array            │
└─────────────────────────────────────────────────────────────┘
```

记忆口诀：

| 课 | 一句话 |
|----|--------|
| P1 | 数据库在“算什么” |
| P2 | 用户怎么把意图说给系统 |
| P3 | 一行记录在磁盘页里怎么放、怎么定位 |
| P4 | 磁盘页怎样安全地进内存、离开内存 |

---

## 2. 每课内部骨架（不要背细节，背结构）

### P1 关系模型与代数
```text
数据抽象：Relation / Tuple / Attribute / Schema
查询抽象：Select / Project / Join / ...
目的：让查询可推理、可优化、与存储解耦
```

### P2 现代 SQL
```text
SQL = 用户侧声明式接口
内部路径（概念）：
  SQL -> Binder -> Planner(逻辑计划≈代数树) -> Optimizer -> Executor
你现在只要记住：SQL 语法背后对应逻辑运算，不是直接对应 page 读写。
```

### P3 存储：文件、页面与元组
```text
I/O 单位 = Page（常见 4KB）
页内组织 = Slotted Page
  Header + Slot Array + Free Space + Tuple Data
定位单位 = RID(page_id, slot_num)
最小 API = InsertTuple / GetTuple / MarkDelete
```

### P4 缓冲池
```text
Disk Page  ≠  Frame（内存槽位）
BPM 管：
  page_table: page_id -> frame_id
  pin_count : 谁正在用，不能踢走
  dirty     : 内存比磁盘新，踢走前要 flush
  replacer  : pin=0 时选谁被踢
最小 API = NewPage / FetchPage / UnpinPage / FlushPage
```

---

## 3. 把四课串成一次“点查”

假设：`SELECT * FROM T WHERE id = 42;`（有索引时）

```text
P2: SQL 声明“我要 id=42 那一行”
P1: 语义上是 选择/投影 一类逻辑运算
索引: 得到 RID{page=7, slot=3}          ← 后续课会细讲
P4: BPM.FetchPage(7)  -> 拿到内存里的 frame
P3: TablePage.GetTuple(RID) -> 读出 tuple bytes
P4: UnpinPage(7)            -> 用完允许被替换
```

没有索引时（全表扫）也一样：只是从“按 RID 取”变成“逐页 Fetch + 扫 slot”。

---

## 4. P3 与 P4 的边界（最容易懵的地方）

```text
TablePage（P3）只回答：这一页里面的字节怎么解释成 tuple
BufferPool（P4）只回答：这一页现在在不在内存、能不能踢走、脏了怎么办

错误混层例子：
  ✗ 在 TablePage 里直接 fopen 读磁盘
  ✗ 在 BPM 里解析 tuple schema
```

正确协作：

```text
上层
  -> BPM::FetchPage(page_id)     // 拿到 Page* / bytes
  -> reinterpret 为 TablePage    // 做 Insert/Get/MarkDelete
  -> BPM::UnpinPage(page_id, dirty?)
```

---

## 5. P4 状态机（学 P4 时盯这一张）

```text
[Free frame]
   --New/Fetch 装入--> [Occupied, pin>=1, clean/dirty]
   --Unpin 到 pin=0--> [Occupied, pin=0, evictable]
   --Evict(若 dirty 先 Flush)--> [Free 或装新 page]
```

硬规则（不变量）：

1. `pin_count > 0` ⇒ 不可 evict  
2. dirty 且即将 evict ⇒ 必须先 flush  
3. page_table 与 frame 内容必须双向一致  
4. 忘记 unpin ⇒ 池子假满，Victim 失败  

---

## 6. 和你 Code/ 的对应

| 讲义 | 代码目录 | 你要留下的最小闭环 |
|------|----------|-------------------|
| P3 | `Code/p3_page` | 单页 Insert/Get/MarkDelete |
| P4 | `Code/p4_buffer_pool` | 单线程 New/Fetch/Unpin/Flush + 能 evict |

学习节奏建议：

- 学概念时：先能口述上面总图  
- 写代码时：每课只做一个最小闭环  
- 第一轮实验：可与学习并行，但先做 micro-benchmark，别一上来做论文级评测  

---

## 7. 自测：你是否真的不懵了

闭卷回答这 6 句：

1. P1 和 P3 分别解决什么问题？  
2. 为什么需要 RID 而不是直接存 byte offset？  
3. page 和 frame 差在哪？  
4. Fetch 未命中时，BPM 大概做哪几步？  
5. 为什么 pin>0 不能 evict？  
6. TablePage 改了数据后，谁负责 dirty / flush？  

答得出 5/6，就可以继续 P5；答不出，回对应小节补，不要硬往下刷。
