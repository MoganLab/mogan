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
tree
decode_id_node (string& b, int& pos,
                hashmap<tree_rep*, mogan_tree_id>& id_map,
                hashmap<mogan_tree_id, path>& rev_id_map, path acc) {
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
  uint32_t n    = get_u32 ();

  tree r;
  if (kind == LORO_ATOMIC) r= tree (text);
  else {
    int op= (kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (label)
                                    : as_int (label (8, N (label)));
    r     = tree (op, (int) n);
    for (uint32_t i= 0; i < n; i++)
      r[i]= decode_id_node (b, pos, id_map, rev_id_map, acc * path (i));
  }
  id_map (inside (r))= tid;
  rev_id_map (tid)   = acc; // 与 id_map 同处维护：TreeID -> buffer-相对 path
  return r;
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
  id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map= hashmap<mogan_tree_id, path> (path ());
  int pos   = 0;
  out_buffer= decode_id_node (ir_bytes, pos, id_map, rev_id_map, path ());
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
