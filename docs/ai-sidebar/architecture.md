# Mogan AI Sidebar 总体架构

## 1. 分层架构概览

AI Sidebar 采用**五层架构**，严格分离关注点：

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 5: UI 容器层 (Qt/C++)                                    │
├─────────────────────────────┬───────────────────────────────────┤
│  当前会话 (Current)          │  历史会话记录 (History)            │
├─────────────────────────────┼───────────────────────────────────┤
│  • messageWidget (TeXmacs)  │  • QListView (Qt 原生)            │
│  • inputWidget              │  • 历史列表项点击                  │
│  • 发送/取消按钮             │  • 恢复/删除按钮                   │
│  • 流式输出展示              │  • 元数据展示(标题/时间/数量)      │
└─────────────────────────────┴───────────────────────────────────┘
                              │
                              ▼ 信号/Glue
┌─────────────────────────────────────────────────────────────────┐
│  Layer 4: 适配层 (Scheme)                                       │
├─────────────────────────────┬───────────────────────────────────┤
│  当前会话 (Current)          │  历史会话记录 (History)            │
├─────────────────────────────┼───────────────────────────────────┤
│  • chat-block->tree         │  • (无需适配层)                   │
│  • chat-item->tree          │    历史列表直接用 QStringList     │
│  • buffer-append-tree       │    或标准模型，不经过 TeXmacs     │
│  • buffer-append-to-block   │    Tree 转换                      │
│  • 增量更新 UI              │                                   │
└─────────────────────────────┴───────────────────────────────────┘
                              │
                              ▼ 调用
┌─────────────────────────────────────────────────────────────────┐
│  Layer 3: 业务层 (Scheme)                                       │
├─────────────────────────────┬───────────────────────────────────┤
│  当前会话 (Current)          │  历史会话记录 (History)            │
├─────────────────────────────┼───────────────────────────────────┤
│  • send-message             │  • load-history-list              │
│  • on-chunk (流式回调)       │  • restore-session                │
│  • cancel-stream            │  • delete-history                 │
│  • archive-current          │                                   │
└─────────────────────────────┴───────────────────────────────────┘
                              │
                              ▼ 调用
┌─────────────────────────────────────────────────────────────────┐
│  Layer 2: 服务层 (Scheme)                                       │
├─────────────────────────────┬───────────────────────────────────┤
│  当前会话 (Current)          │  历史会话记录 (History)            │
├─────────────────────────────┼───────────────────────────────────┤
│  • llm-stream (HTTP/SSE)    │  • (无)                           │
│  • llm-complete             │    历史记录是静态数据，            │
│  • markdown-stream-chunk    │    无需额外服务                   │
│  • markdown-stream-flush    │                                   │
└─────────────────────────────┴───────────────────────────────────┘
                              │
                              ▼ 调用
┌─────────────────────────────────────────────────────────────────┐
│  Layer 1: 数据层 (Scheme)                                       │
├─────────────────────────────┬───────────────────────────────────┤
│  当前会话 (Current)          │  历史会话记录 (History)            │
├─────────────────────────────┼───────────────────────────────────┤
│  • chat-session (record)    │  • chat-history-meta (record)     │
│  • chat-block               │  • sessions.json 索引             │
│  • chat-item                │  • 历史文件读写                    │
│  • 运行时状态(可变)          │  • 自动归档/数量限制               │
└─────────────────────────────┴───────────────────────────────────┘
```

## 2. 各层职责详解

### 2.1 Layer 1: 数据层

**核心原则**: 只关心数据是什么、如何存储，**完全不依赖 TeXmacs**

**当前会话 vs 历史会话的分工**:

| 模块 | 当前会话 (Current) | 历史会话记录 (History) |
|------|-------------------|----------------------|
| **数据结构** | `chat-session`, `chat-block`, `chat-item` | `chat-history-meta` |
| **状态管理** | `chat-current-session` (运行时) | `chat-history-list-cache` (列表缓存) |
| **持久化** | 无（仅内存） | `persist-save`, `persist-load-full`, `persist-delete` |
| **文件操作** | 自动归档时触发 | 索引文件 `sessions.json` + 历史文件 |

#### chat-domain.scm - 数据定义（两边共用）

```scheme
;; 当前会话数据结构
(define-record-type chat-session
  (make-chat-session id title blocks metadata)
  chat-session?
  (id session-id)
  (title session-title)
  (blocks session-blocks)
  (metadata session-metadata))

