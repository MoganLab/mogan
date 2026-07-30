/** \file loro_shadow_reconcile.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 *
 *  \details 远端同步的身份对账（identity reconcile），替代 diff_walk 的位置型
 *           调和。shadow 的「带 TreeID 增强 IR」是 CRDT 真相源，buffer 按
 *           TreeID 与之对齐。删除只删「buffer 有 id 而 IR 已无该 id」的节点
 *           （= 真被远端删的）；本地新建节点（buffer 有 id 且 IR 也有 = 本地刚
 *           镜像上去，或 buffer 无 id = pre-edit 临时）一律保留。因此远端操作
 *           只会动它指向的 TreeID，绝不碰本端刚敲的字符。
 *
 *           对账方式：按 IR 期望顺序逐位置对齐。第 ai 个位置，比较 buffer 该
 *           位置孩子与 IR 该位置孩子的 id——
 *           同 id：递归对账子树；不同 id 且 buffer 项已被远端删：删除；其余：
 *           插入 IR 项（并把本地新建的孩子顺到后面保留）。
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"
#include <moebius/tree_label.hpp>

// 带 TreeID 的增强 IR 节点（对应 lib.rs 的 write_node_with_id 输出）。
// 在全局命名空间，匹配 loro_shadow.hpp 里 reconcile_walk 的前置声明。
struct IrNodeId {
  mogan_tree_id   id{mogan_tree_id{0, 0}};
  int             kind= 0; // LORO_ATOMIC / LORO_COMPOUND / LORO_GENERIC
  string          label;   // COMPOUND: 标签名；GENERIC: "generic:<op>"
  string          text;    // ATOMIC: 文本内容（含远端合并结果）
  array<IrNodeId> children;
};

namespace {

// lolly array 只有 resize/operator[]/operator<<，这里补单点插入/删除
template <class T>
void
arr_insert_at (array<T>& a, int pos, T x) {
  int n= N (a);
  a->resize (n + 1);
  for (int i= n; i > pos; i--)
    a[i]= a[i - 1];
  a[pos]= x;
}
template <class T>
void
arr_remove_at (array<T>& a, int pos) {
  int n= N (a);
  for (int i= pos; i < n - 1; i++)
    a[i]= a[i + 1];
  a->resize (n - 1);
}

struct IrReader {
  const string& b;
  int           pos= 0;
  explicit IrReader (const string& bytes) : b (bytes) {}
  uint32_t
  u32 () {
    uint32_t v= (uint32_t) (unsigned char) b[pos] |
                ((uint32_t) (unsigned char) b[pos + 1] << 8) |
                ((uint32_t) (unsigned char) b[pos + 2] << 16) |
                ((uint32_t) (unsigned char) b[pos + 3] << 24);
    pos+= 4;
    return v;
  }
  string
  str () {
    uint32_t n= u32 ();
    string   r;
    for (uint32_t i= 0; i < n; i++)
      r << b[pos + i];
    pos+= n;
    return r;
  }
};

bool
parse_id_node (IrReader& r, IrNodeId& out) {
  if (r.pos + 13 > N (r.b)) return false;
  uint64_t peer= 0;
  for (int i= 0; i < 8; i++)
    peer|= ((uint64_t) (unsigned char) r.b[r.pos + i]) << (8 * i);
  r.pos+= 8;
  out.id   = mogan_tree_id{peer, (int32_t) r.u32 ()};
  out.kind = (int) (unsigned char) r.b[r.pos++];
  out.label= r.str ();
  out.text = r.str ();
  uint32_t n= r.u32 ();
  for (uint32_t i= 0; i < n; i++) {
    IrNodeId c;
    if (!parse_id_node (r, c)) return false;
    out.children << c;
  }
  return true;
}

void
collect_ids (const IrNodeId& n, hashmap<mogan_tree_id, bool>& set) {
  set (n.id)= true;
  for (int i= 0; i < N (n.children); i++)
    collect_ids (n.children[i], set);
}

tree
ir_to_tree_plain (const IrNodeId& n) {
  if (n.kind == LORO_ATOMIC) return tree (n.text);
  int op= (n.kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (n.label)
                                    : as_int (n.label (8, N (n.label)));
  tree r (op, N (n.children));
  for (int i= 0; i < N (n.children); i++)
    r[i]= ir_to_tree_plain (n.children[i]);
  return r;
}

void
bind_ids_from_ir (tree node, const IrNodeId& irn, path acc,
                  hashmap<tree_rep*, mogan_tree_id>& id_map,
                  hashmap<mogan_tree_id, path>& rev_id_map) {
  id_map (inside (node))= irn.id;
  rev_id_map (irn.id)   = acc;
  if (is_compound (node)) {
    int m= N (node);
    for (int i= 0; i < m && i < N (irn.children); i++)
      bind_ids_from_ir (node[i], irn.children[i], acc * path (i), id_map,
                        rev_id_map);
  }
}

int
ir_op (const IrNodeId& irn) {
  return (irn.kind == LORO_COMPOUND)
             ? (int) moebius::make_tree_label (irn.label)
             : as_int (irn.label (8, N (irn.label)));
}

// buffer 子树与 IR 子树「结构与内容完全一致」（用于判断无 id 的本地项是否就是
// IR 期望的那一份，是则按位置绑定身份而非重复插入）
bool
structurally_eq (tree t, const IrNodeId& irn) {
  if (is_atomic (t)) return irn.kind == LORO_ATOMIC && t->label == irn.text;
  if (irn.kind == LORO_ATOMIC) return false;
  if (L (t) != ir_op (irn)) return false;
  if (N (t) != N (irn.children)) return false;
  for (int i= 0; i < N (t); i++)
    if (!structurally_eq (t[i], irn.children[i])) return false;
  return true;
}

void
emit_text_diff_id (path p, string b, string a, list<modification>& mods) {
  int bn= N (b), an= N (a);
  int pre= 0;
  while (pre < bn && pre < an && b[pre] == a[pre])
    pre++;
  int suf= 0;
  while (suf < bn - pre && suf < an - pre && b[bn - 1 - suf] == a[an - 1 - suf])
    suf++;
  int rm_len = bn - pre - suf;
  int ins_len= an - pre - suf;
  if (rm_len > 0) mods= mods * mod_remove (p, pre, rm_len);
  if (ins_len > 0) mods= mods * mod_insert (p, pre, tree (a (pre, pre + ins_len)));
}

} // namespace

/******************************************************************************
 * reconcile_ids：身份对账
 ******************************************************************************/

