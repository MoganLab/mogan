/******************************************************************************
 * MODULE      : QTMQmlDialogInternal.hpp
 * DESCRIPTION : QML 对话框底座的内部纯函数，供单元测试直接调用。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef QTM_QML_DIALOG_INTERNAL_H
#define QTM_QML_DIALOG_INTERNAL_H

#include "tree.hpp"

#include <QStringList>
#include <QVariantMap>

// 字段节点下标协议（见 QTMQmlDialog.hpp @par 数据协议）：
// (<type> <label> <key> (<options>...) <value> <live?>)
extern const int FIELD_LABEL;
extern const int FIELD_KEY;
extern const int FIELD_OPTIONS;
extern const int FIELD_VALUE;
extern const int FIELD_LIVE;

/**
 * @brief 字段节点是否形状合法（compound 且至少含到 value 的位置）。
 */
bool field_valid (tree f);

/**
 * @brief 取字段节点的 key 子树（透传，不拷贝字符串）。
 */
tree field_key (tree f);

/**
 * @brief 取字段节点的 value 子树（透传）。
 */
tree field_value (tree f);

/**
 * @brief 单个字段节点 tree → QML 可消费的 QVariantMap。
 * @param f 字段节点，形如 (enum <label> <key> (<opt>...) <value> <live?>)。
 * @return 含 type/label/key/options/value/live 的 map；形状不符返回空。
 *
 * label/key/value 纯透传，不做翻译或类型转换（value 在 scm 侧已 string 化）。
 */
QVariantMap field_tree_to_qml (tree f);

/**
 * @brief 把按钮文案数组（英文 key）翻译成 QML 可消费的 QStringList。
 * @param buttons 英文 key 数组（如 "OK" / "Cancel" / "Save"）。
 * @return 经 qt_translate 翻译后的文案列表；字典未命中则原样回退。
 *
 * 供两类弹窗注入 dialogButtons，统一走 mogan translate 通道避免 QML
 * 硬编码漏译。
 */
QStringList translate_buttons (array<string> buttons);

#endif // defined QTM_QML_DIALOG_INTERNAL_H
