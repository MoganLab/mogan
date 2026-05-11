# Mogan AI Sidebar 总体架构

## 1. 架构概述

AI Sidebar 是 Mogan 的嵌入式 AI 对话组件，采用**分层架构**设计，严格分离 UI、业务逻辑和数据，复用 TeXmacs 原生能力实现富文本聊天体验。

```
┌─────────────────────────────────────────────────────────────────┐
│                    UI 容器层 (Qt/C++)                           │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ ChatSidebarWidget                                         │  │
│  │  ├─ Message Area: texmacs_custom_message_widget           │  │
│  │  └─ Input Area:   texmacs_custom_input_widget             │  │
│  │                                                           │  │
│  │ 职责: 纯容器，无业务逻辑，仅转发事件到 Scheme 层           │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     适配层 (Scheme)                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ chat-tree.scm          - 数据模型 ↔ TeXmacs Tree 转换      │  │
│  │ chat-buffer.scm        - Buffer 生命周期管理               │  │
│  │                                                           │  │
│  │ 职责: 数据序列化与底层资源管理                              │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     业务层 (Scheme)                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ chat-controller.scm    - 核心业务流程编排                  │  │
│  │                          ├─ 用户输入处理                   │  │
│  │                          ├─ 流式响应编排                   │  │
│  │                          └─ 消息状态管理                   │  │
│  │                                                           │  │
│  │ 职责: 编排服务层完成完整业务流程                            │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     服务层 (Scheme)                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ llm-client.scm         - HTTP/SSE 客户端                   │  │
│  │ markdown-parser.scm    - Markdown 流式解析                 │  │
│  │                                                           │  │
│  │ 职责: 纯技术能力，无状态，可独立复用                         │  │
│  └───────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                     数据层 (Scheme)                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │ chat-domain.scm        - 纯数据模型定义（record-type）     │  │
│  │ chat-state.scm         - 全局状态管理                      │  │
│  │                                                           │  │
│  │ 职责: 数据定义和状态存储，无业务逻辑                        │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## 2. 分层职责详解

### 2.1 UI 容器层 (Qt/C++)

**文件**: `src/Plugins/Qt/ChatSidebarWidget.cpp`

**核心原则**: 纯容器，**不包含任何业务逻辑**

**职责边界**:
| 属于本层 | 不属于本层 |
|---------|-----------|
| Qt 窗口容器管理 | 消息数据处理 |
| TeXmacs Widget 嵌入 | HTTP 请求 |
| 布局与尺寸调整 | Markdown 解析 |
| 事件转发到 Scheme | 消息状态管理 |

**关键实现**:
```cpp
// 转发用户输入到 Scheme
void ChatSidebarWidget::onSendButtonClicked(QString text) {
    call("chat-controller-on-user-input",
         object(text.toStdString()));
}

// 接收 Scheme 调用更新 UI
void chat_sidebar_append_tree(string buffer_name, tree content) {
    // 仅负责将 tree 显示到 message_widget
}
```

---

### 2.2 适配层 (Scheme)

**核心原则**: 负责**数据 ↔ Tree ↔ Buffer** 的转换，不包含业务逻辑

#### chat-tree.scm
**替代原**: `chat-render.scm`（重命名，避免与 TeXmacs 渲染混淆）

**职责**:
- 数据模型 → TeXmacs Tree 的**纯转换**（序列化）
- 应用样式包装（用户/AI 消息区分）
- **增量更新**: 使用 `tree-insert!` 而非 `buffer-set-body`

**关键函数**:
```scheme
;; 纯函数：模型 → Tree
(tm-define (chat-block->tree block)
  ;; 返回：(chat-user-box ...) 或 (chat-ai-box ...)
  )

;; 副作用：追加到 document
(tm-define (chat-tree-append! doc block-tree)
  ;; 使用 tree-insert! 实现增量追加
  (tree-insert! doc (tree-arity doc) (list block-tree)))
```

**与 TeXmacs 的关系**:
```
数据模型 --chat-tree.scm--> TeXmacs Tree --TeXmacs内核--> 屏幕像素
    ↑                                              ↑
  我们实现                                      已有能力
