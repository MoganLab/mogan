/** \file loro_shadow_diff.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_shadow.hpp"
#include "tree_helper.hpp"

namespace {
void
emit_text_diff (path p, string b, string a, list<modification>& mods) {
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
  if (ins_len > 0) {
    string ins= a (pre, pre + ins_len);
    mods      = mods * mod_insert (p, pre, tree (ins));
  }
}

bool
diff_walk (tree b, tree a, path base, list<modification>& mods) {
  if (b == a) return true;

  if (is_atomic (b) && is_atomic (a)) {
    if (b->label != a->label) emit_text_diff (base, b->label, a->label, mods);
    return true;
  }

  if (is_compound (b) && is_compound (a) && L (b) == L (a)) {
    int bn= N (b), an= N (a);
    if (bn == an) {
      for (int i= 0; i < bn; i++) {
        if (!diff_walk (b[i], a[i], base * path (i), mods)) {
          mods= mods * mod_assign (base * path (i), a[i]);
        }
      }
      return true;
    }
    else {
      int pre= 0;
      while (pre < bn && pre < an && b[pre] == a[pre])
        pre++;
      int suf= 0;
      while (suf < bn - pre && suf < an - pre &&
             b[bn - 1 - suf] == a[an - 1 - suf])
        suf++;

      int rm_len = bn - pre - suf;
      int ins_len= an - pre - suf;

      if (rm_len > 0) {
        mods= mods * mod_remove (base, pre, rm_len);
      }
      if (ins_len > 0) {
        tree ins (L (a), ins_len);
        for (int i= 0; i < ins_len; i++)
          ins[i]= a[pre + i];
        mods= mods * mod_insert (base, pre, ins);
      }
      return true;
    }
  }
  return false;
}

/******************************************************************************
 * 身份对账（identity reconciliation）
 *
 * 取代上面位置型的 diff_walk：跨 merge 之后 buffer 与 shadow 的子节点顺序可能
 * 不同，位置下标不可信。这里按 TreeID（而非位置）匹配节点，emit 的 mod 下标
 * 始终相对「实时工作副本」（边算边更新的 buffer 状态），逐条 apply 到活跃
 * buffer 合法。存活节点的 tree_rep* 被复用（保住光标/IP 稳定）。
 *
 * 依赖成员：
 *   id_map        —— buffer 当前 tree_rep* -> TreeID（对账前已同步）
 *   after_id_map  —— after 树 tree_rep* -> TreeID（to_tree_with_ids 刚填好）
 ******************************************************************************/

static void reconcile_node (loro_shadow_rep* self, tree b, tree a, path base,
                            list<modification>& mods);

// lolly 的 array 无元素级 remove/insert（只有 resize/<<），这里重建式实现。
template <class T>
static void
arr_remove (array<T>& a, int i) {
  array<T> r (N (a) - 1);
  for (int j= 0; j < i; j++)
    r[j]= a[j];
  for (int j= i + 1; j < N (a); j++)
    r[j - 1]= a[j];
  a= r;
}
template <class T>
static void
arr_insert (array<T>& a, int i, T x) {
  array<T> r (N (a) + 1);
  for (int j= 0; j < i; j++)
    r[j]= a[j];
  r[i]= x;
  for (int j= i; j < N (a); j++)
    r[j + 1]= a[j];
  a= r;
}

