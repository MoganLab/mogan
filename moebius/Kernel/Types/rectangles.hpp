
/******************************************************************************
 * MODULE     : rectangles.hpp
 * DESCRIPTION: Rectangles and lists of rectangles with reference counting.
 *              Used in graphical programs.
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef RECTANGLES_H
#define RECTANGLES_H

#include "list.hpp"
#include "tree.hpp"

class rectangle_rep : concrete_struct {
public:
  SI x1, y1;
  SI x2, y2;

  rectangle_rep (SI x1b, SI y1b, SI x2b, SI y2b);
  friend class rectangle;
};

class rectangle {
  CONCRETE (rectangle);
  rectangle (SI x1b= 0, SI y1b= 0, SI x2b= 0, SI y2b= 0);
  operator tree ();
};
CONCRETE_CODE (rectangle);

tm_ostream& operator<< (tm_ostream& out, rectangle r);
rectangle   copy (rectangle r);
bool        operator== (rectangle r1, rectangle r2);
bool        operator!= (rectangle r1, rectangle r2);
bool        intersect (rectangle r1, rectangle r2);
bool        operator<= (rectangle r1, rectangle r2);

/**
 * @brief 平移单个矩形
 * @param r 待平移的矩形
 * @param x 水平方向平移量（向右为正）
 * @param y 垂直方向平移量（向上为正）
 * @return 平移后的新矩形，原矩形不变
 */
rectangle translate (const rectangle& r, SI x, SI y);

rectangle operator* (rectangle r, int d);
rectangle operator/ (rectangle r, int d);
rectangle operator* (rectangle r, double x);
rectangle operator/ (rectangle r, double x);

/**
 * @brief 加厚单个矩形：四边向外扩
 * @param r 待加厚的矩形
 * @param width 水平方向的扩展量（左右各扩 width）
 * @param height 垂直方向的扩展量（上下各扩 height）
 * @return 加厚后的新矩形，原矩形不变
 */
rectangle thicken (rectangle r, SI width, SI height);

rectangle least_upper_bound (rectangle r1, rectangle r2);
double    area (rectangle r);
bool      is_zero (rectangle r);

typedef list<rectangle> rectangles;

/**
 * @brief 矩形列表差集：从 l1 中挖去所有与 l2 相交的部分
 * @param l1 被减列表
 * @param l2 减数列表
 * @return 差集结果的新列表，元素按 l1 顺序逐个产出，原列表不变
 * @note 与 l2 全不交的 l1 元素原样保留；完全被覆盖的元素被丢弃；
 *   部分相交的元素被切成至多数块碎片。结果可能含相邻可合并的碎片，
 *   需要时由调用方再用 simplify 收敛。
 */
rectangles operator- (rectangles l1, rectangles l2);
rectangles operator& (rectangles l1, rectangles l2);

/**
 * @brief 矩形列表并集：先求 l1 - l2 去掉重叠，再把 l2 各矩形逐个并入
 * @param l1 第一个列表
 * @param l2 第二个列表
 * @return 并集结果的新列表，原列表不变
 * @note 并入时与累积列表中相邻的矩形会被合并（disjoint union 语义），
 *   与全部元素都不相邻的矩形追加到尾部。典型用于失效/重绘区域的
 *   逐矩形累积合并。
 */
rectangles operator| (rectangles l1, rectangles l2);
rectangles disjoint_union (rectangles l, rectangle r);
rectangles operator* (rectangles l, int d);
rectangles operator/ (rectangles l, int d);

/**
 * @brief 平移矩形列表中的所有矩形
 * @param l 待平移的矩形列表
 * @param x 水平方向平移量（向右为正）
 * @param y 垂直方向平移量（向上为正）
 * @return 所有矩形均平移后的新列表，元素顺序保持不变，原列表不变
 */
rectangles translate (const rectangles& l, SI x, SI y);

/**
 * @brief 加厚矩形列表中的所有矩形：每个矩形四边向外扩
 * @param l 待加厚的矩形列表
 * @param width 水平方向的扩展量（左右各扩 width）
 * @param height 垂直方向的扩展量（上下各扩 height）
 * @return 所有矩形均加厚后的新列表，元素顺序保持不变，原列表不变
 */
rectangles thicken (const rectangles& l, SI width, SI height);

rectangles outlines (rectangles l, SI pixel);

/**
 * @brief 剔除列表中的退化矩形
 *
 * 宽或高非正（x1 >= x2 或 y1 >= y2）的矩形视为退化，被丢弃；其余矩形
 * 原样保留，顺序不变。
 *
 * @param l 待修正的矩形列表
 * @return 仅含非退化矩形的新列表；空列表原样返回
 */
rectangles correct (const rectangles& l);

/**
 * @brief 化简矩形列表：合并可合并的相邻矩形
 *
 * 列表较短（不超过 25 个元素）时，反复做不相交并（operator|），把共享
 * 边界且可拼成矩形的相邻元素合并；过长时直接返回副本，避免 O(n^2) 合并
 * 开销。
 *
 * @param l 待化简的矩形列表
 * @return 化简后的新列表，原列表不变
 */
rectangles simplify (rectangles l);

rectangle least_upper_bound (rectangles l);
rectangle least_upper_bound (array<rectangle> l);
double    area (rectangles r);

#endif // defined RECTANGLES_H
