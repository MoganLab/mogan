# [moebius] moebius 模块性能优化

每次挑选 moebius 里的一个函数做性能优化，配套单元测试与 nanobench 基准。
参考模板：[dddd.md](dddd.md)

## 1 相关文档
- [dddd.md](dddd.md) - 任务文档模板

## 2 任务相关的代码文件
- `moebius/` - 被优化模块（Data/Kernel/Scheme/moebius）
- `moebius/tests/**_test.cpp` - 单元测试（xmake 自动发现，目标名 `moebius_tests_<name>`）
- `moebius/bench/**_bench.cpp` - nanobench 基准（xmake 自动发现，目标名 `<name>`）

## 3 如何测试（只构建 moebius 模块）

### 3.1 确定性测试（单元测试）
```bash
xmake test moebius_tests/<name>     # 如 moebius_tests/tree_traverse_test
```

### 3.2 基准测试
```bash
xmake b <name>_bench && xmake r <name>_bench   # 如 tree_traverse_bench
```

## 4 如何提交
```bash
gf fmt --changed-since=main
git commit -m "[moebius] <函数> <优化简述>"
```

## 5 已完成的优化记录

### 5.1 tree_utf8_to_herk / tree_herk_to_utf8（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_traverse.cpp`
- **What**: 原子节点加 ASCII 恒等快路径。herk 双向映射表中字节 32..126 除
  0x60（反引号）外均恒等；herk->utf8 方向额外排除 `<#` 十六进制转义。
  命中快路径时直接返回 `tree (t->label)`（字符串引用共享），跳过 lolly
  逐字符解码/重排循环。
- **Why**: 文档加载/保存热路径（`input.cpp` 打开文件、`edit_complete.cpp`
  补全、`connection.cpp` 链接），典型文档绝大多数原子是纯 ASCII，
  原实现一律走逐字符转换。
- **How**: 新增 `utf8_herk_identity` / `herk_utf8_identity` 快速扫描；
  复合节点递归不变，RAW_DATA 短路不变。
- **结果**: 1000 段×10 词文档（90% ASCII）utf8->herk 7.2ms→3.0ms（2.4x），
  herk->utf8 4.9ms→3.3ms（1.5x）；纯 ASCII 文档 6.1ms→2.6ms（2.3x）。
- **测试**: `moebius/tests/Data/Tree/tree_traverse_test.cpp`（9 用例：
  恒等、反引号例外、非 ASCII 转换、往返、`<#` 转义、孤立 `<`、RAW_DATA 保持）
- **基准**: `moebius/bench/Data/Tree/tree_traverse_bench.cpp`（含优化前
  实现的同二进制 A/B 对比）

### 5.2 correct_node 消除标签字符串绕行（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_modify.cpp`、`moebius/moebius/drd/drd_info.{hpp,cpp}`
- **What**: `correct_node` 原来调 `the_drd->contains (as_string (L (t)))`，
  每个节点一次字符串名转换 + existing_tree_label/as_tree_label 两次哈希查表。
  新增 `drd_info::contains (tree_label)` 重载（直接 `info->contains (l)`），
  热路径改走标签编号。
- **Why**: `correct_downwards`/`correct_node` 对每个节点调用（粘贴、
  scheme 侧修正等编辑路径）；与已合入的 1228 系列"缓存标签编号，
  消除热路径字符串比较"同类。
- **How**: drd_info 经 CONCRETE 宏自动获得转发包装；树节点上 `L(t)`
  已是 interned 标签，`as_tree_label(as_string(l)) == l`，语义等价。
- **结果**: 预校正树全树 sweep x100：10.1ms→5.5ms（**1.84x**）；
  含 `copy(doc)` 的端到端场景差异被稀释（~1.6ms 持平）。
- **测试**: `moebius/tests/Data/Tree/tree_modify_test.cpp`（9 用例：
  contains 两个重载一致性、arity 修正、concat 原子合并、递归下探、
  simplify_concat/document 展平）