// 递归身份对账（成员函数）。mods 仅尾部追加；同一层内 insert/remove 改变运行
// buffer，故用一个「运行游标」追踪后续位置，保证 mod 下标在应用坐标系下正确。
void
loro_shadow_rep::reconcile_walk (tree t, const IrNodeId& irn, path base,
                                 hashmap<mogan_tree_id, bool>& ir_ids,
                                 list<modification>& mods) {
  if (is_atomic (t) || irn.kind == LORO_ATOMIC) {
    if (is_atomic (t) && irn.kind == LORO_ATOMIC) {
      id_map (inside (t))= irn.id;
      rev_id_map (irn.id)= base;
      if (t->label != irn.text) emit_text_diff_id (base, t->label, irn.text, mods);
    }
    return; // 原子↔复合不一致由父层 assign 处理
  }
  if (L (t) != ir_op (irn)) { // label 不一致 → 整棵替换为 IR
    tree nt= ir_to_tree_plain (irn);
    mods   = mods * mod_assign (base, nt);
    bind_ids_from_ir (nt, irn, base, id_map, rev_id_map);
    return;
  }
  // 同 label 复合：运行坐标系对齐
  id_map (inside (t))= irn.id;
  rev_id_map (irn.id)= base;
  int an= N (irn.children);
  int bn= N (t);
  array<mogan_tree_id> run_ids; // 运行 buffer 每个位置的 id（{0,0}=本地新建）
  array<tree>          run_tree;
  for (int i= 0; i < bn; i++) {
    run_ids << get_id (t[i]);
    run_tree << t[i];
  }
  int ai= 0, pos= 0;
  while (ai < an) {
    mogan_tree_id a_id= irn.children[ai].id;
    if (pos < N (run_ids)) {
      mogan_tree_id b_id= run_ids[pos];
      if (b_id == a_id) { // 同 id 同位置：递归对账子树
        reconcile_walk (run_tree[pos], irn.children[ai], base * path (pos),
                        ir_ids, mods);
        ai++;
        pos++;
        continue;
      }
      if (b_id.peer != 0 && !ir_ids->contains (b_id)) { // 被远端删：删除
        mods= mods * mod_remove (base, pos, 1);
        arr_remove_at (run_ids, pos);
        arr_remove_at (run_tree, pos);
        continue; // 不推进，检查顶替上来的下一个
      }
      if (b_id.peer == 0 && structurally_eq (run_tree[pos], irn.children[ai])) {
        // 无 id 的本地项与 IR 期望项结构一致：就是同一份，按位置绑定身份（不重复
        // 插入，保留 buffer 的 rep），递归对账子树
        id_map (inside (run_tree[pos]))= a_id;
        rev_id_map (a_id)              = base * path (pos);
        reconcile_walk (run_tree[pos], irn.children[ai], base * path (pos),
                        ir_ids, mods);
        ai++;
        pos++;
        continue;
      }
      // 本地新建（结构不同）/ 位置不符：在其前插入 IR 期望项
      tree ins= ir_to_tree_plain (irn.children[ai]);
      mods      = mods * mod_insert (base, pos, ins);
      bind_ids_from_ir (ins, irn.children[ai], base * path (pos), id_map,
                        rev_id_map);
      arr_insert_at (run_ids, pos, a_id);
      arr_insert_at (run_tree, pos, ins);
      ai++;
      pos++;
      continue;
    }
    // 运行 buffer 用尽：IR 期望项插入末尾
    tree ins= ir_to_tree_plain (irn.children[ai]);
    mods      = mods * mod_insert (base, pos, ins);
    bind_ids_from_ir (ins, irn.children[ai], base * path (pos), id_map,
                      rev_id_map);
    arr_insert_at (run_ids, pos, a_id);
    arr_insert_at (run_tree, pos, ins);
    ai++;
    pos++;
  }
  // IR 用尽后，运行 buffer 余下位置若是「被远端删」的也清掉；本地新建保留
  while (pos < N (run_ids)) {
    mogan_tree_id b_id= run_ids[pos];
    if (b_id.peer != 0 && !ir_ids->contains (b_id)) {
      mods= mods * mod_remove (base, pos, 1);
      arr_remove_at (run_ids, pos);
      arr_remove_at (run_tree, pos);
    }
    else
      pos++;
  }
}

list<modification>
loro_shadow_rep::reconcile_ids (tree buffer) {
  list<modification> mods;
  uint8_t*           out    = nullptr;
  size_t             out_len= 0;
  if (mogan_loro_doc_to_ir_with_ids (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return mods; // shadow 空：无远端内容可对账
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  IrReader r (ir_bytes);
  IrNodeId ir_root;
  if (!parse_id_node (r, ir_root)) return mods;
  if (root_id.peer == 0) root_id= ir_root.id;

  // buffer 有 id 但整棵 IR 都没有的节点 = 被远端删除
  hashmap<mogan_tree_id, bool> ir_ids (false);
  collect_ids (ir_root, ir_ids);

  reconcile_walk (buffer, ir_root, path (), ir_ids, mods);
  return mods;
}

list<modification>
loro_shadow_rep::remote_diff_mods (string bytes, tree buffer) {
  if (!import_data (bytes)) return list<modification> ();
  return reconcile_ids (buffer);
}
