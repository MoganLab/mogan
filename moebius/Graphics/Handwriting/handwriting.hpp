
/******************************************************************************
 * MODULE     : handwriting.hpp
 * DESCRIPTION: Facilities for handwriting
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "poly_line.hpp"

/**
 * @brief 单个已学习字形的完整记录。
 * @note 一二级不变量及其哈希在学习时一次性算好，识别时只读；
 *       hash1/hash2 分别是 disc1/disc2 的缓存哈希，用于识别前的快速预过滤。
 */
struct glyph_record {
  string        name;  ///< 字形名
  contours      gl;    ///< 原始轮廓
  array<tree>   disc1; ///< 一级离散不变量
  array<double> cont1; ///< 一级连续不变量
  array<tree>   disc2; ///< 二级离散不变量
  array<double> cont2; ///< 二级连续不变量
  int           hash1; ///< disc1 的缓存哈希
  int           hash2; ///< disc2 的缓存哈希
};

extern array<glyph_record> learned_glyphs;

void   clear_learned_glyphs ();
void   register_glyph (string name, contours gl);
void   recognize_glyph_one (contours gl, int& level, string& best,
                            double& best_rec);
string recognize_glyph (contours gl);

array<point> simplify (array<point> a, double eps, double thr);