- **基准**: `moebius/bench/Data/Tree/tree_modify_bench.cpp`（含优化前
  实现的同二进制 A/B 对比；隔离 sweep 场景避免 copy 稀释）

### 5.3 scaling/an_scaling 直变换消除中间 point 临时（2026-08-20）
- **文件**: `moebius/Kernel/Types/frame.cpp`
- **What**: `scaling_rep`/`an_scaling_rep` 的 `direct_transform`/
  `inverse_transform` 原写作 `shift + magnify * p`，每次调用产生两个
  中间 point（`array<double>`，各一次堆分配）。改为逐分量直写唯一
  结果数组。
- **Why**: scaling 是图形系统的标准坐标系框架（设备变换），曲线/边框
  的每个采样点渲染时都要过 `operator()`；`frame::enclose` 求包围盒
  每个矩形 4 条边 × 采样点同样密集调用。
- **How**: 循环内直接 `q[i]= shift[i] + magnify * p[i]`（按轴版用
  `magnify[i]`），任意维度通用。linear_2D 的 2x2 展开尝试过无收益
  （通用矩阵乘本就单次分配），已回退。
- **结果**: x1024 点循环：scaling 39.4µs→17.1µs（**2.3x**），
  an_scaling 53.7µs→17.6µs（**3.0x**）。
- **测试**: `moebius/tests/Kernel/Types/frame_test.cpp` 追加 3 用例
  （3 分量 scaling 往返、按轴 scaling 逆变换、内存泄漏检查沿用）
- **基准**: `moebius/bench/Kernel/Types/frame_bench.cpp`（含优化前
  实现的同二进制 A/B 对比 + linear_2D 基线 + enclose 包围盒场景）

### 5.4 segment/poly_segment 求值与 rotate_2D 消除中间 point 临时（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`、`moebius/Kernel/Types/point.cpp`
- **What**:
  1. `segment_rep::evaluate` 由 `(1-t)*p1 + t*p2`（3 次分配）改为逐分量
     线性插值（1 次分配）；
  2. `poly_segment_rep::evaluate` 同样逐分量插值；
  3. `poly_segment_rep::grad` 由 `n*(a[i+1]-a[i])`（2 次分配）改为
     逐分量差值放大；
  4. `rotate_2D` 加 2D 快路径，避免 `p-o` 与 `+o` 两个中间临时；
     非常规维度走原 mult 回退路径。
- **Why**: 曲线求值是图形渲染/取直(rectify)/边框计算的最内层循环，
  每个采样点一次 evaluate；poly_segment 是折线图形的通用表示。
- **结果**: x1024 循环：segment 53.0µs→15.6µs（**3.4x**），
  poly_segment 32.5µs→18.1µs（1.8x），rotate_2D 54.6µs→22.8µs（2.4x）。
- **测试**: `curve_test.cpp` 追加 3 用例（segment 端点/中点/三维维度、
  poly_segment 分段边界 n=2 语义、grad 倍率）；`point_test.cpp` 追加
  rotate_2D 退化维度回退路径用例
- **基准**: `moebius/bench/Kernel/Types/curve_eval_bench.cpp`（含优化前
  实现的同二进制 A/B 对比）

### 5.5 spline 求值：interval_no 缓存 + 跳过单位标量乘（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`（spline_rep）
- **What**:
  1. `interval_no` 加上次命中缓存（求值常按 t 单调推进，先验上次区间）；
  2. `evaluate(t,o)` 在 o=0 时跳过 `prod(k,o)*res`（系数为 1，省一次
     整点分配）；
  3. `approx` 中 `norm(p1-p2)` 改用 `norm2_diff` 开平方，避免差向量临时。
- **Why**: spline 求值是样条曲线渲染/取直的最内层循环；每次求值都要
  interval_no 全表线性扫描定位区间。
