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

### 5.17 曲线最近点搜索消除差向量临时（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`
- **What**: `curvet_closest_points`（最近点扫描主循环）、`closest`
  （外层迭代）与 `intersection`（牛顿求交）中的 `norm (a - b)` 全部
  改为 `sqrt (norm2_diff (a, b))`，免去每步一个差向量 point 临时；
  `straight_edge_midpoints` 的 `norm(...) < 1e-6` 同改平方比较。
- **Why**: 图形点选/框选（graphical_select→find_closest_points）与
  曲线求交每步采样都要算一次距离。
- **结果**: 128 段折线最近点搜索：31.7µs→28.9µs（**1.10x**）。
  已用 git stash 对照确认新旧实现行为逐位一致。
- **测试**: `curve_test.cpp` 追加 4 用例（segment/poly_segment 最近点
  精确命中、closest 距离下界、交叉线段求交闭式解）
- **踩坑记录**: `find_closest_point` 对曲线外侧查询点可能只返回
  起点 t=0（既有算法局限，新旧一致）——测试用曲线上的点作查询。
- **基准**: 新建 `bench/Kernel/Types/curve_closest_bench.cpp`

### 5.18 arc/ellipse 求值逐分量直写（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`（arc_rep/ellipse_rep）
- **What**: `evaluate`（`center + r1*cos*i + r2*sin*j`）与 `grad`
  原写法每次产生 2–4 个中间 point 临时，改为单次分配逐分量直写；
  维度取原表达式 min 链，数值逐位一致。
- **Why**: 圆弧/椭圆是图形里最常见的曲线，取直（渲染采样）以固定
  步长全参数域扫 evaluate。
- **结果**: 512 点求值扫：ellipse 26.6µs→9.9µs（**2.7x**）、
  arc 28.0µs→11.1µs（**2.5x**）；rectify eps=0.1：1.53ms→0.60ms
  （**2.55x**）。A/B 用 git stash 实测。
- **测试**: `curve_test.cpp` 追加 4 用例（椭圆上点到焦点距离和恒定、
  长短轴端点、grad 正交性、圆弧落圆、闭合 rectify 首尾相接）
- **踩坑记录**: ellipse 的 i 轴从圆心指向第一焦点，t=0 是 (-r1,0)
  方向端点而非 (+r1,0)
- **基准**: `curve_closest_bench.cpp` 追加 conic 场景

### 5.19 bezier 求值/取直逐分量直写（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`（bezier_rep）
- **What**:
  1. `evaluate`（链式 Horner，6 个中间 point 临时）改逐分量 Horner；
  2. `grad` 同改；
  3. `rectify_cumul` 的弦插值点与 `norm(q-r)>=e/10` 改逐分量插值 +
     平方距离比较。
- **Why**: 贝塞尔是图形平滑路径的通用表示，poly_bezier 包装后
  渲染/取直高频调用 evaluate。
- **结果**: 512 点求值扫 35.7µs→10.6µs（**3.4x**）；rectify
  eps=0.1 29.7µs→7.3µs（**4.1x**）。A/B 用 git stash 实测。
- **测试**: `curve_test.cpp` 追加 3 用例（端点/中点闭式值、grad、
  rectify 首尾端点）
- **踩坑记录**: 生产 `bezier_rep::grad` 公式为 `3*P3*t + 2*P2 + P1`
  （2*P2 项缺 t，非标准导数）——保持原语义，测试按代码行为断言；
  三次贝塞尔中点 x=(P0+3P1+3P2+P3)/8。
- **基准**: `curve_closest_bench.cpp` 追加 bezier 场景

### 5.20 hyperbola/parabola 求值逐分量直写（2026-08-20）
- **文件**: `moebius/Kernel/Types/curve.cpp`（hyperbola_rep/parabola_rep）
- **What**: 与 5.18 conic 同法——`evaluate`/`grad` 的链式
  `center ± r1*cosh*i + r2*sinh*j`、`vertex + (u²/2d)*i + u*j`
  改单次分配逐分量直写，双曲线两支用 sign 合并分支。
- **结果**: 512 点求值扫：hyperbola 30.3µs→12.8µs（**2.4x**）、
  parabola 21.2µs→6.6µs（**3.2x**）。A/B 用 git stash 实测。
- **测试**: `curve_test.cpp` 追加 2 用例（双曲线到两焦点距离差恒定、
  抛物线顶点闭式值与对称性）
