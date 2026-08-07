'use strict';
// validateDocName 纯函数测试：不 spawn 服务端，直接 require 校验模块。
const { test } = require('node:test');
const assert = require('node:assert');
const { validateDocName } = require('../validate');

test('validateDocName 纯函数：边界与禁字符', () => {
  // 合法：CJK、空格、连字符、下划线（代理对按 1 个 code point 计）
  assert.strictEqual(validateDocName('文档'), '文档');
  assert.strictEqual(validateDocName('my doc_v2-final'), 'my doc_v2-final');
  assert.strictEqual(validateDocName('a'.repeat(64)), 'a'.repeat(64));
  assert.strictEqual(validateDocName('文'.repeat(64)), '文'.repeat(64));
  // trim：首尾空白去除
  assert.strictEqual(validateDocName('  hi  '), 'hi');
  // 未提供：空/全空格/非字符串 → null
  assert.strictEqual(validateDocName(''), null);
  assert.strictEqual(validateDocName('   '), null);
  assert.strictEqual(validateDocName(undefined), null);
  // 非法：超长、禁字符、控制字符 → undefined
  assert.strictEqual(validateDocName('a'.repeat(65)), undefined);
  assert.strictEqual(validateDocName('文'.repeat(65)), undefined);
  for (const ch of ['\\', '/', ':', '*', '?', '"', '<', '>', '|', '\t', '\n', '']) {
    assert.strictEqual(
      validateDocName(`a${ch}b`),
      undefined,
      `禁字符 ${JSON.stringify(ch)}`
    );
  }
});
