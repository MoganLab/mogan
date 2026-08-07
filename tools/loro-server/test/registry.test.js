'use strict';
// DocEntry 容错测试：不 spawn 服务端，用桩 store 注入故障。
// 重点验证落盘健壮性增强：persistQueue 单点错误不致永久停摆、snapshot 全量自愈、
// snapshot 解析失败回退 .bak。
const { test } = require('node:test');
const assert = require('node:assert');
const { LoroDoc } = require('loro-crdt');
const { DocEntry } = require('../registry');

// 由独立 LoroDoc 产出一条可被 import 的 update 载荷
function makeUpdate (text) {
  const d = new LoroDoc();
  d.getText('content').insert(0, text);
  d.commit();
  return Buffer.from(d.export({ mode: 'update' }));
}

// 桩 store：appendUpdate 前 failAppends 次抛错，之后成功；记录已 append 与 snapshot。
// 状态挂在 this 上（非闭包变量），以便测试从外部读取最新值。
function makeStubStore ({ failAppends = 0 } = {}) {
  let calls = 0;
  return {
    appended: [],
    snapshotted: null,
    async readState () {
      return {
        snapshot: this.snapshotted ? Buffer.from(this.snapshotted) : null,
        updates: this.appended.slice(),
      };
    },
    async readMeta () {
      return {
        docId: 'd',
        updateCount: this.appended.length,
        snapshotSeq: this.snapshotted ? this.appended.length : 0,
        name: null,
      };
    },
    async appendUpdate (_docId, payload) {
      calls += 1;
      if (calls <= failAppends) throw new Error('disk full');
      this.appended.push(payload);
      return this.appended.length;
    },
    async writeSnapshot (_docId, snap) {
      this.snapshotted = snap;
      return this.readMeta();
    },
    async readSnapshotBak () {
      return null;
    },
  };
}

function clearTimer (entry) {
  if (entry.flushTimer) {
    clearTimeout(entry.flushTimer);
    entry.flushTimer = null;
  }
}

test('persistQueue 容错：appendUpdate 连续抛错不中毒链，后续 snapshot 全量自愈', async () => {
  const store = makeStubStore({ failAppends: 2 }); // 前 2 条 append 失败
  const entry = new DocEntry('d', store);

  const u1 = makeUpdate('aaa');
  const u2 = makeUpdate('bbb');
  const u3 = makeUpdate('ccc');

  entry.applyUpdate(u1, () => {});
  entry.applyUpdate(u2, () => {});
  entry.applyUpdate(u3, () => {}); // 第 3 条成功：证明队列未中毒（body 仍能跑到）
  await entry.persistQueue; // 若被中毒会 reject → 用例崩

  assert.strictEqual(store.appended.length, 1, '只有第 3 条成功 append');
  assert.deepStrictEqual(store.appended[0], u3);

  // 影子已 import 全部 3 条（含 append 失败的 u1/u2）→ force snapshot 应导出含全部内容
  await entry.flushSnapshot(true);
  assert.ok(store.snapshotted, '应已写 snapshot');

  const recovered = new LoroDoc();
  recovered.import(store.snapshotted);
  const text = recovered.getText('content').toString();
  assert.ok(
    text.includes('aaa') && text.includes('bbb') && text.includes('ccc'),
    `snapshot 应含全部 3 条编辑（自愈），实际: ${text}`
  );

  clearTimer(entry);
});

test('load：snapshot 解析失败（文件可读但 Loro 内容损坏）时回退读 .bak', async () => {
  const good = new LoroDoc();
  good.getText('content').insert(0, 'good');
  good.commit();
  const goodSnap = Buffer.from(good.export({ mode: 'snapshot' }));

  const store = {
    // 模拟 store 层无法察觉的损坏：snapshot.bin 读得出来但 Loro 解析不了
    async readState () {
      return { snapshot: Buffer.from('corrupt-bytes'), updates: [] };
    },
    async readMeta () {
      return { docId: 'd', updateCount: 0, snapshotSeq: 0, name: null };
    },
    async readSnapshotBak () {
      return goodSnap;
    },
  };
  const entry = new DocEntry('d', store);
  await entry.load();
  assert.strictEqual(
    entry.shadow.getText('content').toString(),
    'good',
    'snapshot 解析失败应回退 .bak 恢复'
  );
});

test('applyUpdate 失败后 persistQueue 仍可正常 await（stateForSync 不被阻塞）', async () => {
  const store = makeStubStore({ failAppends: 1 });
  const entry = new DocEntry('d', store);

  entry.applyUpdate(makeUpdate('x'), () => {}); // 失败，被 catch
  await entry.persistQueue;

  // stateForSync 依赖 await persistQueue；链未中毒故不应抛
  await assert.doesNotReject(() => entry.stateForSync());
  clearTimer(entry);
});