(define-record-type chat-block
  (make-chat-block id actor items status timestamp)
  chat-block?
  (id block-id)
  (actor block-actor)       ; 'user | 'assistant | 'system
  (items block-items)
  (status block-status)     ; 'pending | 'streaming | 'complete
  (timestamp block-timestamp))

(define-record-type chat-item
  (make-chat-item type content metadata)
  chat-item?
  (type item-type)          ; 'text | 'code | 'math | 'image
  (content item-content)
  (metadata item-metadata))

;; 历史会话轻量元数据
(define-record-type chat-history-meta
  (make-chat-history-meta id title timestamp message-count model)
  chat-history-meta?
  (id chat-history-meta-id)
  (title chat-history-meta-title)
  (timestamp chat-history-meta-timestamp)
  (message-count chat-history-meta-count)
  (model chat-history-meta-model))
```

#### chat-state.scm - 运行时状态（仅当前会话）

```scheme
;; 当前活跃会话（内存中，不直接持久化）
(define chat-current-session #f)

;; block-id -> tree 映射（用于增量更新定位）
(define chat-block-tree-map (make-ahash-table))

;; 流式解析状态
(define chat-stream-tokenizer (make-stream-tokenizer))
(define chat-stream-pending "")
```

#### chat-history-state.scm - 历史列表状态（仅历史会话）

```scheme
;; 历史列表缓存（避免重复读取文件）
(define chat-history-list-cache '())

;; 当前激活的 Tab（'current 或 'history）
(define chat-active-tab 'current)
```

#### chat-persist.scm - 持久化（仅历史会话）

```scheme
;; 保存会话到历史（当前会话归档时调用）
(tm-define (persist-save session)
  ;; 序列化为 JSON
  ;; 写入 ~/.mogan/chat-history/
  ;; 更新索引文件
  )

;; 加载历史元数据列表（轻量，用于历史列表展示）
(tm-define (persist-list-metas)
  ;; 读取 sessions.json
  ;; 返回 chat-history-meta 列表
  )

;; 加载完整会话（恢复时调用）
(tm-define (persist-load-full id)
  ;; 读取 session-xxx.json
  ;; 反序列化为 chat-session
  )

;; 删除历史
(tm-define (persist-delete id))

;; 限制数量
(tm-define (persist-truncate max-count))
```

### 2.2 Layer 2: 服务层（仅当前会话使用）

**核心原则**: 纯技术能力，无业务逻辑，无状态

**分工说明**: 服务层仅服务于当前对话的实时交互场景。历史会话记录是静态数据，直接通过数据层的持久化接口读写，无需经过服务层。

| 服务 | 当前会话 (Current) | 历史会话记录 (History) |
|------|-------------------|----------------------|
| LLM 请求 | `llm-stream`, `llm-complete` | 无 |
| Markdown 解析 | `markdown-stream-chunk`, `markdown-stream-flush` | 无 |

#### llm-client.scm - HTTP/SSE 客户端
```scheme
;; 纯技术接口，不感知 chat 业务

(tm-define (llm-stream config payload on-chunk on-done on-error))
;; config: 包含 api-key, base-url, model 等
;; payload: 请求体
;; on-chunk: (lambda (chunk) ...) 流式回调
;; on-done: (lambda () ...) 完成回调
;; on-error: (lambda (err) ...) 错误回调

(tm-define (llm-complete config payload))
;; 非流式请求，直接返回完整响应
```

#### markdown-parser.scm - Markdown 解析
```scheme
;; 流式 Markdown 解析器

(tm-define (markdown-stream-init)
  ;; 返回初始状态
  (make-stream-tokenizer))

(tm-define (markdown-stream-chunk state text)
  ;; 返回: (new-state blocks)
  ;; blocks: 解析出的段落/代码块/公式列表
  )

(tm-define (markdown-stream-flush state)
  ;; 返回剩余内容
  )
```

### 2.3 Layer 3: 业务层

**核心原则**: 编排业务流程，**唯一可以调用服务层和数据层的层级**

**分工说明**:

| 场景 | 当前会话 (Current) | 历史会话记录 (History) |
|------|-------------------|----------------------|
| **核心操作** | 发送消息、流式响应、取消请求 | 加载列表、恢复会话、删除历史 |
| **调用服务层** | 是（LLM、Markdown） | 否 |
| **调用数据层** | 运行时状态（chat-state） | 持久化（chat-persist） |
| **调用适配层** | 是（Tree 转换、Buffer 更新） | 恢复会话时调用 |

#### chat-controller.scm - 业务流程编排

**当前对话场景**:
```scheme
;; 发送消息
(tm-define (chat-controller-send-message text)
  ;; 1. 数据层: (chat-block-new 'user) 创建 block
  ;; 2. 数据层: (chat-block-append-item ...) 添加内容
  ;; 3. 适配层: (chat-block->tree ...) 转换
  ;; 4. 适配层: (buffer-append-tree ...) 更新 UI
  ;; 5. 服务层: (llm-stream ...) 启动请求
  ;; 6. 在回调中协调更新
  )

;; 流式响应回调
(tm-define (chat-controller-on-chunk chunk)
  ;; 1. 服务层: (markdown-stream-chunk ...) 解析
  ;; 2. 数据层: 更新 block 状态
  ;; 3. 适配层: (buffer-append-to-block ...) 增量更新
  )

;; 取消流式请求
(tm-define (chat-controller-cancel-stream)
  ;; 终止 LLM 请求，更新 block 状态为 'cancelled
  )

;; 归档当前会话到历史
(tm-define (chat-controller-archive-current)
  ;; 1. 数据层: (persist-save chat-current-session)
  ;; 2. 数据层: (persist-truncate max-count)
  ;; 3. 清空当前会话状态
  )
```

**历史记录场景**:
```scheme
;; 加载历史列表
(tm-define (chat-controller-load-history-list)
  ;; 1. 数据层: (persist-list-metas) 读取索引
  ;; 2. 更新 chat-history-list-cache
  ;; 3. 通知 UI 层: (cpp-chat-set-history-list metas)
  )

;; 恢复历史会话
(tm-define (chat-controller-restore-session id)
  ;; 1. 数据层: (persist-load-full id) 加载完整会话
  ;; 2. 可选: (chat-controller-archive-current) 归档当前
  ;; 3. 数据层: (set! chat-current-session session)
  ;; 4. 适配层: (buffer-clear ...) + (buffer-append-tree ...) 渲染
  ;; 5. 通知 UI 层: (cpp-chat-switch-tab 'current)
  )

;; 删除历史记录
(tm-define (chat-controller-delete-history id)
  ;; 1. 数据层: (persist-delete id) 删除文件
  ;; 2. 刷新列表: (chat-controller-load-history-list)
  )
```

### 2.4 Layer 4: 适配层

**核心原则**: 数据 ↔ UI 的桥梁，管理 TeXmacs 特定资源

**分工说明**:

| 功能 | 当前会话 (Current) | 历史会话记录 (History) |
|------|-------------------|----------------------|
| **数据转换** | `chat-block->tree`, `chat-item->tree` | 无需转换，直接用元数据 |
| **Buffer 管理** | `buffer-append-tree`, `buffer-append-to-block` | 仅恢复会话时批量渲染 |
| **渲染方式** | 增量更新（流式） | 无（Qt 原生控件展示列表） |

**历史会话的特殊处理**: 历史列表使用 Qt 原生 `QListView` + `QStringListModel` 展示，不经过 TeXmacs Tree 转换。仅在**恢复会话**时，才通过适配层将历史数据渲染到当前会话的 Buffer 中。

#### chat-tree.scm - 数据 → Tree 转换（主要服务当前会话）
```scheme
;; 将数据结构序列化为 TeXmacs Tree

(tm-define (chat-block->tree block)
  ;; chat-block -> (chat-user-box ...) 或 (chat-ai-box ...)
  )

(tm-define (chat-item->tree item)
  ;; chat-item -> 对应的 TeXmacs tree
  ;; 'text -> (document text)
  ;; 'code -> (code-block lang content)
  ;; 'math -> (math content)
  )

;; 辅助：历史元数据转展示字符串（可选，供 UI 层参考）
(tm-define (chat-history-meta->string meta)
  ;; chat-history-meta -> "2024-01-15 证明三角形..."
  ;; 注意：实际历史列表由 Qt 直接渲染，此函数仅作格式化参考
  )
```

#### chat-buffer.scm - Buffer 资源管理（仅当前会话）
```scheme
;; 管理 TeXmacs Buffer（UI 容器）- 仅当前对话 Tab 使用

(tm-define (buffer-ensure name)
  ;; 创建或查找 buffer
  ;; 返回: document tree
  )

(tm-define (buffer-get name)
  ;; 获取 buffer 的 body
  )

(tm-define (buffer-clear name)
  ;; 清空 buffer（恢复历史会话时使用）
  )

(tm-define (buffer-append-tree name tree)
  ;; 追加 tree 到 buffer（增量更新）
  ;; 使用 tree-insert! 而非 buffer-set-body
  )

(tm-define (buffer-append-to-block name block-id tree)
  ;; 定位到已有 block，追加内容（流式更新）
  ;; 使用 chat-block-tree-map 定位
  )

(tm-define (buffer-save-scroll name)
  ;; 保存滚动位置
  )

(tm-define (buffer-restore-scroll name)
  ;; 恢复滚动位置
  )
```

### 2.5 Layer 5: UI 容器层

**核心原则**: 纯容器，**不包含业务逻辑**，只负责物理展示

**分工说明**:

| 组件 | 当前会话 (Current) | 历史会话记录 (History) |
|------|-------------------|----------------------|
| **容器** | `currentTab` (QWidget) | `historyTab` (QWidget) |
| **展示组件** | `messageWidget` (TeXmacs) | `historyList` (QListView) |
| **输入组件** | `inputWidget` (TeXmacs) | 无（仅展示） |
| **交互元素** | 发送按钮、取消按钮 | 恢复按钮、删除按钮 |
| **渲染技术** | TeXmacs Tree | Qt Model/View |

#### ChatSidebarWidget (C++)
```cpp
class ChatSidebarWidget : public QWidget {
    // 物理容器管理
    QTabWidget* tabWidget;
    QWidget* currentTab;      // 当前对话 Tab
    QWidget* historyTab;      // 历史记录 Tab

    // 当前对话 Tab 组件
    widget messageWidget;     // texmacs_custom_message_widget
    widget inputWidget;       // texmacs_custom_input_widget

    // 历史记录 Tab 组件
    QListView* historyList;   // 历史列表（Qt 原生）
    QStringListModel* historyModel;  // 历史列表数据模型

public:
    // 创建/销毁
    void createWidgets();
    void destroyWidgets();

    // Tab 切换
    void onTabChanged(int index);

    // 当前对话 Tab 事件（转发到 Scheme 业务层）
    void onSendButtonClicked();     // -> chat-controller-send-message
    void onCancelButtonClicked();   // -> chat-controller-cancel-stream

    // 历史记录 Tab 事件（转发到 Scheme 业务层）
    void onHistoryItemClicked(QString sessionId);    // -> chat-controller-restore-session
    void onDeleteButtonClicked(QString sessionId);   // -> chat-controller-delete-history
    void setHistoryList(QStringList titles);         // <- chat-controller-load-history-list

    // 焦点同步
    void syncFocusToInput();
    void syncFocusToMain();
};
```

## 3. 层间调用规则

### 3.1 调用方向

**当前会话 (Current) 的调用链**:
```
UI 层 (C++)
    ↓ 调用（Glue）
适配层 (Scheme)      ← 数据 ↔ Tree 转换
    ↓ 调用
业务层 (Scheme)
    ↓ 调用
服务层 (Scheme)      ← HTTP/SSE, Markdown 解析
    ↓ 调用
数据层 (Scheme)      ← 运行时状态 (chat-state)
```

**历史会话记录 (History) 的调用链**:
```
UI 层 (C++)          ← Qt QListView 原生渲染
    ↓ 调用（Glue）
业务层 (Scheme)      ← 注意：跳过适配层和服务层！
    ↓ 调用
数据层 (Scheme)      ← 持久化 (chat-persist)
```

**关键区别**：历史会话记录直接通过 Qt 原生控件展示，不经过 TeXmacs 适配层转换，也不需要 LLM/Markdown 等服务层能力。

### 3.2 跨层数据流

| 方向 | 数据类型 | 当前会话示例 | 历史会话示例 |
|------|---------|-------------|-------------|
| UI → 业务 | 原始输入/ID | 用户输入的文本 | sessionId |
| 业务 → 服务 | 配置 / 回调 | config, on-chunk | **无** |
| 业务 → 数据 | 模型对象 | chat-block, chat-item | chat-history-meta |
| 服务 → 业务 | 原始数据 | HTTP chunk, parsed blocks | **无** |
| 数据 → 业务 | 模型对象 | loaded session | loaded history meta |
| 业务 → 适配 | Tree / Buffer | (chat-user-box ...) | **仅恢复时** |
| 适配 → UI | 更新指令 | (buffer-append-tree ...) | **仅恢复时** |
| 业务 → UI | 列表数据 | **无** | QStringList |

## 4. 双 Tab 场景下的层协作

### 4.1 当前对话 Tab

```
【用户发送消息】

UI 层: 用户点击发送按钮
    ↓ (cpp-chat-on-send text)

适配层: 获取输入，转换为 tree
    ↓ (chat-controller-send-message text)

业务层: 编排流程
    ├─→ 数据层: (chat-block-new 'user) 创建 block
    ├─→ 数据层: (chat-block-append-item ...) 添加内容
    ├─→ 适配层: (chat-block->tree ...) 转换
    ├─→ 适配层: (buffer-append-tree ...) 更新 UI
    └─→ 服务层: (llm-stream ...) 启动请求
            ↓
        HTTP Chunk 到达
            ↓ (回调)
        服务层: (markdown-stream-chunk ...) 解析
            ↓
        业务层: (chat-controller-on-chunk ...)
            ├─→ 数据层: 更新 block 状态
            └─→ 适配层: (buffer-append-to-block ...) 增量更新
```

### 4.2 历史记录 Tab

```
【切换到历史 Tab】

UI 层: 用户点击"历史记录"Tab
    ↓ (cpp-chat-on-tab-changed 'history)

业务层: (chat-controller-load-history-list)
    ↓
数据层: (persist-list-metas) 读取索引
    ↓
业务层: 更新状态缓存
    ↓ (cpp-chat-set-history-list metas)

UI 层: 刷新 QListView（Qt 原生渲染，不经过 TeXmacs）


【用户点击恢复】

UI 层: 用户点击"恢复"按钮
    ↓ (cpp-chat-on-restore id)

业务层: (chat-controller-restore-session id)
    ├─→ 数据层: (persist-load-full id) 加载完整会话
    ├─→ 数据层: (set! chat-current-session session)
    ├─→ 适配层: (buffer-clear ...) + (buffer-append-tree ...) 渲染
    └─→ UI 层: (cpp-chat-switch-tab 'current)

UI 层: 切换到"当前对话"Tab，显示恢复的会话
```

### 4.3 跨 Tab 协作差异

| 功能 | 当前对话 Tab | 历史记录 Tab |
|------|-------------|-------------|
| **展示组件** | TeXmacs message widget | Qt QListView |
| **适配层参与** | 是（tree 转换、buffer 管理） | 否（纯 Qt） |
| **服务层参与** | 是（LLM、Markdown） | 否（静态数据） |
| **数据层参与** | 运行时状态 | 持久化读写 |
| **流式更新** | 支持 | 不支持 |
| **富文本** | 支持 | 不支持（仅文本摘要） |

## 5. 存储设计

### 5.1 存储结构

```
~/.mogan/chat-history/
├── sessions.json                    # 历史索引（轻量）
│   {
│     "version": "1.0",
│     "max_count": 20,
│     "sessions": [
│       {
│         "id": "20240115143022-abc123",
│         "title": "2024-01-15 证明三角形...",
│         "timestamp": "2024-01-15T14:30:22Z",
│         "message_count": 12,
│         "model": "deepseek-chat",
│         "file": "20240115143022-abc123.json"
│       }
│     ]
│   }
│
├── 20240115143022-abc123.json       # 完整会话
│   {
│     "id": "20240115143022-abc123",
│     "blocks": [
│       {
│         "id": "block-001",
│         "actor": "user",
│         "items": [...],
│         "timestamp": "..."
│       }
│     ]
│   }
│
└── 20240115143022-abc123/           # 复杂附件（可选）
    └── complex-math.tmu
```

### 5.2 自动归档策略

| 场景 | 行为 |
|------|------|
| 消息完成 | 自动保存到历史 |
| 达到 20 个 | 删除最旧的历史 |
| 程序退出 | 归档当前会话（如果非空） |

## 6. 实现顺序

### Phase 1: 数据层 (Week 1)
- [ ] chat-domain.scm：定义 record-type
- [ ] chat-state.scm：运行时状态
- [ ] chat-persist.scm：基础 save/load

### Phase 2: 服务层 (Week 1-2)
- [ ] llm-client.scm：HTTP/SSE
- [ ] markdown-parser.scm：流式解析

### Phase 3: 适配层 (Week 2)
- [ ] chat-tree.scm：数据 → Tree
- [ ] chat-buffer.scm：增量更新

### Phase 4: 业务层 (Week 2-3)
- [ ] chat-controller.scm：编排所有场景

### Phase 5: UI 层 (Week 3)
- [ ] ChatSidebarWidget.cpp：双 Tab 框架
- [ ] 焦点同步机制

### Phase 6: 集成 (Week 4)
- [ ] 端到端测试
- [ ] 性能优化
