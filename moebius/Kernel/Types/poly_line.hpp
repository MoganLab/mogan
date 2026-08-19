
/******************************************************************************
 * MODULE     : poly_line.hpp
 * DESCRIPTION: Poly lines
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

//! @file poly_line.hpp
//! @brief 折线（poly_line）与轮廓集合（contours）的基础几何例程。
//!
//! 折线由有序点列构成，轮廓是若干条折线的集合。本模块提供点级工具
//! （范数、距离、投影、上下界）、折线/轮廓的仿射变换、归一化、
//! 弧长参数化访问、顶点检测以及手写识别用的不变量提取。

#include "point.hpp"

typedef array<point>     poly_line;
typedef array<poly_line> contours;

/** @name 点级工具例程 */
/**@{*/

/**
 * @brief 计算 point 的 L2 范数（欧氏长度）。
 * @param p 输入点
 * @return sqrt(sum(p[i]^2))
 */
double l2_norm (point p);

/**
 * @brief 计算两点间的欧氏距离。
 * @param p 第一个点
 * @param q 第二个点
 * @return 两点距离；要求两点半维数相同
 */
double distance (point p, point q);

/**
 * @brief 将点 p 投影到线段 q1q2 上（含端点截断）。
 * @param p 被投影的点
 * @param q1 线段起点
 * @param q2 线段终点
 * @return 线段上离 p 最近的点
 */
point project (point p, point q1, point q2);

/**
 * @brief 计算点 p 到线段 q1q2 的距离。
 * @param p 被测点
 * @param q1 线段起点
 * @param q2 线段终点
 * @return 点到线段的最短距离
 */
double distance (point p, point q1, point q2);

/**
 * @brief 分量式下界：r[i] = min(p[i], q[i])。
 * @param p 第一个点
 * @param q 第二个点
 * @return 各分量取 min 后的新点
 */
point inf (point p, point q);

/**
 * @brief 分量式上界：r[i] = max(p[i], q[i])。
 * @param p 第一个点
 * @param q 第二个点
 * @return 各分量取 max 后的新点
 */
point sup (point p, point q);

/**@}*/

/** @name 折线例程 */
/**@{*/

/**
 * @brief 计算点到折线的最短距离（逐段取最小）。
 * @param p 被测点
 * @param pl 折线
 * @return 点 p 到所有线段距离的最小值
 */
double distance (point p, const poly_line& pl);

/**
 * @brief 判断点是否在折线附近（距离 <= 5.0）。
 * @param p 被测点
 * @param pl 折线
 * @return 距离不超过 5.0 时返回 true
 */
bool nearby (point p, poly_line pl);

/**
 * @brief 折线包围盒的下角点（各分量最小值）。
 * @param pl 非空折线
 * @return 所有顶点分量式 min 构成的点
 */
point inf (const poly_line& pl);

/**
 * @brief 折线包围盒的上角点（各分量最大值）。
 * @param pl 非空折线
 * @return 所有顶点分量式 max 构成的点
 */
point sup (const poly_line& pl);

/**
 * @brief 折线平移：每个顶点加 p。
 * @param pl 折线
 * @param p 平移向量
 * @return 平移后的新折线
 */
poly_line operator+ (poly_line pl, point p);

/**
 * @brief 折线反向平移：每个顶点减 p。
 * @param pl 折线
 * @param p 平移向量
 * @return 平移后的新折线
 */
poly_line operator- (poly_line pl, point p);

/**
 * @brief 折线缩放：每个顶点乘标量 x。
 * @param x 缩放系数
 * @param pl 折线
 * @return 缩放后的新折线
 */
poly_line operator* (double x, poly_line pl);

/**
 * @brief 折线归一化：先平移使下角点为原点，再缩放使最大分量为 1。
 * @param pl 折线
 * @return 归一化后的折线；空折线或退化（缩放因子为 0）时原样返回
 */
poly_line normalize (poly_line pl);

/**
 * @brief 计算折线总弧长（逐段长度求和）。
 * @param pl 折线
 * @return 各段欧氏长度之和
 */
double length (poly_line pl);

