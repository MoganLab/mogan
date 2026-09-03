
/******************************************************************************
 * MODULE     : qt_chat_model.hpp
 * DESCRIPTION: LLM 聊天模型清单：值类型、来源抽象与内置实现
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
 * @brief 模型菜单项的显示与能力数据（值类型）。
 */
struct ChatModelInfo {
  string key;           ///< 模型键名：%chat 协议与 manifest 的唯一标识
  string name;          ///< 菜单显示名（如 "K3"）
  string icon;          ///< 图标名，空串显示占位圆点
  string description;   ///< 描述徽标文字，空串不渲染徽标
  string dscColor;      ///< 徽标背景色："red" | "orange"，未知值按 orange
  bool   allowThinking; ///< false 时隐藏深度思考按钮（本期数据恒 true）
  bool   allowSearch;   ///< false 时隐藏联网搜索按钮（本期数据恒 true）
};

/**
 * @brief 模型清单来源抽象。本期仅 BuiltinModelProvider；
 * liii 插件重构完成后新增 LiiiModelProvider 桥接实现。
 */
class ChatModelProvider {
public:
  virtual ~ChatModelProvider ()               = default;
  virtual QList<ChatModelInfo> models () const= 0; ///< 返回顺序即菜单展示顺序
  virtual string               defaultModelKey () const= 0;
  bool          contains (const string& key) const; ///< 非虚，遍历 models()
  ChatModelInfo find (const string& key) const;     ///< 找不到返回 key 兜底构造
};

/**
 * @brief 内置清单：仅 Kimi-VLM，行为与现状一致。
 *
 * 环境变量 MOGAN_LLM_MODELS_FILE 存在且可读时改从该 JSON 文件读取
 * （调试通道，用于本地联调多条目 UI）。
 */
class BuiltinModelProvider : public ChatModelProvider {
public:
  BuiltinModelProvider ();
  QList<ChatModelInfo> models () const override;
  string               defaultModelKey () const override;

private:
  QList<ChatModelInfo> models_;
  string               defaultKey_;
};

/**
 * @brief 解析模型清单 JSON；字段缺失按既定规则补默认。
 * @param jsonText      JSON 文本
 * @param outModels     输出模型列表（解析失败时不变）
 * @param outDefaultKey 输出默认模型键名（解析失败时不变）
 * @return 解析成功返回 true；非法 JSON、models 缺失/为空、条目缺 key
 *         时返回 false
 */
bool chat_model_parse_list (const string&         jsonText,
                            QList<ChatModelInfo>& outModels,
                            string&               outDefaultKey);

#endif // QT_CHAT_MODEL_HPP