```

#### chat-buffer.scm
**替代原**: `chat-widgets.scm` 的部分职责

**职责**:
- Buffer 的创建、查找、销毁
- Buffer 与 Tree 的映射关系维护
- 滚动位置保存/恢复

**关键函数**:
```scheme
(tm-define (chat-buffer-ensure name))
(tm-define (chat-buffer-append name tree-content))
(tm-define (chat-buffer-save-scroll name))
(tm-define (chat-buffer-restore-scroll name))
```

---

### 2.3 业务层 (Scheme)

**核心原则**: 编排服务层完成完整业务流程，是**唯一能调用服务层**的层级

#### chat-controller.scm
**替代原**: `chat-stream.scm` + `chat-llm.scm` 的业务部分

**职责**:
- 处理用户输入完整流程
- 管理流式响应生命周期
- 协调 Markdown 解析、Tree 转换、Buffer 更新

**关键业务流程**:
```scheme
(tm-define (chat-controller-send-message text)
  ;; 1. 创建用户 block (chat-domain)
  ;; 2. 转换并追加到 UI (chat-tree + chat-buffer)
  ;; 3. 启动 LLM 请求 (llm-client)
  ;; 4. 流式处理响应
  ;;    ├─ 接收 chunk (llm-client callback)
  ;;    ├─ 解析 Markdown (markdown-parser)
  ;;    ├─ 转换 Tree (chat-tree)
  ;;    └─ 增量更新 (chat-buffer)
  ;; 5. 完成/错误处理
  )

(tm-define (chat-controller-on-chunk block-id chunk)
  ;; 内部使用服务层完成增量更新
  )
```

**为何合并 stream 和 llm**:
- "发送消息"是一个完整业务场景
- 不应分散在多个文件
- 服务层 (`llm-client`, `markdown-parser`) 保持独立可复用

---

### 2.4 服务层 (Scheme)

**核心原则**: 提供**纯技术能力**，无状态，可被其他功能复用

#### llm-client.scm
**提取自**: 原 `chat-llm.scm` 的 HTTP 部分

**职责**:
- HTTP/SSE 流式请求
- 配置管理（API Key、Base URL）
- 多模型抽象（OpenAI、DeepSeek 等）

**关键函数**:
```scheme
;; 纯技术接口，无业务逻辑
(tm-define (llm-stream config payload on-chunk on-done on-error))
(tm-define (llm-complete config payload))
```

**特点**:
- 可被 Session 插件复用
- 可被其他需要 LLM 的功能复用
- 不依赖 chat 业务

#### markdown-parser.scm
**提取自**: 原 `chat-stream.scm` 的解析部分

**职责**:
- Markdown 流式解析
- 分块识别（段落、代码块、公式等）
- 与 `liii/llm-chat` 的 tokenizer 保持一致

**关键函数**:
```scheme
;; 流式解析
(tm-define (markdown-stream-init))
(tm-define (markdown-stream-chunk state text)
  ;; 返回: (new-state blocks)
  )
(tm-define (markdown-stream-flush state)
  ;; 返回: 剩余 blocks
  )
```

---

### 2.5 数据层 (Scheme)

**核心原则**: 纯数据定义和状态存储，**无业务逻辑**

#### chat-domain.scm
**替代原**: `chat-model.scm`（重命名，更准确）

**职责**:
- 定义 `chat-block`, `chat-message`, `chat-session` 等 record-type
- 提供不可变更新函数
- 数据验证

```scheme
(define-record-type chat-block
  (make-chat-block id actor items status timestamp)
  chat-block?
  (id block-id)
  (actor block-actor)       ; 'user | 'assistant | 'system
  (items block-items set-block-items)  ; 列表
  (status block-status set-block-status))  ; 'pending | 'streaming | 'complete

;; 纯函数：创建新 block，不修改原数据
(tm-define (chat-block-append-item block item)
  (make-chat-block
    (block-id block)
    (block-actor block)
    (append (block-items block) (list item))
    (block-status block)
    (block-timestamp block)))
