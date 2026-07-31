/** \file linear_ir_ops.cpp
 *  \copyright GPLv3
 *
 * 实现 linear_ir_apply_mod（clean_apply 等价变换）与 compute_markup_edit（body
 * LoroText 最小字节 splice）。统一包裹方案下 SPLIT/JOIN 对原子与复合一致：
 * CLOSE+OPEN(label) 的插入/删除；存活字符 op-id 不变。
 *
 *  \author Jim Zhou
 *  \date   2026
 */

#include "linear_ir_ops.hpp"

#include "tree_helper.hpp"

/******************************************************************************
 * 路径 -> item 索引解析（相对 buffer 根；根的子节点为 [0],[1],...）
 *****************************************************************************/

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

// 节点（相对根路径 target）的 OPEN item 索引；target 为 nil -> 根 OPEN。未找到
// -1。
static int
item_index_of_path (array<linear_item>& items, path target) {
  if (N (items) == 0) return -1;
  array<int> prefix;
  array<int> saved;
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
      if (!inside_root) return -1; // 统一方案：根必为 OPEN
      next_child++; // 内容项不占子节点槽（属其 OPEN("") 原子框），这里不会命中
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

// 复合/原子（OPEN 在 open_idx）的各直接子节点的 OPEN item 索引
static array<int>
direct_child_starts (array<linear_item>& items, int open_idx) {
  array<int> starts;
  int        n= N (items);
  int        i= open_idx + 1;
  while (i < n) {
    linear_item_kind k= items[i].kind;
    if (k == LI_CLOSE) break;
    if (k == LI_OPEN) {
      starts << i;
      int depth= 1;
      i++;
      while (i < n && depth > 0) {
        if (items[i].kind == LI_OPEN) depth++;
        else if (items[i].kind == LI_CLOSE) depth--;
        i++;
      }
    }
    else i++; // TEXT/BINARY（原子框内的内容项），跳过
  }
  return starts;
}

/******************************************************************************
 * linear_ir_apply_mod：clean_apply 等价变换
 *****************************************************************************/
array<linear_item>
linear_ir_apply_mod (array<linear_item> items, modification mod) {
  switch (mod->k) {
  case MOD_SPLIT:
  case MOD_JOIN:
  case MOD_INSERT_NODE:
  case MOD_REMOVE_NODE:
    break;
  default:
    return items; // ASSIGN 等：本次不动
  }
  tree t = linear_ir_to_tree (items);
  tree t2= clean_apply (t, mod);
  return tree_to_linear_ir (t2);
}

/******************************************************************************
 * compute_markup_edit：body LoroText 最小字节 splice
 *****************************************************************************/

static const char MESC   = '\x01'; // 与 linear_ir.cpp 的 ESC 一致
static const char MESCAPE= '\x02'; // 与 linear_ir.cpp 的 ESCC 一致

static int
escape_size (char c) {
  return (c == MESC || c == MESCAPE) ? 2 : 1;
}

static string
escape_str (string s) {
  string r;
  for (int i= 0; i < N (s); i++) {
    char c= s[i];
    if (c == MESC) {
      r << MESCAPE;
      r << '1';
    }
    else if (c == MESCAPE) {
      r << MESCAPE;
      r << '2';
    }
    else r << c;
  }
  return r;
}

static int
escaped_byte_offset (string s, int char_pos) {
  int off= 0;
  for (int i= 0; i < char_pos && i < N (s); i++)
    off+= escape_size (s[i]);
  return off;
}

static int
escaped_byte_len (string s, int from, int nr) {
  int off= 0;
  for (int i= 0; i < nr && from + i < N (s); i++)
    off+= escape_size (s[from + i]);
  return off;
}

// item k 在 markup 中的起始 utf-8 字节偏移（序列化 [0,k) 前缀取长度）
static int
markup_offset_of_item (array<linear_item>& items, int k) {
  if (k <= 0) return 0;
  array<linear_item> prefix;
  for (int i= 0; i < k; i++)
    prefix << items[i];
  return N (linear_ir_to_markup (prefix));
}

// CLOSE + OPEN(label) 两 item 的 markup 字节（SPLIT 插入 / JOIN 删除的内容）
static string
close_open_markup (string label) {
  array<linear_item> toks;
  linear_item        c;
  c.kind= LI_CLOSE;
  toks << c;
  linear_item o;
  o.kind = LI_OPEN;
  o.label= label;
  toks << o;
  return linear_ir_to_markup (toks);
}

markup_edit
compute_markup_edit (array<linear_item> items, modification mod) {
  markup_edit ed;
  ed.ok        = false;
  ed.offset    = 0;
  ed.delete_len= 0;
  switch (mod->k) {
  case MOD_INSERT: {
    int k= item_index_of_path (items, root (mod));   // 原子 OPEN("")
    if (k < 0 || N (items[k].label) != 0) return ed; // 复合插入 → coarse
    int    pos     = index (mod);
    string c       = mod->t->label;
    int    text_idx= k + 1;
    int    cstart  = markup_offset_of_item (items, text_idx);
    string text= (text_idx < N (items) && (items[text_idx].kind == LI_TEXT ||
                                           items[text_idx].kind == LI_BINARY))
                     ? items[text_idx].text
                     : string ("");
    ed.ok      = true;
    ed.offset  = cstart + escaped_byte_offset (text, pos);
    ed.insert_bytes= escape_str (c);
    return ed;
  }
  case MOD_REMOVE: {
    int k= item_index_of_path (items, root (mod));
    if (k < 0 || N (items[k].label) != 0) return ed;
    int pos= index (mod), nr= argument (mod);
    int text_idx= k + 1;
    if (text_idx >= N (items) ||
        (items[text_idx].kind != LI_TEXT && items[text_idx].kind != LI_BINARY))
      return ed;
    string text  = items[text_idx].text;
    int    cstart= markup_offset_of_item (items, text_idx);
    ed.ok        = true;
    ed.offset    = cstart + escaped_byte_offset (text, pos);
    ed.delete_len= escaped_byte_len (text, pos, nr);
    return ed;
  }
  case MOD_SPLIT: {
    path parent= root (mod);
    int  pos   = index (mod);
    int  at    = argument (mod);
    int  popen = item_index_of_path (items, parent);
    if (popen < 0) return ed;
    array<int> kids= direct_child_starts (items, popen);
    if (pos < 0 || pos >= N (kids)) return ed;
    int nopen = kids[pos];
    int cclose= matching_close (items, nopen);
    if (cclose < 0) return ed;
    bool atomic= (N (items[nopen].label) == 0);
    if (atomic) {
      // 在 TEXT 内容的字符边界插 CLOSE + OPEN("")
      int    text_idx= nopen + 1;
      string text= (text_idx < N (items) && (items[text_idx].kind == LI_TEXT ||
                                             items[text_idx].kind == LI_BINARY))
                       ? items[text_idx].text
                       : string ("");
      if (at < 0 || at > N (text)) return ed;
      ed.ok    = true;
      ed.offset= markup_offset_of_item (items, text_idx) +
                 escaped_byte_offset (text, at);
      ed.insert_bytes= close_open_markup ("");
      return ed;
    }
    else {
      // 在子节点边界插 CLOSE + OPEN(label)
      array<int> gkids= direct_child_starts (items, nopen);
      int        bidx = (at >= 0 && at < N (gkids)) ? gkids[at] : cclose;
      ed.ok           = true;
      ed.offset       = markup_offset_of_item (items, bidx);
      ed.insert_bytes = close_open_markup (items[nopen].label);
      return ed;
    }
  }
  case MOD_JOIN: {
    path parent= root (mod);
    int  pos   = index (mod);
    int  popen = item_index_of_path (items, parent);
    if (popen < 0) return ed;
    array<int> kids= direct_child_starts (items, popen);
    if (pos < 0 || pos + 1 >= N (kids)) return ed;
    int c1  = kids[pos];
    int c1cl= matching_close (items, c1);
    int c2  = kids[pos + 1];
    if (c1cl < 0 || c1cl + 1 != c2) return ed; // 须相邻兄弟
    ed.ok        = true;
    ed.offset    = markup_offset_of_item (items, c1cl);
    ed.delete_len= N (close_open_markup (items[c2].label));
    return ed;
  }
  default:
    return ed; // INSERT_NODE/REMOVE_NODE/ASSIGN → coarse（v1）
  }
}
