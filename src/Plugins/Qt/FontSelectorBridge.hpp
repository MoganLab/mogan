/******************************************************************************
 * MODULE      : FontSelectorBridge.hpp
 * DESCRIPTION : 字体选择器 QML 对话框的 C++↔QML 桥。透传、不持字体状态：
 *               scheme 侧 font-selector-* facade（font-new-widgets.scm）经
 *               specsKey 句柄持有 specs，bridge 每次 Q_INVOKABLE 调用都带上
 *               specsKey，拼 scheme 串经 eval_scheme 调 facade，把结果转 QML
 *               可消费的类型。详见 record/qml/font-selector.md Phase 2。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file FontSelectorBridge.hpp
 *  @brief QML 字体选择器 bridge：把 QML 的请求转发到 scheme facade。
 *
 * @par 设计
 * - @b 透传无状态：字体状态真相源在 scheme（selector-table 按 buffer
 * 为键），bridge 除 specsKey 外不持有任何字体数据。
 * - @b specsKey 句柄：specs 含 getter/setter 过程，不能跨 glue 序列化。scheme
 * 维护 specs-registry（int → specs），bridge 只存 int，每次调用带上。
 * - @b eval_scheme 调 facade：每个 Q_INVOKABLE 方法拼 scheme 串调
 * font-selector-* proc，结果（scheme list/string）转
 * QStringList/QString/QVariantList。
 * - @b 联动返回：setter 直接返回需刷新的依赖（如 setFamily 返回 styles +
 * preview）， QML 在同一 handler 更新 model，省二次往返。
 *
 * @par 不变量
 * - @c selector-table 按 buffer 为键，同 buffer 单对话框（模态契约，见
 * QTMQmlDialog.hpp）。 bridge 不复刻此约束，依赖既有 scheme 契约。
 * - @c live=true：selector-set 实时写 buffer；Cancel 经打开时快照写回撤销，OK
 * 补齐差异落定。
 * - @b 非阻塞模态：run_modal_qml_dialog 用 setModal+show（非
 * exec），既独占输入又 让主窗口 live 重绘。详见 QTMQmlDialog.cpp 的
 * run_modal_qml_dialog。
 *
 * @note 生命期：host 堆分配（run_modal_qml_dialog 内 new +
 * WA_DeleteOnClose），bridge 不挂 parent；submit/cancel 调 close() →
 * WA_DeleteOnClose 触发 host 析构 → destroyed 信号 deleteLater 掉 bridge。
 */

#ifndef FONT_SELECTOR_BRIDGE_HPP
#define FONT_SELECTOR_BRIDGE_HPP

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QWindow>

class FontSelectorBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象。
   * @param host 宿主 QDialog（非模态 show 的窗口，submit/cancel 调其
   * close()），不挂 parent。
   * @param specsKey scheme specs-registry 的 int 句柄。
   */
  FontSelectorBridge (QDialog* host, int specsKey)
      : QObject (), m_host (host), m_specsKey (specsKey) {
    ASSERT (host != NULL, "FontSelectorBridge expects a valid QDialog host");
  }

  // ---- 三栏：family / style / size ----
  /// family 列表（据当前 filter）。
  Q_INVOKABLE QStringList requestFamilies ();
  /// 指定 family 下的 style 列表。
  Q_INVOKABLE QStringList requestStyles (const QString& family);
  /// 字号档列表。
  Q_INVOKABLE QStringList requestSizes ();
  /// 当前选中 family/style/size。
  Q_INVOKABLE QString currentFamily ();
  Q_INVOKABLE QString currentStyle ();
  Q_INVOKABLE QString currentSize ();
  /// 设 family，返回 {styles, preview} 供 QML 同步刷新 style 列表与预览。
  Q_INVOKABLE QVariantMap setFamily (const QString& family);
  /// 设 style，返回 {preview}。
  Q_INVOKABLE QVariantMap setStyle (const QString& style);
  /// 设 size，返回 {preview}。
  Q_INVOKABLE QVariantMap setSize (const QString& size);

  // ---- 9 项 Filter ----
  /// 全部 filter 元数据：(label, var, options, value) ×9。
  Q_INVOKABLE QVariantList filterMeta ();
  /// 设 filter，返回 {families, preview}（filter 收窄 family 列表）。
  Q_INVOKABLE QVariantMap setFilter (const QString& var, const QString& val);

  // ---- 预览 ----
  /// 同步返回 data URL（family/style/size/filter/customize 变更后拉取）。
  Q_INVOKABLE QString requestPreview ();

  // ---- 样本类型 ----
  Q_INVOKABLE QStringList sampleKinds ();
  Q_INVOKABLE QString     currentSampleKind ();
  /// 设样本类型，返回 {preview}。
  Q_INVOKABLE QVariantMap setSampleKind (const QString& kind);

  // ---- Advanced 定制 ----
  /// 全部定制项元数据：(group, label, which, options, value) ×N。
  Q_INVOKABLE QVariantList customizeMeta ();
  Q_INVOKABLE QVariantMap  setCustomize (const QString& which,
                                         const QString& val);

  // ---- 动作 ----
  /// Advanced 子对话框用 QML Stack，无需新 glue；openAdvanced 仅占位通知。
  Q_INVOKABLE void openAdvanced () {}
  Q_INVOKABLE void importFont ();
  Q_INVOKABLE void reset ();

  /// 固定 UI 文案的翻译（family/style/size 标题、按钮等），QML 一次性拉取。
  Q_INVOKABLE QVariantMap uiLabels ();

  /// OK：font-selector-commit 补齐差异落定，close() 关闭对话框
  /// （WA_DeleteOnClose → host delete → 本 bridge deleteLater）。
  Q_INVOKABLE void submit ();
  /// Cancel：font-selector-cancel 经快照写回撤销，close() 关闭。
  Q_INVOKABLE void cancel ();

  /**
   * @brief 拖动无边框窗口，委托底层 QWindow 系统级移动。
   */
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ())
      m_host->windowHandle ()->startSystemMove ();
  }

private:
  /// 拼 `(proc specsKey)` 求 list of string → QStringList。
  QStringList evalStringList (const string& proc);
  /// 拼 `(proc specsKey)` 求单 string → QString。
  QString evalString (const string& proc);
  /// 拼 `(proc specsKey arg)` 求单 string → QString（arg 经 scheme quote）。
  QString evalString1 (const string& proc, const string& arg);
  /// 拼 `(proc specsKey)` 求 data URL → QString（同 evalString，语义清晰）。
  QString evalPreview ();

  QDialog* m_host;
  int      m_specsKey;
};

#endif // defined FONT_SELECTOR_BRIDGE_HPP
