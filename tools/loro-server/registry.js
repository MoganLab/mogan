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

  // force=true 时无论 bytesSinceSnapshot 都重写 snapshot（关停用，确保落盘最新态）。
  async flushSnapshot (force = false) {
    if (!this.shadow) return;
    if (!force && this.bytesSinceSnapshot === 0) return;
    this.shadow.commit (); // 确保待提交 op 进 oplog，export snapshot 才完整
    const snap = this.shadow.export ({ mode: 'snapshot' });
    console.log (
      `[store] ${this.docId}: 落盘 snapshot ${snap.length} 字节（seq=${this.seq}）`
    );
    await this.store.writeSnapshot (this.docId, Buffer.from (snap));
    this.snapshottedSeq = this.seq;
    this.bytesSinceSnapshot = 0;
  }

  // 供新连接/RESYNC：当前持久化状态（snapshot + 其后 updates）
  async stateForSync () {
    // 先落盘在途的写入，保证读到完整状态
    await this.persistQueue;
    return this.store.readState (this.docId);
  }
}

class DocRegistry {
  constructor (store) {
    this.store = store;
    this.docs = new Map(); // docId -> DocEntry
    this.shuttingDown = false; // 关停期间禁止 unload（见 maybeUnload）
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
    // 关停期间不卸载：flushAll 正在 load/flush，卸载会把 shadow 置 null 导致
    // load 的 import 崩溃（await readState 让出期间被 unload 打断）。
    if (this.shuttingDown) return;
    if (entry.clients.size === 0) entry.unload();
  }

  // 关停落盘：逐文档串行（避免并发），每个先 await persistQueue + load() 确保
  // shadow 是完整磁盘态（即便刚因 client 离开被 unload 也能重载），再 force flush。
  // 旧实现 Promise.all 并发 + 依赖内存 shadow，client 断开触发的 unload 会让
  // flushSnapshot 因 shadow=null 跳过 → snapshot 不写或写残缺。
  async flushAll () {
    for (const e of [...this.docs.values ()]) {
      await e.persistQueue;
      if (e.seq === 0 && !e.shadow) continue; // 无任何数据可落盘
      await e.load ();
      await e.flushSnapshot (true);
    }
  }
}

module.exports = { DocRegistry, DocEntry };
