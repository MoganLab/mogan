# 开发规范

## C++ 代码规范

1. **不使用标准 C++ 库**：Mogan 使用自研的 C++ 基础设施（如 lolly/moebius 库），内部类型（`string`、`list`、`array`、`tree`、`path` 等）均有自定义实现，与 `std::` 不兼容。
2. **输出流使用项目内置 `cout`**：调试输出应使用全局 `cout`（类型为 `tm_ostream`），而非 `std::cout`；换行使用 `LF` 宏或 `"\n"`，不要使用 `std::endl`。
3. **容器不支持现代 C++ 特性**：自定义容器（如 `rectangles`、`list`）不支持范围 for 循环（range-based for），需使用传统的迭代器或 `is_nil()`/`next` 遍历。
4. **类型转换使用项目函数**：自定义类型（如 `path`）没有标准 `operator<<` 重载，输出前需先用 `as_string()` 转换。

### 调试日志

1. **临时调试**：使用 `#ifdef LIII_DEBUG` / `#endif` 包裹全局 `cout`（`tm_ostream` 类型）。该宏仅在 debug 编译模式下定义，release 模式下整段代码会被编译器剔除，避免影响性能：
   ```cpp
   #ifdef LIII_DEBUG
   cout << "Assign " << p << ", " << u << " in " << st << "\n";
   #endif
   ```

2. **标准调试流**：使用项目预定义的调试输出流（如 `debug_std`、`debug_typeset`、`debug_boot`、`debug_edit` 等），配合 `DEBUG_STD`、`DEBUG_AUTO` 等宏开关，可通过外部配置启用/禁用：
   ```cpp
   if (DEBUG_STD) debug_boot << "Loading welcome message...\n";
   ```

3. **性能调试**：使用 `bench_start`、`bench_end` 等函数进行性能计时：
   ```cpp
   bench_start ("my_task");
   // ... 代码 ...
   bench_end ("my_task");
   ```

### 注释规范

1. **文档注释用 Doxygen 风格**：文件级、函数级说明用 `/** ... */` 或 `/*! ... */`，配合 `@file`、`@brief`、`@param`、`@return`、`@note`、`@par` 等标签，便于工具解析。中文撰写。

2. **代码注释精简，避免冗余**：
   - 函数内注释只写「为什么」（Why），不写「做什么」（What）——后者代码本身已表达。
   - 不逐行复述代码。整段显而易见的逻辑不需注释。
   - 一行注释能说清的不拆成多行段落。

3. **版权块保持独立**：`MODULE / DESCRIPTION / COPYRIGHT / LICENSE` 标准版权块单独成块闭合，Doxygen 设计说明放在它之外（另起一个注释块），不混在一块。

## 分支命名规则

分支格式：`username/200_27/xxx`

- `username`: 开发者用户名
- `200_27`: 项目标识符
- `xxx`: 功能描述或任务编号

例如：
- `da/200_27/xmake_debug`
- `da/200_27/fix_pdf_rendering`

## 任务文档

每个任务在 `devel/<编号>.md` 维护一份文档。分支名中的任务编号即文档名,
例如分支 `da/1113/backward` 对应 `devel/1113.md`。开始工作前先按分支定位
任务文档,完成后把本次改动(What/Why/How/涉及文件)追加到文档里。

## 提交规范

1. 一个 PR 至少分为两个 commit：
   - 第一个 commit 更新 `devel/xxxx.md` 任务文档
   - 后续 commit 为代码改动
2. **提交前必须运行 `gf fmt --changed-since=main`** 格式化变更的 `.scm` 和 C++（`.cpp`/`.hpp`）文件
3. 保持提交信息清晰、简洁，格式：`[编号] 简述`

## 代码推送规则

1. 如果 remote 是 GitHub，使用 `gh` 命令推送代码并创建 PR
2. 如果 remote 是 Gitee，直接使用 `git push` 推送代码
3. 推送前确保代码已通过本地测试
4. 保持提交信息清晰、简洁

## 单元测试

