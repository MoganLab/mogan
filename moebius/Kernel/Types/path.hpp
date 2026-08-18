
/******************************************************************************
 * MODULE     : path.hpp
 * DESCRIPTION: paths are integer lists,
 *              which are for instance useful to select subtrees in trees
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef PATH_H
#define PATH_H

#include "list.hpp"
#include "tree.hpp"

typedef list<int> path;

/******************************************************************************
 * General routines
 ******************************************************************************/

bool   zero_path (path p);
int    hash (path p);
string as_string (path p);
path   as_path (string s);
bool   version_inf_eq (string v1, string v2);
bool   version_inf (string v1, string v2);

/******************************************************************************
 * Operations on paths
 ******************************************************************************/

path path_up (path p);
path path_up (path p, int times);
bool path_inf (path p1, path p2);
bool path_inf_eq (path p1, path p2);
bool path_less (path p1, path p2);
bool path_less_eq (path p1, path p2);
path path_add (path p, int plus);
path path_add (path p, int plus, int pos);
#define path_inc(p) path_add (p, 1)
#define path_dec(p) path_add (p, -1)
path operator/ (path p, path q);
path common (path start, path end);
inline path
strip (path p, path q) {
  return p / q;
}

/******************************************************************************
 * Getting subtrees from paths
 ******************************************************************************/

/**
 * @brief 判断树 t 中是否存在 path p 所指的子树
 * @param t  待查询的树
 * @param p  子树坐标
 * @return   存在返回 true；p 为空 path 或逐段索引均落在 compound 范围内时成立
 */
bool has_subtree (tree t, const path& p);
/**
 * @brief 取树 t 中 path p 所指的子树引用
 * @param t  待查询的树
 * @param p  子树坐标，p 为空时返回 t 本身
 * @return   所指子树的引用
 * @note     p 越界或命中原子节点时打印一次诊断信息并返回 t 本身
 */
tree& subtree (tree& t, const path& p);
/**
 * @brief 取树 t 中 path p 所指子树的父节点引用
 * @param t  待查询的树
 * @param p  子树坐标，至少长度为 1
 * @return   所指子树父节点的引用；p 长度为 1 时返回 t
 */
tree& parent_subtree (tree& t, const path& p);

#endif // defined PATH_H
