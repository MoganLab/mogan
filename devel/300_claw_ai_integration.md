# Claw AI 集成指南

## 概述

本文档说明如何将 Claw AI 集成到 Mogan 中。

## 文件清单

### 新增文件

```
src/Plugins/Qt/
├── QTMClawAIWidget.hpp      # Claw AI 窗口组件头文件
├── QTMClawAIWidget.cpp      # Claw AI 窗口组件实现
└── claw_ai_glue.cpp         # Scheme/C++ Glue 代码

TeXmacs/progs/claw-ai/
├── claw-ai.scm              # Scheme 主模块
└── claw-ai-tests.scm        # Scheme 单元测试

tests/claw-ai/
└── claw_ai_widget_test.cpp  # C++ 单元测试

devel/
├── 300_claw_ai.md           # 开发文档
└── 300_claw_ai_build.md     # 构建说明
```

### 需要修改的文件

```
src/Scheme/Scheme/glue.cpp              # 添加 Glue 初始化
src/Scheme/L5/init_glue_l5.cpp          # 或在这里添加 Glue
src/Plugins/Qt/qt_tm_widget.hpp         # 可选：添加成员变量
src/Plugins/Qt/qt_tm_widget.cpp         # 可选：初始化
xmake.lua                               # 添加构建目标
```

## 集成步骤

### 步骤 1：添加 Glue 代码

在 `src/Scheme/Scheme/glue.cpp` 中添加：

```cpp
#include "claw_ai_glue.cpp"

void initialize_glue () {
  // ... 现有代码 ...
  initialize_glue_plugins ();
  initialize_claw_ai_glue ();  // 添加这一行
}
```

或者创建 `src/Scheme/L5/init_glue_claw_ai.cpp`：

```cpp
void initialize_claw_ai_glue () {
  // 注册 Scheme 函数
}
```

### 步骤 2：修改 xmake.lua

在 `xmake.lua` 中添加：

```lua
-- 添加到 libmogan 目标
target("libmogan")
    -- ... 现有代码 ...
    add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
    add_files("src/Plugins/Qt/claw_ai_glue.cpp")
```

### 步骤 3：安装 Scheme 文件

确保 `TeXmacs/progs/claw-ai/` 目录下的文件被正确安装。

## 使用方法

### 在 Mogan 中启用 Claw AI

1. 启动 Mogan
2. 在 Scheme 会话中执行：

```scheme
(use-modules (claw-ai))
(claw-ai-show)
```

或使用快捷键：
- `Ctrl+`` - 切换 Claw AI 面板
- `Alt+`` - 切换 Claw AI 面板

### 发送消息

```scheme
(claw-ai-send "Hello, Claw AI!")
```

### 运行测试

```scheme
(use-modules (claw-ai-tests))
(run-claw-ai-tests)
```

或快捷命令：

```scheme
(claw-ai-test)
```

## 测试

### C++ 单元测试

```bash
# 构建测试
xmake build claw_ai_widget_test

# 运行测试
xmake run claw_ai_widget_test
```

### Scheme 单元测试

```bash
# 运行所有 Scheme 测试
xmake run --yes -vD --group=scheme_tests
```

## 故障排除

### 问题：Scheme 找不到模块

**解决**：确保 `TeXmacs/progs/claw-ai/` 目录在加载路径中。

```scheme
(add-to-load-path "/path/to/mogan/TeXmacs/progs")
(use-modules (claw-ai))
```

### 问题：C++ 函数未注册

**解决**：检查 `initialize_claw_ai_glue()` 是否被调用。

### 问题：Qt 信号不触发

**解决**：确保 QApplication 事件循环正在运行。

## 下一步开发

1. [ ] 实现 HTTP 客户端连接 OpenClaw
2. [ ] 实现流式输出
3. [ ] 添加代码高亮
4. [ ] 集成文档上下文

## 参考

- [Mogan 开发文档](devel/Test_ZH.md)
- [Qt 文档](https://doc.qt.io/)
- [TeXmacs Scheme 接口](https://www.texmacs.org/)
