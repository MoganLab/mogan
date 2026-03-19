# AI 聊天框开发计划与进度记录

> 本文件用于持续记录开发阶段、里程碑与当前进度，可随时追加更新。

---

## 阶段 1：UI 聊天框功能（AI 回复固定模拟）
目标：先让聊天界面跑通，AI 暂时返回固定字符串，验证 C++ ↔ Scheme 链路。

### 1.1 C++ 基础组件能力定义
| 组件 | 能力 | 状态 | 备注 |
|---|---|---|---|
| `chat-message-view` | 创建空列表 | ⏳ 待实现 | 构造函数 |
| | appendMessage(role, content) | ⏳ 待实现 | slot |
| | updateLastMessage(content) | ⏳ 待实现 | slot |
| | clearMessages() | ⏳ 待实现 | slot |
| | 自动滚动到底部 | ⏳ 待实现 | 内部逻辑 |
| `chat-input-area` | 创建输入区 | ⏳ 待实现 | 构造函数 |
| | 信号 inputSubmitted(text) | ⏳ 待实现 | Qt 信号 |
| | 信号 clearRequested() | ⏳ 待实现 | Qt 信号 |
| | slot setInputText(text) | ⏳ 待实现 | slot |
| | slot getInputText() | ⏳ 待实现 | slot |
| | slot setEnabled(bool) | ⏳ 待实现 | slot |

### 1.2 Glue 暴露给 Scheme 的接口
| 接口 | 签名 | 状态 | 备注 |
|---|---|---|---|
| 构造函数 | `(tm-chat-message-view)` | ⏳ 待实现 | 返回 widget id |
| | `(tm-chat-input-area)` | ⏳ 待实现 | 返回 widget id |
| 消息区 slot | `(tm-chat-view-append! id role content)` | ⏳ 待实现 | 包装 append |
| | `(tm-chat-view-update-last! id content)` | ⏳ 待实现 | 包装 update |
| | `(tm-chat-view-clear! id)` | ⏳ 待实现 | 包装 clear |
| 输入区 slot | `(tm-chat-input-set-text! id text)` | ⏳ 待实现 | 包装 set |
| | `(tm-chat-input-get-text id)` | ⏳ 待实现 | 包装 get |
| | `(tm-chat-input-set-enabled! id bool)` | ⏳ 待实现 | 包装 enable |
| 信号转发 | 自动连接 `inputSubmitted` → `(chat-handle-input text)` | ⏳ 待实现 | glue 内部 |
| | 自动连接 `clearRequested` → `(chat-clear)` | ⏳ 待实现 | glue 内部 |

### 1.3 Scheme 业务逻辑能力定义
| 功能 | 函数 | 状态 | 备注 |
|---|---|---|---|
| 拼装布局 | `(make-chat-ui)` | ⏳ 待实现 | 垂直放置 message-view + input-area |
| 消息列表 | `*chat-messages*` | ⏳ 待实现 | 纯 Scheme list 维护 |
| 输入处理 | `(chat-handle-input text)` | ⏳ 待实现 | 固定回复 "Hello from mock AI" |
| 追加消息 | `(chat-append-message! role content)` | ⏳ 待实现 | 调 glue |
| 更新最后 | `(chat-update-last-message! content)` | ⏳ 待实现 | 调 glue |
| 清空消息 | `(chat-clear)` | ⏳ 待实现 | 调 glue |
| 禁用输入 | `(chat-set-streaming! bool)` | ⏳ 待实现 | 调 set-enabled |

---

## 阶段 2：后端 AI 调用（可插拔）
目标：把固定回复替换为真实流式调用，支持 OpenAI / Claude / 本地模型。

| 任务 | 状态 | 备注 |
|---|---|---|
| 定义统一接口 `chat-send-message!` | ⏳ 待实现 | Scheme 侧 |
| 实现 OpenAI 适配器 | ⏳ 待实现 | Scheme 侧 |
| 实现 Claude 适配器 | ⏳ 待实现 | Scheme 侧 |
| 实现本地模型适配器 | ⏳ 待实现 | Scheme 侧 |
| 配置管理（文件 + 运行时） | ⏳ 待实现 | Scheme 侧 |
| 取消/重试机制 | ⏳ 待实现 | Scheme 侧 |
| 错误提示与日志 | ⏳ 待实现 | Scheme 侧 |

---

## 里程碑与检查点
| 里程碑 | 完成标准 | 预计时间 | 状态 |
|---|---|---|---|
| M1 基础 UI 跑通 | 能发送/显示固定回复 | 1-2 天 | ⏳ |
| M2 流式模拟 | 逐字符显示固定字符串 | 0.5 天 | ⏳ |
| M3 真 AI 接入 | 任一后端正常工作 | 1-2 天 | ⏳ |
| M4 多后端切换 | 配置切换即可换 AI | 0.5 天 | ⏳ |
| M5 完整体验 | 错误/取消/日志全齐 | 1 天 | ⏳ |

---

## 当前进度
- **日期**：2026-03-19
- **阶段**：阶段 1 未开始
- **下一步**：开始实现 C++ 基础组件 `chat-message-view` 与 `chat-input-area`