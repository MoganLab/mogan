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

## 分支命名规则

分支格式：`username/200_27/xxx`

- `username`: 开发者用户名
- `200_27`: 项目标识符
- `xxx`: 功能描述或任务编号

例如：
- `da/200_27/xmake_debug`
- `da/200_27/fix_pdf_rendering`

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

## C++ 单元测试

1. 所有 `tests/**_test.cpp` 文件会自动被 xmake 识别为测试目标
2. 构建方式：`xmake b xxx_test`
3. 运行方式：`xmake r xxx_test`

### Qt 窗口测试

测试中 `show()` 了顶层 `QWidget` 的用例，必须在测试类的 `cleanup()` 槽里调用共享工具函数 `cleanup_qt_top_level_widgets()`（声明在 `tests/Base/base.hpp`）：

```cpp
class TestMyWidget : public QObject {
  Q_OBJECT
private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }
  // ...
};
```

**原因**：用例中途断言失败时，`new` 出来的 widget 不会被 `delete`，泄漏的窗口会持续显示，导致：
- 批量跑 `xmake run --group=tests` 时整个套件卡住，需要手动关弹窗
- 下一个测试进程启动时 Qt 的 `DllMain` 初始化失败（Windows 错误码 `0xC000013A`）

`cleanup()` 会在每条用例结束后执行，即使断言失败也会兜底隐藏窗口。

### GUI 集成测试

排查 GUI 专属代码路径（如 tab 切换、菜单重建）时，headless 模式无法复现。
`TeXmacs/tests/*.scm`（`add_target_integration_test`）支持在真实 GUI 进程里跑：

```bash
xmake b stem
MOGAN_TEST_GUI=1 xmake r <test名>
```

- `MOGAN_TEST_GUI=1`：去掉 `-headless`，在真实 GUI 跑，调试日志直接进终端；
  且不自动 `(quit-TeXmacs)`，由测试脚本自己延迟退出。
- 不带该环境变量则保持默认 headless + 自动 quit 行为，对其他测试无影响。

测试脚本（`TeXmacs/tests/<name>.scm`，入口 `(test_<name>)`）用 `exec-delayed-at`
串异步链驱动 GUI（不要用同步 sleep，会阻塞 Qt 事件循环），链尾自己
`(quit-TeXmacs)`。夹具放 `TeXmacs/tests/tmu/`，运行时复制到 `/tmp` 避免
save/编辑污染检入副本。配合 `#ifdef LIII_DEBUG` 的临时日志定位根因
（参考 `TeXmacs/tests/2014.scm`）。

## 构建命令

主项目构建：`xmake b stem`

## 工作流程

1. 基于主分支创建新分支
2. 按规范命名分支
3. 开发完成后直接 `git push` 推送
4. 不需要使用 GitHub CLI 工具