'use strict';
// Mogan × Loro 云文档协作服务端。
//
// 职责（CRDT 合并不在本层——内容一致性由客户端 Loro 引擎保证）：
//   1. 文档生命周期：CREATE 分配 UUID docId 并初始化持久化记录
//   2. 持久化：每文档维护 snapshot.bin（全量状态）+ updates.log（增量 commit 记录）
//   3. 同步：客户端 JOIN / RESYNC 时下发 snapshot + 缺失的 update；
//      之后作为广播通道把新 update 推给同文档其他客户端
//
// 协议（文本帧为控制消息，二进制帧为 Loro update/snapshot 载荷）：
//   C→S  CREATE            申请新文档
//   C→S  JOIN <docId>      加入文档（首次连接/重连均走此路径，服务端补发历史）
//   C→S  RESYNC            主动重新同步（重新下发 snapshot + updates）
//   C→S  PING              心跳（回复 PONG）
//   S→C  DOC <docId>       CREATE/JOIN 成功确认
//   S→C  ERR <code> <msg>  错误（NO_SUCH_DOC / BAD_REQUEST / INTERNAL）
//   S→C  PONG
//
// 运行：npm install && node server.js
// 环境变量：
//   MOGAN_LORO_HOST      绑定地址（默认 0.0.0.0）
//   MOGAN_LORO_PORT      监听端口（默认 8765）
//   MOGAN_LORO_DATA_DIR  持久化目录（默认 ./data）
const http = require('http');
const https = require('https');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');
const { DocStore } = require('./store');
const { DocRegistry } = require('./registry');

const HOST = process.env.MOGAN_LORO_HOST || '0.0.0.0';
const PORT = parseInt(process.env.MOGAN_LORO_PORT || '8765', 10);
// TLS：HTTPS 页面（如 GitHub Pages）只能连 wss://+https://（浏览器拦 Mixed
// Content）。提供 cert/key 即启用 TLS。LAN IP 可用 mkcert 生成自签证书（浏览器
// 需信任一次）。
const TLS_CERT = process.env.MOGAN_LORO_TLS_CERT; // 证书文件路径
const TLS_KEY = process.env.MOGAN_LORO_TLS_KEY; // 私钥文件路径
const USE_TLS = Boolean(TLS_CERT && TLS_KEY);
// 默认相对 server.js 所在目录（tools/loro-server/data），不随 cwd 漂移到仓库根
const DATA_DIR =
  process.env.MOGAN_LORO_DATA_DIR || path.join(__dirname, 'data');
// 网络延迟模拟（ms）：>0 时每条收到的消息延迟该毫秒后再处理，便于测试弱网下
// 的同步/收敛行为。默认 0（关闭）。
const LATENCY_MS = parseInt(process.env.MOGAN_LORO_LATENCY_MS || '0', 10);

function ts () {
  return new Date().toISOString();
}

