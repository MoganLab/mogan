'use strict';
// 服务端集成测试：用真实 LoroDoc 客户端驱动完整生命周期。
//   1. CREATE → 分配 UUID，磁盘初始化 meta.json
//   2. 客户端 A 提交编辑（二进制帧）→ 服务端记账 + 广播 + 落盘
//   3. 客户端 B JOIN → 收到 snapshot/updates 后内容与 A 一致
//   4. B 编辑 → A 收到增量并收敛
//   5. 服务端重启 → 从磁盘恢复，新客户端 JOIN 仍拿到完整状态
//   6. RESYNC → 重复下发历史，import 幂等，内容不变
//   7. 错误路径：JOIN 不存在的文档 → ERR NO_SUCH_DOC
const { test, before, after } = require('node:test');
const assert = require('node:assert');
const { spawn } = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const WebSocket = require('ws');
const { LoroDoc } = require('loro-crdt');

const PORT = 18765;
const URL = `ws://127.0.0.1:${PORT}`;

let dataDir;
let serverProc = null;

function startServer () {
  return new Promise((resolve, reject) => {
    serverProc = spawn(process.execPath, [path.join(__dirname, '..', 'server.js')], {
      env: {
        ...process.env,
        MOGAN_LORO_HOST: '127.0.0.1',
        MOGAN_LORO_PORT: String(PORT),
        MOGAN_LORO_DATA_DIR: dataDir,
      },
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    serverProc.stdout.on('data', (d) => process.stderr.write(`[server] ${d}`));
    serverProc.stderr.on('data', (d) => process.stderr.write(`[server:err] ${d}`));
    serverProc.on('exit', (code) => {
      if (code !== 0 && code !== null) reject(new Error(`server exited ${code}`));
    });
    // 等 healthz 可用再视为就绪
    const deadline = Date.now() + 10000;
    (function poll () {
      fetch(`http://127.0.0.1:${PORT}/healthz`)
        .then(() => resolve())
        .catch(() => {
          if (Date.now() > deadline) reject(new Error('server 启动超时'));
          else setTimeout(poll, 100);
        });
    })();
  });
}

async function stopServer () {
  if (!serverProc) return;
  const p = serverProc;
  serverProc = null;
  p.kill('SIGTERM');
  await new Promise((resolve) => p.on('exit', resolve));
}

// 最小测试客户端：文本帧进队列，二进制帧直接 import 进本地 LoroDoc
class TestClient {
  constructor () {
    this.doc = new LoroDoc();
    this.control = []; // 收到的文本帧
    this.waiters = [];
  }

  connect () {
    this.ws = new WebSocket(URL);
    this.ws.on('message', (data, isBinary) => {
      if (isBinary) {
        this.doc.import(data);
        this.notify();
      } else {
        this.control.push(data.toString('utf8'));
        this.notify();
      }
    });
    return new Promise((resolve) => this.ws.on('open', resolve));
  }

  notify () {
    const ws = this.waiters.splice(0);
    for (const w of ws) w();
  }

  send (text) {
    this.ws.send(text);
  }

  sendUpdate () {
    // 把本地未同步的修改作为二进制帧发出（模拟编辑器 broadcast_update）
    this.ws.send(this.doc.export({ mode: 'update' }));
  }

  waitFor (pred, timeoutMs = 5000) {
    return new Promise((resolve, reject) => {
      // 消息触发 + 周期轮询双保险：文件系统等外部状态变化不产生 WS 消息
      const timer = setTimeout(() => {
        clearInterval(iv);
        reject(new Error('waitFor 超时'));
      }, timeoutMs);
      const check = () => {
        let ok = false;
        try {
          ok = pred();
        } catch {
          ok = false;
        }
        if (ok) {
          clearTimeout(timer);
          clearInterval(iv);
          const i = this.waiters.indexOf(check);
          if (i >= 0) this.waiters.splice(i, 1);
          resolve();
        }
      };
      const iv = setInterval(check, 50);
      this.waiters.push(check);
      check();
    });
  }

  close () {
    this.ws.close();
  }
}

function lastDocId (client) {
  const docMsgs = client.control.filter((m) => m.startsWith('DOC '));
  assert.ok(docMsgs.length > 0, '应收到 DOC 确认');
  return docMsgs[docMsgs.length - 1].slice(4).trim();
}

before(async () => {
  dataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'loro-server-test-'));
  await startServer();
});

after(async () => {
  await stopServer();
  fs.rmSync(dataDir, { recursive: true, force: true });
});

