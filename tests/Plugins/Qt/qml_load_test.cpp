/******************************************************************************
 * MODULE     : qml_load_test.cpp
 * DESCRIPTION: 加载真实 QML 弹窗文档，断言 setSource 后 status()==Ready。
 *              devel/2027.md Phase 0 安全网：改造 ConfirmClose / FormDialog /
 *              各原子板块后，确保 QML 仍能解析、实例化。不验证交互（需可见窗口
 *              + 人工），只验证「文档加载不失败」——import 缺失、语法错、context
 *              property 误用的第一道关。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "base.hpp"

#include <QDialog>
#include <QObject>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest/QtTest>

// closeBridge 占位：加载测试不点按钮，invokable 桩避免 QML 调用时 TypeError。
class StubBridge : public QObject {
  Q_OBJECT
public:
  explicit StubBridge (QObject* p= nullptr) : QObject (p) {}
  Q_INVOKABLE void choose (int) {}
  Q_INVOKABLE void submit (const QVariantMap&) {}
  Q_INVOKABLE void startMove () {}
};

class TestQmlLoad : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_confirm_close_loads ();
  void test_form_dialog_loads ();
  void test_font_selector_loads (); // Phase 3 起填充
};

// 共用：构造带 closeBridge/dpScale/isDark 的 QQuickWidget，加载给定 qrc url。
// 返回 status；非 Ready 时把 warnings 打到测试日志。
static QQuickWidget::Status
load_qml (const QString& qrcUrl) {
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl (qrcUrl));
  return qw->status ();
}

void
TestQmlLoad::test_confirm_close_loads () {
  QCOMPARE (load_qml ("qrc:/qml/ConfirmClose.qml"), QQuickWidget::Ready);
}

void
TestQmlLoad::test_form_dialog_loads () {
  // FormDialog 还需 formFields/dialogButtons；注入最小占位（空表 + 默认按钮）。
  QVariantList fields;
  QStringList  buttons;
  buttons << "OK"
          << "Cancel";
  // 重新走一遍，多注入两个 context property。
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("formFields", fields);
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->setSource (QUrl ("qrc:/qml/FormDialog.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_font_selector_loads () {
  // FontSelector.qml 在 Phase 3 创建后启用此用例。
  QSKIP ("FontSelector.qml not yet created (Phase 3)");
}

#ifdef QTTEXMACS
QTEST_MAIN (TestQmlLoad)
#else
int
main () {
  return 0;
}
#endif
#include "qml_load_test.moc"
