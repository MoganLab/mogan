/** \file linear_ir_ops.cpp
 *  \copyright GPLv3
 *
 * 实现 linear_ir_apply_mod：在 item 序列上以最小编辑完成结构 modification，
 * 语义对齐 clean_apply（modification.cpp）。路径解析、括号匹配均在规范的
 * OPEN/CLOSE 序列上进行（Phase 2 不产出 MARKER，故无需 marker 感知）。
 *
 *  \author Jim Zhou
 *  \date   2026
 */

#include "linear_ir_ops.hpp"

#include "tree_helper.hpp"

/******************************************************************************
 * 路径 -> item 索引解析（相对 buffer 根；根的子节点为 [0],[1],...）
 *****************************************************************************/

// 候选路径 prefix+[last] 是否等于 target（moebius path 链表）
static bool
path_eq_prefix (array<int>& prefix, int last, path target) {
  path p= target;
  for (int i= 0; i < N (prefix); i++) {
    if (is_nil (p) || p->item != prefix[i]) return false;
    p= p->next;
  }
  if (is_nil (p) || p->item != last) return false;
  p= p->next;
  return is_nil (p);
}

// 节点（相对根的路径 target）在 item 序列中的首项索引：
//   复合 -> 其 OPEN；原子 -> 其 TEXT/BINARY；target 为 nil -> 根。未找到返回
//   -1。
static int
item_index_of_path (array<linear_item>& items, path target) {
  if (N (items) == 0) return -1;
  array<int> prefix; // 当前最内层复合内部的路径前缀
  array<int> saved;  // 各复合层（除根）恢复用的 child 索引
  int        next_child = 0;
  bool       inside_root= false;
  int        n          = N (items);
  for (int i= 0; i < n; i++) {
    linear_item_kind k= items[i].kind;
    if (k == LI_OPEN) {
      if (!inside_root) { // 根 OPEN
        inside_root= true;
        if (is_nil (target)) return i;
        continue;
      }
      if (path_eq_prefix (prefix, next_child, target)) return i;
      saved << next_child;
      prefix << next_child;
      next_child= 0;
    }
    else if (k == LI_TEXT || k == LI_BINARY) {
      if (!inside_root) { // 根为原子
        if (is_nil (target)) return i;
        return -1;
      }
      if (path_eq_prefix (prefix, next_child, target)) return i;
      next_child++;
    }
    else if (k == LI_CLOSE) {
      if (N (saved) > 0) {
        next_child= saved[N (saved) - 1] + 1;
        saved->resize (N (saved) - 1);
        prefix->resize (N (prefix) - 1);
      }
    }
  }
  return -1;
}

// open_idx(OPEN) 的配对 CLOSE 索引（括号匹配；序列规范时不遇 MARKER）
static int
matching_close (array<linear_item>& items, int open_idx) {
  int depth= 0;
  int n    = N (items);
  for (int i= open_idx; i < n; i++) {
    if (items[i].kind == LI_OPEN) depth++;
    else if (items[i].kind == LI_CLOSE) {
      depth--;
      if (depth == 0) return i;
    }
  }
  return -1;
}

// 复合（OPEN 在 open_idx）的各直接子节点的首项索引
static array<int>
direct_child_starts (array<linear_item>& items, int open_idx) {
  array<int> starts;
  int        n= N (items);
  int        i= open_idx + 1;
  while (i < n) {
    linear_item_kind k= items[i].kind;
    if (k == LI_CLOSE) break; // 本复合闭合
    starts << i;
    if (k == LI_OPEN) { // 跳到其配对 CLOSE 之后
      int depth= 1;
      i++;
      while (i < n && depth > 0) {
        if (items[i].kind == LI_OPEN) depth++;
        else if (items[i].kind == LI_CLOSE) depth--;
        i++;
      }
    }
    else i++; // TEXT/BINARY
  }
  return starts;
}

static void
push_range (array<linear_item>& dst, array<linear_item>& src, int from,
            int len) {
  for (int i= 0; i < len; i++)
    dst << src[from + i];
}

static bool
is_atomic_item (linear_item& it) {
  return it.kind == LI_TEXT || it.kind == LI_BINARY;
}

/******************************************************************************
 * 各结构操作
 *****************************************************************************/

static array<linear_item>
apply_split (array<linear_item>& items, modification mod) {
  path parent= root (mod);
  int  pos   = index (mod);
  int  at    = argument (mod);
  int  popen = item_index_of_path (items, parent);
  if (popen < 0) return items;
  array<int> kids= direct_child_starts (items, popen);
  if (pos < 0 || pos >= N (kids)) return items;
  int cstart= kids[pos];
  if (is_atomic_item (items[cstart])) {
    // 原子切分：TEXT/BINARY 一项拆成 head + tail
    string s= items[cstart].text;
    if (at < 0 || at > N (s)) return items;
    linear_item_kind   hk= items[cstart].kind;
    array<linear_item> r;
    push_range (r, items, 0, cstart);
    linear_item h;
    h.kind= hk;
    h.text= s (0, at);
    r << h;
    linear_item t;
    t.kind= hk;
    t.text= s (at, N (s));
    r << t;
    push_range (r, items, cstart + 1, N (items) - cstart - 1);
    return r;
  }
  if (items[cstart].kind == LI_OPEN) {
    // 复合切分：在切分点插入 CLOSE + OPEN(同 label)
    int cclose= matching_close (items, cstart);
    if (cclose < 0) return items;
    array<int>         gkids= direct_child_starts (items, cstart);
    int                ins  = (at >= 0 && at < N (gkids)) ? gkids[at] : cclose;
    string             lbl  = items[cstart].label;
    array<linear_item> r;
    push_range (r, items, 0, ins);
    linear_item c;
    c.kind= LI_CLOSE;
    r << c;
    linear_item o;
    o.kind = LI_OPEN;
    o.label= lbl;
    r << o;
    push_range (r, items, ins, N (items) - ins);
    return r;
  }
  return items;
}