test('完整生命周期：创建、协作、重连、重启恢复、RESYNC', async () => {
  // 1. A 创建文档
  const a = new TestClient();
  await a.connect();
  a.send('CREATE');
  await a.waitFor(() => a.control.some((m) => m.startsWith('DOC ')));
  const docId = lastDocId(a);
  assert.match(docId, /^[0-9a-f-]{36}$/, 'docId 应为 UUID');
  assert.ok(
    fs.existsSync(path.join(dataDir, docId, 'meta.json')),
    '磁盘应初始化 meta.json'
  );

  // 2. A 编辑并发 update
  a.doc.getText('content').insert(0, 'hello from A');
  a.doc.commit();
  a.sendUpdate();
  await a.waitFor(
    () =>
      fs.existsSync(path.join(dataDir, docId, 'updates.log')) &&
      fs.statSync(path.join(dataDir, docId, 'updates.log')).size > 0
  );

  // 3. B JOIN → 同步到 A 的内容
  const b = new TestClient();
  await b.connect();
  b.send(`JOIN ${docId}`);
  await b.waitFor(() => b.control.some((m) => m.startsWith('DOC ')));
  await b.waitFor(() => b.doc.getText('content').toString() === 'hello from A');

  // 4. B 编辑 → A 收敛
  b.doc.getText('content').insert(b.doc.getText('content').length, ' + B');
  b.doc.commit();
  b.sendUpdate();
  await a.waitFor(
    () => a.doc.getText('content').toString() === 'hello from A + B'
  );

  // 5. 服务端重启 → C 拿到完整状态
  a.close();
  b.close();
  await stopServer();
  await startServer();
  const c = new TestClient();
  await c.connect();
  c.send(`JOIN ${docId}`);
  await c.waitFor(
    () => c.doc.getText('content').toString() === 'hello from A + B'
  );

  // 6. RESYNC 幂等：重复下发历史，内容不变
  c.send('RESYNC');
  await new Promise((r) => setTimeout(r, 500));
  assert.strictEqual(c.doc.getText('content').toString(), 'hello from A + B');
  c.close();
});

test('JOIN 不存在的文档返回 NO_SUCH_DOC', async () => {
  const c = new TestClient();
  await c.connect();
  c.send('JOIN 00000000-0000-0000-0000-000000000000');
  await c.waitFor(() => c.control.some((m) => m.startsWith('ERR NO_SUCH_DOC')));
  c.close();
});

test('CURSOR 帧原样转发给同文档其他客户端（排除发送者）', async () => {
  const a = new TestClient();
  await a.connect();
  a.send('CREATE');
  await a.waitFor(() => a.control.some((m) => m.startsWith('DOC ')));
  const docId = lastDocId(a);
  const b = new TestClient();
  await b.connect();
  b.send(`JOIN ${docId}`);
  await b.waitFor(() => b.control.some((m) => m.startsWith('DOC ')));

  // 多光标帧：瞬态、不落盘，服务端应原样转发
  const frame = 'CURSOR p1 a1b2c3d4e5f60718:3:4 a1b2c3d4e5f60718:3:4 a1b2c3d4e5f60718:3:9';
  a.send(frame);
  await b.waitFor(() => b.control.includes(frame));
  // 发送者自己不应收到（broadcast 排除 from）
  await new Promise((r) => setTimeout(r, 300));
  assert.ok(!a.control.includes(frame), '发送者不应收到自己的 CURSOR 帧');
  a.close();
  b.close();
});

test('snapshot 截断：累积超过阈值后生成 snapshot.bin 并清空 updates.log', async () => {
  const a = new TestClient();
  await a.connect();
  a.send('CREATE');
  await a.waitFor(() => a.control.some((m) => m.startsWith('DOC ')));
  const docId = lastDocId(a);

  // 发送 105 条 update（阈值 100）
  for (let i = 0; i < 105; i++) {
    a.doc.getText('content').insert(a.doc.getText('content').length, `${i} `);
    a.doc.commit();
    a.sendUpdate();
  }
  // 等合并窗口（5s）后的 snapshot 落盘
  await a.waitFor(
    () =>
      fs.existsSync(path.join(dataDir, docId, 'snapshot.bin')) &&
      fs.statSync(path.join(dataDir, docId, 'snapshot.bin')).size > 0 &&
      fs.statSync(path.join(dataDir, docId, 'updates.log')).size === 0,
    15000
  );

  // 新客户端从 snapshot 恢复
  const b = new TestClient();
  await b.connect();
  b.send(`JOIN ${docId}`);
  await b.waitFor(() => b.doc.getText('content').length > 0);
  assert.strictEqual(
    b.doc.getText('content').toString(),
    a.doc.getText('content').toString()
  );
  a.close();
  b.close();
});
