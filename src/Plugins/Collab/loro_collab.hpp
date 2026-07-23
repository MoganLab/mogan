/******************************************************************************
 * MODULE     : loro_collab.hpp
 * DESCRIPTION: 云文档协作会话层（ImGui 前端）。
 *              职责：管理 WebSocket 连接与协议状态机（CREATE/JOIN/同步），
 *              把服务端补发的 snapshot/update 经编辑器 apply_remote 落到
 *              buffer，把本地编辑经 shadow 的 local-update 回调上行。
 *              CRDT 合并由客户端 shadow（loro_shadow）与服务端各自完成，
 *              本层只做传输与会话编排。仅 LORO_ENABLED 下存在。
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LORO_COLLAB_HPP
#define LORO_COLLAB_HPP

#include "array.hpp"
#include "string.hpp"

inline bool
loro_enabled () {
#ifdef LORO_ENABLED
  return true;
#else
  return false;
#endif
}

/**
 * @brief 取协作服务端地址（运行期可配，无需重编译）：
 *        - native：OS 环境变量 MOGAN_LORO_SERVER
 *        - WASM：window.MOGAN_LORO_SERVER 或 ?loro_server= 查询参数（浏览器无
 * OS env） 未设置时回落 ws://127.0.0.1:8765。
 */
string loro_collab_server_url ();

/**
 * @brief 以当前编辑器为_target，连服务端创建新云文档。
 *        连接后发 CREATE；服务端回 DOC <uuid> 后置位协作开关，
 *        之后本地首编辑会 seed shadow 并广播初始全量。
 * @return 服务端分配的文档 UUID（连接已发起但 DOC 尚未回来时为空串）。
 */
string loro_collab_create (string server_url);

/**
 * @brief 以当前编辑器为 target，连服务端加入已有云文档。
 *        连接后发 JOIN <doc_id>；服务端回 DOC 后补发 snapshot/updates，
 *        首帧到达时 apply_remote 构建 buffer 并置位协作开关。
 */
void loro_collab_join (string server_url, string doc_id);

/** @brief 断开协作会话（关闭 WS、清空 target、收回上行回调）。 */
void loro_collab_disconnect ();

/** @brief 是否处于活跃协作会话（已连接且已就绪）。 */
bool loro_collab_is_active ();

/** @brief 当前会话的服务端文档 UUID（未就绪时为空串）。 */
string loro_collab_doc_id ();

/**
 * @brief 异步触发：在后台线程 HTTP GET <server>/docs（不建 WS、不阻塞 GUI），
 *        结果存入内部缓存。重复调用在加载中时为 no-op。WASM 暂不支持。
 */
void loro_collab_fetch_docs (string server_url);

/**
 * @brief 文档列表查询状态："idle"(未查询)/"loading"(后台拉取中)/
 *        "ready"(完成，可能为空)/"error"(失败)。
 */
string loro_collab_docs_status ();

/** @brief 已缓存的文档 UUID 列表（未就绪时为空）。GUI 线程读取。 */
array<string> loro_collab_docs ();

/** @brief 在 GUI 帧循环里驱动（drain WS 回调）。由 GUI 的 _interpose 调用。 */
void loro_collab_poll ();

#endif // LORO_COLLAB_HPP