- **How**: 新增成员 `last_interval`（默认 -1），命中直接返回，未命中
  全扫后记录。A/B 用 `git stash` 临时还原旧实现测得旧值。
- **结果**: 513 点求值扫：单调 21.0µs→14.9µs（**1.41x**）、震荡
  20.9µs→15.6µs（1.33x）；rectify 7.9µs→7.5µs（基本持平，取直主要
  开销在递归细分而非求值）。
- **测试**: `curve_test.cpp` 追加 3 用例（端点插值、缓存命中/失效
  一致性、rectify 首末点、grad/bound 契约）
- **基准**: `curve_bench.cpp` 追加 spline 求值扫（单调/震荡）与 rectify

### 5.6 raw_split/raw_join 逐元素搬移改 memmove 块移动（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_observer.cpp`
- **What**: `raw_split` 的孩子右移与 `raw_join` 的孩子左移原为逐元素
  `ref[i]=ref[i-1]` 循环（每次引用计数加减），改为 memmove 整块搬移
  （句柄所有权随位移动转移），洞/尾部 stale 位分别用 placement-new
  覆盖与 resize 截断丢弃——与已合入 4398（raw_insert/raw_remove）同法。
- **Why**: 编辑器里每个回车/删段落/文本切分合并都走 split/join；
  宽文档中部操作要搬移数百个孩子句柄。
- **结果**: 1000 孩子文档中部 split+join x500：1.92ms→0.26ms（**7.4x**）。
- **测试**: 新建 `moebius/tests/Data/Tree/tree_observer_test.cpp`
  （8 用例：split 兄弟保持/尾边界/原子文本、join 原子合并/复合孩子
  合并/尾边界、split+join 往返、insert+remove 往返）
- **基准**: `tree_observer_bench.cpp` 追加 split+join A/B 对比
- **注**: raw_split/raw_join/raw_remove 未在 hpp 声明，测试与基准中
  补 extern 原型

### 5.7 get_env_child 空 cenv 快路径（2026-08-20）
- **文件**: `moebius/moebius/drd/drd_info.cpp`
- **What**: `get_env_child` 两个重载加空环境快路径：
  1. `(t,i,env)` 重载：`drd_decode(ci[index].env)` 为空时直接透传 env，
    免去逐对 `drd_env_write` 重建 env 树；
  2. `(t,i,var,val)` 重载：非 WITH 且子节点无绑定时直接返回缺省值，
    免去 ATTR 构造、合并与读取扫描。
- **Why**: `is_accessible_cursor`（光标可达性校验，每次光标移动逐节点
  调用）里 `get_env_child(t,i,MODE,"")=="src"` 是 default 分支的必经
  路径；绝大多数标签的子节点没有任何环境绑定，全走无用功。
- **结果**: 500 段×8 词文档逐节点读 mode：306µs→217µs（**1.41x**）。
- **测试**: 新建 `tests/moebius/drd/drd_env_test.cpp`（5 用例：空绑定
  返回缺省、越界索引、WITH 绑定读取/非最后孩子、env 重载合并透传、
  get_env_descendant）
- **基准**: 新建 `bench/moebius/drd/drd_env_bench.cpp`（含优化前实现
  的同二进制 A/B 对比）

### 5.8 can_* 适用性检查单趟下探（2026-08-20）
- **文件**: `moebius/Kernel/Types/modification.cpp`
- **What**: `can_insert/can_remove/can_split/can_join/can_assign_node/
  can_set_cursor` 原来先 `has_subtree(t,p)` 再 `subtree(t,p)`——同一条
  路径走两遍树。新增内部 `descend` 助手单趟下探返回指针（越界/命中
  原子返回空），一次遍历完成存在性检查与取子树。
- **Why**: `apply()` 每次树修改前都调 `is_applicable`；编辑路径上
  深路径的检查是纯开销。
