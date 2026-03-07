# Claw AI 构建配置

## xmake.lua 修改

在 `mogan/xmake.lua` 中添加以下内容：

```lua
-- Claw AI Widget
target("claw_ai_widget")
    set_kind("object")
    add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
    add_includedirs("src/Plugins/Qt")
    add_packages("qt6widgets")
    
-- Claw AI 测试
target("claw_ai_widget_test")
    set_kind("binary")
    add_files("tests/claw-ai/claw_ai_widget_test.cpp")
    add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
    add_includedirs("src/Plugins/Qt")
    add_packages("qt6widgets", "gtest")
    add_deps("libmogan")
```

## 文件清单

### C++ 文件
- `src/Plugins/Qt/QTMClawAIWidget.hpp` - 头文件
- `src/Plugins/Qt/QTMClawAIWidget.cpp` - 实现
- `tests/claw-ai/claw_ai_widget_test.cpp` - 单元测试

### Scheme 文件
- `TeXmacs/progs/claw-ai/claw-ai.scm` - 主模块
- `TeXmacs/progs/claw-ai/claw-ai-tests.scm` - 单元测试

## 编译步骤

```bash
# 1. 配置
xmake config -vD --yes

# 2. 构建 Claw AI 组件
xmake build claw_ai_widget

# 3. 运行 C++ 单元测试
xmake run claw_ai_widget_test

# 4. 运行 Scheme 单元测试
xmake run --yes -vD --group=scheme_tests
```

## 集成到 Mogan

### 1. 修改 qt_tm_widget.hpp

在 `qt_tm_widget_rep` 类中添加：

```cpp
#include "QTMClawAIWidget.hpp"

class qt_tm_widget_rep : public qt_window_widget_rep {
    // ... 现有成员 ...
    QTMAuxiliaryWidget* auxiliaryWidget;
    QTMClawAIWidget* clawAIWidget;  // 新增
    // ...
};
```

### 2. 修改 qt_tm_widget.cpp

在构造函数中添加：

```cpp
// 创建 Claw AI 窗口
clawAIWidget = new QTMClawAIWidget(0);
clawAIWidget->setAllowedAreas(Qt::RightDockWidgetArea);
clawAIWidget->setFloating(false);
mw->addDockWidget(Qt::RightDockWidgetArea, clawAIWidget);
clawAIWidget->setVisible(false);

// 连接信号
connect(clawAIWidget, &QTMClawAIWidget::messageSent,
        [](const QString& msg) {
            exec_delayed(scheme_cmd(
                "(claw-ai-send \"" + msg.toUtf8().constData() + "\")"));
        });
```

### 3. 添加 Scheme Glue

在 `src/Scheme/L5/init_glue_l5.cpp` 中添加：

```cpp
tmscm_to_blackbox<QTMClawAIWidget*>(...);
```

## 测试

### C++ 单元测试

```bash
# 运行所有 C++ 测试
bash bin/test_all

# 只运行 Claw AI 测试
bash bin/test_only claw_ai_widget_test
```

### Scheme 单元测试

在 Mogan 中执行：

```scheme
(use-modules (claw-ai-tests))
(run-claw-ai-tests)
```

或快捷命令：

```scheme
(claw-ai-test)
```

## 下一步

1. [ ] 实现 C++ Glue 代码（Scheme/C++ 绑定）
2. [ ] 实现 HTTP 客户端
3. [ ] 集成 OpenClaw API
4. [ ] 实现流式输出
5. [ ] 添加代码高亮