async function main () {
  const store = new DocStore(DATA_DIR);
  await store.init();
  const registry = new DocRegistry(store);
  const restored = await registry.restore();
  console.log(
    `${ts()} [server] 持久化目录 ${DATA_DIR}，恢复 ${restored} 个文档` +
      (LATENCY_MS > 0 ? `，延迟模拟 ${LATENCY_MS}ms` : '') +
      (USE_TLS ? '，TLS 已启用（wss+https）' : '（明文 ws+http）')
  );

  // 内嵌 HTTP(S) 服务：/healthz 供部署探活，/docs 列出可用文档 UUID（供客户端
  // 在 JOIN 前发现文档，纯文本每行一个 UUID，便于 C++ 端按行解析），同时
  // 承载 WebSocket upgrade。所有响应带 CORS 头：WASM 客户端从浏览器 fetch /docs
  // 是跨源请求，无 CORS 头会被浏览器拦截。提供 TLS cert/key 时用 https（HTTPS
  // 页面如 GitHub Pages 必须走 https/wss，否则浏览器拦 Mixed Content）。
  const requestHandler = async (req, res) => {
    // 统一 CORS 头 + 处理预检（OPTIONS）
    const setCors = (r) =>
      r.setHeader('access-control-allow-origin', '*');
    if (req.method === 'OPTIONS') {
      setCors(res);
      res.setHeader('access-control-allow-methods', 'GET, OPTIONS');
      res.setHeader('access-control-allow-headers', '*');
      res.writeHead(204);
      res.end();
      return;
    }
    setCors(res);
    if (req.url === '/healthz') {
      res.writeHead(200, { 'content-type': 'application/json' });
      res.end(JSON.stringify({ ok: true, docs: registry.docs.size }));
    } else if (req.url === '/docs') {
      // 仅返回已落盘的文档（磁盘记录稳定，避免内存中瞬时 entry 抖动）
      let ids = [];
      try {
        ids = await store.listDocs();
      } catch (err) {
        console.error(`${ts()} [http] listDocs 失败:`, err);
      }
      res.writeHead(200, { 'content-type': 'text/plain; charset=utf-8' });
      res.end(ids.join('\n'));
    } else {
      res.writeHead(404);
      res.end();
    }
  };
  const httpServer = USE_TLS
    ? https.createServer(
        { key: fs.readFileSync(TLS_KEY), cert: fs.readFileSync(TLS_CERT) },
        requestHandler
      )
    : http.createServer(requestHandler);

  const wss = new WebSocket.Server({ server: httpServer });
  let nextClientId = 1;

  function sendErr (ws, code, msg) {
    if (ws.readyState === WebSocket.OPEN) ws.send(`ERR ${code} ${msg}`);
  }

  // 把 snapshot + updates 按序下发给单个客户端
  async function sendSyncState (ws, entry) {
    const { snapshot, updates } = await entry.stateForSync();
    let sent = 0;
    if (snapshot && ws.readyState === WebSocket.OPEN) {
      ws.send(snapshot);
      sent += 1;
    }
    for (const u of updates) {
      if (ws.readyState !== WebSocket.OPEN) return;
      ws.send(u);
      sent += 1;
    }
    console.log(
      `${ts()} [client:${ws.clientId}] 同步下发完成 [doc:${entry.docId}] snapshot+${updates.length} updates（共 ${sent} 帧）`
    );
  }

  function broadcast (entry, from, payload) {
    let n = 0;
    for (const client of entry.clients) {
      if (client === from || client.readyState !== WebSocket.OPEN) continue;
      client.send(payload);
      n += 1;
    }
    return n;
  }

  function leaveDoc (ws) {
    const entry = ws.docEntry;
    if (!entry) return;
    entry.clients.delete(ws);
    ws.docEntry = null;
    console.log(
      `${ts()} [client:${ws.clientId}] 离开文档 [doc:${entry.docId}]，剩余 ${entry.clients.size} 人`
    );
    registry.maybeUnload(entry);
  }

  function joinDoc (ws, entry) {
    leaveDoc(ws);
    entry.clients.add(ws);
    ws.docEntry = entry;
    ws.send(`DOC ${entry.docId}`);
    console.log(
      `${ts()} [client:${ws.clientId}] 加入文档 [doc:${entry.docId}]，当前 ${entry.clients.size} 人`
    );
    // 补发历史：新加入者、断线重连者经此恢复到最新状态（import 幂等）
    sendSyncState(ws, entry).catch((err) => {
      console.error(`${ts()} [client:${ws.clientId}] 同步下发失败:`, err);
    });
  }

  function onTextMessage (ws, text) {
    const [cmd, ...rest] = text.split(' ');
    const arg = rest.join(' ').trim();
    switch (cmd) {
      case 'CREATE': {
        const docId = crypto.randomUUID();
        registry
          .create(docId)
          .then((entry) => {
            console.log(`${ts()} [client:${ws.clientId}] 创建文档 [doc:${docId}]`);
            joinDoc(ws, entry);
          })
          .catch((err) => {
            console.error(`${ts()} [client:${ws.clientId}] 创建文档失败:`, err);
            sendErr(ws, 'INTERNAL', 'create_failed');
          });
        return;
      }
      case 'JOIN': {
        if (!arg) return sendErr(ws, 'BAD_REQUEST', 'join_requires_doc_id');
        const entry = registry.get(arg);
        if (!entry) return sendErr(ws, 'NO_SUCH_DOC', arg);
        joinDoc(ws, entry);
        return;
      }
      case 'RESYNC': {
        if (!ws.docEntry) return sendErr(ws, 'BAD_REQUEST', 'not_in_doc');
        sendSyncState(ws, ws.docEntry).catch((err) => {
          console.error(`${ts()} [client:${ws.clientId}] RESYNC 失败:`, err);
          sendErr(ws, 'INTERNAL', 'resync_failed');
        });
        return;
      }
      case 'PING':
        ws.send('PONG');
        return;
      default:
        sendErr(ws, 'BAD_REQUEST', `unknown_command:${cmd}`);
    }
  }

  wss.on('connection', (ws) => {
    ws.clientId = nextClientId++;
    ws.docEntry = null;
    console.log(
      `${ts()} [client:${ws.clientId}] 新连接，当前总连接数 ${wss.clients.size}`
    );

    ws.on('message', (data, isBinary) => {
      // 延迟模拟：>0 时拦截所有消息，延迟后再处理（弱网行为测试）
      const handle = () => {
        if (!isBinary) {
          const text = data.toString('utf8').trim();
          console.log(`${ts()} [client:${ws.clientId}] 文本消息: ${text.slice(0, 100)}`);
          try {
            onTextMessage(ws, text);
          } catch (err) {
            console.error(`${ts()} [client:${ws.clientId}] 消息处理异常:`, err);
            sendErr(ws, 'INTERNAL', 'message_handler_error');
          }
          return;
        }
        // 二进制帧：Loro update（或创建者的初始 snapshot；服务端不区分，
        // import 均幂等）。记账 → 广播 → 异步落盘。
        const entry = ws.docEntry;
        if (!entry) {
          console.log(`${ts()} [client:${ws.clientId}] 未加入文档，忽略 ${data.length} 字节二进制帧`);
          return;
        }
        entry.applyUpdate(Buffer.from(data), (payload) => {
          const n = broadcast(entry, ws, payload);
          console.log(
            `${ts()} [client:${ws.clientId}] update seq=${entry.seq}，${payload.length} 字节，广播给 ${n} 人 [doc:${entry.docId}]`
          );
        }).catch((err) => {
          console.error(`${ts()} [client:${ws.clientId}] update 处理失败:`, err);
          sendErr(ws, 'INTERNAL', 'update_failed');
        });
      };
      if (LATENCY_MS > 0) {
        console.log(
          `${ts()} [client:${ws.clientId}] 收到消息（${isBinary ? 'binary' : 'text'} ${data.length}B），延迟 ${LATENCY_MS}ms 后处理`
        );
        setTimeout(handle, LATENCY_MS);
      } else {
        handle();
      }
    });

    ws.on('close', (code, reason) => {
      console.log(
        `${ts()} [client:${ws.clientId}] 连接关闭 (code=${code}, reason=${reason || '无'})`
      );
      leaveDoc(ws);
    });

    ws.on('error', (err) => {
      console.error(`${ts()} [client:${ws.clientId}] WebSocket error:`, err);
    });
  });

  httpServer.listen(PORT, HOST, () => {
    console.log(
      `${ts()} [server] Mogan Loro 协作服务 listening on ${USE_TLS ? 'wss' : 'ws'}://${HOST}:${PORT}`
    );
  });

  // 优雅退出：先把在途写入与快照落盘，再退出
  let shuttingDown = false;
  async function shutdown (signal) {
    if (shuttingDown) return;
    shuttingDown = true;
    console.log(`${ts()} [server] 收到 ${signal}，正在落盘并关闭...`);
    wss.close();
    for (const ws of wss.clients) ws.terminate();
    try {
      await registry.flushAll();
    } catch (err) {
      console.error(`${ts()} [server] 关闭时落盘失败:`, err);
    }
    httpServer.close(() => process.exit(0));
    setTimeout(() => process.exit(1), 5000).unref();
  }
  process.on('SIGINT', () => shutdown('SIGINT'));
  process.on('SIGTERM', () => shutdown('SIGTERM'));
}

main().catch((err) => {
  console.error('server 启动失败:', err);
  process.exit(1);
});
