# TeXmacs AI 聊天功能

在 TeXmacs (Mogan Research) 中集成的简单 AI 聊天消息功能。

## 功能特性

- **全局快捷键**：`Alt+I` (Windows/Linux) / `Option+I` (macOS)
- **菜单入口**：工具 → AI 聊天框
- **浮动窗口**：使用 `auxiliary-widget` 系统实现的 400x300 像素消息窗口
- **富文本输入**：基于 `texmacs-input` 组件，支持 TeXmacs 原生格式
- **消息回显**：点击"发送"按钮显示输入内容

## 使用方法

1. 按 `Alt+I`（或 macOS 的 `Option+I`）打开聊天窗口
2. 在输入框中输入内容
3. 点击"发送"按钮查看回显
4. 点击"关闭"按钮关闭窗口

## 文件结构

```
TeXmacs/progs/simple-message/
├── init-simple-message.scm   # 模块初始化入口
├── simple-message.scm        # 主模块（加载所有组件）
├── message-widgets.scm       # 界面定义（窗口、按钮、输入框）
├── message-kbd.scm           # 键盘绑定（保留用于未来扩展）
├── message-menu.scm          # 工具菜单扩展
└── message-utils.scm         # 工具函数

TeXmacs/progs/generic/generic-kbd.scm    # 添加全局快捷键
TeXmacs/progs/init-research.scm          # 模块加载配置
```

## 核心实现

### 1. 窗口界面 (message-widgets.scm)

```scheme
(tm-widget ((simple-message-widget aux) quit)
  (padded
    (resize "400px" "300px"
      (texmacs-input `(document (paragraph "请输入内容..."))
                     `(style (tuple "generic")) aux))
    ===
    (explicit-buttons
      ("关闭" (quit))
      >>>
      ("发送" (simple-message-send aux)))))
```

### 2. 全局快捷键 (generic-kbd.scm)

```scheme
(:use (simple-message message-widgets))
...
("cmd i" (simple-message))
```

跨平台映射：
- `"cmd"` 前缀在 Windows/Linux = `Alt`
- `"cmd"` 前缀在 macOS = `Option`

### 3. 模块加载 (init-research.scm)

```scheme
;; Booting simple-message (AI chat)
(lazy-define (simple-message message-widgets) simple-message open-simple-message-window)
(lazy-menu (simple-message message-menu) tools-menu)
```

使用 `lazy-define` 和 `lazy-menu` 实现延迟加载，避免启动时加载。

### 4. 菜单扩展 (message-menu.scm)

```scheme
(tm-menu (tools-menu)
  (former)
  ---
  ("AI 聊天框" (open-simple-message-window)))
```

使用 `(former)` 保留原有菜单项，在工具菜单末尾添加新选项。

## 技术要点

### TeXmacs Widget 语法

| 元素 | 用途 | 示例 |
|------|------|------|
| `padded` | 添加内边距 | `(padded ...)` |
| `resize` | 设置尺寸 | `(resize "400px" "300px" ...)` |
| `texmacs-input` | 富文本输入框 | `(texmacs-input doc style buffer)` |
| `explicit-buttons` | 按钮组 | `(explicit-buttons ("文本" 动作))` |
| `===` | 垂直分隔 | 布局元素 |
| `>>>` | 水平填充 | 将按钮推到右侧 |

### 辅助窗口系统

```scheme
(auxiliary-widget widget-promise quit-cmd title buffer)
```

- `widget-promise`: 窗口内容定义
- `quit-cmd`: 关闭时执行的命令
- `title`: 窗口标题
- `buffer`: 关联的缓冲区

## 遇到的问题与解决

### 1. 快捷键跨平台定义

**问题**：`"C-M-i"` 在不同平台表现不一致
- macOS: Ctrl+Cmd（不是 Alt）
- Linux: Ctrl+Alt

**解决**：使用 `"cmd i"`，跨平台统一为 Alt+I / Option+I

### 2. Widget 内不能直接调用函数

**问题**：在 `tm-widget` 内调用 `(debug-log ...)` 报错

**错误信息**：
```
gui-make: invalid menu item ~S ((debug-log "..."))
```

**解决**：`tm-widget` 内只能包含 widget 元素，不能调用 Scheme 函数。移除所有调试日志。

### 3. 按钮语法错误

**问题**：使用 `(button "文本" 动作)` 报错

**错误信息**：
```
gui-make: invalid menu item ~S ((button "关闭" (quit)))
```

**解决**：使用 `explicit-buttons` 语法：
```scheme
(explicit-buttons
  ("关闭" (quit))
  >>>
  ("发送" (simple-message-send)))
```

### 4. `buffer-new` 参数错误

**问题**：`(buffer-new "url")` 报错

**错误信息**：
```
buffer-new: too many arguments
```

**解决**：`buffer-new` 是无参数函数，改为 `(buffer-new)`

### 5. 菜单不显示

**问题**：工具菜单中没有出现 "AI 聊天框"

**解决**：在 `init-research.scm` 中添加 `lazy-menu` 声明：
```scheme
(lazy-menu (simple-message message-menu) tools-menu)
```

## 开发经验

### TeXmacs 模块开发流程

1. **创建模块目录**：`TeXmacs/progs/simple-message/`
2. **定义模块文件**：使用 `(texmacs-module ...)`
3. **添加功能代码**：widgets、kbd、menu 等
4. **注册到系统**：在 `init-research.scm` 添加 `lazy-define` / `lazy-menu`
5. **添加快捷键**：在 `generic-kbd.scm` 添加全局快捷键

### 调试技巧

- Scheme 错误会显示在 TeXmacs 控制台
- 使用 `(:interactive #t)` 标记交互式函数
- `tm-widget` 内不能直接调用函数，只能在按钮动作中使用

## 后续扩展方向

### 短期目标
- [ ] 集成真实 AI 服务 API
- [ ] 添加消息历史记录
- [ ] 支持 Markdown 格式显示
- [ ] Esc 键关闭窗口

### 中期目标
- [ ] 多轮对话上下文维护
- [ ] 代码块语法高亮
- [ ] 导出对话记录
- [ ] 多 AI 提供商支持

### 长期目标
- [ ] 与 TeXmacs 文档深度集成
- [ ] LaTeX 公式渲染
- [ ] 语音输入
- [ ] 团队协作

## 参考资料

- `TeXmacs/progs/generic/search-widgets.scm` - 搜索功能实现参考
- `TeXmacs/progs/kernel/gui/menu-define.scm` - Widget 语法定义
- `TeXmacs/progs/kernel/gui/menu-widget.scm` - 菜单系统
- `TeXmacs/progs/texmacs/keyboard/prefix-kbd.scm` - 键盘前缀定义

## 作者

- 开发时间：2026-03-24
- 基于：Mogan Research / TeXmacs
