# Mogan Loro 协作编辑转发服务器

这是一个仅用于本地测试的 WebSocket 转发服务器。它把每个编辑器发来的 Loro 增量 update 按房间（room）广播给其他编辑器，从而可以用两个 Mogan 编辑器打开同一文件来验证协作编辑。

## 环境要求

- Node.js 18+
- `ws` 包（运行 `npm install` 安装）

## 启动

```bash
cd tools/loro-relay
npm install
node server.js
```

默认监听 `ws://127.0.0.1:8765`。

环境变量：
- `MOGAN_LORO_RELAY_HOST`：绑定地址，默认 `127.0.0.1`
- `MOGAN_LORO_RELAY_PORT`：监听端口，默认 `8765`

## 使用

1. 先启动服务器。
2. 用 Mogan 打开一个文件，并启用协作编辑（见下文）。
3. 在另一个 Mogan 窗口打开同一文件。
4. 在任一窗口编辑，另一窗口应自动同步。

## 编辑器启用方式

构建时打开 `loro_relay` 开关（依赖 `libloro`）：

```bash
xmake f --libloro=y --loro_relay=y --qt_frontend=n
xmake b stem
```

运行编辑器：

```bash
MOGAN_LORO_RELAY=ws://127.0.0.1:8765 ./MoganSTEM .../your_file.tmu
```

若未设置 `MOGAN_LORO_RELAY`，默认尝试连接 `ws://127.0.0.1:8765`。连接失败时编辑器会自动降级为单机编辑。

## 协议

- 客户端连接后先发文本帧：`JOIN <room_id>`，其中 `room_id` 为文档路径。
- 之后所有二进制帧都被视为 Loro update，服务器会原样广播给同房间的其他客户端。
- 服务器会保存每个房间最后一条消息，新加入者会收到它作为基线。

## 注意

- 这是**本地测试/开发工具**，不是生产级协作后端。
- 目前仅支持 ImGui 后端（Qt 后端尚未适配）。
- Loro 更新是二进制帧，不会回发给发送者自己。
