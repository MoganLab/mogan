# Mogan Loro 云文档协作服务端

面向部署的协作服务端，负责**文档生命周期管理、持久化存储与同步数据传输**。
不参与 CRDT 合并逻辑——文档内容一致性由客户端的 Loro CRDT 引擎保证；
服务端的影子 `LoroDoc` 仅用于推进版本向量（import 记账）和按需导出 snapshot。

## 职责

1. **新文档创建**：`CREATE` 请求 → 分配 UUID docId → 初始化磁盘记录。
2. **持久化**：每个文档维护 `snapshot.bin`（全量快照）+ `updates.log`（增量 commit 记录）。
3. **同步**：客户端首次连接（`JOIN`）、断线重连（重新 `JOIN`）或主动 `RESYNC` 时，
   按序下发 snapshot 与其后的全部 update，客户端 `import`（幂等）后恢复到最新状态。

## 环境要求

- Node.js 20+
- `npm install`（依赖 `ws` 与 `loro-crdt`）

## 启动

```bash
cd tools/loro-server
npm install
node server.js
```

环境变量：

| 变量 | 默认 | 说明 |
|---|---|---|
| `MOGAN_LORO_HOST` | `0.0.0.0` | 绑定地址 |
| `MOGAN_LORO_PORT` | `8765` | 监听端口 |
| `MOGAN_LORO_DATA_DIR` | `tools/loro-server/data`（相对 server.js） | 持久化目录 |
| `MOGAN_LORO_LATENCY_MS` | `0` | 网络延迟模拟（ms）：>0 时每条收到的消息延迟该毫秒后再处理，用于测试弱网下的同步/收敛 |
| `MOGAN_LORO_TLS_CERT` | _（空）_ | TLS 证书文件路径；与 `MOGAN_LORO_TLS_KEY` 同时设置时启用 TLS（`wss://`+`https://`）。HTTPS 页面（如 GitHub Pages）必须走 TLS，否则浏览器拦 Mixed Content |
| `MOGAN_LORO_TLS_KEY` | _（空）_ | TLS 私钥文件路径 |

## TLS（HTTPS 页面必需）

浏览器从 **HTTPS** 页面（GitHub Pages、`https://` 部署）禁止发起不安全的 `http://`/`ws://`
（Mixed Content）。故 HTTPS 前端必须连 `wss://`+`https://`，服务端需上 TLS：

```bash
# LAN IP 自签证书（mkcert，浏览器需信任一次）：
mkcert -install 10.0.178.250            # 生成根 CA 并信任
mkcert 10.0.178.250                     # 生成 10.0.178.250.pem / -key.pem
MOGAN_LORO_TLS_CERT=./10.0.178.250.pem \
MOGAN_LORO_TLS_KEY=./10.0.178.250-key.pem \
node server.js
# 启动后是 wss+https；前端用 ?loro_server=wss://10.0.178.250:8765
```

无公网域名时自签证书浏览器会警告（接受一次即可测试）；生产用真实域名 + Let's Encrypt，
或前置 Caddy/nginx 做 TLS 终结反向代理。WASM 客户端把 `ws://` 自动映射 `http://`（/docs），
`wss://` 自动映射 `https://`。

部署探活：`GET /healthz` → `{"ok":true,"docs":<内存中文档数>}`。
文档发现：`GET /docs` → 纯文本，每行一个已落盘文档的 UUID（供客户端在 JOIN 前列出可加入的文档，不建立 WebSocket）。

## 协议

文本帧为控制消息，二进制帧为 Loro update/snapshot 载荷（服务端不区分，`import` 均幂等）。

| 方向 | 帧 | 说明 |
|---|---|---|
| C→S | `CREATE` | 申请新文档 |
| C→S | `JOIN <docId>` | 加入文档（首次连接与断线重连均走此路径） |
| C→S | `RESYNC` | 主动重新同步 |
| C→S | `PING` | 心跳 |
| S→C | `DOC <docId>` | `CREATE`/`JOIN` 成功确认，携带文档 UUID |
| S→C | `ERR <code> <msg>` | 错误：`NO_SUCH_DOC` / `BAD_REQUEST` / `INTERNAL` |
| S→C | `PONG` | 心跳响应 |
| C→S / S→C | `CURSOR <peerId> <payload>` | 多光标：客户端发，服务端原样转发给同文档其他客户端（瞬态，不落盘）；payload 为位置编码，传输层不解析 |
| S→C | 二进制帧 | 历史补发（snapshot → 按序 updates）或其他客户端的实时 update |

### 典型时序

```
创建者                    服务端                     后加入者
  │── CREATE ─────────────→│ 分配 UUID、初始化磁盘      │
  │←── DOC <uuid> ─────────│                          │
  │── 二进制 update ──────→│ import 记账→广播→落盘      │
  │                        │←── JOIN <uuid> ──────────│
  │                        │── DOC <uuid> ───────────→│
  │                        │── snapshot + updates ───→│ import 后即为最新状态
  │←── 新 update ──────────│←── 二进制 update ────────│
```

## 持久化布局

```
data/<docId>/
  snapshot.bin   最近一次 export(snapshot) 的完整状态
  updates.log    snapshot 之后的增量记录：[4B 大端长度][payload] 重复
  meta.json      { docId, createdAt, snapshotSeq, updateCount }
```

- update 先 append 到 `updates.log` 再广播；写 snapshot 用临时文件 + rename 保证原子性。
- `updates.log` 达到 100 条或累计 1MB 后，经 5 秒合并窗口统一导出 snapshot 并截断日志
  （`SNAPSHOT_UPDATE_THRESHOLD` / `SNAPSHOT_BYTE_THRESHOLD`，见 `registry.js`）。
- 启动时扫描数据目录恢复全部文档索引；影子 doc 懒加载（首条 update/同步请求时才从磁盘回放）。
- 房间无人后影子卸载，状态全在磁盘；`SIGINT`/`SIGTERM` 触发优雅退出（在途写入与快照落盘后再退出）。

## 编辑器接入

构建（依赖 `libloro`）：

```bash
xmake f --loro=y
xmake b stem
```

运行：

```bash
# 创建新云文档（服务端分配 UUID，见编辑器日志 [WS] Joined collaboration doc: <uuid>）
MOGAN_LORO_SERVER=ws://<host>:8765 ./MoganSTEM your_file.tmu

# 加入已有云文档
MOGAN_LORO_SERVER=ws://<host>:8765 MOGAN_LORO_DOC_ID=<uuid> ./MoganSTEM your_file.tmu
```

- 未设置 `MOGAN_LORO_SERVER` 时默认连 `ws://127.0.0.1:8765`。
- 断线 3 秒后自动重连，重连后服务端补发历史，Loro `import` 幂等，自动收敛。
- 连接失败时编辑器降级为单机编辑。

## 测试

```bash
npm test   # node --test：完整生命周期（创建/协作/重连/重启恢复/RESYNC/快照截断）
```

## 与旧 relay 的关系

`tools/loro-relay/` 是纯转发的早期开发工具（无持久化、无文档管理），保留作参考；
新开发一律使用本服务。

## 已知边界

- **无鉴权**：任何拿到 docId 的人都能读写对应文档。生产部署需在前面加鉴权网关
  （或把鉴权做进 upgrade 请求校验），并将 `ws://` 换成 `wss://`（TLS 终结于反向代理）。
- 同步为"从头顶一把"（snapshot + 全部 updates），未做基于版本向量的差量协商；
  单文档 update 量极大时可加 `VV` 控制帧按需补发。
- 单进程：多实例部署需共享存储 + 广播总线（如 Redis pub/sub），当前不支持。
