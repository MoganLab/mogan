/******************************************************************************
 * MODULE      : QTMQmlDialogBridge.hpp
 * DESCRIPTION : 暴露给 QML 的桥对象，承担两类弹窗的交互回流：
 *               确认型 choose(index) / 表单型 submit({key:value})。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef QTM_QML_DIALOG_BRIDGE_H
#define QTM_QML_DIALOG_BRIDGE_H

#include "boot.hpp"

#include <QDialog>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QWindow>

class QmlDialogBridge : public QObject {
  Q_OBJECT

public:
  explicit QmlDialogBridge (QDialog* host) : QObject (host), m_host (host) {
    ASSERT (host != NULL, "QmlDialogBridge expects a valid QDialog host");
  }

  /**
   * @brief 确认型弹窗：以按钮下标结束宿主模态 QDialog。
   * @param index 按钮下标，>= 0 表示点按钮；< 0（如 choose(-1)）表示取消。
   */
  Q_INVOKABLE void choose (int index) { m_host->done (index); }

  /**
   * @brief 拖动无边框窗口，委托给底层 QWindow 的系统级移动（startSystemMove）。
   *
   * 由 QML 背景区域的 DragHandler 在拖拽开始时调用。
   */
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ()) {
      m_host->windowHandle ()->startSystemMove ();
    }
  }

  /**
   * @brief 表单型弹窗：暂存整张表单的最终值并以 Accepted 结束宿主 QDialog。
   * @param values {key: value, ...}，value 均为 string（见 QTMQmlDialog.hpp 的
   *   value 约定）。cpp 侧随后用 results() 取出，遍历构造返回 tree。
   */
  Q_INVOKABLE void submit (QVariantMap values) {
    m_results= values;
    m_host->done (QDialog::Accepted);
  }

  /**
   * @return submit() 暂存的结果；Cancel / 关闭时为空 map。
   */
  const QVariantMap& results () const { return m_results; }

  /**
   * @brief live=true 字段的实时回写：QML 控件 onChange 时 emit，cpp 在槽里走
   *        glue 调 scm setter。
   * @note TODO 首案不启用（全 live=false）。高频控件（color picker / SpinBox）
   *   需在 QML 侧 debounce，避免压垮主线程。
   */
signals:
  void fieldChanged (const QString& key, const QString& value);

private:
  QDialog*    m_host;
  QVariantMap m_results;
};

#endif // defined QTM_QML_DIALOG_BRIDGE_H
