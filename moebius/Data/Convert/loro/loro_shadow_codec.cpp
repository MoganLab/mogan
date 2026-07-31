/** \file loro_shadow_codec.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_ir_codec.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"
#include <moebius/tree_label.hpp>

namespace {
/// 读一个 IR-with-ids 子节点，返回 1 或 N+1 个 tree（split 原子展开）。
/// wrap_para=true 时，每段原子包进 PARA（DOCUMENT 下的段落文本块）。
/// 所有 split 段映射到同一 TreeID（共享 LoroText 容器）。
array<tree>
decode_id_expanded (string& b, int& pos,
                    hashmap<tree_rep*, mogan_tree_id>& id_map,
                    hashmap<mogan_tree_id, path>& rev_id_map, path acc,
                    bool wrap_para) {
  auto get_u32= [&] () -> uint32_t {
    uint32_t v= (uint32_t) (unsigned char) b[pos] |
                ((uint32_t) (unsigned char) b[pos + 1] << 8) |
                ((uint32_t) (unsigned char) b[pos + 2] << 16) |
                ((uint32_t) (unsigned char) b[pos + 3] << 24);
    pos+= 4;
    return v;
  };
  auto get_str= [&] () -> string {
    uint32_t n= get_u32 ();
    string   r;
    for (uint32_t i= 0; i < n; i++)
      r << b[pos + i];
    pos+= n;
    return r;
  };

  uint64_t peer= 0;
  for (int i= 0; i < 8; i++)
    peer|= ((uint64_t) (unsigned char) b[pos + i]) << (8 * i);
  pos+= 8;
  mogan_tree_id tid{peer, (int32_t) get_u32 ()};

  uint8_t  kind = (uint8_t) (unsigned char) b[pos++];
  string   label= get_str ();
  string   text = get_str ();
  uint32_t n    = get_u32 (); // n_children

  if (kind == LORO_ATOMIC) {
    for (uint32_t i= 0; i < n; i++)
      decode_id_expanded (b, pos, id_map, rev_id_map, acc * path (i), false);
    uint32_t   ns= get_u32 ();
    array<int> splits;
    for (uint32_t i= 0; i < ns; i++)
      splits << (int) get_u32 ();
    // 把段原子包进 PARA 或直接返回裸原子
    auto make_seg= [&] (string s) -> tree {
      id_map (inside (tree (s)))= tid; // 先注册再包装
      if (wrap_para) {
        tree para ((tree_label) moebius::PARA, 1);
        para[0]= tree (s);
        return para;
      }
      return tree (s);
    };
    array<tree> result;
    if (ns == 0) {
      tree r          = make_seg (text);
      rev_id_map (tid)= acc;
      result << r;
    }
    else {
      int start= 0;
      for (uint32_t i= 0; i < ns; i++) {
        result << make_seg (text (start, splits[i]));
        start= splits[i];
      }
      result << make_seg (text (start, N (text)));
      rev_id_map (tid)= acc;
    }
    return result;
  }

  // 复合节点
  int  op= (kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (label)
                                   : as_int (label (8, N (label)));
  bool child_wrap= (op == (int) moebius::DOCUMENT);
  array<tree> all_children;
  int         buf_idx= 0;
  for (uint32_t i= 0; i < n; i++) {
    array<tree> child_set= decode_id_expanded (
        b, pos, id_map, rev_id_map, acc * path (buf_idx), child_wrap);
    for (int j= 0; j < N (child_set); j++)
      all_children << child_set[j];
    buf_idx+= N (child_set);
  }
  uint32_t ns= get_u32 ();
  for (uint32_t i= 0; i < ns; i++)
    get_u32 ();

  tree r (op, N (all_children));
  for (int i= 0; i < N (all_children); i++)
    r[i]= all_children[i];
  id_map (inside (r))= tid;
  rev_id_map (tid)   = acc;
  array<tree> result;
  result << r;
  return result;
}
} // namespace

bool
loro_shadow_rep::import_and_build (string bytes, tree& out_buffer) {
  if (!import_data (bytes)) return false;
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir_with_ids (doc, &out, &out_len) != 0 ||
      out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return false;
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  id_map         = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map     = hashmap<mogan_tree_id, path> (path ());
  int         pos= 0;
  array<tree> roots=
      decode_id_expanded (ir_bytes, pos, id_map, rev_id_map, path (), false);
  out_buffer= N (roots) > 0 ? roots[0] : tree ("");
  return true;
}

tree
loro_shadow_rep::to_tree () {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return tree ("");
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return loro_ir_to_tree (loro_ir_decode (ir_bytes));
}
