# AI 聊天功能实现进度记录

## 项目概述

在 TeXmacs (Mogan Research) 中实现一个 AI 聊天功能，通过快捷键调出聊天窗口。

---

## 技术调研阶段 (已完成)

### 1. 搜索功能架构分析

参考 TeXmacs 搜索功能的实现机制：

```
┌─────────────────────────────────────────┐
│         Scheme 层 (用户界面)            │
│   search-widgets.scm / search-kbd.scm  │
├─────────────────────────────────────────┤
│         C++ 接口层                      │
│   edit_interface.hpp / edit_search.cpp  │
├─────────────────────────────────────────┤
│         核心算法层                      │
│      tree_search.cpp / tree_search.hpp  │
├─────────────────────────────────────────┤
│         数据结构层                      │
│         tree / range_set                │
└─────────────────────────────────────────┘
```

### 2. Ctrl+F 触发机制研究

完整调用链：
```
用户按 Ctrl+F
    ↓
generic-kbd.scm ("std f" (interactive-search))
    ↓
search-widgets.scm (interactive-search)
    ↓
search-widgets.scm (open-search)
    ↓
auxiliary-widget 系统
    ↓
显示搜索窗口
```

关键发现：
- 使用 `auxiliary-widget` 系统管理浮动窗口
- `texmacs-input` 组件用于富文本输入
- 键盘绑定使用 `(kbd-map ...)` 定义

---

## 实现阶段 (已完成)

### 第一阶段：基础框架搭建

#### 文件结构
```
TeXmacs/progs/simple-message/
├── init-simple-message.scm   # 初始化入口
├── simple-message.scm        # 主模块，加载所有组件
├── message-kbd.scm           # 键盘绑定
├── message-menu.scm          # 菜单扩展
├── message-utils.scm         # 工具函数
└── message-widgets.scm       # 界面定义
```

#### 核心实现

**1. 消息窗口界面 (message-widgets.scm)**

```scheme
(tm-widget ((simple-message-widget aux) quit)
  (padded
    (resize "400px" "300px"
      (texmacs-input `(document (paragraph "请输入内容..."))
                     `(style (tuple "generic")) aux))
    ===
    (hlist
      (button "关闭" (quit))
      >>>
      (button "发送" (simple-message-send aux)))))
```

**2. 窗口打开函数**

```scheme
(tm-define (simple-message)
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (let* ((aux (buffer-new)))
    (buffer-set-master aux (current-buffer))
    (auxiliary-widget (simple-message-widget aux)
                      (lambda () (buffer-close aux))
                      (translate "Simple Message") aux)))
```

**3. 全局快捷键绑定 (generic-kbd.scm)**

```scheme
("cmd i" (simple-message))
```

快捷键映射：
- macOS: Option + I
- Windows/Linux: Alt + I

**4. 菜单扩展 (message-menu.scm)**

```scheme
(tm-menu (tools-menu)
  (former)
  ---
  ("AI 聊天框" (open-simple-message-window)))
```

**5. 模块加载配置 (init-research.scm)**

```scheme
;; Booting simple-message (AI chat)
(lazy-define (simple-message message-widgets) simple-message open-simple-message-window)
(lazy-menu (simple-message message-menu) tools-menu)
```

---

## 遇到的问题及解决方案

### 问题 1：快捷键定义方式

**问题**：最初使用 `"C-M-i"` 定义快捷键，但跨平台不一致。

**分析**：
- `"C-M-i"` = Ctrl+Meta
  - macOS: Ctrl+Cmd（不是 Alt）
  - Linux: Ctrl+Alt

**解决**：使用 `"cmd i"`，跨平台统一为 Alt+I / Option+I

### 问题 2：全局快捷键 vs 局部快捷键

**问题**：快捷键应该全局触发还是只在特定模式有效？

**分析**：
- 搜索功能：全局快捷键 `"std f"` 在 generic-kbd.scm 定义
- 搜索框内部快捷键：在 search-kbd.scm 定义，使用 `:require` 限制

**解决**：采用全局触发方式，快捷键定义在 generic-kbd.scm

### 问题 3：菜单不显示

