/** \file loro_shadow.hpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 * \details Mogan Tree 与 LoroDoc 之间的实时镜像
 */

#ifndef LORO_SHADOW_H
#define LORO_SHADOW_H

#include "array.hpp"
#include "hashmap.hpp"
#include "list.hpp"
#include "modification.hpp"
#include "tree.hpp"

#include "loro.hpp"

class loro_shadow_rep : public concrete_struct {
public:
  void* doc; // mogan_loro_doc 句柄，所有操作都通过 FFI。
  hashmap<tree_rep*, mogan_tree_id>
                id_map;  // 节点身份：mogan tree_rep* -> Loro TreeID
  mogan_tree_id root_id; // shadow 中根节点对应的 TreeID

  mogan_local_update_cb _update_cb       = nullptr;
  void*                 _update_user_data= nullptr;

  loro_shadow_rep ();
  ~loro_shadow_rep ();

  // 用树构建 live doc + 填充 id_map
  void seed (tree root);

  /** 如果 shadow 已有内容（从远端 import），把 buffer 的 tree_rep 关联到现有
   * TreeID （不创建新根，复用共享血统）。返回 true=成功同步，false=shadow
   * 为空。 */
  bool sync_id_map_from_shadow (tree buffer);

  /** 一个 modification 镜像到 live doc。doc_root 是 mod->p 所相对的树根。
   *
   * 绝大多数操作（例如 INSERT/REMOVE）精确镜像到 LoroText
   * 其余暂走整树重 seed 兜底，保证 shadow 始终与 buffer 一致 */
  void mirror_mod (tree doc_root, modification mod);

  // live doc -> snapshot 字节（包成 lolly string）
  string export_snapshot ();

  // 把 snapshot/update 字节 import（合并）进 live doc 反向同步
  bool import_data (string bytes);

  /** import 后用"带 TreeID 的增强 IR"重建 buffer 并把 buffer rep
   * 关联到导入的 TreeID（填 id_map），使本端后续编辑能与对端合并。out_buffer
   * 即新的 buffer。 */
  bool import_and_build (string bytes, tree& out_buffer);

  /** 订阅本地编辑产生的增量 update：每次本地 op
   * 提交时回调。事件级增量同步。 */
  void on_local_update (mogan_local_update_cb cb, void* user_data);

  /** commit 后把自上次广播以来的全部本地 op 作为增量 update
   * 推给已订阅的 local-update 回调。用于 seed 之后主动广播初始状态：Loro 的
   * subscribe_local_update 只发送每个 commit 事务内的 op，seed 的 create ops
   * 在首个 commit 里被发成"仅创建"骨架（无文本），接收端须靠这次补发拿到
   * 完整初始内容。 */
  void broadcast_update ();

  /** 把 export vv 水位推进到当前（不导出/不广播）。JOIN 同步 import 远端
   * snapshot/updates 后调用，标记这些 op "已知"，避免下次
   * broadcast_update 把刚收到的内容当本地增量回传。仅在无 pending 本地 op
   * 时安全（首次同步）。 */
  void advance_export_vv ();

  /** 把远端 update 导入后，diff buffer（旧）vs shadow（新），生成把
   * buffer 变到 新状态所需的 modification 序列（文本字符级 diff →
   * INSERT/REMOVE；结构差异 → 整树 ASSIGN 兜底）。供编辑器在 versioning
   * 模式下经 edit_modify 应用到 buffer。 */
  list<modification> remote_diff_mods (string bytes, tree buffer);

  /** diff buffer（旧）vs shadow 当前状态（新），生成把 buffer 变到
   * shadow 所需的 modification（不 import，用于 debug_loro 本地 round-trip）。
   */
  list<modification> diff_from_current (tree buffer);

  tree          to_tree (); // live doc -> tree（经 to_ir + loro_ir_to_tree）
  bool          has_id (tree t); // id_map 是否含该节点
  mogan_tree_id get_id (tree t); // 取节点的 TreeID（不在表中返回 {0,0}）

private:
  // 取某 LoroTree 节点的子 TreeID 列表（用于 REMOVE 按位置删）
  array<mogan_tree_id> node_children (mogan_tree_id parent);
  mogan_tree_id        seed_node (tree t, mogan_tree_id parent, uint32_t index);

  // loro_shadow_mod
  bool mirror_insert (tree doc_root, modification mod);
  bool mirror_remove (tree doc_root, modification mod);
  bool mirror_assign_node (tree doc_root, modification mod);
  bool mirror_split (tree doc_root, modification mod);
  bool mirror_insert_node (tree doc_root, modification mod);
  bool mirror_remove_node (tree doc_root, modification mod);
  bool mirror_assign (tree doc_root, modification mod);
  bool mirror_join (tree doc_root, modification mod);
};

class loro_shadow {
  CONCRETE (loro_shadow);
  loro_shadow ();
};
CONCRETE_CODE (loro_shadow);

#endif // LORO_SHADOW_H
