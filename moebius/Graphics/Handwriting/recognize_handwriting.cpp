
/******************************************************************************
 * MODULE     : recognize_handwriting.cpp
 * DESCRIPTION: Recognition of handwriting
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "handwriting.hpp"

/******************************************************************************
 * Recognize one glyph
 ******************************************************************************/

/**
 * @brief 在已学习字形中按指定级别搜索最佳匹配。
 * @param n_contours 输入轮廓的折线条数
 * @param level 不变量级别（1 或 2）
 * @param disc 输入轮廓的离散不变量
 * @param cont 输入轮廓的连续不变量
 * @param best [in,out] 当前最佳匹配名
 * @param best_rec [in,out] 当前最佳匹配得分
 */
static void
search_level (int n_contours, int level, const array<tree>& disc,
              const array<double>& cont, string& best, double& best_rec) {
  int h= hash (disc);
  for (int i= 0; i < N (learned_glyphs); i++) {
    const glyph_record& r= learned_glyphs[i];
    if (N (r.gl) != n_contours) continue;
    if (h != (level == 1 ? r.hash1 : r.hash2)) continue;
    if (disc != (level == 1 ? r.disc1 : r.disc2)) continue;
    // 内联距离计算，避免 l2_norm(cont - cont) 的临时数组分配
    const array<double>& ref= (level == 1 ? r.cont1 : r.cont2);
    double               s  = 0.0;
    for (int k= 0; k < N (cont); k++) {
      double d= ref[k] - cont[k];
      s+= d * d;
    }
    double rec= 1.0 - sqrt (s) / sqrt (N (cont));
    if (rec > best_rec) {
      best_rec= rec;
      best    = r.name;
    }
    // cout << r.name << ": " << 100.0 * rec << "%\n";
  }
}

void
recognize_glyph_one (contours gl, int& level, string& best, double& best_rec) {
  array<tree>   disc1;
  array<double> cont1;
  invariants (gl, 1, disc1, cont1);

  best    = "";
  best_rec= -100.0;
  search_level (N (gl), 1, disc1, cont1, best, best_rec);
  if (best != "") {
    level= 1;
    return;
  }

  // 二级不变量仅在一级无匹配时才计算（省一次采样 + 顶点检测开销）
  array<tree>   disc2;
  array<double> cont2;
  invariants (gl, 2, disc2, cont2);
  search_level (N (gl), 2, disc2, cont2, best, best_rec);
  level= 2;
}

/******************************************************************************
 * Recognize several glyphs
 ******************************************************************************/

bool
attached (poly_line pl1, poly_line pl2) {
  point p1= inf (pl1), q1= sup (pl1);
  point p2= inf (pl2), q2= sup (pl2);
  // cout << "<< " << p1 << ", " << q1 << "\n";
  // cout << ">> " << p2 << ", " << q2 << "\n";
  if (p2[1] > q1[1]) return true;
  if (p2[0] > q1[0]) return false;
  return true;
}

string
recognize_glyph (contours gl) {
  string r;
  for (int i= 0; i < N (gl);) {
    int eat= 1;
    while (i + eat < N (gl) && attached (gl[i + eat - 1], gl[i + eat]))
      eat++;
    int    lev = 3;
    string subr= "";
    double rec = -100.0;
    recognize_glyph_one (range (gl, i, i + eat), lev, subr, rec);
    r << subr;
    i+= eat;
    // cout << "Add " << eat << "\n";
  }
  return r;
}
