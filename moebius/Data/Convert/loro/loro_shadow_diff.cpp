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
