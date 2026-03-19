# AI 聊天框 UI 实现文档

> 合并原 01-architecture 与 02-ui-design，专注 UI 部分落地细节。

---

## 1. 设计总览
- **职责分离**：C++ 仅提供可复用原子组件；Scheme 负责拼装布局、驱动业务。
- **原子组件**：
  1. `chat-message-view` – 可滚动消息列表（append / update-last / clear）
  2. `chat-input-area` – 多行输入 + 发送/清空按钮
- **无业务状态**：组件不保存 AI 语义，仅响应 slot 与发出信号。

---

## 2. 组件树与布局
```
┌─ chat-message-view (QWidget) ----------------┐
│ ├─ QScrollArea                              │
│ │  └─ QWidget (消息容器)                    │
│ │     └─ QVBoxLayout (垂直排列消息气泡)     │
│ │        ├─ ChatMessageBubble (用户)        │ 右对齐
│ │        ├─ ChatMessageBubble (AI)          │ 左对齐
│ │        └─ ...                             │
└---------------------------------------------┘

┌─ chat-input-area (QWidget) ----------------┐
│ ├─ QHBoxLayout                             │
│ │  ├─ QTextEdit (多行输入)                 │ 占位符：
│ │  │  "输入消息，Ctrl+Enter 发送"           │
│ │  └─ QVBoxLayout (按钮)                   │
│ │     ├─ QPushButton "发送" (Send)         │
│ │     └─ QPushButton "清空" (Clear)        │
└--------------------------------------------┘
```

---

## 3. 消息气泡规范
| 属性 | 用户消息 | AI 消息 |
|---|---|---|
| 对齐 | 右对齐 | 左对齐 |
| 背景 | `#0078D4` | `#F3F2F1` |
| 文字 | 白色 | 黑色 |
| 圆角 | 8 px | 8 px |
| 内边距 | 12 px | 12 px |
| 最大宽度 | 父容器 75 % | 父容器 75 % |
| 间距 | 相邻气泡 8 px | 相邻气泡 8 px |

富文本支持：
- 载体：`QTextEdit`（`setReadOnly(true)`）
- 支持 HTML 子集：`<b> <i> <code> <pre>`
- 代码块：`<pre style="background:#F3F3F3;padding:8px">`
- 行内代码：`<code style="background:#F3F3F3;padding:2px 4px">`

---

## 4. 交互与快捷键
| 快捷键 | 行为 |
|---|---|
| Ctrl+Enter | 发送消息 |
| Ctrl+Shift+Enter | 输入换行 |
| Tab | 输入框 ↔ 按钮间切换 |
| Esc | 清空输入框 |

按钮状态：
- **发送按钮**：空输入禁用（灰色），非空启用（蓝色），流式等待禁用（"等待回复..."）
- **清空按钮**：始终启用

占位符：
- 空输入：`"输入消息，Ctrl+Enter 发送"`
- 流式等待：`"AI 正在思考..."`（只读）

---

## 5. 滚动与性能
- **自动滚动**：新增消息时，若用户未手动滚动历史，则滚动到底部
- **手动锁定**：用户滚动查看历史时暂停自动滚动
- **恢复机制**：用户滚动回底部后重新启用自动滚动
- **性能**：
  - 最多保留 1000 条消息，超出从顶部移除
  - 使用 `QTimer::singleShot` 合并多次更新，避免频繁 repaint

---

## 6. C++ 原子组件接口
### 6.1 chat-message-view
```cpp
class ChatMessageView : public QWidget {
    Q_OBJECT
public:
    explicit ChatMessageView(QWidget* parent = nullptr);
public slots:               // Scheme 通过 glue 调用
    QString appendMessage(const QString& role, const QString& content);  // 返回消息ID
    void updateMessage(const QString& messageId, const QString& content); // 通过ID更新
    void updateLastMessage(const QString& content);  // 保持向后兼容
    void clearMessages();
private slots:
    void scrollToBottomIfNeeded();   // 内部自动滚动逻辑
};
```

