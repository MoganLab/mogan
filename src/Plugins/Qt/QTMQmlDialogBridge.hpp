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

  // 确认型弹窗：在 QML 中以 closeBridge.choose(<按钮下标>) 调用，下标 >= 0 表示
  // 点按钮；下标 < 0（或 choose(-1)）表示取消。
  Q_INVOKABLE void choose (int index) { m_host->done (index); }

  // 拖动无边框窗口：由 QML 背景区域的 DragHandler 在拖拽开始时调用，
  // 委托给底层 QWindow 的系统级移动（startSystemMove）。
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ()) {
      m_host->windowHandle ()->startSystemMove ();
    }
  }

  // 表单型弹窗：QML 点 OK 时以 closeBridge.submit({key: value, ...}) 调用，
  // 暂存整张表单的最终值（value 均为 string，见 QTMQmlDialog.hpp 的 value
  // 约定） 并以 Accepted 结束宿主模态 QDialog，让 exec() 返回。cpp 侧随后用
  // results() 取出，遍历构造返回 tree (result (key value) ...)。
  Q_INVOKABLE void submit (QVariantMap values) {
    m_results= values;
    m_host->done (QDialog::Accepted);
  }

  // 取 submit() 暂存的结果（Cancel / 关闭时为空 map）。
  const QVariantMap& results () const { return m_results; }

  // live=true 字段的实时回写：QML 控件 onChange 时 emit，cpp 在槽里收到后走
  // glue 调 scm setter。首案不启用（全 live=false）。注意：高频控件（color
  // picker / SpinBox）需在 QML 侧 debounce，避免压垮主线程。
signals:
  void fieldChanged (const QString& key, const QString& value);

private:
  QDialog*    m_host;
  QVariantMap m_results;
};

#endif // defined QTM_QML_DIALOG_BRIDGE_H
