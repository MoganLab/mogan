
/******************************************************************************
 * MODULE     : archiver.hpp
 * DESCRIPTION: manage undo/redo history
 * COPYRIGHT  : (C) 2009  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef ARCHIVER_H
#define ARCHIVER_H
#include "patch.hpp"

/**
 * @name 全局历史操作：对所有归档器批量提交/撤销（编辑事务用）。
 */
///@{
void global_clear_history (); //!< 清空所有归档器的历史
void global_confirm ();       //!< 提交所有待确认归档器的当前修改
void global_cancel ();        //!< 取消所有待确认归档器的当前修改
///@}

/**
 * @brief undo/redo 历史归档器。每个文档关联一个 archiver，维护其修改历史。
 *
 * 历史以 patch 组织成多分支树：
 *  - #archive：已确认的历史（第 0 分支为 undo 部分，其余为 redo 分支）；
 *  - #current：尚未确认的修改序列，由 add() 累积，confirm() 后并入 archive。
 *
 * 用 #depth、#last_save、#last_autosave 三个深度跟踪状态演进，进而判断
 * 文档相对最近一次「保存 / 自动保存」是否被修改。
 */
class archiver_rep : public concrete_struct {
  patch    archive;       //!< undo 与 redo 历史归档树
  patch    current;       //!< 尚未确认的当前修改序列
  int      depth;         //!< 当前 archive 深度（已确认的历史条目数）
  int      last_save;     //!< 最近一次保存时的深度，-1 表示未保存
  int      last_autosave; //!< 最近一次自动保存时的深度，-1 表示未自动保存
  double   the_author;    //!< 本归档器对应的作者标识
  double   the_owner;     //!< 当前修改序列的作者
  path     rp;            //!< 文档在 the_et 中的根路径
  observer undo_obs;      //!< 捕获文档修改的观察者
  bool     versioning;    //!< 处于 undo/redo 回放期间（禁用增量记录）

protected:
  /**
   * @name 内部子例程（历史树操作）
   */
  ///@{
  void  apply (patch p);                              //!< 应用补丁到 the_et，期间置 versioning=true
  void  split (patch p1, patch p2, patch& re1, patch& re2); //!< 按作者拆分 p2 分支：可交换者入 re2，余入 re1
  patch make_future (patch p1, patch p2);             //!< 跨作者 redo 时构造 p1 之后的历史分支
  patch expose (patch archive);                       //!< 把被他人遮蔽的本作者条目暴露到 undo 链顶（递归）
  void  expose ();                                    //!< expose() 的封装，作用于本对象 #archive
  void  normalize ();                                 //!< 规范化：把跨作者的 redo 分支重排进 undo 链
  int   corrected_depth ();                           //!< 扣除分组 marker 偏移后的深度，用于判定保存一致性
  ///@}

public:
  /**
   * @name 构造、析构、清理与打印
   */
  ///@{
  archiver_rep (double author, path rp); //!< 构造：绑定作者与文档根路径，挂载观察者
  ~archiver_rep ();                      //!< 析构：注销作者、摘除观察者
  void clear ();                         //!< 清空全部历史与当前修改（重置为空文档）
  void show_all ();                      //!< 调试：打印完整历史树
  ///@}

  /**
   * @name 当前修改序列的累积（操作尚未确认进历史的 #current）
   */
  ///@{
  void add (modification m);   //!< 累积一次修改进 #current
  void start_slave (double a); //!< 插入标记作者切换的占位补丁
  bool active ();              //!< #current 是否非空
  bool has_history ();         //!< archive 是否存在可撤销条目
  void cancel ();              //!< 取消当前一系列修改（回滚 #current）
  void confirm ();             //!< 把当前修改提交进历史
  bool retract ();             //!< 重开最近一个历史条目以继续修改
  bool forget ();              //!< 撤销并丢弃最近一个历史条目
  void forget_cursor ();       //!< 丢弃最近历史条目中的光标修改
  void simplify ();            //!< 合并相邻可合并条目以简化历史树
  ///@}

  /**
   * @name Undo / Redo
   */
  ///@{
  int  undo_possibilities (); //!< 可撤销条目数（0 或 1）
  int  redo_possibilities (); //!< 可重做的分支数
  path undo_one (int i);      //!< 撤销一个条目，返回光标提示
  path redo_one (int i);      //!< 重做第 i 条分支，返回光标提示
  path undo (int i= 0);       //!< 跨作者连续撤销，直到本作者可撤销条目
  path redo (int i= 0);       //!< 跨作者连续重做，直到本作者可重做分支
  ///@}

  /**
   * @name 分组标记：用 marker 把若干修改归为一组，便于整组提交或取消。
   */
  ///@{
  void mark_start (double m);  //!< 插入起始 marker，开始一个分组
  bool mark_cancel (double m); //!< 取消自起始 marker 起的所有修改
  void mark_end (double m);    //!< 结束并提交分组，移除 marker
  ///@}

  /**
   * @name 保存 / 自动保存状态
   *
   * 比较 #depth 与 #last_save / #last_autosave 判断文档是否相对最近一次
   * 保存（自动保存）有变化。当历史重排使原保存点条目不可达时，
   * 相关深度置 -1（视为未保存）。
   */
  ///@{
  void require_save ();     //!< 标记需保存：last_save = -1
  void require_autosave (); //!< 标记需自动保存：last_autosave = -1
  void notify_save ();      //!< 已保存：用 corrected_depth() 更新 last_save
  void notify_autosave ();  //!< 已自动保存：用当前 depth 更新 last_autosave
  bool conform_save ();     //!< 是否与最近一次保存一致
  bool conform_autosave (); //!< 是否与最近一次自动保存一致
  ///@}

  friend void archive_announce (archiver_rep* arch, modification mod);
  friend void global_clear_history ();
  friend void global_confirm ();
  friend void global_cancel ();
};

/**
 * @brief archiver 的引用计数句柄（concrete 模式）。
 * 实际逻辑由 archiver_rep 承载，本类持有其智能指针 rep，用法类似值类型。
 */
class archiver {
  CONCRETE (archiver);
  archiver (double author, path rp); //!< 构造归档器并绑定作者与文档根路径
};
CONCRETE_CODE (archiver);

/**
 * @brief 文档修改通知入口。
 * 由 tree 观察者捕获 modification 时调用，累积进对应归档器；
 * 处于 versioning（回放）期间忽略，避免递归记录。
 * @param buf 目标归档器
 * @param mod 被捕获的修改
 */
void archive_announce (archiver_rep* buf, modification mod);

#endif // defined ARCHIVER_H