static array<linear_item>
apply_join (array<linear_item>& items, modification mod) {
  path parent= root (mod);
  int  pos   = index (mod);
  int  popen = item_index_of_path (items, parent);
  if (popen < 0) return items;
  array<int> kids= direct_child_starts (items, popen);
  if (pos < 0 || pos + 1 >= N (kids)) return items;
  int  c1= kids[pos], c2= kids[pos + 1];
  bool a1= is_atomic_item (items[c1]);
  bool a2= is_atomic_item (items[c2]);
  if (a1 && a2) {
    // 原子合并：两个 TEXT 合一
    linear_item_kind   hk    = items[c1].kind;
    string             merged= items[c1].text * items[c2].text;
    array<linear_item> r;
    push_range (r, items, 0, c1);
    linear_item m;
    m.kind= hk;
    m.text= merged;
    r << m;
    push_range (r, items, c2 + 1, N (items) - c2 - 1);
    return r;
  }
  if (!a1 && !a2) {
    // 复合合并：删去 c1 的 CLOSE 与 c2 的 OPEN（二者相邻）
    int c1close= matching_close (items, c1);
    if (c1close < 0 || c1close + 1 != c2) return items;
    array<linear_item> r;
    push_range (r, items, 0, c1close);
    push_range (r, items, c2 + 1, N (items) - c2 - 1);
    return r;
  }
  return items; // 混合原子/复合：罕见/非法，兜底不动
}

static array<linear_item>
apply_insert_node (array<linear_item>& items, modification mod) {
  path node_path= root (mod);     // 被包裹节点路径
  int  pos      = argument (mod); // wrapper u 中该节点所占子槽
  tree u        = mod->t;         // wrapper
  int  nstart   = item_index_of_path (items, node_path);
  if (nstart < 0) return items;
  int nend; // 被包裹节点的 item 跨度末尾（exclusive）
  if (items[nstart].kind == LI_OPEN) {
    int cl= matching_close (items, nstart);
    if (cl < 0) return items;
    nend= cl + 1;
  }
  else nend= nstart + 1;

  // wrapper u 的 IR：[OPEN(ulabel), ...u 子节点..., CLOSE]
  array<linear_item> u_ir= tree_to_linear_ir (u);
  int                un  = N (u_ir);
  if (un < 2) return items;
  int u_close= matching_close (u_ir, 0);
  if (u_close < 0) return items;
  array<int> u_kids= direct_child_starts (u_ir, 0);
  int        lo    = (N (u_kids) > 0) ? u_kids[0] : u_close; // u 子节点起点
  if (pos < 0 || pos > N (u_kids)) return items;
  int split_pt= (pos < N (u_kids)) ? u_kids[pos] : u_close;

  array<linear_item> r;
  push_range (r, items, 0, nstart);                   // 节点之前
  r << u_ir[0];                                       // OPEN(ulabel)
  push_range (r, u_ir, lo, split_pt - lo);            // u 的 [0,pos) 子节点
  push_range (r, items, nstart, nend - nstart);       // 被包裹节点（原位保留）
  push_range (r, u_ir, split_pt, u_close - split_pt); // u 的 [pos,end) 子节点
  r << u_ir[u_close];                                 // CLOSE
  push_range (r, items, nend, N (items) - nend);      // 节点之后
  return r;
}

static array<linear_item>
apply_remove_node (array<linear_item>& items, modification mod) {
  path wrapper_path= root (mod);  // wrapper 路径
  int  k           = index (mod); // 被提升的子节点序号
  int  wopen       = item_index_of_path (items, wrapper_path);
  if (wopen < 0) return items;
  int wclose= matching_close (items, wopen);
  if (wclose < 0) return items;
  array<int> kids= direct_child_starts (items, wopen);
  if (k < 0 || k >= N (kids)) return items;
  int kstart= kids[k];
  int kend  = (k + 1 < N (kids)) ? kids[k + 1] : wclose; // exclusive
  // 仅保留 [kstart, kend)，删去 wrapper 的 OPEN/CLOSE 与其余子节点
  array<linear_item> r;
  push_range (r, items, 0, wopen);
  push_range (r, items, kstart, kend - kstart);
  push_range (r, items, wclose + 1, N (items) - wclose - 1);
  return r;
}

array<linear_item>
linear_ir_apply_mod (array<linear_item> items, modification mod) {
  switch (mod->k) {
  case MOD_SPLIT:
    return apply_split (items, mod);
  case MOD_JOIN:
    return apply_join (items, mod);
  case MOD_INSERT_NODE:
    return apply_insert_node (items, mod);
  case MOD_REMOVE_NODE:
    return apply_remove_node (items, mod);
  default:
    return items; // ASSIGN 等：本次不动
  }
}