- **更正 5.20 初判**: 上轮记录的 "parabola 对某 fixture SIGSEGV"
  排查后确认**不是 parabola 的 bug**——是测试 fixture 写
  `point (-4, 0)` 时 int 字面量解析到 `array (n, ...)` 长度构造器
  （负长度）导致崩溃。构造点必须用 `point (2)`+赋值或 double 字面量
  `point (-4.0, 0.0)`（踩坑：与 5.9 轮 point (1.0) 同源，
  int/double 重载解析陷阱）。parabola 行为正常，测试已补回。

### 5.21 keep_positive 无负索引快路径（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_cursor.cpp`
- **What**: `keep_positive` 原来无条件逐层递归重建整条 path；先线性
  扫描，全为非负索引（常见情况）时原样返回，仅含负索引时走原重建。
- **Why**: `correct_cursor`（每次按键后的光标校正）以 keep_positive
  开头。
- **结果**: correct_cursor 100 步扫：25.7µs→22.2µs（**1.16x**）。
- **测试**: `tree_traverse_test.cpp` 追加 2 用例（负索引路径截断
  校正、合法路径校正稳定）；并全量回归 moebius 24 个测试全部通过。
- **基准**: `tree_cursor_bench.cpp` 追加 correct_cursor 扫描

### 5.22 EXTERN 派生标签单条备忘缓存（2026-08-20）
- **文件**: `moebius/moebius/drd/drd_info.cpp`
- **What**: 5 处 `make_tree_label ("extern:" * t[0]->label)`（含
  `is_accessible_child`/`get_type_child` 等热路径）每次都要字符串
  拼接 + 标签查表。新增 `extern_label` 助手：单条备忘缓存
  (宏名→派生标签)，同名宏高频重复时直接命中。
- **Why**: 可执行标记（EXTERN）文档里每个节点的光标可达性校验都
  要派生标签；同一宏名的节点大量重复。
- **结果**: 200 个 EXTERN 节点逐孩子 is_accessible_child 扫：
  110µs→52µs（**2.1x**）。注：与 5.16 的 make_tree_label 备忘失败
  不同，此处省的是每次的 "extern:" 字符串拼接分配。
- **测试**: `drd_env_test.cpp` 追加 1 用例（同名宏重复出现时备忘
  命中/未命中结果一致，不同宏名备忘失效正常）；全量 24 测试通过。
- **基准**: `drd_env_bench.cpp` 追加 EXTERN 场景

### 5.23 tmu_reader::decode 成段追加 + 标签分支单查表（2026-08-20）
- **文件**: `moebius/Data/Convert/tmu.cpp`
- **What**:
  1. `decode`（每个词元解码）原来逐字符 `r << s[i]`——即使无转义
     也逐字符 append，改为普通字符成段一次追加；
  2. `read` 的标签分支先 `tree (make_tree_label (name))` 构造再被
     codes 命中覆盖——改为 codes 命中直接构造，免去一次标签查表。
- **结果**: `tmu_to_tree` 500 段文档：602µs→538µs（本轮 **1.12x**，
  相对最初实现累计 1512µs→538µs = **2.81x**）。
- **测试**: 复用 `tmu_test.cpp` 全部 6 用例（转义往返覆盖 decode
  路径），通过。
- **基准**: `tmu_read_bench.cpp` 复用
- **追加（第 29 轮）**: `read_apply` 同款双查表（先 make_tree_label
  构造再被 codes 覆盖）同法修复；538→532µs（噪声级，严格少一次
  标签查表，累计相对最初 1512µs = **2.84x**）。

### 5.24 end(t,p) 免去第二次全树下探（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_cursor.cpp`
- **What**: `end(tree, path)` 原来 `parent_subtree(t,p)` 与
  `subtree(t,p)` 各做一次全树下探；父节点的第 `last_item(p)` 个孩子
  即 p 所指节点，复用父引用省一趟。`start` 本就单趟未动。
- **结果**: 浅路径基准（depth-1）持平（25.9→25.5µs，correct_cursor
  占主导）；深路径场景省 O(depth) 一次下探，为结构性改进。
  语义严格等价（subtree(t,p) ≡ parent_subtree(t,p)[last_item(p)]）。
- **测试**: 复用 `tree_traverse_test`（end(doc) 全扫收敛于不动点）与
  `tree_observer_test`，全部通过。
- **基准**: `tree_cursor_bench.cpp` 追加 end per para 场景

### 5.25 frame::enclose 采样插值逐分量直写（2026-08-20）
- **文件**: `moebius/Kernel/Types/frame.cpp`
- **What**: `enclose` 每个采样点的 `p1 + a*(p2-p1)` 产生差向量/
  标量乘/加法三个中间 point 临时，改为逐分量插值直写唯一采样点。
