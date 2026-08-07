'use strict';
// DocStore 纯磁盘层测试：不 spawn 服务端，直接实例化 + 临时目录。
// 重点验证落盘健壮性增强：fsync 开关、snapshot.bak 回退、meta 损坏重建。
const { test } = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { DocStore } = require('../store');

function mkStore () {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'loro-store-test-'));
  const store = new DocStore(dir);
  return { dir, store, cleanup: () => fs.rmSync(dir, { recursive: true, force: true }) };
}

test('createDoc 初始化 meta.json，hasDoc 命中，readMeta 字段正确', async () => {
  const { store, cleanup } = mkStore();
  try {
    await store.init();
    await store.createDoc('doc1', '名');
    assert.ok(await store.hasDoc('doc1'));
    const meta = await store.readMeta('doc1');
    assert.strictEqual(meta.updateCount, 0);
    assert.strictEqual(meta.snapshotSeq, 0);
    assert.strictEqual(meta.name, '名');
  } finally {
    cleanup();
  }
});

test('appendUpdate 累计 updateCount，readState 按序读回', async () => {
  const { store, cleanup } = mkStore();
  try {
    await store.init();
    await store.createDoc('doc1');
    const n = await store.appendUpdate('doc1', Buffer.from('aaa'));
    assert.strictEqual(n, 1);
    await store.appendUpdate('doc1', Buffer.from('bb'));
    const { snapshot, updates } = await store.readState('doc1');
    assert.strictEqual(snapshot, null);
    assert.strictEqual(updates.length, 2);
    assert.strictEqual(updates[0].toString(), 'aaa');
    assert.strictEqual(updates[1].toString(), 'bb');
    const meta = await store.readMeta('doc1');
    assert.strictEqual(meta.updateCount, 2);
  } finally {
    cleanup();
  }
});

test('writeSnapshot 保留上一代为 snapshot.bin.bak', async () => {
  const { store, cleanup, dir } = mkStore();
  const d = path.join(dir, 'doc1');
  try {
    await store.init();
    await store.createDoc('doc1');
    await store.writeSnapshot('doc1', Buffer.from('SNAP-1'));
    assert.ok(!fs.existsSync(path.join(d, 'snapshot.bin.bak')), '首次 snapshot 无前代，不应有 .bak');
    assert.strictEqual(fs.readFileSync(path.join(d, 'snapshot.bin')).toString(), 'SNAP-1');

    await store.writeSnapshot('doc1', Buffer.from('SNAP-2'));
    assert.ok(fs.existsSync(path.join(d, 'snapshot.bin.bak')), '二次 snapshot 应保留上一代 .bak');
    assert.strictEqual(
      fs.readFileSync(path.join(d, 'snapshot.bin.bak')).toString(),
      'SNAP-1',
      '.bak 为上一代内容'
    );
    assert.strictEqual(fs.readFileSync(path.join(d, 'snapshot.bin')).toString(), 'SNAP-2');
  } finally {
    cleanup();
  }
});

test('readState：snapshot.bin 为空/缺失时回退读 .bak', async () => {
  const { store, cleanup, dir } = mkStore();
  const d = path.join(dir, 'doc1');
  try {
    await store.init();
    await store.createDoc('doc1');
    await store.writeSnapshot('doc1', Buffer.from('FIRST'));
    await store.writeSnapshot('doc1', Buffer.from('SECOND')); // .bin=SECOND, .bak=FIRST

    // 情形1：snapshot.bin 被截空（写入到一半崩溃的典型残骸）→ 回退 .bak
    fs.writeFileSync(path.join(d, 'snapshot.bin'), Buffer.alloc(0));
    let st = await store.readState('doc1');
    assert.strictEqual(st.snapshot.toString(), 'FIRST');

    // 情形2：snapshot.bin 被删除 → 回退 .bak
    fs.rmSync(path.join(d, 'snapshot.bin'));
    st = await store.readState('doc1');
    assert.strictEqual(st.snapshot.toString(), 'FIRST');
  } finally {
    cleanup();
  }
});

test('readMeta：meta.json 损坏时按 updates.log 帧数重建计数', async () => {
  const { store, cleanup, dir } = mkStore();
  try {
    await store.init();
    await store.createDoc('doc1');
    await store.appendUpdate('doc1', Buffer.from('aaa'));
    await store.appendUpdate('doc1', Buffer.from('bb'));
    fs.writeFileSync(path.join(dir, 'doc1', 'meta.json'), '{ 不是合法 json');
    const meta = await store.readMeta('doc1');
    assert.strictEqual(meta.updateCount, 2, '按 updates.log 帧数重建');
    assert.strictEqual(meta.snapshotSeq, 0, '无 snapshot → 0');
    assert.strictEqual(meta.name, null);
  } finally {
    cleanup();
  }
});

test('reconstructMeta：有 snapshot 时 snapshotSeq 取 updateCount', async () => {
  const { store, cleanup } = mkStore();
  try {
    await store.init();
    await store.createDoc('doc1');
    await store.appendUpdate('doc1', Buffer.from('a'));
    await store.writeSnapshot('doc1', Buffer.from('snap')); // 截断 updates.log
    await store.appendUpdate('doc1', Buffer.from('b')); // snapshot 后再 1 条
    const meta = await store.reconstructMeta('doc1');
    assert.strictEqual(meta.updateCount, 1, 'snapshot 后只剩 1 条 update');
    assert.strictEqual(meta.snapshotSeq, 1, '有 snapshot → snapshotSeq=updateCount');
  } finally {
    cleanup();
  }
});

test('readMeta：meta.json 真不存在时抛 ENOENT', async () => {
  const { store, cleanup, dir } = mkStore();
  try {
    await store.init();
    await store.createDoc('doc1');
    fs.rmSync(path.join(dir, 'doc1', 'meta.json'));
    await assert.rejects(
      () => store.readMeta('doc1'),
      (err) => err.code === 'ENOENT'
    );
  } finally {
    cleanup();
  }
});

test('appendUpdate：FSYNC 开关控制是否真正 fsync', async () => {
  const { store, cleanup, dir } = mkStore();
  await store.init();
  await store.createDoc('doc1');

  // fsyncFile 通过 fsp.open(file,'r') 打开目标文件；fsp.appendFile 走内部
  // binding 不经 fsp.open，故只拦截 fsp.open 即可区分是否 fsync。
  const fsp = fs.promises;
  const origOpen = fsp.open;
  const logPath = path.join(dir, 'doc1', 'updates.log');
  let openedLog = false;
  fsp.open = async function (file, ...rest) {
    if (String(file) === logPath) openedLog = true;
    return origOpen.call(this, file, ...rest);
  };
  const orig = process.env.MOGAN_LORO_FSYNC;
  try {
    process.env.MOGAN_LORO_FSYNC = 'on';
    openedLog = false;
    await store.appendUpdate('doc1', Buffer.from('a'));
    assert.ok(openedLog, 'FSYNC=on 时 appendUpdate 应 fsync（打开 updates.log）');

    process.env.MOGAN_LORO_FSYNC = 'off';
    openedLog = false;
    await store.appendUpdate('doc1', Buffer.from('b'));
    assert.strictEqual(openedLog, false, 'FSYNC=off 时不应 fsync');
  } finally {
    fsp.open = origOpen;
    if (orig === undefined) delete process.env.MOGAN_LORO_FSYNC;
    else process.env.MOGAN_LORO_FSYNC = orig;
    cleanup();
  }
});
