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

// mogan_tree_id 作为 hashmap key 所需的 hash 与相等（id_map 的反向 rev_id_map
// 用）
inline int
hash (mogan_tree_id id) {
  return (int) ((id.peer ^ (id.peer >> 32)) ^ (uint64_t) id.counter);
}
inline bool
operator== (mogan_tree_id a, mogan_tree_id b) {
  return a.peer == b.peer && a.counter == b.counter;
}

class loro_shadow_rep : public concrete_struct {
public:
  void* doc; // mogan_loro_doc 句柄，所有操作都通过 FFI。
  hashmap<tree_rep*, mogan_tree_id>
      id_map; // 节点身份：mogan tree_rep* -> Loro TreeID
  hashmap<mogan_tree_id, path>
      rev_id_map; // 反向：TreeID -> 节点 buffer-相对 path（与 id_map 同处维护）
  mogan_tree_id root_id; // shadow 中根节点对应的 TreeID
  hashmap<string, mogan_tree_id>
      meta_root_ids; // body 之外的 section（style/initial/...）-> 其 root
                     // TreeID

  // 身份对账用：最近一次 to_tree_with_ids 解出的 after 树的 节点 rep -> TreeID
  // 映射（独立于 id_map，对账期间只读，避免污染 shadow 主映射）。
  hashmap<tree_rep*, mogan_tree_id> after_id_map;

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

  tree to_tree (); // live doc -> tree（经 to_ir + loro_ir_to_tree）
  /** live doc -> 带身份的 after 树（经 to_ir_with_ids）：返回重建的树，并填
   * after_id_map（after 树节点 rep -> TreeID）。供身份对账（reconcile_walk）
   * 用，使远端合并后的 buffer 对齐按 TreeID 而非位置进行。 */
  tree          to_tree_with_ids ();
  bool          has_id (tree t); // id_map 是否含该节点
  mogan_tree_id get_id (tree t); // 取节点的 TreeID（不在表中返回 {0,0}）
  /** 身份对账：diff buffer（旧）vs 带身份的 after 树（新），按 TreeID（而非
   * 位置）匹配节点，生成把 buffer 变到 after 的 modification（并发 merge 重排
   * 子节点时仍把删除/插入落到正确节点）。after 由 to_tree_with_ids 给出并填好
   * after_id_map。供 diff_from_current 使用。 */
  list<modification> reconcile_walk (tree buffer, tree after);
  /** 反查：用 rev_id_map 把 TreeID 解析为节点 buffer-相对 path 并追加偏移
   * offset（原子节点内即 LoroText 字符偏移；复合节点即子索引）。用于把远端
   * peer 的光标/选区 TreeID 解析回本端 path，达成 CRDT 级稳定。节点未找到
   * （尚未同步到/已被删除）返回 nil，调用方据此跳过渲染。rev_id_map 与 id_map
   * 同处维护（seed_node / sync_walk / decode_id_node），故始终与当前 buffer
   * 一致。 */
  path cursor_path_of (mogan_tree_id id, int offset);
  /** 取 TreeID 对应节点的 buffer-相对 path（不追加偏移）。节点未找到返回 nil。
   */
  path node_path_of (mogan_tree_id id);

  /** 把原子文本节点（id 的 LoroText）在 unicode offset 处的**稳定位置**（Loro
   * Cursor，op-id 锚定）编码为 hex 字符串（postcard 字节的 hex）。失败返回 ""。
   * 稳定位置在并发编辑下自动跟随内容位移，是 CRDT 级光标同步的偏移表示。 */
  string encode_cursor_hex (mogan_tree_id id, int offset);
  /** 反向：hex（encode_cursor_hex 产出）→ 按**当前 doc** 解析为 unicode 偏移。
   * 锚点被删时 Loro 自愈到邻近位置；容器消失等返回 -1（调用方丢弃）。 */
  int decode_cursor_hex (string hex);

  // ===== meta section（body 之外的文档部分）的 coarse 镜像 =====
  /** 首次把一个 meta section（style/initial/final/project/attachments）灌入
   * CRDT： 作为带 __section__ 标签的独立 root。幂等（若已存在先删旧 root
   * 再建）。*/
  void seed_meta (string name, tree section_tree);
  /** 本地 meta section 改动：删旧 root + 重建（coarse，section 粒度 LWW），并
   * commit。*/
  void mirror_meta_replace (string name, tree section_tree);
  /** 读回某 section 当前树（经 section_to_ir + ir_to_tree）。不存在返回空树。*/
  tree          meta_to_tree (string name);
  bool          has_meta (string name); // 该 meta section 是否存在
  array<string> list_meta_sections ();  // 当前 shadow 中所有 meta section 名
  /** 用 FFI list_sections 重建 meta_root_ids 账本（import 远端数据后调用）。
   *  同名多 root（并发 coarse 改动）取字典序最大 TreeID，保证两端确定性一致。*/
  void sync_meta_from_shadow ();

private:
  void replace_meta (string name, tree section_tree); // seed/replace 共用
  // 取某 LoroTree 节点的子 TreeID 列表（用于 REMOVE 按位置删）
  array<mogan_tree_id> node_children (mogan_tree_id parent);
  mogan_tree_id        seed_node (tree t, mogan_tree_id parent, uint32_t index,
                                  path p);

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
