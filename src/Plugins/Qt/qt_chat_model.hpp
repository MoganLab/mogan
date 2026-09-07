
/******************************************************************************
 * MODULE     : qt_chat_model.hpp
 * DESCRIPTION: LLM 聊天的模型清单数据类型与加载器
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_MODEL_HPP
#define QT_CHAT_MODEL_HPP

#include "string.hpp"
#include <QList>

/**
 * @brief 模型菜单项数据（值类型；统一字段的 Qt 消费子集 + 发送所需字段）。
 */
struct ChatModelInfo {
  string key;  ///< = 真实模型 id（如 "kimi-k3"），菜单键/manifest/协议共用
  string name; ///< 显示名，缺省 = key
  string icon; ///< 图标名，空串显示占位圆点
  string description;         ///< 徽标文字，空串不渲染
  string dscColor;            ///< "red" | "orange"，未知按 orange
  bool   allowThinking= true; ///< 是否允许推理模式，缺省 true
  bool   allowSearch  = true; ///< 是否允许网络搜索，缺省 true
  string baseUrl;             ///< 服务端接口（可为相对路径；PR-M5 发送时使用）
  string defaultSystem;       ///< 模型默认系统提示（PR-M5 发送时使用）
};

/**
 * @brief 模型清单：从固定路径 JSON 加载（HOME 优先、PATH
 * 兜底），失败回退内置清单。
 *
 * 路径规则：$TEXMACS_HOME_PATH/plugins/llm/goldfish/data/liii_llm_menu.json
 * 优先，否则 $TEXMACS_PATH/plugins/llm/goldfish/data/liii_llm_menu.json。
 * 文件不存在或解析失败时使用内置兜底清单（Kimi-VLM 单条目）。
 */
class ChatModelStore {
public:
  /// 构造即按上述路径规则 load
  ChatModelStore ();

  /// 顺序即菜单展示顺序（JSON 内书写顺序）
  QList<ChatModelInfo> models () const { return models_; }

  /// 默认模型 key（清单 default，缺失时第一个条目）
  string defaultKey () const { return defaultKey_; }

  /**
   * @brief 清单是否包含指定模型 key。
   */
  bool contains (const string& key) const;

  /**
   * @brief 查找指定 key 的模型信息。
   * @return 找到的条目；找不到时返回以 key 兜底构造的 Info
   */
  ChatModelInfo find (const string& key) const;

private:
  QList<ChatModelInfo> models_;     ///< 模型条目（仅含 enable 条目）
  string               defaultKey_; ///< 默认模型 key
};

/**
 * @brief 解析模型清单 JSON（新格式，兼容旧格式）。
 *
 * 新格式：顶层 { "default": ..., "models": [条目数组] }。
 * 旧格式：顶层无 "models" 键，顶层键即条目 key（可含名为 "default" 的
 * 条目，其 model 字段指定默认 key）。
 *
 * @param jsonText     JSON 文本
 * @param outModels    输出：解析出的模型条目（仅含 enable 条目），失败时不变
 * @param outDefaultKey 输出：默认模型 key，失败时不变
 * @return 解析成功返回 true；非法 JSON / models 非数组 / 过滤后为空返回 false
 */
bool chat_model_parse_list (const string&         jsonText,
                            QList<ChatModelInfo>& outModels,
                            string&               outDefaultKey);

#endif // QT_CHAT_MODEL_HPP