- **Why**: `frame::enclose(rectangle)` 是图形包围盒计算入口
  （失效区域/边框），非线性框架每边 20 个采样点。
- **结果**: scaling 框架矩形 enclose：244ns→152ns（**1.60x**）。
- **测试**: `frame_test.cpp` 追加 2 用例（线性框架包围盒恰为四角
  变换极值、逆向 enclose 除以放大率）
- **基准**: `frame_bench.cpp` 复用 enclose 场景

### 5.26 inside_contiguous_document 边界祖先单趟下探（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_traverse.cpp`
- **What**: 原来对 op/oq 各自逐层 `path_up` + `is_boundary(t,p)`
  （每次 is_boundary 两次 subtree 全树下探），深路径 O(depth²)；
  新增 `closest_boundary_ancestor`：沿路径自根单趟下探，途经
  DOCUMENT/GRAPHICS 节点时记录最深边界前缀路径，O(depth)。
- **Why**: `inside_contiguous_document` 是图形文档中每次光标移动
  （move_valid 的 graphics 钩子）的必经检查。
- **结果**: 24 层 WITH 嵌套同段两光标 ×100 次调用：
  1498µs→262µs（**5.7x**）。
- **测试**: `tree_traverse_test.cpp` 追加 2 用例（同段两光标为真、
  跨段为假）
- **踩坑记录**: 首版 fixture 路径多了一层 0 前缀，inside_same 早退
  假 → 基准测的是无效路径的垃圾行为（旧实现还会打印
  "The required path does not exist" 诊断）；修正后才是真实 5.7x。
  inside_same 要求两光标处于最近 DOCUMENT 祖先的同一个孩子内。
- **基准**: `tree_cursor_bench.cpp` 追加 inside_contiguous deep24

### 5.27 get_env_child WITH 快路径直扫绑定对（2026-08-20）
- **文件**: `moebius/moebius/drd/drd_info.cpp`
- **What**: `(t,i,var,val)` 重载遇到 WITH 末孩子时，原来走
  `get_env_child(t,i,ATTR)` —— `t(0,N-1)` 子树拷贝 + 逐对
  `drd_env_write` 重建 env 树 + 线性读取。改为直接扫描绑定对取
  **末次匹配**（与 merge 的覆盖语义一致），零分配。
- **Why**: 源码模式文档充满 `with "mode" "src"` 包裹，
  `is_accessible_cursor` 逐节点读 mode 环境全走此路径。
- **结果**: 500 个 WITH 节点读 mode：112µs→21µs（**5.2x**）。
- **测试**: `drd_env_test.cpp` 追加 1 用例（同名绑定对后者覆盖、
  无匹配返回缺省）；全量 24 测试通过。
- **基准**: `drd_env_bench.cpp` 追加 WITH mode sweep

### 5.28 get_env_child(env 变体) WITH 分支免子树拷贝（2026-08-20）
- **文件**: `moebius/moebius/drd/drd_info.cpp`
- **What**: `(t,i,env)` 重载的 WITH 分支原来 `drd_env_merge (env,
  t (0, N (t) - 1))`——先做一次子树拷贝再逐对合并；改为直接在
  原树 [0, N-1) 上迭代绑定对调用 `drd_env_write`（与 merge 逐对
  语义严格一致），免去每次一棵孩子数组的分配。
- **Why**: `get_env_descendant (t, p, env)` 沿路径逐层调用此重载
  （排版环境求值/光标环境链）。
- **结果**: 24 层 WITH 嵌套链 get_env_descendant(env 变体)：
  6.69µs→5.00µs（**1.34x**）；字符串变体链（走 5.27 快路径）持平。
- **测试**: 复用 `drd_env_test.cpp`（env 变体合并透传用例覆盖）；
  全量 24 测试通过。
- **基准**: `drd_env_bench.cpp` 追加 WITH chain24（env 变体）

### 5.29 drd_env_write 单次分配重建（2026-08-20）
- **文件**: `moebius/moebius/drd/drd_info.cpp`
- **What**: `drd_env_write` 的追加/插入/替换三种情形原来都要
  两次切片 + 元组构造 + 两次拼接（约五次树分配）；改为按情形
  单次分配结果 ATTR、前缀/后缀直拷。
- **Why**: `drd_env_merge`（WITH 环境合并、DRD 环境链）逐对调用；
  深环境链原来是 O(k) 次五连分配。