**问题**：工具菜单中没有出现 "AI 聊天框" 选项。

**原因**：`message-menu.scm` 没有被加载到系统中。

**解决**：在 init-research.scm 添加 `lazy-menu` 声明：
```scheme
(lazy-menu (simple-message message-menu) tools-menu)
```

### 问题 4：buffer-new 参数错误

**问题**：运行时错误 `(buffer-new "tmfs://simple-message")` 参数过多。

**原因**：`buffer-new` 是无参数函数。

**解决**：修改为 `(buffer-new)`

---

## 修改的文件列表

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `TeXmacs/progs/generic/generic-kbd.scm` | 修改 | 添加 `:use` 模块引用和 `("cmd i" (simple-message))` 快捷键 |
| `TeXmacs/progs/init-research.scm` | 修改 | 添加 simple-message 模块的 lazy-define 和 lazy-menu |
| `TeXmacs/progs/simple-message/init-simple-message.scm` | 新增 | 初始化入口 |
| `TeXmacs/progs/simple-message/simple-message.scm` | 新增 | 主模块 |
| `TeXmacs/progs/simple-message/message-kbd.scm` | 新增 | 键盘绑定（保留用于未来内部快捷键） |
| `TeXmacs/progs/simple-message/message-menu.scm` | 新增 | 菜单扩展 |
| `TeXmacs/progs/simple-message/message-utils.scm` | 新增 | 工具函数 |
| `TeXmacs/progs/simple-message/message-widgets.scm` | 新增 | 界面定义 |

---

## 功能验证

### 使用方法

1. **快捷键**：按 `Alt+I`（Windows/Linux）或 `Option+I`（macOS）
2. **菜单**：工具 → AI 聊天框

### 预期行为

1. 弹出 400x300 像素的浮动窗口
2. 窗口内包含 `texmacs-input` 输入区域
3. 点击"发送"按钮回显输入内容
4. 点击"关闭"按钮关闭窗口

---

## 后续扩展方向

### 短期目标
- [ ] 集成真实 AI 服务 API
- [ ] 添加消息历史记录功能
- [ ] 支持 Markdown 格式显示
- [ ] 添加快捷键关闭窗口 (Esc)

### 中期目标
- [ ] 多轮对话上下文维护
- [ ] 支持代码块语法高亮
- [ ] 导出对话记录为文档
- [ ] 多 AI 提供商支持

### 长期目标
- [ ] 与 TeXmacs 文档深度集成
- [ ] 支持 LaTeX 公式渲染
- [ ] 语音输入集成
- [ ] 团队协作功能

---

## 技术要点总结

### TeXmacs 扩展开发模式

1. **Scheme 模块结构**：
   - 使用 `(texmacs-module ...)` 定义模块
   - 使用 `(:use ...)` 导入依赖

2. **键盘绑定**：
   - 全局快捷键：在 `generic-kbd.scm` 使用 `kbd-map`
   - 局部快捷键：在自定义文件中使用 `(:require ...)` 限制

3. **菜单扩展**：
   - 使用 `(tm-menu (原菜单名) ...)` 语法
   - 使用 `(former)` 保留原有菜单项
   - 在 `init-research.scm` 注册 `lazy-menu`

4. **辅助窗口**：
   - 使用 `auxiliary-widget` 系统创建浮动窗口
   - `texmacs-input` 组件提供富文本编辑能力
   - `buffer-new` 创建独立缓冲区

### 跨平台注意事项

| 前缀 | macOS | Windows/Linux |
|------|-------|---------------|
| `"std"` | Command | Ctrl |
| `"cmd"` | Option | Alt |
| `"special"` | Option+Control | Alt+Ctrl |
| `"extra"` | Command+Option | Meta+Alt |

---

## 参考资料

- `TeXmacs/progs/generic/search-widgets.scm` - 搜索功能实现
- `TeXmacs/progs/generic/search-kbd.scm` - 搜索键盘绑定
- `TeXmacs/progs/kernel/gui/menu-widget.scm` - 菜单系统
- `TeXmacs/progs/texmacs/keyboard/prefix-kbd.scm` - 键盘前缀定义

---

## 更新时间

2026-03-24