/**
 * @brief 按弧长参数 t 取折线上的点。
 *
 * t 是从起点出发沿折线走过的距离（绝对长度，不是 [0,1] 比例参数）。
 * 实现为逐段弧长扫描：依次扣除各段长度定位到所在段，再在段内按剩余
 * 比例线性插值，故沿折线等速取点。
 *
 * @param pl 折线
 * @param t 弧长参数；t <= 0 返回首点，t >= 总弧长（length(pl)）返回末点
 * @return 折线上距起点弧长为 t 处的点
 * @note 与 vertices 的归一化参数配套使用：t = p * length(pl)。
 * @see length, vertices
 */
point access (poly_line pl, double t);

/**
 * @brief 顶点检测：返回折线中"转折处"的归一化弧长参数。
 *
 * 归一化弧长参数 p 定义为「该点到起点的弧长 ÷ 折线总弧长」，
 * 取值 [0,1]：0 对应首点，1 对应末点，p=0.5 对应沿折线走到一半路程
 * 的位置（不是顶点序号的一半，也不是几何中点）。要取回实际坐标，
 * 需乘回总长：access(pl, p * length(pl))。
 *
 * 检测方式：折线先缩放到总长 1，对每个采样顶点 p_i，在参数
 * t_i ± 0.025 邻域内取两点与 p_i 做点积，点积非负（局部夹角 >= 90°）
 * 视为候选转折，每个邻窗取点积最大者报告。
 *
 * @param pl 折线
 * @return 严格递增的参数数组，首元素 0.0、末元素 1.0，中间为检测到的
 *         顶点参数；相邻报告点间距至少 0.025
 */
array<double> vertices (poly_line pl);

/**@}*/

/** @name 轮廓（折线集合）例程 */
/**@{*/

/**
 * @brief 计算点到轮廓集合的最短距离（逐条折线取最小）。
 * @param p 被测点
 * @param gl 轮廓集合
 * @return 点到所有折线距离的最小值
 */
double distance (point p, contours gl);

/**
 * @brief 判断点是否在轮廓附近（距离 <= 5.0）。
 * @param p 被测点
 * @param gl 轮廓集合
 * @return 距离不超过 5.0 时返回 true
 */
bool nearby (point p, contours gl);

/**
 * @brief 轮廓整体包围盒的下角点。
 * @param gl 非空轮廓集合
 * @return 所有折线顶点分量式 min 构成的点
 */
point inf (contours gl);

/**
 * @brief 轮廓整体包围盒的上角点。
 * @param gl 非空轮廓集合
 * @return 所有折线顶点分量式 max 构成的点
 */
point sup (contours gl);

/**
 * @brief 轮廓平移：每条折线的每个顶点加 p。
 * @param gl 轮廓集合
 * @param p 平移向量
 * @return 平移后的新轮廓
 */
contours operator+ (contours gl, point p);

/**
 * @brief 轮廓反向平移：每条折线的每个顶点减 p。
 * @param gl 轮廓集合
 * @param p 平移向量
 * @return 平移后的新轮廓
 */
contours operator- (contours gl, point p);

/**
 * @brief 轮廓缩放：每条折线的每个顶点乘标量 x。
 * @param x 缩放系数
 * @param gl 轮廓集合
 * @return 缩放后的新轮廓
 */
contours operator* (double x, contours gl);

/**
 * @brief 轮廓归一化：整体平移使下角点为原点，再缩放使最大分量为 1。
 * @param gl 轮廓集合
 * @return 归一化后的轮廓；空轮廓或退化时原样返回
 */
contours normalize (contours gl);

/**
 * @brief 提取轮廓的不变量特征（手写识别用）。
 * @param gl 轮廓集合
 * @param level 不变量级别；level <= 1 时额外附上顶点信息
 * @param disc [out] 离散特征（轮廓条数、每条折线的顶点数）
 * @param cont [out] 连续特征（沿每条折线均匀采样的坐标，以及顶点参数）
 * @note 输入 gl 会先做归一化再采样。
 */
void invariants (contours gl, int level, array<tree>& disc,
                 array<double>& cont);
