/******************************************************************************
 * MODULE      : QTMQmlDialogBridge.hpp
 * DESCRIPTION : 暴露给 QML 的桥对象，让任意 QML 弹窗里的按钮点击能以可区分
 *               的结果码结束宿主模态 QDialog。按钮用下标（0 起）标识，由
 *               QML 以 closeBridge.choose(index) 调用。单独成头（而非塞进
 *               QTMQmlDialog.hpp），使不含 Qt 头的底座头保持精简，而这个
 *               Q_OBJECT 类仍能被 src/Plugins/Qt 下 .hpp 的自动 moc 扫描到。
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
#include <QWindow>

class QmlDialogBridge : public QObject {
  Q_OBJECT

public:
  explicit QmlDialogBridge (QDialog* host) : QObject (host), m_host (host) {
    ASSERT (host != NULL, "QmlDialogBridge expects a valid QDialog host");
  }

  // 在 QML 中以 closeBridge.choose(<按钮下标>) 调用，下标 >= 0 表示点按钮。
  // 下标 < 0（或 choose(-1)）表示取消。
  Q_INVOKABLE void choose (int index) { m_host->done (index); }

  // 拖动无边框窗口：由 QML 背景区域的 DragHandler 在拖拽开始时调用，
  // 委托给底层 QWindow 的系统级移动（startSystemMove）。
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ()) {
      m_host->windowHandle ()->startSystemMove ();
    }
  }

private:
  QDialog* m_host;
};

#endif // defined QTM_QML_DIALOG_BRIDGE_H
