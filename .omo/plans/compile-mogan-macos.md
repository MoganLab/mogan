# Mogan STEM 编译计划（macOS ARM64）

> **约束**：不修改仓库中的任何已跟踪文件（tracked files）。编译过程中生成的 `.gitignore` 忽略文件（如 `build/`、`.xmake/`、`src/System/config.h`、`src/System/tm_configure.hpp`）属于正常构建产物，视为可接受。
> **目标平台**：macOS ARM64（Apple Silicon）
> **构建系统**：xmake
> **构建目标**：`stem`（产物为 `MoganSTEM.app` / `MoganSTEM`）

---

## 关键决策

| 决策 | 取值 | 理由 |
|------|------|------|
| 构建模式 | `releasedbg` | 首次编译的平衡选择：有调试信息、体积和性能适中；`xmake.lua` 允许的 mode 为 `releasedbg` / `release` / `debug` |
| Qt 来源 | Homebrew `qt` | 通过 `--qt=$(brew --prefix qt)` 显式指定，避免 xmake 因硬编码 Qt 6.8.3 而重新从源码构建 Qt |
| 包管理器 | Homebrew | 当前环境已安装 Homebrew；macOS 官方开发指南亦推荐 |
| 是否运行 GUI | 可选 | `xmake run stem` 作为独立可选步骤，需用户确认后再启动 |
| 生成文件处理 | 允许 | `src/System/config.h`、`src/System/tm_configure.hpp` 在 `.gitignore` 中，不视为违反“不改动仓库” |

---

## 范围

**IN（本计划包含）**：
- 安装编译所需工具（xmake、Qt6、pkg-config）
- 更新 xmake 包索引
- 配置并编译 `stem` 目标
- 验证编译产物存在
- 可选的运行步骤

**OUT（本计划不包含）**：
- 修改任何仓库源码、构建脚本或配置文件
- 运行测试套件（`devel/Test_EN.md` 另行参考）
- 打包、签名、生成 DMG/安装包
- 提交代码或创建分支

---

## 前置条件

1. 已安装 **Homebrew**。
2. 已安装 **Xcode Command Line Tools**（`xcode-select --install`，若未安装）。
3. 网络可访问 GitHub / xmake 仓库 / Homebrew（无代理或已配置好代理）。
4. 磁盘空间充足（建议预留 **30–50 GB**；首次编译会下载 Qt、MuPDF、FreeType 等依赖）。
5. 电源稳定、时间充足（首次编译可能需要数小时，取决于是否命中缓存）。

---

## 执行步骤

### 步骤 1：环境检查

依次执行以下命令，确认环境满足前置条件：

```bash
# 1.1 确认 Homebrew 可用
brew --version
# 期望输出类似：Homebrew 4.x.x

# 1.2 确认 Xcode Command Line Tools 已安装
xcode-select --install
# 若已安装会提示已安装；未安装则按提示安装

# 1.3 确认架构
uname -m
# 期望输出：arm64

# 1.4 确认仓库未改动
cd /Users/mokun/git/mogan
git status --short
# 期望：仅显示已有的未跟踪文件/目录（如 .omo/），无 tracked file 的修改
```

**失败处理**：
- 若 `brew` 不存在 → 先安装 Homebrew（超出本计划范围）。
- 若 `git status` 显示 tracked file 被修改 → 先恢复或提交，再继续。

---

### 步骤 2：安装依赖

```bash
# 2.1 安装 xmake、Qt6、pkg-config
brew install xmake qt pkg-config

# 2.2 更新 shell 环境变量，使新安装命令可用
# 若使用的是 zsh（macOS 默认），执行：
eval "$(/opt/homebrew/bin/brew shellenv)"

# 2.3 验证安装
xmake --version
qmake6 --version
pkg-config --version
```

**失败处理**：
- 若 `qmake6` 不存在，尝试 `qmake --version`。
- 若 `xmake` 仍找不到，关闭并重新打开终端后再试。

---

### 步骤 3：更新 xmake 包仓库索引

```bash
xrepo update-repo
```

**失败处理**：
- 若网络失败，检查代理或重试；首次更新可能需要一段时间。

---

### 步骤 4：配置项目

```bash
cd /Users/mokun/git/mogan
xmake f -m releasedbg -vD --yes --qt="$(brew --prefix qt)"
```

说明：
- `-m releasedbg`：显式指定构建模式。
- `-vD`：输出详细调试日志，便于排错。
- `--yes`：自动确认依赖下载。
- `--qt="$(brew --prefix qt)"`：强制 xmake 使用 Homebrew 安装的 Qt，避免其因 `xmake.lua` 硬编码 Qt 6.8.3 而从源码构建 Qt。