项目有三类单元测试，xmake 自动发现（`tests/**_test.cpp`、`TeXmacs/progs/**/*-test.scm`、`TeXmacs/tests/*.scm`），无需手动登记。

### 1. C++ 单元测试（`tests/**_test.cpp`）

所有 `tests/**_test.cpp` 自动识别，链接 `libmogan` + `libmoebius`。

```bash
xmake b xxx_test && xmake r xxx_test
```

#### Qt 窗口测试

测试中 `show()` 了顶层 `QWidget` 的用例，必须在测试类的 `cleanup()` 槽里调用
`cleanup_qt_top_level_widgets()`（声明在 `tests/Base/base.hpp`）：

```cpp
class TestMyWidget : public QObject {
  Q_OBJECT
private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }
  // ...
};
```

**原因**：用例中途断言失败时，`new` 出来的 widget 不会被 `delete`，泄漏的窗口会持续显示，
导致批量跑 `xmake run --group=tests` 时整个套件卡住，需要手动关弹窗；Windows 下
下一个测试进程启动时 Qt `DllMain` 初始化失败（错误码 `0xC000013A`）。

### 2. Scheme 纯逻辑测试（`TeXmacs/progs/**/tests/*-test.scm`）

无 GUI、headless，适合测数据契约/编码一致性/纯函数。函数名 `(regtest-<basename>)`。
参考 `TeXmacs/progs/texmacs/menus/tests/print-widgets-test.scm`：

```scheme
(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/progs/.../target-module.scm")  ;; 加载被测模块

(define (test-foo) (check expr => expected))

(tm-define (regtest-print-widgets)
  (test-foo)
  (check-report)
) ;tm-define
```

```bash
xmake b stem && xmake r print-widgets-test
```

### 3. 集成测试（`TeXmacs/tests/*.scm`）

函数名 `(test_<NNNN>)`，参考 `TeXmacs/tests/2044.scm`：

```scheme
(import (liii check))
(load "./TeXmacs/progs/.../target-module.scm")

(tm-define (test_2044)
  (run-chain (list
    (cons "step 1" (lambda () ...))
    (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
  ))
) ;tm-define
```

**两种模式**：

| 模式 | 命令 | 行为 |
|------|------|------|
| Headless | `xmake r 2044` | `-headless`，自动 `quit-TeXmacs`，冒烟验证进程不崩 |
| GUI | `MOGAN_TEST_GUI=1 xmake r 2044` | 真实 GUI，不自动 quit，异步链真正调度执行断言 |

- headless 下 `exec-delayed-at` 来不及调度就 `quit-TeXmacs`，断言不跑
- GUI 模式下调试日志直接进终端，测试脚本自己延迟 `(quit-TeXmacs)`

### 测试策略

- C++ bridge 纯逻辑优先放 Scheme 纯逻辑测试（`*-test.scm`），headless 秒级反馈
- C++ 测试仅覆盖 Qt 钩子/返回值形状/bridge 入口（如 `MOGAN_TEST_*=ok|cancel`）
- GUI 专属代码路径（tab 切换、菜单重建）用 GUI 集成测试

### Scheme 诊断

