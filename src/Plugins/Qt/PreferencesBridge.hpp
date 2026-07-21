/******************************************************************************
 * MODULE      : PreferencesBridge.hpp
 * DESCRIPTION : 首选项 QML 对话框的 C++↔QML 桥（无状态透传）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file PreferencesBridge.hpp
 *  @brief QML 首选项 bridge：把 QML 的请求转发到 scheme facade。
 *
 * @par 设计
 * - @b 无状态透传：首选项是全局的，无 specs-registry 句柄（区别于字体选择器 /
 *   段落格式的 specsKey 模式）。bridge 不持有任何偏好数据，每次 Q_INVOKABLE
 * 调用 都拼 scheme 串经 eval_scheme 调 facade。
 * - @b 本地暂存 + OK 一次性提交（FormDialog 模式）：QML 打开时拉一次 meta
 * 建本地 values 快照、改动只改 QML 本地、OK 时算 diff 调 submit
 * 一次性应用、Cancel 丢弃。 故 bridge 无 setter——QML 不 live 写偏好。
 * - @b eval_scheme 调 facade：meta() 拼 `(preferences-qml-meta)`、submit() 拼
 *   `(preferences-qml-submit <assoc-literal>)`。结果（scheme list of
 * field-descriptor / 4 态字符串）转 QVariantMap/QVariantList/QString。
 * - @b CJK 标签编码：scheme 侧 (translate "key") 到 Cork，bridge 必须用
 *   cork_to_utf8 + utf8_to_qstring（勿用 to_qstring——其对纯 Cork
 * 中文启发式不稳定， 见 FontSelectorBridge.cpp:42-48）。
 *
 * @par 不变量
 * - @b 统一字符串 wire：toggle 在 QML 内用 bool，但提交到 scheme 统一序列化为
 *   "on"/"off" 串（与 get/set-preference 直通，避免类型分派）。meta 里 toggle
 * 的 value 也用 "on"/"off" 串。
 * - @b 先确认再 apply：submit 内部若 diff 含需重启字段，facade 先弹
 *   cpp-confirm-restart（标题用首个改动重启字段的
 * restart-preference-title）再按 用户选择 apply / silent 写值 / 不
 * apply。bridge 只透传 4 态返回串。
 *
 * @note 生命期：走 run_qml_dialog（exec 阻塞模态，同 FormDialog）。bridge 不挂
 * parent，由调用方（cpp_preferences_dialog）在 exec 后 delete。host destroyed
 * 信号 deleteLater 自清（form-pattern，详见 QTMQmlDialog.cpp）。
 */

#ifndef PREFERENCES_BRIDGE_HPP
#define PREFERENCES_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QWindow>

class PreferencesBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog（exec 模态的窗口，submit/cancel 调其 close()），
   * 不挂 parent。
   */
  explicit PreferencesBridge (QDialog* host) : QObject (), m_host (host) {
    ASSERT (host != NULL, "PreferencesBridge expects a valid QDialog host");
  }

  /// 一次性拉全部 tab / 字段描述符树（scheme preferences-qml-meta 返回）。
  Q_INVOKABLE QVariantMap meta ();
  /// 应用 diff：changed 为 {key: value} 只含改动项（QML 算好的 diff），value 均
  /// string（toggle 为 "on"/"off"）。返回 "applied" / "restart" / "later" /
  /// "cancel"。
  Q_INVOKABLE QString submit (const QVariantMap& changed);
  /// Cancel：丢弃本地改动，关窗。scheme 侧 no-op。
  Q_INVOKABLE void cancel ();
  /// 拖动无边框窗口，委托底层 QWindow 系统级移动。
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ())
      m_host->windowHandle ()->startSystemMove ();
  }

private:
  /// 拼 `(preferences-qml-meta)` 求 tab 描述符树 → QVariantMap（含 tabs
  /// 列表）。 由 C++ 侧 list-walking 解析 field-descriptor 列表为
  /// QVariantMap/QVariantList。
  QVariantMap eval_meta ();
  /// 拼 `(preferences-qml-submit <assoc-literal>)` 应用 diff，返回 4 态字符串。
  /// assoc-literal 由本函数从 changed map 序列化：(("k1" "v1") ...) 每项
  /// qt_scheme_quote。
  QString eval_submit (const QVariantMap& changed);

  QDialog* m_host;
};

#endif // defined PREFERENCES_BRIDGE_HPP