### 6.2 chat-input-area
```cpp
class ChatInputArea : public QWidget {
    Q_OBJECT
public:
    explicit ChatInputArea(QWidget* parent = nullptr);
    QString inputText() const;
public slots:               // Scheme 通过 glue 调用
    void setInputText(const QString& text);
    void setEnabled(bool enabled);
    void setValidator(const QString& pattern);  // 正则验证输入
signals:                    // glue 自动转发到 Scheme
    void textSubmitted(const QString& text);    // 命名与现有input组件保持一致
    void clearRequested();
};
```

---

## 7. Glue ↔ Scheme 接口清单
| 功能 | C++ 侧 | Scheme 侧 | 说明 |
|---|---|---|---|
| 创建组件 | `ChatMessageView()` | `(tm-chat-message-view)` | 返回 widget id |
| | `ChatInputArea()` | `(tm-chat-input-area)` | 返回 widget id |
| 消息区 slot | `appendMessage` | `(tm-chat-view-append! id role content)` | 追加气泡，返回消息ID |
| | `updateMessage` | `(tm-chat-view-update! id message-id content)` | 通过ID更新消息 |
| | `updateLastMessage` | `(tm-chat-view-update-last! id content)` | 更新最后一条（兼容） |
| | `clearMessages` | `(tm-chat-view-clear! id)` | 清空列表 |
| 输入区 slot | `setInputText` | `(tm-chat-input-set-text! id text)` | 设置输入框 |
| | `inputText` | `(tm-chat-input-get-text id)` | 获取输入框 |
| | `setEnabled` | `(tm-chat-input-set-enabled! id bool)` | 启用/禁用 |
| | `setValidator` | `(tm-chat-input-set-validator! id pattern)` | 设置输入验证 |
| 信号转发 | `textSubmitted` → Scheme | `(chat-handle-input text)` | 用户输入事件 |
| | `clearRequested` → Scheme | `(chat-clear)` | 清空事件 |

---

## 8. Scheme 业务拼装（阶段 1 固定回复版）
```scheme
;; 全局消息列表
(define *chat-messages* '())

;; 创建聊天 UI
(define (make-chat-ui)
  (let* ((view   (tm-chat-message-view))
         (input  (tm-chat-input-area))
         (layout (vertical-layout view input)))
    layout))

;; 处理用户输入（固定回复）
(define (chat-handle-input text)
  (chat-append-message! 'user text)
  (chat-input-set-enabled! input-id #f)          ; 禁用输入
  (chat-append-message! 'assistant "") 获取AI消息ID用于更新
  (let ((ai-id (chat-view-append! view-id 'assistant "")))
    (let loop ((chars "Hello from mock AI")
               (index 0))
      (when (< index (string-length chars))
        (chat-view-update! view-id ai-id (string-take chars (+ index 1)))
        (sleep 0.05)
        (loop chars (+ index 1)))))
  (chat-input-set-enabled! input-id #t))         ; 恢复输入
```

---

## 9. 样式接口（运行时切换）
```scheme
;; 设置气泡样式字符串
(set-chat-message-style! view-id "user"
  "background:#0078D4;color:white;border-radius:8px;padding:12px;")
(set-chat-message-style! view-id "assistant"
  "background:#F3F2F1;color:black;border-radius:8px;padding:12px;")
```

---

## 10. 无障碍与响应式
- **最小尺寸**：消息区 ≥ 300×200 px，输入区 ≥ 300×100 px
- **键盘导航**：完整 Tab 顺序
- **屏幕阅读器**：气泡 `role="log"` + `aria-label`
- **高对比度**：自动适配系统高对比度

---

## 11. 下一步任务（阶段 1）
1. 实现 `ChatMessageView` 头文件与 cpp
2. 实现 `ChatInputArea` 头文件与 cpp
3. 实现 `glue_chat.cpp` 注册所有接口
4. 实现 Scheme 侧 `chat-ui.scm` 固定回复逻辑
5. 集成测试：发送 → 固定回复 → 逐字符显示 → 完成