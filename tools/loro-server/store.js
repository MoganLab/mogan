'use strict';
// 文档持久化存储层：每个文档一个目录，内含
//   snapshot.bin      最近一次 export(Snapshot) 的完整状态
//   snapshot.bin.bak  上一代 snapshot（写新快照前保留一代，作损坏回退兜底）
//   updates.log       snapshot 之后的增量 commit 记录（[4B 大端长度][payload] 重复）
//   meta.json         { docId, createdAt, snapshotSeq, updateCount, name? }
// 写 snapshot/meta 用临时文件 + rename 保证原子性；并在写数据后、rename 后 fsync
// 保证持久性（防断电/kill -9 丢最近编辑）。updates.log 只追加、每条追加后 fsync。
// fsync 默认开启；CI/基准可设 MOGAN_LORO_FSYNC=off 关闭。
const fs = require('fs');
const fsp = fs.promises;
const path = require('path');

// 实时读 env，便于测试在进程内切换开关
function fsyncEnabled () {
  return (process.env.MOGAN_LORO_FSYNC || 'on').toLowerCase() !== 'off';
}

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

  // best-effort 刷一个文件的数据到磁盘；失败仅记日志不抛，避免拖垮落盘链
  async fsyncFile (file) {
    if (!fsyncEnabled()) return;
    let fh;
    try {
      fh = await fsp.open(file, 'r');
      await fh.sync();
    } catch (err) {
      console.warn(`[store] fsync ${file} 失败（忽略）: ${err.message}`);
    } finally {
      if (fh) await fh.close().catch(() => {});
    }
  }

  // best-effort 刷目录的目录项（保证 rename/新建持久）。POSIX 上可 open 目录；
  // 某些平台/权限下失败属预期，静默忽略。
  async fsyncDir (dir) {
    if (!fsyncEnabled()) return;
    let fh;
    try {
      fh = await fsp.open(dir, 'r');
      await fh.sync();
    } catch {
      // 目录 fsync 在部分平台不支持，属预期，不打扰日志
    } finally {
      if (fh) await fh.close().catch(() => {});
    }
  }

  async listDocs () {
    const entries = await fsp.readdir(this.dataDir, { withFileTypes: true });
    return entries.filter((e) => e.isDirectory()).map((e) => e.name);
  }

  // 列出全部文档及其显示名（供 /docs）。单篇 meta 读取失败（缺失/损坏）
  // 不拖垮整个列表，该文档 name 记 null（客户端回退显示 docId）。
  async listDocsWithNames () {
    const ids = await this.listDocs();
    return Promise.all(
      ids.map(async (docId) => {
        try {
          const meta = await this.readMeta(docId);
          return { docId, name: typeof meta.name === 'string' ? meta.name : null };
        } catch {
          return { docId, name: null };
        }
      })
    );
  }

  async createDoc (docId, name = null) {
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
    if (name !== null) meta.name = name;
    await this.writeMeta(docId, meta);
    // 新建文档：刷 docDir（meta.json 的 dirent）与 dataDir（docDir 自身的 dirent），
    // 防断电后整个新文档目录连同其内文件一起消失。
    await this.fsyncDir(dir);
    await this.fsyncDir(this.dataDir);
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

  // 读 meta.json；损坏（非 ENOENT）时按 updates.log 重建最小可用 meta，避免
  // 一篇坏 meta 拖垮整个文档的落盘链。ENOENT 仍抛，交上层（hasDoc/load）处理。
  async readMeta (docId) {
    const file = path.join(this.docDir(docId), 'meta.json');
    let raw;
    try {
      raw = await fsp.readFile(file, 'utf8');
    } catch (err) {
      if (err.code === 'ENOENT') throw err;
      return this.reconstructMeta(docId);
    }
    try {
      return JSON.parse(raw);
    } catch {
      console.warn(`[store] ${docId}: meta.json 损坏，按 updates.log 重建计数`);
      return this.reconstructMeta(docId);
    }
  }

  // meta 丢失/损坏时的兜底：按 updates.log 帧计数得 updateCount，探测 snapshot.bin
  // 是否存在近似 snapshotSeq。复用 readState 的帧格式（[4B 大端长度][payload]）。
  async reconstructMeta (docId) {
    let updateCount = 0;
    try {
      const log = await fsp.readFile(path.join(this.docDir(docId), 'updates.log'));
      let off = 0;
      while (off + 4 <= log.length) {
        const len = log.readUInt32BE(off);
        if (off + 4 + len > log.length) break; // 尾部半条，不计
        updateCount += 1;
        off += 4 + len;
      }
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    const hasSnap = await fsp
      .access(path.join(this.docDir(docId), 'snapshot.bin'))
      .then(() => true)
      .catch(() => false);
    return {
      docId,
      createdAt: new Date(0).toISOString(),
      snapshotSeq: hasSnap ? updateCount : 0,
      updateCount,
      name: null,
    };
  }

  async writeMeta (docId, meta) {
    const dir = this.docDir(docId);
    const file = path.join(dir, 'meta.json');
    const tmp = file + '.tmp';
    await fsp.writeFile(tmp, JSON.stringify(meta, null, 2));
    await this.fsyncFile(tmp); // 数据先落盘，再 rename 切换 dirent
    await fsp.rename(tmp, file);
    // meta 仅记账、丢失可由下次 snapshot 校正，故不在此 per-edit fsync 目录；
    // snapshot 路径会在 writeSnapshot 中统一 fsyncDir。
  }

  // 追加一条 commit/update 记录，返回追加后的总条数
  async appendUpdate (docId, payload) {
    const file = path.join(this.docDir(docId), 'updates.log');
    const header = Buffer.alloc(4);
    header.writeUInt32BE(payload.length, 0);
    await fsp.appendFile(file, Buffer.concat([header, payload]));
    await this.fsyncFile(file); // 每条 edit 的内容保证落盘（防断电丢最近编辑）
    const meta = await this.readMeta(docId);
    meta.updateCount += 1;
    await this.writeMeta(docId, meta);
    return meta.updateCount;
  }

  // 覆盖 snapshot.bin 并截断 updates.log；snapshotSeq 记为当前已持久化的 update 条数。
  // 写新 snapshot 前保留上一代为 snapshot.bin.bak 作损坏回退。
  async writeSnapshot (docId, snapshot) {
    const dir = this.docDir(docId);
    const snapFile = path.join(dir, 'snapshot.bin');
    const bakFile = snapFile + '.bak';
    const tmp = snapFile + '.tmp';
    // 保留上一代 snapshot（若存在）作为 .bak 兜底
    try {
      await fsp.copyFile(snapFile, bakFile);
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    await fsp.writeFile(tmp, snapshot);
    await this.fsyncFile(tmp); // 快照数据先落盘
    await fsp.rename(tmp, snapFile);
    await this.fsyncDir(dir); // 保证 rename 的 dirent 持久
    const logFile = path.join(dir, 'updates.log');
    await fsp.writeFile(logFile, Buffer.alloc(0));
    await this.fsyncFile(logFile); // 截断也要落盘，避免重启后复活已并入快照的旧 update
    const meta = await this.readMeta(docId);
    meta.snapshotSeq = meta.updateCount;
    await this.writeMeta(docId, meta);
    return meta;
  }

  // 仅读 snapshot.bin.bak（供 registry 在 .bin 的 Loro 解析失败时兜底重试）。
  async readSnapshotBak (docId) {
    try {
      const buf = await fsp.readFile(path.join(this.docDir(docId), 'snapshot.bin.bak'));
      return buf.length > 0 ? buf : null;
    } catch (err) {
      if (err.code !== 'ENOENT') {
        console.warn(`[store] ${docId}: 读 snapshot.bin.bak 失败: ${err.message}`);
      }
      return null;
    }
  }

  // 读出 snapshot 与其后的全部 update（按写入顺序）。snapshot.bin 损坏/为空/缺失时
  // 回退读 snapshot.bin.bak，尽最大可能保住上一代好快照。
  async readState (docId) {
    const dir = this.docDir(docId);
    let snapshot = null;
    for (const cand of ['snapshot.bin', 'snapshot.bin.bak']) {
      try {
        const buf = await fsp.readFile(path.join(dir, cand));
        if (buf.length > 0) {
          snapshot = buf;
          break;
        }
      } catch (err) {
        if (err.code !== 'ENOENT') {
          // 读取异常（IO 错误等）：记日志后继续尝试下一个候选
          console.warn(`[store] ${docId}: 读 ${cand} 失败: ${err.message}`);
        }
      }
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