- **结果**: 深度 9 路径：can_insert 95.6ns→41.9ns（**2.3x**），
  can_remove 76.8ns→42.3ns（1.8x）；失败路径原本就单趟（has_subtree
  提前返回），持平。
- **测试**: `modification_test.cpp` 追加 5 用例（insert 原子/复合/
  越界/pos 越界、原子内插、remove、join 原子+复合混合、assign/节点
  操作）
- **基准**: 新建 `bench/Kernel/Types/modification_bench.cpp`（A/B 对比）
- **注**: `mod_insert(p,pos,t)` 等会把 pos 追加到 p 尾部，测试里
  顶层操作应传 `path()` 而非 `path(0)`（踩坑记录）

### 5.9 move_any 单趟下探（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_traverse.cpp`
- **What**: `move_any`（`next_any`/`previous_any`/`next_valid`/
  `next_accessible`/`next_word` 的公共底层）原来每次移动调三次
  `subtree(t, path_up(...))`——重复全树下探外加 path_up 临时分配。
  改为单趟下探同时持有 `path_up(p)` 处节点与其父节点。
- **Why**: 光标移动/选择/删除是编辑器最频繁的操作，每次按键的
  valid/accessible 光标搜索循环里 move_any 逐位置调用。
- **结果**: 100 段文档逐字符全扫：714µs→545µs（**1.31x**）。
- **测试**: `tree_traverse_test.cpp` 追加 3 用例（全扫收敛且停在
  不动点、next/previous 20 步往返一致、首步深入文档内部）
- **基准**: 新建 `bench/Data/Tree/tree_cursor_bench.cpp`（含优化前
  实现的同二进制 A/B 对比）
- **注**: `next_any/previous_any` 未在 hpp 声明，测试与基准中补原型；
  `tm_char_forwards` 在 `cork.hpp` 不在 `analyze.hpp`

### 5.10 tmu_reader::read_next 字节级扫描（2026-08-20）
- **文件**: `moebius/Data/Convert/tmu.cpp`
- **What**: `read_next` 的词元累积循环原来逐字符调 `read_char`——
  每个字符一次子串分配（文档加载百万级小分配）。改为字节级扫描：
  分隔符均为单字节 ASCII，普通字符成段（run）一次追加，转义 `\\`
  连同后续一个 utf8 序列一起复制，行续接 `\<newline>` 原样跳过。
- **Why**: TMU 是 Mogan 原生文档格式，`tmu_to_tree` 是打开文件的
  必经路径；词元读取是最内层循环。
- **结果**: 500 段×10 词文档 `tmu_to_tree`：1.51ms→0.61ms（**2.47x**）；
  `tree_to_tmu` 持平（未改动）。A/B 用 `git stash` 还原旧实现实测。
- **测试**: 新建 `tests/Data/Convert/tmu_test.cpp`（6 用例：纯词、
  词内空格、`\\`/`<`/`|`/`>` 转义往返、utf8 不截断、concat+标记结构、
  密集反斜杠）
- **基准**: 新建 `bench/Data/Convert/tmu_read_bench.cpp`
- **注**: 未 `init_std_drd` 时 `as_string(RIGID)` 返回 "?"，writer 会
  把标记写成 `<?|...>`，往返后标签编号变化——标记结构测试需先初始化

### 5.11 三对角求解逐分量就地写（2026-08-20）
- **文件**: `moebius/Kernel/Types/equations.cpp`、
  `moebius/Data/Convert/tmu.cpp`（cr 原地截断）
- **What**:
  1. `tridiag_solve` 前代 `(y-a*x)/li` 与回代 `x-u*x` 每行产生 1–2 个
     中间 point，改为逐分量就地写（x 行初始为空点，维度不足时整行重建）；
  2. `quasitridiag_solve` 的 `vx= vx + v[i]*x[i]` 累加（O(n) 个临时）
     改为预分配单点累加；末尾修正 `x[i]= x[i]-z[i][0]*vx` 逐分量就地减；
  3. `tmu_writer::cr` 行尾空格改写由整段前缀拷贝改为 `resize` 原地截断
     （实测常规文档无差异，渐近防御性改进）。
