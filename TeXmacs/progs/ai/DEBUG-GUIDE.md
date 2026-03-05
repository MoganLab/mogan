# AI Widget 调试日志使用指南

## 📋 概述

已为 AI Assistant widget 添加了完整的调试日志系统，方便开发和排查问题。

## 🎯 日志位置

调试日志输出到 Mogan 的**标准输出**（控制台/终端）。

### 在 Windows 上查看日志

1. **如果从命令行启动**：
   ```powershell
   cd d:\repos\mogan
   .\mogan.exe
   ```
   日志会直接显示在命令行窗口中。

2. **如果从快捷方式启动**：
   - 日志会输出到标准输出（可能被重定向）
   - 建议从命令行启动以便查看日志

## 🔧 启用调试日志

### 方法 1：修改代码（推荐用于开发）

编辑 `d:\repos\mogan\TeXmacs\progs\ai\ai-widget.scm`：

```scheme
;; 第 21 行
(define ai-debug? #t)  ; 将 #f 改为 #t
```

### 方法 2：在运行时通过控制台修改

如果 Mogan 有 Scheme 控制台，可以在运行时输入：

```scheme
(set! ai-debug? #t)
```

## 📊 日志示例

### 启用调试后的输出示例

```
[AI] Loading AI Assistant module...
[AI] Registering AI panel widget type...
[AI] AI Assistant module loaded successfully.
```

**按 Ctrl+I 打开面板时**：
```
[AI] toggle-ai-panel called, will set to visible
[AI] show-ai-panel called, flag=#t, current visible=#f
[AI] Opening AI panel widget
```

**发送消息时**：
```
[AI] Send message triggered, input length=12
[AI] Adding message, role=user, content length=12
[AI] Adding message, role=assistant, content length=62
[AI] Message sent successfully, history size=2
```

**清除历史时**：
```
[AI] Clearing history, current message count=2
```

**关闭面板时**：
```
[AI] toggle-ai-panel called, will set to hidden
[AI] show-ai-panel called, flag=#f, current visible=#t
[AI] Closing AI panel widget
```

## 📝 日志级别说明

当前实现使用简单的开关控制：

- `ai-debug? = #f`：关闭所有调试日志（生产模式）
- `ai-debug? = #t`：开启所有调试日志（调试模式）

## 🔍 日志分类

### 1. **模块加载日志**（`init-ai.scm`）

```
[AI] Loading AI Assistant module...
[AI] Registering AI panel widget type...
[AI] AI Assistant module loaded successfully.
```

**用途**：确认模块是否正确加载和注册。

---

### 2. **面板状态日志**（`show-ai-panel`）

```
[AI] show-ai-panel called, flag=#t, current visible=#f
[AI] Opening AI panel widget
```

**用途**：追踪面板的显示/隐藏状态变化。

---

### 3. **快捷键响应日志**（`toggle-ai-panel`）

```
[AI] toggle-ai-panel called, will set to visible
```

**用途**：确认快捷键是否被正确触发。

---

### 4. **消息处理日志**（`ai-send-message`）

```
[AI] Send message triggered, input length=12
[AI] Adding message, role=user, content length=12
[AI] Message sent successfully, history size=2
```

**用途**：追踪消息发送流程。

---

### 5. **历史记录日志**（`ai-add-message`, `ai-clear-history`）

```
[AI] Adding message, role=user, content length=12
[AI] Clearing history, current message count=2
```

**用途**：追踪聊天历史的变化。

---

## 🐛 常见问题排查

### 问题 1：按 Ctrl+I 没反应

**查看日志**：
- 如果没有看到 `[AI] toggle-ai-panel called`，说明快捷键未注册
- 如果看到了但没看到 `[AI] Opening AI panel widget`，说明面板已打开

**解决方案**：
- 检查模块是否加载成功
- 检查是否有其他快捷键冲突

---

### 问题 2：面板打开后立即关闭

**查看日志**：
```
[AI] Opening AI panel widget
[AI] Closing AI panel widget  ← 立即关闭
```

**可能原因**：
- `auxiliary-widget` 调用有问题
- 状态同步有问题

---

### 问题 3：消息发送后没有响应

**查看日志**：
- 如果看到 `Send message triggered` 但没有 `Message sent successfully`
- 可能是输入为空或刷新失败

---

## 📈 性能影响

调试日志对性能的影响：

- **关闭时**（`ai-debug? = #f`）：几乎零影响（只有一个条件判断）
- **开启时**：每次调用会输出到控制台，频繁操作可能影响性能

**建议**：
- 开发时：开启调试
- 生产/发布时：关闭调试

---

## 🎨 日志格式规范

当前使用的日志格式：

```
[AI] <组件名> <操作描述>, <关键参数>
```

例如：
```
[AI] show-ai-panel called, flag=#t, current visible=#f
```

**优点**：
- 统一的前缀 `[AI]` 便于过滤
- 包含函数名和关键参数
- 易于搜索和定位

---

## 🔮 未来扩展

如果需要更完善的日志系统，可以考虑：

1. **多级别日志**：
   ```scheme
   (define ai-log-level 'info)  ; 'debug, 'info, 'warn, 'error
   ```

2. **日志输出到文件**：
   ```scheme
   (define ai-log-port (open-output-file "ai-widget.log"))
   (display* ai-log-port "[AI] ...\n")
   ```

3. **条件日志**（只记录特定类型的操作）：
   ```scheme
   (define ai-log-messages? #t)
   (define ai-log-state-changes? #f)
   ```

---

## 📚 参考资料

- mogan 项目的日志风格：`init-research.scm` 中的 `display*` 用法
- Scheme 的 `display` 和 `display*` 函数
- 其他 widget 的调试实现：`search-widgets.scm`

---

## ✅ 检查清单

在提交代码前：

- [ ] 确保 `ai-debug?` 设置为 `#f`（除非需要调试）
- [ ] 测试日志输出是否正确
- [ ] 确认日志不影响正常功能
- [ ] 移除临时的调试代码（如果有）

---

**最后更新**: 2026-03-05  
**作者**: AI Assistant
