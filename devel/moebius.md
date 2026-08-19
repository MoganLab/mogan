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

## 6 Why（总体）
moebius 是 Mogan 的 C++ 内核库，排版/编辑热路径大量经过其中函数；
逐个函数做可度量（bench 前后对比）、可回归（单元测试）的优化。

## 7 How（总体）
- 优化前先写 bench（同二进制内保留旧实现做 A/B 对比）
- 优化后跑 `xmake test moebius_tests/<name>` 回归
- 每轮记录到本文档第 5 节