- **结果**: 24 层 WITH 链 get_env_descendant(env 变体)：
  同变量（替换路径）4.94µs→2.17µs（**2.28x**）、不同变量
  （插入/追加路径）26.9µs→11.8µs（**2.28x**）。
- **踩坑记录**: 首版插入分支的后缀拷贝从 i+2 起步（应为 i 起步
  右移两格），导致插入后旧孩子丢失——被新增的排序/覆盖语义测试
  当场抓住。单测先行的价值再次体现。
- **测试**: `drd_env_test.cpp` 追加 1 用例（追加、排序插入、
  同名替换长度不变、末尾追加）；全量 24 测试通过。
- **基准**: `drd_env_bench.cpp` 追加 distinct vars chain24

### 5.30 移除已禁用的 next_without_border 死调用（2026-08-20）
- **文件**: `moebius/Data/Tree/tree_cursor.cpp`
- **What**: `next_without_border` 上游已禁用（函数体首行 `return
  false;`，逻辑被注释），但 `is_accessible_cursor`/`valid_cursor`/
  `closest_accessible` 三个热光标例程仍每步调用它。移除三处调用
  与函数体（原始逻辑保留在 git 历史）。
- **结果**: 各光标基准持平（预期内——每次省一个恒假调用）；
  本项为死代码清理而非性能项。复测中曾出现一次 3x 假回归
  （75.9µs vs 26µs），重跑确认为机器扰动，已排除。
- **同轮放弃**: `drd_env_read` 改二分搜索——外部调用者
  （tree_correct/edit_select 等）传入自建 env，排序不变量无保证，
  正确性风险大于收益。
- **测试**: 全量 24 测试通过。
- **基准**: `curve_closest_bench.cpp` 追加 hyperbola/parabola 场景

## 6 成绩单（2026-08-20 全量复测，24/24 测试通过）

| # | 函数/路径 | 提速 | 场景 |
|---|---|---|---|
| 5.1 | tree_utf8↔herk | 5.3x/2.1x | 1000 段文档加载转换 |
| 5.2 | correct_node | 1.96x | 预校正树 sweep×100 |
| 5.3 | scaling 直变换 | 2.3–3.1x | x1024 点设备变换 |
| 5.4 | segment/poly_segment 求值 | 4.4x/1.8x | 渲染采样 |
| 5.5 | spline 求值 | 1.4x | 单调扫 |
| 5.6 | raw_split/raw_join | 7.4x | 1000 孩子中部×500 |
| 5.7 | get_env_child | 1.37x | 光标校验 mode 读 |
| 5.8 | can_* 适用性检查 | 1.9–2.0x | 深度 9 路径 |
| 5.9 | move_any | 1.33x | 逐字符全扫 |
| 5.10 | tmu_to_tree | 2.47x | TMU 文档解析 |
| 5.11 | 三对角求解 | 1.10–1.33x | spline 构造 |
| 5.12 | scheme 解析 OOB 修复 | — | 正确性（3 处越界读） |
| 5.13 | slash/scm_quote | 1.11x | 序列化 |
| 5.14 | move_word/tm_codepoint_at | 1.12x | Ctrl+方向键 |
| 5.15 | simplify_correct | 结构性 | 未变子树共享 |
| 5.16 | .tm 加载 | 1.04x | codes 跳过+unquote |
| 5.17 | 曲线最近点/求交 | 1.07x | 图形点选 |
| 5.18 | arc/ellipse 求值 | 2.5–2.7x | conic 采样 |
| 5.19 | bezier 求值/取直 | 3.4–4.1x | 平滑路径 |
| 5.20 | hyperbola/parabola | 2.4–3.2x | 求值采样 |
| 5.21 | keep_positive | 1.16x | 光标校正 |

负结果（已回退并记录）：linear_2D 展开、is_compound 标签比较、
tmu write 成段追加、make_tree_label 备忘缓存、simplify_correct
第一版先建后判。

剩余未动的区域：patch/commute（undo 冷路径）、s7 object glue
（薄封装）、observers 内部（需联动 mogan 主程序）。

## 7 Why（总体）
moebius 是 Mogan 的 C++ 内核库，排版/编辑热路径大量经过其中函数；
逐个函数做可度量（bench 前后对比）、可回归（单元测试）的优化。

## 8 How（总体）
- 优化前先写 bench（同二进制内保留旧实现做 A/B 对比）
- 优化后跑 `xmake test moebius_tests/<name>` 回归
- 每轮记录到本文档第 5 节