1. **纯 scheme 逻辑用 `gf eval` 快速验证**：不依赖 mogan 内置（`translate` /
   `get-pretty-preference` 等 tm 库）的纯函数，可用项目自带的 Goldfish Scheme
   解释器直接跑，秒级反馈，无需构建 mogan：
   ```bash
   gf eval '(define (f x) `(a ,x)) (display (f 1)) (newline)'
   ```
   适合验证 quasiquote、列表处理等纯语言行为。

2. **mogan scheme 列表字面量在求值位置会被求值**：裸写 `("a" "b")` 出现在
   函数实参位置时，car `"a"` 被当函数应用而崩（`string ref: too many
   indices`）。传常量列表必须 quote：`(f key '("a" "b"))`。quasiquote 内无
   前置 `,` 的列表字面量原样保留，可裸写。

3. **需 mogan 内置的脚本用真实二进制跑**：依赖 tm 库的诊断脚本，写临时
   `.scm` 文件，用构建产物加载：
   ```bash
   TEXMACS_PATH=$(pwd)/TeXmacs \
     build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM \
     -headless -d -x "(load \"/tmp/diag.scm\")"
   ```

## 单元测试

项目有三类单元测试，xmake 自动发现并构建（无需手动登记）：

### 1. C++ 测试（`tests/**_test.cpp`）

自动发现、链接 `libmogan` + `libmoebius`。参考 `tests/Plugins/Qt/font_selector_bridge_test.cpp`：

```cpp
#include "base.hpp"  // init_lolly
class TestFoo : public QObject {
  Q_OBJECT
private slots:
  void init () { init_lolly (); }
  void test_case ();
};
// ... 实现 ...
#ifdef QTTEXMACS
QTEST_MAIN (TestFoo)
#else
int main () { return 0; }
#endif
#include "foo_test.moc"
```

构建与运行：
```bash
xmake b foo_test && xmake r foo_test
```

### 2. Scheme 纯逻辑测试（`TeXmacs/progs/**/*-test.scm`）

无 GUI、headless，适合测数据契约/编码一致性/纯函数。函数名 `(regtest-<basename>)`。
参考 `TeXmacs/progs/texmacs/menus/print-widgets-test.scm`：

```scheme
(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/progs/.../target-module.scm")  ;; 加载被测模块

(define (test-foo) (check expr => expected))

(tm-define (regtest-print-widgets)
  (test-foo)
  (check-report)
) ;tm-define
```

构建与运行：
```bash
xmake b stem && xmake r print-widgets-test
```

### 3. GUI 集成测试（`TeXmacs/tests/*.scm`）

真实 GUI 进程，通过 `exec-delayed-at` 串异步链驱动。函数名 `(test_<NNNN>)`。
参考 `TeXmacs/tests/2044.scm`：

```scheme
(import (liii check))
(load "./TeXmacs/progs/.../target-module.scm")

(tm-define (test_2044)
  (run-chain (list
    (cons "step 1" (lambda () ...))
    (cons "step 2" (lambda () ...))
    (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
  ))
) ;tm-define
```

需 `MOGAN_TEST_GUI=1` 才真正跑断言（headless 模式仅冒烟进程不崩）：
```bash
xmake b stem && MOGAN_TEST_GUI=1 xmake r 2044
```

### 测试策略

- C++ bridge 纯逻辑轮子优先放 Scheme 纯逻辑测试（`*-test.scm`），headless 秒级反馈
- C++ 测试仅覆盖 Qt 钩子/返回值形状/bridge 入口（如 `MOGAN_TEST_*=ok|cancel`）
- GUI 专属路径（tab 切换、菜单重建）用 GUI 集成测试

## 构建命令

主项目构建：`xmake b stem`

如果构建失败（例如配置缓存陈旧、依赖路径错乱），执行 `xmake f -c --yes` 清理配置缓存后重新构建。

### Scheme Glue（C++ ↔ scheme 绑定）

- **glue 声明在 `.lua` 不在 `.scm`**：mogan 的 glue 由 xmake 规则 `xmake/rules/glue.lua`
   在构建期生成 `build/.gens/.../glue/glue_*.cpp`。**声明源是 `src/Scheme/Glue/glue_*.lua`**
   （如 `glue_editor.lua`），不是 texmacs 遗留的 `build-glue-editor.scm` /
   `TeXmacs/progs/prog/glue-symbols.scm`——那两个 `.scm` 文件 mogan 不使用，改了不生效。
   新增一个 scheme 可调的 C++ 函数（编辑器方法）：
   - C++：在 `edit_modify_rep` 等加方法（glue 规则给所有调用加 `get_current_editor()->`
     前缀，故只能绑编辑器方法，不能绑自由函数——自由函数要包一层方法转调）。
   - `glue_*.lua`：加 `{ scm_name = "foo", cpp_name = "foo", ret_type = "...", arg_list = {...} }`。

## 工作流程

1. 基于主分支创建新分支
2. 按规范命名分支
3. 开发完成后直接 `git push` 推送
4. 不需要使用 GitHub CLI 工具