- **Why**: 三对角求解是 spline 构造的必经路径（闭合样条 xtridiag 走
  quasitridiag）。尝试过 tmu_writer::write 成段追加——短词场景子串
  分配反而更贵，已回退。
- **结果**: 256 控制点 spline 构造：开样条 117µs→106µs（1.10x），
  闭合样条 1.08ms→0.81ms（**1.33x**）。A/B 用 git stash 实测。
- **测试**: 新建 `tests/Kernel/Types/equations_test.cpp`（3 用例：
  3x3 二维方程残差校验、维度保持、零耦合恒等与秩一修正
  Sherman-Morrison 闭式解）
- **基准**: 新建 `bench/Kernel/Types/spline_bench.cpp`（开/闭合样条
  构造）；`bench/Data/Convert/tmu_write_bench.cpp`（写路径对照）

### 5.12 scheme 解析器越界读修复（2026-08-20）
- **文件**: `moebius/moebius/data/scheme_der.cpp`
- **What**（正确性为主，性能持平）:
  1. 词元扫描与引号串扫描的 `ch= s[end_index]` 先读后判界，缓冲区
     末尾各越界读一字节——改为循环顶先判界；
  2. `unslash` 尾部 `ch= s[i]` 同样越界——同样改为循环顶读取；
  3. `string_to_scheme_tree` 的 `replace(s,"\\015","")` 整串拷贝改为
     先探测含 CR 才替换；
  4. 修复 `block_bench.cpp` 失效的资源路径（bench/ → moebius/bench/，
     子目录迁移后无人发现）。
- **Why**: 越界读是未定义行为（ASAN 会报）；scheme 解析是启动加载
  全部 .scm 的必经路径。
- **结果**: block_bench 解析 5.69→5.77 ns/char（噪声内持平）。
- **尝试后回退**: `is_compound(t,s)` 改标签编号比较——名字比较短路
  vs 哈希全串，实测反而慢 54%（32.9→50.7µs），已回退。
- **测试**: 新建 `tests/moebius/data/scheme_der_test.cpp`（9 用例：
  词元在缓冲区末尾、末尾反斜杠、引号转义、未闭合引号、注释、
  quote 糖、CR 剔除、block 多表达式）
- **基准**: `block_bench.cpp`（路径修复后可正常运行）

### 5.13 slash/scm_quote 成段追加（2026-08-20）
- **文件**: `moebius/moebius/data/scheme_ser.cpp`
- **What**: 序列化转义 `slash`/`scm_quote` 原来逐字符 `r << s[i]`
  （每次一次 resize 调用），改为普通字符成段（run）一次子串追加、
  特殊字符单独转义。
- **Why**: scheme 序列化是保存 .scm/样式文件与 block 协议的写路径。
- **结果**: block_bench：简单元素序列化 12.15→10.97 ns/char（**1.11x**）；
  复杂树/单树持平（base64 长原子场景逐字符 append 本已摊销良好）。
  A/B 用 git stash 实测。
- **测试**: `scheme_der_test.cpp` 追加 3 用例（scm_quote 引号/反斜杠
  转义、slash 特殊字符/控制字符/已引号串豁免、slash→解析器往返）
- **踩坑记录**: 测试中给未导出的 `slash` 补 extern 原型时必须写全
  命名空间 `moebius::data::slash`——声明成全局 `::slash` 会让链接器
  解析到库中同名符号、拉入错误成员，报出误导性的 tbox MSIL/LTCG
  链接错误（第 2 轮 tree_modify_test 的同类 flaky 报错同源）。
- **基准**: `block_bench.cpp`

