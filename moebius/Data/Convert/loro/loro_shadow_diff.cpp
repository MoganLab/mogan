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

  // 透明 CONCAT 包装：CONCAT(单原子) ≡ 原子。
  // insert_node/remove_node 多步序列的中间态会在 shadow 里产生 CONCAT(原子)
  // 包装，而 buffer 可能还是裸原子（或反过来）。把它们视为等价，避免 diff 走
  // assign 兜底（整块替换毁身份）。
  if (is_atomic (b) && is_concat (a) && N (a) == 1 && is_atomic (a[0])) {
    if (b->label != a[0]->label)
      emit_text_diff (base, b->label, a[0]->label, mods);
    return true;
  }
  if (is_concat (b) && N (b) == 1 && is_atomic (b[0]) && is_atomic (a)) {
    if (b[0]->label != a->label)
      emit_text_diff (base * path (0), b[0]->label, a->label, mods);
    return true;
  }

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

      // SPLIT 检测：1 个原子 → N 个原子，拼接文本一致。
      // 避免 diff 走 remove+insert（删旧重建毁字符身份），改吐 mod_split。
      if (rm_len >= 1 && ins_len >= 2) {
        bool   all_atomic= true;
        string rm_text;
        string ins_text;
        for (int i= 0; i < rm_len && all_atomic; i++) {
          if (!is_atomic (b[pre + i])) all_atomic= false;
          else rm_text= rm_text * b[pre + i]->label;
        }
        for (int i= 0; i < ins_len && all_atomic; i++) {
          if (!is_atomic (a[pre + i])) all_atomic= false;
          else ins_text= ins_text * a[pre + i]->label;
        }
        if (all_atomic && rm_text == ins_text) {
          // 按 shadow 各段的长度逐个 split（split 后下标递增）。
          for (int i= 0; i < ins_len - 1; i++) {
            mods= mods * mod_split (base, pre + i, N (a[pre + i]->label));
          }
          return true;
        }
      }

      // JOIN 检测：N 个原子 → 1 个原子，拼接文本一致。
      if (rm_len >= 2 && ins_len >= 1) {
        bool   all_atomic= true;
        string rm_text;
        string ins_text;
        for (int i= 0; i < rm_len && all_atomic; i++) {
          if (!is_atomic (b[pre + i])) all_atomic= false;
          else rm_text= rm_text * b[pre + i]->label;
        }
        for (int i= 0; i < ins_len && all_atomic; i++) {
          if (!is_atomic (a[pre + i])) all_atomic= false;
          else ins_text= ins_text * a[pre + i]->label;
        }
        if (all_atomic && rm_text == ins_text) {
          // 逐对 join：每次 join child pre 与 pre+1，join 后下一段落到 pre+1
          // → 再 join pre 与 pre+1（即原来的 pre+2）。故始终 join(base, pre)。
          for (int i= 0; i < rm_len - 1; i++) {
            mods= mods * mod_join (base, pre);
          }
          return true;
        }
      }

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
} // namespace

list<modification>
loro_shadow_rep::diff_from_current (tree buffer) {
  list<modification> mods;
  tree               after= to_tree ();
  if (!diff_walk (buffer, after, path (), mods)) {
    mods= list<modification> ();
    mods= mods * mod_assign (path (), after);
  }
  return mods;
}

list<modification>
loro_shadow_rep::remote_diff_mods (string bytes, tree buffer) {
  if (!import_data (bytes)) return list<modification> ();
  return diff_from_current (buffer);
}