**失败处理**：
- 若提示找不到 Qt 模块，尝试先导出 pkg-config 路径再配置：
  ```bash
  export PKG_CONFIG_PATH="$(brew --prefix qt)/lib/pkgconfig:$PKG_CONFIG_PATH"
  xmake f -m releasedbg -vD --yes --qt="$(brew --prefix qt)"
  ```
- 若 xmake 仍坚持下载 Qt 6.8.3，可接受其下载（不修改仓库文件），但耗时较长。

---

### 步骤 5：编译

```bash
xmake build stem -vD
```

说明：
- 明确指定目标 `stem`，避免构建其他非必要目标。
- 首次编译会拉取并构建大量 C++ 依赖，耗时较长。

**失败处理**：
- 若因缓存问题失败，可清理后重试：
  ```bash
  xmake f -c
  xmake c -a
  xmake build stem -vD
  ```
- 若提示内存不足，可减少并行任务数：
  ```bash
  xmake build stem -vD -j4
  ```

---

### 步骤 6：验证编译产物

编译成功后，执行：

```bash
ls -la build/macosx/arm64/releasedbg/MoganSTEM* 2>/dev/null || true
ls -la build/macosx/arm64/releasedbg/*.app 2>/dev/null || true
```

**成功标准**：
- `xmake build stem` 退出码为 `0`。
- 以下至少一个路径存在：
  - `build/macosx/arm64/releasedbg/MoganSTEM`（可执行文件）
  - `build/macosx/arm64/releasedbg/MoganSTEM.app`（应用包）

---

### 步骤 7（可选）：运行验证

> ⚠️ 此步骤会启动 GUI 应用程序，需要用户明确同意后再执行。

```bash
# 若产物是 .app 包，先进行临时签名（避免 Gatekeeper 拦截）
if [ -d "build/macosx/arm64/releasedbg/MoganSTEM.app" ]; then
  codesign --force --deep --sign - "build/macosx/arm64/releasedbg/MoganSTEM.app"
fi

# 运行
xmake run stem -m releasedbg
```

**预期行为**：
- Mogan STEM 主窗口启动，控制台无致命错误。
- 若不想启动 GUI，可跳过此步骤；编译成功即视为完成。

---

## 最终状态检查

完成编译后，确认仓库状态：

```bash
git status --short
```

**期望结果**：
- 无 tracked file 的修改。
- 新增的未跟踪目录/文件应为 `.gitignore` 已忽略内容（`.xmake/`、`build/`、`src/System/config.h`、`src/System/tm_configure.hpp`、`compile_commands.json` 等）。

---

## 已知风险与注意事项

1. **Qt 版本冲突**：
   - `xmake.lua` 硬编码 `qt6widgets 6.8.3`。
   - `xmake/vars.lua` 中仍有 `QT6_VERSION="6.5.3"`（未在 `add_requires` 中使用）。
   - 通过 `--qt="$(brew --prefix qt)"` 显式指定 Homebrew Qt，可降低冲突概率；若 Homebrew Qt 版本与 6.8.3 不完全一致，xmake 可能重新下载 Qt，但不会修改仓库文件。

2. **macOS 26.x 兼容性**：
   - 当前环境 macOS 版本较新，Qt 6.8.3 可能需要补丁或更新 minor 版本才能完美支持。

3. **首次编译耗时长**：
   - 若 xmake 决定从源码构建 Qt、MuPDF、FreeType 等，可能需要数小时和数十 GB 磁盘空间。

4. **生成文件位置**：
   - `src/System/config.h` 和 `src/System/tm_configure.hpp` 由 `xmake config` 生成，位于源码树内但已被 `.gitignore` 忽略。

---

## 最小可执行命令摘要

若环境已准备好，直接执行：

```bash
cd /Users/mokun/git/mogan

# 1. 安装依赖
brew install xmake qt pkg-config
eval "$(/opt/homebrew/bin/brew shellenv)"

# 2. 更新 xmake 仓库
xrepo update-repo

# 3. 配置
xmake f -m releasedbg -vD --yes --qt="$(brew --prefix qt)"

# 4. 编译
xmake build stem -vD

# 5. 验证产物
ls -la build/macosx/arm64/releasedbg/MoganSTEM*
```

---

## 后续可选操作

- 生成 `compile_commands.json` 供 Clangd/VSCode 使用：
  ```bash
  xmake project -k compile_commands
  ```
- 运行测试（超出本计划范围）：参考 `devel/Test_EN.md`。
- 清理构建产物（不影响 tracked files）：
  ```bash
  xmake c -a
  rm -rf build .xmake
  ```
