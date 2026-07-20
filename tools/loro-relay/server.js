#!/usr/bin/env node
// Loro 协作编辑中继服务器（转发型）。
// 编辑器作为 WebSocket 客户端连接，把本地 Loro update（二进制）发来；
// 本服务器按“房间（room）”把每条消息广播给除发送者外的所有连接，
// 从而让多个编辑器在同一文件上协作编辑。
//
// 运行：npm install   然后   node server.js
// 默认监听 ws://0.0.0.0:8765
// 环境变量：MOGAN_LORO_RELAY_HOST, MOGAN_LORO_RELAY_PORT
const WebSocket = require('ws');
const HOST = process.env.MOGAN_LORO_RELAY_HOST || '0.0.0.0';
const PORT = parseInt(process.env.MOGAN_LORO_RELAY_PORT || '8765', 10);
const wss = new WebSocket.Server({ host: HOST, port: PORT });

// room -> { clients: Set<ws> }
const rooms = new Map();
// 用于为每个连接分配一个递增的 ID，便于日志区分
let nextClientId = 1;

function getRoom(roomId) {
  if (!rooms.has(roomId)) {
    rooms.set(roomId, { clients: new Set() });
    console.log(`${ts()} [room:${roomId}] 房间创建`);
  }
  return rooms.get(roomId);
}

function leaveRoom(ws) {
  if (!ws.roomId) return;
  const room = rooms.get(ws.roomId);
  if (room) {
    room.clients.delete(ws);
    console.log(
      `${ts()} [client:${ws.clientId}] 离开房间 [room:${ws.roomId}]，房间当前人数: ${room.clients.size}`
    );
    if (room.clients.size === 0) {
      rooms.delete(ws.roomId);
      console.log(`${ts()} [room:${ws.roomId}] 房间已空，销毁`);
    }
  }
  ws.roomId = null;
}

function joinRoom(ws, roomId) {
  leaveRoom(ws);
  const room = getRoom(roomId);
  room.clients.add(ws);
  ws.roomId = roomId;
  console.log(
    `${ts()} [client:${ws.clientId}] 加入房间 [room:${roomId}]，房间当前人数: ${room.clients.size}`
  );
}

const LATENCY_MS = 0;
function broadcast(ws, data) {
  if (!ws.roomId) return;
  const room = rooms.get(ws.roomId);
  if (!room) return;
  
  let sentCount = 0;
  for (const client of room.clients) {
    if (client === ws) continue;
    if (client.readyState !== WebSocket.OPEN) continue;
    setTimeout(() => {
      if (client.readyState === WebSocket.OPEN)
        client.send(data);
    }, LATENCY_MS);
    sentCount++;
  }
  console.log(
    `${ts()} 延迟 ${LATENCY_MS}ms 后广播给 ${sentCount} 个客户端`
  );
}

wss.on('connection', (ws) => {
  ws.roomId = null;
  ws.clientId = nextClientId++;
  console.log(
    `${ts()} [client:${ws.clientId}] 新连接建立，当前总连接数: ${wss.clients.size}`
  );
  
  ws.on('message', (data, isBinary) => {
    if (!isBinary) {
      const text = data.toString('utf8').trim();
      console.log(
        `${ts()} [client:${ws.clientId}] 收到文本消息: ${text.length > 100 ? text.slice(0, 100) + '...' : text}`
      );
      if (text.startsWith('JOIN ')) {
        const roomId = text.slice(5).trim();
        joinRoom(ws, roomId);
      }
      return;
    }
    
    if (ws.roomId) {
      console.log(
        `${ts()} [client:${ws.clientId}] 收到二进制数据，大小 ${data.length} 字节，准备广播`
      );
      broadcast(ws, data);
    } else {
      console.log(
        `${ts()} [client:${ws.clientId}] 收到二进制数据但未加入任何房间，忽略`
      );
    }
  });
  
  ws.on('close', (code, reason) => {
    console.log(
      `${ts()} [client:${ws.clientId}] 连接关闭 (code: ${code}, reason: ${reason ? reason.toString() : '无'})`
    );
    leaveRoom(ws);
  });
  
  ws.on('error', (err) => {
    console.error(`${ts()} [client:${ws.clientId}] WebSocket error:`, err);
  });
});

wss.on('listening', () => {
  console.log(`[relay] Mogan Loro relay listening on ws://${HOST}:${PORT}`);
});

wss.on('error', (err) => {
  console.error('Relay server error:', err);
  process.exit(1);
});

// 简易时间戳生成
function ts() {
  return new Date().toISOString();
}