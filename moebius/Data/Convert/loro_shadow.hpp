/** \file loro_shadow.hpp
 *  \copyright GPLv3
 *  \details Phase 2 shadow LoroDoc：每个编辑器挂一个 live
 * LoroDoc，随本地编辑逐操作镜像。 seed 用文档树构建 live doc 并填充 rep*→TreeID
 * 身份表（供 Step 4 镜像编辑用）。 仅 LORO_ENABLED 下存在；关闭时不编译，无
 * Rust 依赖。
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LORO_SHADOW_H
#define LORO_SHADOW_H

#include "array.hpp"
#include "hashmap.hpp"
#include "list.hpp"
#include "modification.hpp"
#include "tree.hpp"

#include "loro.hpp" // mogan_tree_id + live API（仅 LORO_ENABLED）

#ifdef LORO_ENABLED

class loro_shadow_rep : public concrete_struct {
public:
  void*                             doc;    // mogan_loro_doc 句柄（不透明）
  hashmap<tree_rep*, mogan_tree_id> id_map; //!< 节点身份：rep* -> Loro TreeID
  mogan_tree_id                     root_id;

  mogan_local_update_cb _update_cb       = nullptr;
  void*                 _update_user_data= nullptr;

  loro_shadow_rep ();
  ~loro_shadow_rep ();

  /** @brief 用树构建 live doc + 填充身份表。 */
  void seed (tree root);
  /** @brief 如果 shadow 已有内容（从远端 import），把 buffer 的 rep 关联到现有
   * TreeID （不创建新根，复用共享血统）。返回 true=成功同步，false=shadow
   * 为空。 */
  bool sync_id_map_from_shadow (tree buffer);
  /** @brief 把一个 modification 镜像到 live doc。doc_root 是 mod->p
   * 所相对的树根。
   *
   *  文本原子的 INSERT/REMOVE 精确镜像到 LoroText（原子身份在敲字时稳定，Step 1
   * 确认）； 其余 mod（结构、SPLIT/JOIN、ASSIGN 等）暂走整树重 seed 兜底，保证
   * shadow 始终与 buffer 一致——后续逐步把这些也精确化。 */
  void mirror_mod (tree doc_root, modification mod);
  /** @brief live doc -> snapshot 字节（包成 lolly string）。 */
  string export_snapshot ();
  /** @brief 把 snapshot/update 字节 import（合并）进 live doc。Phase 3
   * 反向同步。 */
  bool import_data (string bytes);
  /** @brief import 后用"带 TreeID 的增强 IR"重建 buffer 并把 buffer rep
   * 关联到导入的 TreeID（填 id_map），使本端后续编辑能与对端合并。out_buffer
   * 即新的 buffer。 */
  bool import_and_build (string bytes, tree& out_buffer);
  /** @brief 订阅本地编辑产生的增量 update：每次本地 op
   * 提交时回调。事件级增量同步。 */
  void on_local_update (mogan_local_update_cb cb, void* user_data);
  /** @brief commit 后把自上次广播以来的全部本地 op 作为增量 update
   * 推给已订阅的 local-update 回调。用于 seed 之后主动广播初始状态：Loro 的
   * subscribe_local_update 只发送每个 commit 事务内的 op，seed 的 create ops
   * 在首个 commit 里被发成"仅创建"骨架（无文本），接收端须靠这次补发拿到
   * 完整初始内容。 */
  void broadcast_update ();
  /** @brief 把远端 update 导入后，diff buffer（旧）vs shadow（新），生成把
   * buffer 变到 新状态所需的 modification 序列（文本字符级 diff →
   * INSERT/REMOVE；结构差异 → 整树 ASSIGN 兜底）。供编辑器在 versioning
   * 模式下经 edit_modify 应用到 buffer。 */
  list<modification> remote_diff_mods (string bytes, tree buffer);
  /** @brief diff buffer（旧）vs shadow 当前状态（新），生成把 buffer 变到
   * shadow 所需的 modification（不 import，用于 debug_loro 本地 round-trip）。
   */
  list<modification> diff_from_current (tree buffer);
  /** @brief live doc -> tree（经 to_ir + loro_ir_to_tree）。 */
  tree to_tree ();
  /** @brief 身份表是否含该节点。 */
  bool has_id (tree t);
  /** @brief 取节点的 TreeID（不在表中返回 {0,0}）。 */
  mogan_tree_id get_id (tree t);

private:
  mogan_tree_id seed_node (tree t, mogan_tree_id parent, uint32_t index);
  /** @brief 取某 LoroTree 节点的子 TreeID 列表（REMOVE 按位置删用）。 */
  array<mogan_tree_id> node_children (mogan_tree_id parent);
};

class loro_shadow {
  CONCRETE (loro_shadow);
  loro_shadow ();
};
CONCRETE_CODE (loro_shadow);

#endif // LORO_ENABLED
#endif // LORO_SHADOW_H