// 对账复合节点 b 与 a（同 label）的子节点：删除远端已删的、插入新增的、把
// 共有的复用到 after 的权威顺序、对共有的递归。cur 从 b 的当前子集出发、边
// 算边更新成实时工作副本；emit 的 mod 下标都相对 cur。
static void
reconcile_children (loro_shadow_rep* self, tree b, tree a, path base,
                    list<modification>& mods) {
  int         bn= N (b), an= N (a);
  array<tree> cur;  // 实时工作副本：cur[i] 为当前 buffer 子 i
  array<int>  idxs; // idxs[i] = cur[i] 对应的 after 子下标（-1 = after 已无）
  array<tree> kept (an);
  array<int>  keptr (an);
  array<char> aseen (an);
  for (int i= 0; i < an; i++) {
    kept[i] = tree ();
    keptr[i]= -1;
    aseen[i]= 0;
  }
  for (int i= 0; i < bn; i++) {
    mogan_tree_id bid= self->get_id (b[i]);
    int           ai = -1;
    if (bid.peer != 0)
      for (int j= 0; j < an; j++)
        if (self->after_id_map->contains (inside (a[j])) &&
            self->after_id_map (inside (a[j])) == bid) {
          ai= j;
          break;
        }
    cur << b[i];
    idxs << ai;
    if (ai >= 0) {
      kept[ai] = b[i];
      keptr[ai]= i;
      aseen[ai]= 1;
    }
  }
  // 1) 删除远端已删的（倒序，下标稳定）；摘除其 id_map 登记
  for (int i= N (cur) - 1; i >= 0; i--)
    if (idxs[i] < 0) {
      mogan_tree_id tid= self->get_id (cur[i]);
      if (tid.peer != 0) self->id_map->reset (inside (cur[i]));
      mods= mods * mod_remove (base, i, 1);
      arr_remove (cur, i);
      arr_remove (idxs, i);
    }
  // 2) 插入 after 新增的（在其权威 after 下标处），登记新 rep 的身份
  for (int j= 0; j < an; j++)
    if (!aseen[j]) {
      tree ins (L (a[j]), 1);
      ins[0]= a[j];
      mods  = mods * mod_insert (base, j, ins);
      arr_insert (cur, j, a[j]);
      arr_insert (idxs, j, j);
      mogan_tree_id nid           = self->after_id_map (inside (a[j]));
      self->id_map (inside (a[j]))= nid;
    }
  // 3) 排序：把共有/新增子按 after 权威顺序移动（复用 rep）。此时 cur 的
  // 元素集合 == after 的元素集合（删除/新增后），只是顺序可能不同。逐个 after
  // 位置 ai，从 cur 里把对应元素（idxs==ai）移到下标 ai（倒序删、顺序插）。
  for (int ai= 0; ai < an; ai++) {
    int cur_i= ai;
    while (cur_i < N (cur) && idxs[cur_i] != ai)
      cur_i++;
    if (cur_i >= N (cur)) continue; // 理论不可达
    tree rep= cur[cur_i];
    for (int j= cur_i; j > ai; j--) {
      mods= mods * mod_remove (base, j, 1);
      arr_remove (cur, j);
      arr_remove (idxs, j);
    }
    for (int j= cur_i; j > ai; j--) {
      tree ins (L (rep), 1);
      ins[0]= rep;
      mods  = mods * mod_insert (base, ai, ins);
      arr_insert (cur, ai, rep);
      arr_insert (idxs, ai, ai);
    }
  }
  // 4) 对共有的递归（此刻 after 下标 == 工作副本下标）
  for (int j= 0; j < an; j++)
    if (keptr[j] >= 0)
      reconcile_node (self, kept[j], a[j], base * path (j), mods);
}

// 对账单个节点：同身份（TreeID 相同）则按类型/内容细分，否则交给调用方
// （调用方在子节点对账里已按 TreeID 匹配，不会把不同身份的 b/a 传进来）。
static void
reconcile_node (loro_shadow_rep* self, tree b, tree a, path base,
                list<modification>& mods) {
  if (is_atomic (b) && is_atomic (a)) {
    if (b->label != a->label) emit_text_diff (base, b->label, a->label, mods);
  }
  else if (is_compound (b) && is_compound (a) && L (b) == L (a))
    reconcile_children (self, b, a, base, mods);
  else mods= mods * mod_assign (base, a); // 同身份但类型/label 变（罕见）：替换
}
} // namespace

list<modification>
loro_shadow_rep::reconcile_walk (tree buffer, tree after) {
  list<modification> mods;
  reconcile_node (this, buffer, after, path (), mods);
  return mods;
}

list<modification>
loro_shadow_rep::diff_from_current (tree buffer) {
  tree after= to_tree_with_ids ();
  // 仅当 buffer 是协作 body（DOCUMENT 根，如 the_buffer()）才走身份对账。
  // JOIN 时本端 buffer 可能是 TUPLE(空 document,...) 这类多文档容器，与 body
  // 结构异构，逐项对账会产生 remove+空 insert（can_insert 非法）；容器由
  // import_and_build 整体重建，这里回退整树 assign 兜底。
  if (!is_compound (buffer) || L (buffer) != moebius::DOCUMENT)
    return list<modification> (mod_assign (path (), after));
  return reconcile_walk (buffer, after);
}

list<modification>
loro_shadow_rep::remote_diff_mods (string bytes, tree buffer) {
  if (!import_data (bytes)) return list<modification> ();
  return diff_from_current (buffer);
}