### 5.14 move_word 与 tm_codepoint_at 消除子串分配（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_traverse.cpp`
- **What**:
  1. `tm_codepoint_at`（词边界判定的逐字符内层）原来每个字符取子串
     `s(pos,i)` 并做 `starts/ends` 字符串比较——改为直接在原串上按
     字节解析（ASCII 单字节、`<#....>` 十六进制形式、其余记未知）；
  2. `move_word` 循环内 `subtree(t, path_up(q))` 每步全树下探——
     改为 `tt_descend` 单趟下探。
- **Why**: Ctrl+Left/Right 词移动、双击选词走 move_word；每步对光标
  前后两个字符各调一次 tm_codepoint_at。
- **结果**: 100 段词文档逐词全扫：1.49ms→1.33ms（**1.12x**）。
- **测试**: `tree_traverse_test.cpp` 追加 3 用例（逐词推进收敛、
  next/previous 往返、标点与 `<#XXXX>` 十六进制转义边界不崩溃）
- **基准**: `tree_cursor_bench.cpp` 追加 next_word 全扫场景

### 5.15 simplify_correct 未变子树共享原节点（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_modify.cpp`
- **What**: 非 concat/document 的复合节点，递归结果先暂存，全部孩子
  共享原 rep（strong_equal 判定）时直接返回原节点，免去新节点分配；
  concat/document 维持原有重建逻辑（需要合并/展平）。
- **Why**: `simplify_correct` 在每次排版（edit_typeset）与文档加载
  （input.cpp）对全树递归，原实现无条件重建每个复合节点。
- **结果**: 典型文档（document→concat→原子）持平——中间节点全是
  concat/document，快路径不触发；标记密集文档（rigid/with 等非
  format 节点）免去未变子树的节点重建，为结构性改进而非基准提速。
  第一版"先建 r 再判定"反而多付一次比较（442µs vs 401µs），改为
  暂存数组后持平（406µs）。
- **测试**: `tree_modify_test.cpp` 追加 4 用例（普通标记保持、QUOTE
  解包、嵌套 document 展平 + concat 合并、空 concat 收敛）
- **基准**: `tree_modify_bench.cpp` 追加 simplify_correct 全树扫

### 5.16 .tm 加载：codes 查表跳过 + scm_unquote 成段追加（2026-08-20）
- **文件**: `moebius/moebius/data/scheme_der.cpp`
- **What**:
  1. `scheme_tree_to_tree` 在 flag=true（默认加载路径）时先做
     `codes[t[0]->label]` 查表再被 make_tree_label 覆盖——纯浪费，
     改为按 flag 只查一次；
  2. `scm_unquote`（每个文本原子解引号）逐字符 append 改为成段
     追加（与 5.13 slash 同法）。
- **Why**: .tm 文档打开 = 解析 + scheme_tree_to_tree 全树转换；
  每个节点一次多余的字符串哈希查表、每个文本原子一次逐字符循环。
- **结果**: 500 段 .tm 加载：405→388µs（**1.04x**），其中 codes 跳过
  ~2%、scm_unquote ~2%。尝试过 make_tree_label 单条备忘缓存——
  短串哈希本就便宜，备忘串比较无净收益，已回退。
- **测试**: `scheme_der_test.cpp` 追加 3 用例（scm_unquote 解引号/
  反转义、.tm 全链路往返结构与转义、带版权头注释的文档）
- **基准**: 新建 `bench/moebius/data/scheme_load_bench.cpp`
  （scheme_document_to_tree / scheme_to_tree 全链路）

## 6 Why（总体）
moebius 是 Mogan 的 C++ 内核库，排版/编辑热路径大量经过其中函数；
逐个函数做可度量（bench 前后对比）、可回归（单元测试）的优化。

## 7 How（总体）
- 优化前先写 bench（同二进制内保留旧实现做 A/B 对比）
- 优化后跑 `xmake test moebius_tests/<name>` 回归
- 每轮记录到本文档第 5 节
