/******************************************************************************
 * MODULE      : QTMQmlDialog.cpp
 * DESCRIPTION : QML 模态对话框底座（见 QTMQmlDialog.hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "QTMQmlDialog.hpp"
#include "QTMQmlDialogBridge.hpp"

#include "analyze.hpp" // occurs
#include "gui.hpp"     // tm_style_sheet
#include "qt_utilities.hpp"
#include "sys_utils.hpp" // lolly: get_env

#include <QApplication>
#include <QDialog>
#include <QQmlContext>
#include <QQuickWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

// QML context property：dialogMessage（正文）、dialogButtons（按钮文案数组）、
// dpScale（DPI 缩放）、isDark（深浅色）。按钮下标从 1 起，0 = 取消（Esc）。

int
qt_show_qml_dialog (string qml_url, string message, array<string> buttons) {
  // 显式激活 moganqml.qrc 资源（Qt 要求，否则 qrc:/ 加载不到）。
  Q_INIT_RESOURCE (moganqml);
  // 无边框 + 透明背景：圆角由 QML Rectangle 自绘（自带抗锯齿）。
  // 所有窗口属性在构造时传入，并在 show/exec 前 setAttribute，避免 macOS
  // 上透明窗口初始化时闪屏。
  QDialog d (QApplication::activeWindow (),
             Qt::FramelessWindowHint | Qt::Dialog);
  d.setAttribute (Qt::WA_TranslucentBackground);
  d.setAttribute (Qt::WA_NativeWindow);
  // 反制 liii.css 里通用 QDialog 规则（background / border / min-size /
  // padding）： QML 已自绘整个背景，QDialog 自身隐藏即可，圆角外不露方块。
  d.setObjectName ("QTMQmlDialog");
  d.setStyleSheet ("QDialog#QTMQmlDialog { background: transparent; "
                   "border: none; min-width: 0; min-height: 0; padding: 0; } "
                   "QDialog#QTMQmlDialog QWidget { background: transparent; }");
  QVBoxLayout* vl= new QVBoxLayout (&d);
  vl->setContentsMargins (0, 0, 0, 0);

  QQuickWidget* qw= new QQuickWidget (&d);
  // SizeRootObjectToView：QML root（用 anchors.fill）跟随 QQuickWidget 尺寸，
  // QQuickWidget 由 layout 拉伸填满 QDialog，避免 root 固定尺寸被挤到左上角。
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  // QQuickWidget 视口透明：用 setClearColor（QQuickWidget 自己的 API），而非
  // WA_TranslucentBackground（那是给 QWidget 用的，对 QQuickWidget 不完全生效，
  // 默认不透明白色 clear color 会盖住 QDialog 的透明，露出方角）。
  qw->setClearColor (Qt::transparent);
  qw->setStyleSheet ("background: transparent;");

  // buttons 是英文 key，翻译后注入 QML。
  QStringList qmlButtons;
  for (int i= 0; i < N (buttons); i++) {
    qmlButtons << qt_translate (buttons[i]);
  }

  // context property 须在 setSource 之前设置。
  // isDark：跟随当前 tm_style_sheet（liii-night / *-dark 视为深色）。
  bool isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  QmlDialogBridge* bridge= new QmlDialogBridge (&d);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dialogMessage",
                                          to_qstring (message));
  qw->rootContext ()->setContextProperty ("dialogButtons", qmlButtons);
  qw->rootContext ()->setContextProperty ("dpScale", DpiUtils::scaleFactor ());
  qw->rootContext ()->setContextProperty ("isDark", isDark);

  qw->setSource (QUrl (to_qstring (qml_url)));
  if (qw->status () != QQuickWidget::Ready) {
    // QML 加载失败：开发期应直接暴露（qrc 路径 / QML 语法错），不兜底。
    return -1;
  }

  vl->addWidget (qw);
  // 固定弹窗尺寸（基准 400x200 × DPI 缩放）。QML root 用 anchors.fill: parent，
  // 自身无固定 implicit 尺寸，QQuickWidget 的 sizeHint 会塌缩成子项最小包围盒；
  // 若不锁定，QDialog::exec() 时 QVBoxLayout 会按该 sizeHint 把窗口（Linux 上
  // 连同 quickwidget 视口）压到约 100x30，内容只渲染出左上角一小块。三重锁定
  // 后视口恒为 400x200，root 经 anchors.fill 拉满。Win/macOS 同义无副作用。
  const int w= DpiUtils::scaled (400);
  const int h= DpiUtils::scaled (200);
  qw->setFixedSize (w, h);
  vl->setSizeConstraint (QLayout::SetFixedSize);
  d.setFixedSize (w, h);

  int result= d.exec ();
  return result > 0 ? result : -1;
}

string
cpp_confirm_close (string message, bool scratch) {
  // 测试钩子：MOGAN_TEST_CONFIRM_CLOSE 设为 Save/Don't save/Cancel 时直接返回。
  string preset= get_env ("MOGAN_TEST_CONFIRM_CLOSE");
  if (preset == "Save" || preset == "Don't save" || preset == "Cancel")
    return preset;
  // 按钮：肯定（保存/另存为）、不保存、取消 → 下标 1/2/3。
  array<string> buttons;
  buttons << string (scratch ? "Save as" : "Save");
  buttons << string ("Don't save");
  buttons << string ("Cancel");
  switch (qt_show_qml_dialog ("qrc:/qml/ConfirmClose.qml", message, buttons)) {
  case 1:
    return "Save";
  case 2:
    return "Don't save";
  default:
    return "Cancel";
  }
}
