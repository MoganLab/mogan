'use strict';
// 文档注册表：管理所有协作文档的生命周期。
// 每个文档在内存中维护一个 LoroDoc 影子实例——只用于推进版本向量
// （import 记账）与按需导出 snapshot，不解析文档内容；内容一致性由
// 客户端 CRDT 引擎保证。房间无人后影子卸载，状态全部落在磁盘。
const { LoroDoc } = require('loro-crdt');

const SNAPSHOT_UPDATE_THRESHOLD = 100; // updates.log 达到该条数即截断为 snapshot
const SNAPSHOT_BYTE_THRESHOLD = 1024 * 1024; // 或自上次 snapshot 以来累计字节数

class DocEntry {
  constructor (docId, store) {
    this.docId = docId;
    this.store = store;
    this.shadow = null; // LoroDoc 影子，懒加载
    this.seq = 0; // 已持久化的 update 总条数
    this.snapshottedSeq = 0; // 上次 snapshot 对应的 seq（阈值判断基准）
    this.bytesSinceSnapshot = 0;
    this.clients = new Set();
    this.flushTimer = null;
    this.persistQueue = Promise.resolve(); // 串行化该文档的落盘操作
  }

  async load () {
    if (this.shadow) return;
    this.shadow = new LoroDoc();
    const { snapshot, updates } = await this.store.readState(this.docId);
    if (snapshot) this.shadow.import(snapshot);
    for (const u of updates) this.shadow.import(u);
    const meta = await this.store.readMeta(this.docId);
    this.seq = meta.updateCount;
    this.snapshottedSeq = meta.snapshotSeq;
  }

  unload () {
    this.shadow = null;
    this.bytesSinceSnapshot = 0;
    this.snapshottedSeq = this.seq;
  }

  // 记录一条客户端 update：影子 import（推进 vv）、广播、排队落盘。
  // 注意：本方法会被并发调用（ws message 事件 fire-and-forget），
  // 所有共享状态（seq/影子/字节计数）只能由 persistQueue 串行链触碰，
  // await 之外的同步段不得读写它们——否则并发交错会造成 seq 竞态。
  applyUpdate (payload, broadcast) {
    this.persistQueue = this.persistQueue.then(async () => {
      await this.load();
      this.shadow.import(payload); // 仅记账：CRDT merge 是幂等的
      this.seq += 1;
      this.bytesSinceSnapshot += payload.length;
      broadcast(payload);
      await this.store.appendUpdate(this.docId, payload);
      if (this.shouldSnapshot()) this.scheduleFlush();
    });
    return this.persistQueue;
  }

  shouldSnapshot () {
    return (
      this.seq - this.snapshottedSeq >= SNAPSHOT_UPDATE_THRESHOLD ||
      this.bytesSinceSnapshot >= SNAPSHOT_BYTE_THRESHOLD
    );
  }

  scheduleFlush () {
    if (this.flushTimer) return;
    this.flushTimer = setTimeout(() => {
      this.flushTimer = null;
      this.persistQueue = this.persistQueue.then(() => this.flushSnapshot());
    }, 5000); // 合并窗口：阈值触发后等一波再统一落盘
  }

  async flushSnapshot () {
    if (!this.shadow || this.bytesSinceSnapshot === 0) return;
    const snap = this.shadow.export({ mode: 'snapshot' });
    await this.store.writeSnapshot(this.docId, Buffer.from(snap));
    this.snapshottedSeq = this.seq;
    this.bytesSinceSnapshot = 0;
  }

  // 供新连接/RESYNC：当前持久化状态（snapshot + 其后 updates）
  async stateForSync () {
    // 先落盘在途的写入，保证读到完整状态
    await this.persistQueue;
    return this.store.readState(this.docId);
  }
}

class DocRegistry {
  constructor (store) {
    this.store = store;
    this.docs = new Map(); // docId -> DocEntry
  }

  async restore () {
    for (const docId of await this.store.listDocs()) {
      if (await this.store.hasDoc(docId)) {
        this.docs.set(docId, new DocEntry(docId, this.store));
      }
    }
    return this.docs.size;
  }

  async create (docId) {
    await this.store.createDoc(docId);
    const entry = new DocEntry(docId, this.store);
    this.docs.set(docId, entry);
    return entry;
  }

  get (docId) {
    return this.docs.get(docId) || null;
  }

  maybeUnload (entry) {
    if (entry.clients.size === 0) entry.unload();
  }

  async flushAll () {
    await Promise.all(
      [...this.docs.values()].map((e) =>
        e.persistQueue.then(() => e.flushSnapshot())
      )
    );
  }
}

module.exports = { DocRegistry, DocEntry };
