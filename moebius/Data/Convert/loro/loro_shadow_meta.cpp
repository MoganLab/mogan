/** \file loro_shadow_meta.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 * \details body 之外的文档部分（style/initial/final/project/attachments）的
 *          coarse CRDT 镜像。每个 section 作为 LoroTree 的一个带 __section__
 *          标签的独立 root，与 body（roots[0]）共享同一条 update 流（doc 级
 *          export/import/subscribe）。metadata 改动稀疏，采用「删旧 root +
 * 重建」 的 section 粒度 coarse 镜像，并发按 Loro LWW 合并。
 */

#include "loro_ir.hpp"
#include "loro_ir_codec.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

#include <cstdint>

namespace {
uint32_t
read_u32 (const string& b, int pos) {
  return (uint32_t) (unsigned char) b[pos] |
         ((uint32_t) (unsigned char) b[pos + 1] << 8) |
         ((uint32_t) (unsigned char) b[pos + 2] << 16) |
         ((uint32_t) (unsigned char) b[pos + 3] << 24);
}

// TreeID 字典序比较（peer 优先，再 counter）：用于同名多 root 的确定性 LWW
// 选主。 跨 peer 的 counter 不可比，故按 (peer, counter)
// 字典序——确定性，两端一致。
int
cmp_tid (mogan_tree_id a, mogan_tree_id b) {
  if (a.peer != b.peer) return a.peer < b.peer ? -1 : 1;
  if (a.counter != b.counter) return a.counter < b.counter ? -1 : 1;
  return 0;
}
} // namespace

// seed_meta 与 mirror_meta_replace 共用：删旧 root（若有）+ 重建。不 commit——
// 由调用方决定提交时机（seed 批量后统一 broadcast；mirror 单次编辑后立即
// commit）。
void
loro_shadow_rep::replace_meta (string name, tree section_tree) {
  if (meta_root_ids->contains (name)) {
    mogan_loro_node_delete (doc, meta_root_ids[name]);
    meta_root_ids->reset (name);
  }
  string         ir= loro_ir_encode (tree_to_loro_ir (section_tree));
  const uint8_t* ir_p=
      N (ir) > 0 ? reinterpret_cast<const uint8_t*> (ir.begin ()) : nullptr;
  const uint8_t* nm_p= reinterpret_cast<const uint8_t*> (name.begin ());
  mogan_tree_id  rid= mogan_loro_doc_seed_section (doc, nm_p, (size_t) N (name),
                                                   ir_p, (size_t) N (ir));
  if (rid.peer != 0) meta_root_ids (name)= rid;
}

void
loro_shadow_rep::seed_meta (string name, tree section_tree) {
  // 不 commit：ensure_loro_seeded 批量 seed 多个 section 后统一
  // broadcast_update。
  replace_meta (name, section_tree);
}

void
loro_shadow_rep::mirror_meta_replace (string name, tree section_tree) {
  replace_meta (name, section_tree);
  // 与 body 的 mirror_mod 一致：编辑后立即 commit，触发 local_update_cb
  // 上行广播。
  mogan_loro_doc_commit (doc);
}

tree
loro_shadow_rep::meta_to_tree (string name) {
  if (!meta_root_ids->contains (name)) return tree ("");
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_section_to_ir (doc, meta_root_ids[name], &out, &out_len) !=
          0 ||
      out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return tree ("");
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return loro_ir_to_tree (loro_ir_decode (ir_bytes));
}

bool
loro_shadow_rep::has_meta (string name) {
  return meta_root_ids->contains (name);
}

array<string>
loro_shadow_rep::list_meta_sections () {
  array<string>    r;
  iterator<string> it= iterate (meta_root_ids);
  while (it->busy ())
    r << it->next ();
  return r;
}

void
loro_shadow_rep::sync_meta_from_shadow () {
  meta_root_ids   = hashmap<string, mogan_tree_id> (mogan_tree_id{0, 0});
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_list_sections (doc, &out, &out_len) != 0 ||
      out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return;
  }
  string b ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  int pos= 0;
  int nb = N (b);
  // 每条：name_len:u32 name:bytes root_peer:u64 root_counter:i32（均小端）
  while (pos + 4 <= nb) {
    uint32_t nlen= read_u32 (b, pos);
    pos+= 4;
    if (pos + (int) nlen + 12 > nb) break; // 残缺条目，停止
    string name= b (pos, pos + (int) nlen);
    pos+= (int) nlen;
    uint64_t peer= 0;
    for (int k= 0; k < 8; k++)
      peer|= ((uint64_t) (unsigned char) b[pos + k]) << (8 * k);
    pos+= 8;
    int32_t counter= (int32_t) read_u32 (b, pos);
    pos+= 4;
    mogan_tree_id tid{peer, counter};
    // 同名多 root（并发 coarse 改动遗留）取字典序最大
    // TreeID，保证两端确定性一致。
    if (!meta_root_ids->contains (name) ||
        cmp_tid (tid, meta_root_ids[name]) > 0)
      meta_root_ids (name)= tid;
  }
}
