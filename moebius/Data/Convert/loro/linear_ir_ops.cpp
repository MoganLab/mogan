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
item_index_of_path (const array<linear_item>& items, path target) {
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
markup_offset_of_item (const array<linear_item>& items, int k) {
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

// items[from, to) 子序列的 markup 字节长度
static int
markup_len_range (array<linear_item>& items, int from, int to) {
  if (to <= from) return 0;
  array<linear_item> sub;
  for (int i= from; i < to; i++)
    sub << items[i];
  return N (linear_ir_to_markup (sub));
}

// items[from, to) 子序列的 markup 字节
static string
linear_ir_to_range (array<linear_item>& items, int from, int to) {
  array<linear_item> sub;
  for (int i= from; i < to; i++)
    sub << items[i];
  return linear_ir_to_markup (sub);
}

static inline void
add_op (markup_edit& ed, int offset, int delete_len, string insert_bytes) {
  markup_splice s;
  s.offset      = offset;
  s.delete_len  = delete_len;
  s.insert_bytes= insert_bytes;
  ed.ops << s;
}

markup_edit
compute_markup_edit (array<linear_item> items, modification mod) {
  markup_edit ed;
  ed.ok= false;
  switch (mod->k) {
  case MOD_INSERT: {
    int k  = item_index_of_path (items, root (mod));
    int pos= index (mod);
    if (k < 0) return ed;
    if (N (items[k].label) == 0) {
      // 原子文本插入
      string c       = mod->t->label;
      int    text_idx= k + 1;
      int    cstart  = markup_offset_of_item (items, text_idx);
      string text= (text_idx < N (items) && (items[text_idx].kind == LI_TEXT ||
                                             items[text_idx].kind == LI_BINARY))
                       ? items[text_idx].text
                       : string ("");
      ed.ok      = true;
      add_op (ed, cstart + escaped_byte_offset (text, pos), 0, escape_str (c));
      return ed;
    }
    // 复合子树插入：clean_insert 粘贴 u 的**子节点**（片段），故在 parent 的
    // child pos 处插入 u 的子节点 markup（存活内容不动）
    int kclose= matching_close (items, k);
    if (kclose < 0) return ed;
    array<int>         kids= direct_child_starts (items, k);
    int                ins = (pos >= 0 && pos < N (kids)) ? kids[pos] : kclose;
    array<linear_item> u_ir= tree_to_linear_ir (mod->t);
    int                u_close= matching_close (u_ir, 0);
    if (N (u_ir) < 2 || u_close < 0) return ed;
    ed.ok= true;
    add_op (ed, markup_offset_of_item (items, ins), 0,
            linear_ir_to_range (u_ir, 1, u_close));
    return ed;
  }
  case MOD_REMOVE: {
    int k  = item_index_of_path (items, root (mod));
    int pos= index (mod), nr= argument (mod);
    if (k < 0) return ed;
    if (N (items[k].label) == 0) {
      // 原子文本删除
      int text_idx= k + 1;
      if (text_idx >= N (items) || (items[text_idx].kind != LI_TEXT &&
                                    items[text_idx].kind != LI_BINARY))
        return ed;
      string text  = items[text_idx].text;
      int    cstart= markup_offset_of_item (items, text_idx);
      ed.ok        = true;
      add_op (ed, cstart + escaped_byte_offset (text, pos),
              escaped_byte_len (text, pos, nr), "");
      return ed;
    }
    // 复合子树删除：删 parent 的 child [pos, pos+nr)
    int kclose= matching_close (items, k);
    if (kclose < 0) return ed;
    array<int> kids= direct_child_starts (items, k);
    if (pos < 0 || pos + nr > N (kids)) return ed;
    int rstart= kids[pos];
    int rend  = (pos + nr < N (kids)) ? kids[pos + nr] : kclose;
    ed.ok     = true;
    add_op (ed, markup_offset_of_item (items, rstart),
            markup_len_range (items, rstart, rend), "");
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
    if (N (items[nopen].label) == 0) {
      // 原子：在字符边界插 CLOSE + OPEN("")
      int    text_idx= nopen + 1;
      string text= (text_idx < N (items) && (items[text_idx].kind == LI_TEXT ||
                                             items[text_idx].kind == LI_BINARY))
                       ? items[text_idx].text
                       : string ("");
      if (at < 0 || at > N (text)) return ed;
      ed.ok= true;
      add_op (ed,
              markup_offset_of_item (items, text_idx) +
                  escaped_byte_offset (text, at),
              0, close_open_markup (""));
      return ed;
    }
    // 复合：在子节点边界插 CLOSE + OPEN(label)
    array<int> gkids= direct_child_starts (items, nopen);
    int        bidx = (at >= 0 && at < N (gkids)) ? gkids[at] : cclose;
    ed.ok           = true;
    add_op (ed, markup_offset_of_item (items, bidx), 0,
            close_open_markup (items[nopen].label));
    return ed;
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
    ed.ok= true;
    add_op (ed, markup_offset_of_item (items, c1cl),
            N (close_open_markup (items[c2].label)), "");
    return ed;
  }
  case MOD_INSERT_NODE: {
    // 包裹：在节点前后插 wrapper 的 OPEN ... CLOSE（wrapper 既有子节点就位）
    path P    = root (mod);
    int  pos  = argument (mod);
    tree u    = mod->t;
    int  nopen= item_index_of_path (items, P);
    if (nopen < 0) return ed;
    int nclose= matching_close (items, nopen);
    if (nclose < 0) return ed;
    int                nend   = nclose + 1; // 被包裹节点的 item 跨度末尾
    array<linear_item> u_ir   = tree_to_linear_ir (u);
    int                u_close= matching_close (u_ir, 0);
    if (N (u_ir) < 2 || u_close < 0) return ed;
    array<int> u_kids= direct_child_starts (u_ir, 0);
    if (pos < 0 || pos > N (u_kids)) return ed;
    int    split_pt    = (pos < N (u_kids)) ? u_kids[pos] : u_close;
    string before_bytes= linear_ir_to_range (u_ir, 0, split_pt);
    string after_bytes = linear_ir_to_range (u_ir, split_pt, u_close + 1);
    ed.ok              = true;
    add_op (ed, markup_offset_of_item (items, nopen), 0, before_bytes);
    add_op (ed, markup_offset_of_item (items, nend), 0, after_bytes);
    return ed;
  }
  case MOD_REMOVE_NODE: {
    // 脱壳：删去 wrapper 的 OPEN(+前序子节点) 与
    // 后序子节点(+CLOSE)，保留提升子节点
    path W    = root (mod);
    int  k    = index (mod);
    int  wopen= item_index_of_path (items, W);
    if (wopen < 0) return ed;
    int wclose= matching_close (items, wopen);
    if (wclose < 0) return ed;
    array<int> kids= direct_child_starts (items, wopen);
    if (k < 0 || k >= N (kids)) return ed;
    int kstart= kids[k];
    int kend  = (k + 1 < N (kids)) ? kids[k + 1] : wclose; // 子节点 k 跨度末尾
    ed.ok     = true;
    add_op (ed, markup_offset_of_item (items, wopen),
            markup_len_range (items, wopen, kstart), "");
    add_op (ed, markup_offset_of_item (items, kend),
            markup_len_range (items, kend, wclose + 1), "");
    return ed;
  }
  default:
    return ed; // ASSIGN 等 → coarse（罕见；后续精确化）
  }
}

/******************************************************************************
 * body markup 字节偏移 ↔ 树位置（Phase 4 光标映射核心）
 *****************************************************************************/

// 单个 item 的 markup 字节长度（统一方案无 SEP，单 item 序列化与上下文无关）
static int
item_markup_len (const linear_item& it) {
  array<linear_item> one;
  one << it;
  return N (linear_ir_to_markup (one));
}

// text 经转义后的字节流中，byte_off 处属于原文第几个字符
static int
deescaped_char_offset (string text, int byte_off) {
  int off= 0, ch= 0;
  while (ch < N (text)) {
    int sz= escape_size (text[ch]);
    if (byte_off < off + sz) return ch; // 落在该字符的转义段内
    off+= sz;
    ch++;
  }
  return ch; // 末尾
}

// 由前缀 int 数组 + 末项构造 path（cons 自尾向首）
static path
build_path (array<int>& prefix, int last) {
  path p= path (last);
  for (int i= N (prefix) - 1; i >= 0; i--)
    p= path (prefix[i], p);
  return p;
}

int
linear_ir_offset_of_atomic (array<linear_item> items, path atomic_path,
                            int char_off) {
  char anchor= 'T';
  return linear_ir_offset_of_path (items, atomic_path * path (char_off),
                                   anchor);
}

int
linear_ir_offset_of_path (const array<linear_item>& items, path p,
                          char& out_anchor) {
  out_anchor= 'T';
  if (is_nil (p) || is_nil (path_up (p))) return -1;
  path node_path= path_up (p);
  int  char_off = last_item (p);

  int open_idx= item_index_of_path (items, node_path);
  if (open_idx < 0) return -1;
  if (N (items[open_idx].label) == 0) {
    // 原子节点
    int text_idx= open_idx + 1;
    // 若无 LI_TEXT 项或 LI_TEXT 内容为空串 ("")：均视为空原子
    if (text_idx >= N (items) || items[text_idx].kind != LI_TEXT ||
        N (items[text_idx].text) == 0) {
      out_anchor= 'O';
      return markup_offset_of_item (items, open_idx) +
             item_markup_len (items[open_idx]);
    }
    int content_start= markup_offset_of_item (items, text_idx);
    out_anchor       = 'T';
    return content_start + escaped_byte_offset (items[text_idx].text, char_off);
  }

  // 复合节点
  if (char_off == 0) {
    out_anchor= 'O';
    return markup_offset_of_item (items, open_idx) +
           item_markup_len (items[open_idx]);
  }
  else {
    // char_off >= 1: 复合节点的结构 CLOSE (node_path * 1)
    int depth    = 0;
    int close_idx= -1;
    for (int i= open_idx; i < N (items); i++) {
      if (items[i].kind == LI_OPEN) depth++;
      else if (items[i].kind == LI_CLOSE) {
        depth--;
        if (depth == 0) {
          close_idx= i;
          break;
        }
      }
    }
    if (close_idx < 0) return -1;
    out_anchor= 'C';
    return markup_offset_of_item (items, close_idx);
  }
}

path
linear_ir_path_at_offset_with_anchor (const array<linear_item>& items,
                                      int byte_off, char anchor) {
  if (anchor != 'O' && anchor != 'C') {
    bool prefer_start= false;
    int  text_char_idx=
        linear_ir_text_index_of_offset (items, byte_off, prefer_start);
    return linear_ir_path_at_text_index (items, text_char_idx, prefer_start);
  }

  int        n  = N (items);
  int        cur= 0;
  array<int> prefix, saved;
  int        next_child = 0;
  bool       inside_root= false;

  for (int i= 0; i < n; i++) {
    const linear_item& it   = items[i];
    int                len  = item_markup_len (it);
    int                start= cur, end= cur + len;

    if (it.kind == LI_OPEN) {
      if (!inside_root) {
        inside_root= true;
      }
      else {
        saved << next_child;
        prefix << next_child;
        next_child= 0;
      }
      if (anchor == 'O' && byte_off > start && byte_off <= end) {
        return build_path (prefix, 0);
      }
    }
    else if (it.kind == LI_CLOSE) {
      if (anchor == 'C' && byte_off >= start &&
          (byte_off < end || i == n - 1)) {
        return build_path (prefix, 1);
      }
      if (N (saved) > 0) {
        next_child= saved[N (saved) - 1] + 1;
        saved->resize (N (saved) - 1);
        prefix->resize (N (prefix) - 1);
      }
    }

    cur+= len;
  }

  bool prefer_start= false;
  int  text_char_idx=
      linear_ir_text_index_of_offset (items, byte_off, prefer_start);
  return linear_ir_path_at_text_index (items, text_char_idx, prefer_start);
}

path
linear_ir_path_at_offset (array<linear_item> items, int byte_off) {
  int        n  = N (items);
  int        cur= 0;
  array<int> prefix, saved;
  int        next_child = 0;
  bool       inside_root= false;
  path       last_atomic= path (); // 兜底：吸附到最近原子
  for (int i= 0; i < n; i++) {
    linear_item& it = items[i];
    int          len= item_markup_len (it);
    if (it.kind == LI_OPEN) {
      if (!inside_root) inside_root= true;
      else {
        saved << next_child;
        prefix << next_child;
        next_child= 0;
      }
    }
    else if (it.kind == LI_CLOSE) {
      if (N (saved) > 0) {
        next_child= saved[N (saved) - 1] + 1;
        saved->resize (N (saved) - 1);
        prefix->resize (N (prefix) - 1);
      }
    }
    else if (it.kind == LI_TEXT) {
      int start= cur, end= cur + len;
      if (byte_off >= start && byte_off < end) {
        // 严格落在原子内容内 → 精确
        return build_path (prefix,
                           deescaped_char_offset (it.text, byte_off - start));
      }
      if (byte_off >= end)
        last_atomic=
            build_path (prefix, N (it.text)); // 已越过：吸附到该原子末尾
      // byte_off < start：本原子在之后，保留前一个 last_atomic
    }
    cur+= len;
  }
  return last_atomic;
}

int
linear_ir_text_index_of_offset (const array<linear_item>& items, int byte_off,
                                bool& prefer_start) {
  int text_idx= 0;
  int cur     = 0;
  prefer_start= false;
  for (int i= 0; i < N (items); i++) {
    linear_item it = items[i];
    int         len= item_markup_len (it);
    if (it.kind == LI_TEXT || it.kind == LI_BINARY) {
      if (byte_off >= cur && byte_off < cur + len) {
        if (byte_off == cur) prefer_start= true;
        return text_idx + deescaped_char_offset (it.text, byte_off - cur);
      }
      if (byte_off == cur + len) {
        if (len == 0) prefer_start= true; // 空节点视为停在 start
        return text_idx + N (it.text);
      }
      text_idx+= N (it.text);
    }
    cur+= len;
    if (cur > byte_off) {
      prefer_start= true;
      return text_idx;
    }
  }
  return text_idx;
}

path
linear_ir_path_at_text_index (const array<linear_item>& items,
                              int target_text_idx, bool prefer_start) {
  int        text_idx= 0;
  array<int> prefix, saved;
  int        next_child = 0;
  bool       inside_root= false;
  path       last_atomic= path ();
  for (int i= 0; i < N (items); i++) {
    linear_item it= items[i];
    if (it.kind == LI_OPEN) {
      if (!inside_root) inside_root= true;
      else {
        saved << next_child;
        prefix << next_child;
        next_child= 0;
      }
    }
    else if (it.kind == LI_CLOSE) {
      if (N (saved) > 0) {
        next_child= saved[N (saved) - 1] + 1;
        saved->resize (N (saved) - 1);
        prefix->resize (N (prefix) - 1);
      }
    }
    else if (it.kind == LI_TEXT || it.kind == LI_BINARY) {
      int len= N (it.text);
      if (target_text_idx >= text_idx && target_text_idx <= text_idx + len) {
        if (prefer_start && target_text_idx == text_idx + len && len > 0) {
          // 当前节点匹配到了它的末尾（或下个节点开头）。既然意图在段首，且本节点长度>0，跳过本节点，由下个节点兜住
          // 0 偏移
          text_idx+= len;
          last_atomic= build_path (prefix, len);
          continue;
        }
        return build_path (prefix, target_text_idx - text_idx);
      }
      text_idx+= len;
      last_atomic= build_path (prefix, len);
    }
  }
  return last_atomic;
}
