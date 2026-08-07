'use strict';
// 共享文档显示名校验：服务端权威校验（不信任客户端）。
// 规则：trim 后长度 1–64（按 Unicode code point 计，CJK 一字算一），
// 禁止 \ / : * ? " < > | 及所有控制字符（U+0000–001F、U+007F）。
// Scheme 侧 collab-valid-doc-name? 与本文件规则保持一致（预校验，仅即时反馈）。

const NAME_MAX = 64;
// eslint-disable-next-line no-control-regex
const NAME_FORBIDDEN = /[\\/:*?"<>|\u0000-\u001f\u007f]/;

/**
 * 校验并规范化文档显示名。
 * @param {string|null|undefined} raw 用户输入的原始名字
 * @returns {string|null|undefined} 合法时返回 trim 后的名字；
 *   未提供（空/全空格/非字符串）返回 null（按无名文档处理）；非法返回 undefined。
 */
function validateDocName (raw) {
  if (typeof raw !== 'string') return null;
  const name = raw.trim();
  if (name.length === 0) return null; // 未提供：无名文档
  if ([...name].length > NAME_MAX) return undefined;
  if (NAME_FORBIDDEN.test(name)) return undefined;
  return name;
}

module.exports = { validateDocName, NAME_MAX };
