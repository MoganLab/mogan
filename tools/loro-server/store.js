'use strict';
// 文档持久化存储层：每个文档一个目录，内含
//   snapshot.bin  最近一次 export(Snapshot) 的完整状态
//   updates.log   snapshot 之后的增量 commit 记录（[4B 大端长度][payload] 重复）
//   meta.json     { docId, createdAt, snapshotSeq, updateCount }
// 写 snapshot 用临时文件 + rename 保证原子性；updates.log 只追加。
const fs = require('fs');
const fsp = fs.promises;
const path = require('path');

class DocStore {
  constructor (dataDir) {
    this.dataDir = dataDir;
  }

  async init () {
    await fsp.mkdir(this.dataDir, { recursive: true });
  }

  docDir (docId) {
    return path.join(this.dataDir, docId);
  }

  async listDocs () {
    const entries = await fsp.readdir(this.dataDir, { withFileTypes: true });
    return entries.filter((e) => e.isDirectory()).map((e) => e.name);
  }

  async createDoc (docId) {
    const dir = this.docDir(docId);
    await fsp.mkdir(dir, { recursive: false }).catch((err) => {
      if (err.code !== 'EEXIST') throw err;
    });
    const meta = {
      docId,
      createdAt: new Date().toISOString(),
      snapshotSeq: 0,
      updateCount: 0,
    };
    await this.writeMeta(docId, meta);
    return meta;
  }

  async hasDoc (docId) {
    try {
      await fsp.access(path.join(this.docDir(docId), 'meta.json'));
      return true;
    } catch {
      return false;
    }
  }

  async readMeta (docId) {
    const raw = await fsp.readFile(path.join(this.docDir(docId), 'meta.json'), 'utf8');
    return JSON.parse(raw);
  }

  async writeMeta (docId, meta) {
    const file = path.join(this.docDir(docId), 'meta.json');
    const tmp = file + '.tmp';
    await fsp.writeFile(tmp, JSON.stringify(meta, null, 2));
    await fsp.rename(tmp, file);
  }

  // 追加一条 commit/update 记录，返回追加后的总条数
  async appendUpdate (docId, payload) {
    const file = path.join(this.docDir(docId), 'updates.log');
    const header = Buffer.alloc(4);
    header.writeUInt32BE(payload.length, 0);
    await fsp.appendFile(file, Buffer.concat([header, payload]));
    const meta = await this.readMeta(docId);
    meta.updateCount += 1;
    await this.writeMeta(docId, meta);
    return meta.updateCount;
  }

  // 覆盖 snapshot.bin 并截断 updates.log；snapshotSeq 记为当前已持久化的 update 条数
  async writeSnapshot (docId, snapshot) {
    const dir = this.docDir(docId);
    const snapFile = path.join(dir, 'snapshot.bin');
    const tmp = snapFile + '.tmp';
    await fsp.writeFile(tmp, snapshot);
    await fsp.rename(tmp, snapFile);
    await fsp.writeFile(path.join(dir, 'updates.log'), Buffer.alloc(0));
    const meta = await this.readMeta(docId);
    meta.snapshotSeq = meta.updateCount;
    await this.writeMeta(docId, meta);
    return meta;
  }

  // 读出 snapshot 与其后的全部 update（按写入顺序）
  async readState (docId) {
    const dir = this.docDir(docId);
    let snapshot = null;
    try {
      snapshot = await fsp.readFile(path.join(dir, 'snapshot.bin'));
      if (snapshot.length === 0) snapshot = null;
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    const updates = [];
    try {
      const log = await fsp.readFile(path.join(dir, 'updates.log'));
      let off = 0;
      while (off + 4 <= log.length) {
        const len = log.readUInt32BE(off);
        updates.push(log.subarray(off + 4, off + 4 + len));
        off += 4 + len;
      }
      if (off !== log.length) {
        // 尾部半条记录：上次写入中途崩溃，截断忽略（CRDT 允许丢尾部 update，客户端会重发）
        console.warn(`[store] ${docId}: updates.log 尾部 ${log.length - off} 字节不完整，忽略`);
      }
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    return { snapshot, updates };
  }
}

module.exports = { DocStore };