```

#### chat-state.scm
**替代原**: `chat-model.scm` 的状态部分

**职责**:
- 全局可变状态（当前会话、block-id → tree 映射等）
- 流式解析状态

```scheme
;; 全局状态
(define chat-current-session #f)
(define chat-block-tree-map (make-ahash-table))  ; id -> tree 引用
(define chat-stream-state (make-stream-tokenizer))
(define chat-stream-buffer "")
```

**注意**: 状态应尽量收敛，业务逻辑放在 `chat-controller` 中修改状态

## 3. 关键技术决策

### 3.1 增量更新机制（核心优化）

**问题**: 整量替换 `buffer-set-body` 导致闪烁、滚动丢失、选择状态重置

**解决方案**: 使用 `tree-insert!` 实现真正的增量追加

```scheme
;; 旧方式（整量替换）- chat-render.scm 时代
(buffer-set-body name (chat-session->message-document session))

;; 新方式（增量追加）- chat-tree.scm + chat-buffer.scm
(let ((doc (chat-buffer-get name))
      (tree (chat-block->tree block)))
  (chat-tree-append! doc tree))
```

**实现要点**:
1. **映射表**: `chat-block-tree-map` 维护 `block-id -> tree` 快速定位
2. **缓冲策略**: 流式内容按行缓冲（`buf-port` 逻辑），避免逐字更新
3. **状态保持**: `chat-buffer` 负责保存/恢复滚动位置和选择状态

### 3.2 流式处理流程（新架构）

新架构下流式处理由 **业务层编排服务层** 完成：

```
HTTP Chunk 到达
    │
    ▼
llm-client (SSE 解析)
    │
    ▼
chat-controller-on-chunk (业务回调)
    │
    ├─ chat-state 更新 block 状态
    │
    ▼
markdown-parser-stream-chunk
    │
    ▼
chat-block->tree (模型 → Tree)
    │
    ▼
chat-buffer-append (tree-insert!)
    │
    ▼
局部重绘
```

**关键点**: 每一层只调用下一层，不跨层调用

### 3.3 "渲染"术语澄清

在 TeXmacs 语境下明确术语：

| 层级 | 术语 | 对应文件 | 说明 |
|------|------|---------|------|
| 数据模型 → Tree | **序列化/适配** | `chat-tree.scm` | 我们将模型转为 Tree |
| Tree → 样式 → 屏幕 | **排版** | TeXmacs typesetter | TeXmacs 内核处理 |
| 最终显示 | **渲染** | Qt/TeXmacs 渲染管线 | 底层图形 API |

**原 `chat-render.scm` 更名为 `chat-tree.scm**，避免与 TeXmacs 的"渲染"混淆

### 3.3 样式系统

采用 TeXmacs 原生样式系统：

```scheme
;; packages/session/chat.ts
(assign|chat-user-box|
  (macro|body|
    (with|"bg-color"|"#e3f2fd"
          |"par-left"|"1fn"
          |"par-right"|"1fn"
          |"block-padding"|"0.5fn"
      (surround||<arg|body>))))

(assign|chat-ai-box|
  (macro|body|
    (with|"bg-color"|"#ffffff"
          |"par-left"|"1fn"
          |"par-right"|"1fn"
      (surround||<arg|body>))))
```

### 3.4 Message Widget 只读优化

`texmacs_custom_message_widget` 已设置 `message_widget = true`，但还需要：

1. **禁用鼠标交互**: 设置 `Qt::WA_TransparentForMouseEvents`
2. **禁用光标**: 已自动处理（`cursor_blink_visible = false`）
3. **禁用键盘**: 已自动处理（直接返回不处理）

## 4. 与现有代码集成

### 4.1 复用 LLM Session 组件

| 组件 | 来源 | 复用方式 |
|------|------|----------|
| stream-tokenizer | liii/llm-chat | 直接导入或重写 |
| markdown parser | liii/llm-chat | 复用逻辑 |
| HTTP 流式 | liii/llm-chat | 复用 http-post :stream |
| SSE 解析 | liii/llm-chat | 直接复用 |

### 4.2 与 Session 机制的区别

| 特性 | Session | Sidebar |
|------|---------|---------|
| 输出目标 | `docs[channel]` | `tmfs://aux/chat-sidebar-body` |
| 触发方式 | 管道驱动 | 函数调用 |
| 更新机制 | `connection-notify` | 直接 `tree-insert!` |
| 增量能力 | 原生支持 | 需自行实现 |

### 4.3 集成点

```scheme
;; 在现有 chat-widgets.scm 基础上扩展
(tm-define (chat-sidebar-refresh-incremental! block-id content)
  ;; 查找或创建 block
  (with block-tree (chat-sidebar-find-block block-id)
    (if block-tree
        (chat-block-append-content! block-tree content)
        (chat-sidebar-append-block! block-id content))))
```

## 5. 模块拆分与演进计划

### 5.1 新旧文件对照

| 新文件 | 替代原文件 | 职责变化 |
|--------|-----------|---------|
| `chat-domain.scm` | `chat-model.scm` | 纯数据定义，提取不可变更新函数 |
| `chat-state.scm` | `chat-model.scm` | 分离可变状态，收敛全局变量 |
| `chat-tree.scm` | `chat-render.scm` | 重命名，明确"序列化"而非"渲染" |
| `chat-buffer.scm` | `chat-widgets.scm` (部分) | 提取 Buffer 管理职责 |
| `chat-controller.scm` | `chat-stream.scm` + `chat-llm.scm` | 合并业务编排 |
| `llm-client.scm` | `chat-llm.scm` (部分) | 提取通用 HTTP/SSE 客户端 |
| `markdown-parser.scm` | `chat-stream.scm` (部分) | 提取纯 Markdown 解析能力 |

### 5.2 分阶段实现计划

#### Phase 1: 数据层重构（1-2 天）
- [ ] 创建 `chat-domain.scm`：定义 record-type，提取不可变更新
- [ ] 创建 `chat-state.scm`：迁移全局状态，收敛到一处
- [ ] 单元测试：确保数据模型正确性

#### Phase 2: 适配层重构（2-3 天）
- [ ] 创建 `chat-tree.scm`：从 `chat-render.scm` 迁移，重命名函数
- [ ] 创建 `chat-buffer.scm`：提取 Buffer 生命周期管理
- [ ] 验证：现有功能正常工作

#### Phase 3: 服务层提取（2-3 天）
- [ ] 创建 `llm-client.scm`：提取 HTTP/SSE 通用能力
- [ ] 创建 `markdown-parser.scm`：流式 Markdown 解析
- [ ] 独立测试：服务层可独立运行

#### Phase 4: 业务层整合（3-5 天）
- [ ] 创建 `chat-controller.scm`：整合业务流程
- [ ] 实现增量更新：`tree-insert!` 替代 `buffer-set-body`
- [ ] 滚动位置保持机制

#### Phase 5: UI 层优化（2-3 天）
- [ ] C++ 层禁用 message widget 鼠标交互
- [ ] 完善输入区快捷键（Enter 发送、Shift+Enter 换行）
- [ ] 样式微调

### 5.3 复用 LLM Session 组件

| 组件 | 来源 | 复用方式 | 放置层级 |
|------|------|----------|---------|
| `stream-tokenizer` | liii/llm-chat | 复制或重写 | 服务层 (`markdown-parser.scm`) |
| SSE 解析逻辑 | liii/llm-chat | 复用到 `llm-client.scm` | 服务层 |
| HTTP 流式 | liii/llm-chat | 复用 `http-post :stream` | 服务层 |
| Reasoning 折叠 | liii/llm-chat | 复用 UI 逻辑 | 业务层 |

## 6. 关键接口定义

### 6.1 层间调用约定

**核心原则**: 上层可以调用下层，下层**禁止**调用上层

```
UI 层 (C++)
    ↓ 调用
适配层 (chat-tree, chat-buffer)
    ↓ 调用
业务层 (chat-controller)
    ↓ 调用
服务层 (llm-client, markdown-parser)
    ↓ 调用
数据层 (chat-domain, chat-state)
```

### 6.2 C++ → Scheme 回调

```scheme
;; 用户点击发送按钮 → 触发业务流程
(tm-define (chat-sidebar-on-user-input text)
  (chat-controller-send-message text))

;; 窗口关闭 → 清理资源
(tm-define (chat-sidebar-on-close)
  (chat-controller-cleanup))
```

### 6.3 Scheme → C++ Glue

```scheme
;; chat-buffer.scm 中使用
(tm-define (cpp-chat-buffer-create name))
(tm-define (cpp-chat-buffer-show name bool))
(tm-define (cpp-chat-buffer-get-scroll name) → position)
(tm-define (cpp-chat-buffer-set-scroll name position))
```

### 6.4 层内接口

#### 数据层
```scheme
;; chat-domain.scm
(tm-define (chat-block-new actor))
(tm-define (chat-block-append-item block item) → new-block)
(tm-define (chat-session-new title))
(tm-define (chat-session-append-block session block) → new-session)
```

#### 适配层
```scheme
;; chat-tree.scm
(tm-define (chat-block->tree block) → tree)
(tm-define (chat-item->tree item) → tree)
(tm-define (chat-tree-append! doc tree) → void)

;; chat-buffer.scm
(tm-define (chat-buffer-ensure name) → doc)
(tm-define (chat-buffer-append name tree) → void)
(tm-define (chat-buffer-save-scroll name) → position)
```

#### 服务层
```scheme
;; llm-client.scm
(tm-define (llm-stream config payload on-chunk on-done on-error))
(tm-define (llm-complete config payload) → response)

;; markdown-parser.scm
(tm-define (markdown-stream-init) → state)
(tm-define (markdown-stream-chunk state text) → (new-state blocks))
(tm-define (markdown-stream-flush state) → blocks)
```

#### 业务层
```scheme
;; chat-controller.scm
(tm-define (chat-controller-send-message text))
(tm-define (chat-controller-cancel-stream))
(tm-define (chat-controller-retry-block block-id))
(tm-define (chat-controller-clear-session))
```

## 7. 数据结构与状态

### 7.1 领域模型（chat-domain.scm）

不可变数据定义：

```scheme
;; 会话
(define-record-type chat-session
  (make-chat-session id title blocks metadata)
  chat-session?
  (id session-id)
  (title session-title)
  (blocks session-blocks)           ; 列表
  (metadata session-metadata))

;; 消息块（一轮对话）
(define-record-type chat-block
  (make-chat-block id actor items status timestamp)
  chat-block?
  (id block-id)
  (actor block-actor)               ; 'user | 'assistant | 'system
  (items block-items)               ; chat-item 列表
  (status block-status)             ; 'pending | 'streaming | 'complete
  (timestamp block-timestamp))

;; 消息项（块内的具体内容）
(define-record-type chat-item
  (make-chat-item type content metadata)
  chat-item?
  (type item-type)                  ; 'text | 'code | 'math | 'image | 'reasoning
  (content item-content)
  (metadata item-metadata))        ; 如语言、折叠状态等
```

### 7.2 全局状态（chat-state.scm）

可变状态收敛到一处：

```scheme
;; 当前会话
(define chat-current-session #f)

;; block-id → tree 映射（用于增量更新定位）
(define chat-block-tree-map (make-ahash-table))

;; 流式解析状态
(define chat-stream-tokenizer (make-stream-tokenizer))
(define chat-stream-pending "")
(define chat-current-block-id #f)
```

## 8. 性能与优化

### 8.1 更新频率控制

- **按行缓冲**: `buf-port` 逻辑，收到完整行才更新
- **防抖处理**: 快速输入时合并请求，避免频繁触发

### 8.2 渲染优化

- **增量插入**: `tree-insert!` 替代 `buffer-set-body`，避免整量重排
- **分块渲染**: Markdown 分块后批量插入，减少渲染次数

### 8.3 内存管理

- **历史归档**: 大会话历史自动归档到磁盘
- **状态收敛**: 全局状态统一在 `chat-state.scm` 管理

## 9. 下一步行动

按照新架构的分层顺序逐步实现：

### Phase 1: 数据层（本周）
- [ ] 重构 `chat-domain.scm`（原 `chat-model.scm`）
- [ ] 创建 `chat-state.scm` 收敛全局状态

### Phase 2: 适配层（下周）
- [ ] 创建 `chat-tree.scm`（原 `chat-render.scm`）
- [ ] 创建 `chat-buffer.scm` 管理 Buffer 生命周期
- [ ] 实现增量更新：`tree-insert!` 替换整量刷新

### Phase 3: 服务层（第3周）
- [ ] 创建 `llm-client.scm` 提取 HTTP 能力
- [ ] 创建 `markdown-parser.scm` 流式解析

### Phase 4: 业务层（第4周）
- [ ] 创建 `chat-controller.scm` 整合业务流程
- [ ] 集成测试，打通端到端

### Phase 5: 完善（第5周）
- [ ] UI 优化（禁用鼠标交互、快捷键）
- [ ] Reasoning 折叠、代码块复制等交互

---

## 附录：参考实现

- **LLM Session**: `TeXmacs/plugins/goldfish/goldfish/liii/llm-chat.scm`
- **Message Widget**: `src/Texmacs/Window/tm_window.cpp:408`
- **Session Notify**: `TeXmacs/progs/dynamic/session-edit.scm:278`
- **Tree Insert**: `src/Data/Convert/Generic/input.cpp:181`

## 术语对照

| 旧术语 | 新术语 | 说明 |
|--------|--------|------|
| `chat-render.scm` | `chat-tree.scm` | 避免与 TeXmacs "渲染"混淆 |
| `chat-model.scm` | `chat-domain.scm` + `chat-state.scm` | 分离不可变数据与可变状态 |
| `chat-stream.scm` | `chat-controller.scm` | 按业务场景而非技术机制组织 |
| `chat-llm.scm` | `llm-client.scm` | 提取通用 HTTP 能力 |
