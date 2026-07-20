/******************************************************************************
 * MODULE     : qml_load_test.cpp
 * DESCRIPTION: 加载真实 QML 弹窗文档，断言 setSource 后 status()==Ready。
 *              安全网：改造四个成品弹窗（ConfirmClose / FormDialog /
 *              FontSelector / ParagraphFormat）与 atoms/ 原子板块后，确保 QML
 *              仍能解析、实例化。不验证交互（需可见窗口 + 人工），只验证
 *              「文档加载不失败」——import 缺失、语法错、id 悬空、context
 *              property 误用的第一道关。新增弹窗在此加一个 test_xxx_loads
 *              用例即可（注入对应 bridge 桩）。
 * COPYRIGHT   : (C) 2026 Yuki Lu
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
  Q_INVOKABLE void cancel () {}
  Q_INVOKABLE void submit (const QVariantMap&) {}
  Q_INVOKABLE void startMove () {}
};

// live 弹窗（FontSelector / ParagraphFormat）bridge 占位：加载阶段 QML 顶层会调
// 一批 uiLabels/meta/currentXxx/requestXxx 取初始值，桩统一返回空（空串/空
// list/空 map）， 仅保证文档能实例化、不验证交互语义。
class StubLiveBridge : public QObject {
  Q_OBJECT
public:
  explicit StubLiveBridge (QObject* p= nullptr) : QObject (p) {}
  Q_INVOKABLE QString      requestPreview () { return QString (); }
  Q_INVOKABLE QVariantMap  uiLabels () { return QVariantMap (); }
  Q_INVOKABLE QString      currentFamily () { return QString (); }
  Q_INVOKABLE QString      currentStyle () { return QString (); }
  Q_INVOKABLE QString      currentSize () { return QString (); }
  Q_INVOKABLE QVariantList requestFamilies () { return QVariantList (); }
  Q_INVOKABLE QVariantList requestStyles (const QString&) {
    return QVariantList ();
  }
  Q_INVOKABLE QVariantList requestSizes () { return QVariantList (); }
  Q_INVOKABLE QVariantList sampleKinds () { return QVariantList (); }
  Q_INVOKABLE QString      currentSampleKind () { return QString (); }
  Q_INVOKABLE QVariantMap  setFamily (const QString&) { return QVariantMap (); }
  Q_INVOKABLE QVariantMap  setStyle (const QString&) { return QVariantMap (); }
  Q_INVOKABLE QVariantMap  setSize (const QString&) { return QVariantMap (); }
  Q_INVOKABLE QVariantMap  setSampleKind (const QString&) {
    return QVariantMap ();
  }
  Q_INVOKABLE QVariantList filterMeta () { return QVariantList (); }
  Q_INVOKABLE QVariantList customizeMeta () { return QVariantList (); }
  Q_INVOKABLE QVariantMap  setFilter (const QString&, const QString&) {
    return QVariantMap ();
  }
  Q_INVOKABLE QVariantMap setCustomize (const QString&, const QString&) {
    return QVariantMap ();
  }
  Q_INVOKABLE void importFont () {}
  Q_INVOKABLE void reset () {}
  Q_INVOKABLE void submit () {}
  Q_INVOKABLE void cancel () {}
  // ParagraphFormat
  Q_INVOKABLE QVariantList basicMeta () { return QVariantList (); }
  Q_INVOKABLE QVariantList advancedMeta () { return QVariantList (); }
  Q_INVOKABLE void         setPara (const QString&, const QString&) {}
};

class TestQmlLoad : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_confirm_close_loads ();
  void test_confirm_restart_loads ();
  void test_form_dialog_loads ();
  void test_font_selector_loads ();
  void test_paragraph_format_loads ();
  void test_statistics_loads ();
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
TestQmlLoad::test_confirm_restart_loads () {
  // ConfirmRestart 复用 ConfirmClose 的 dialogMessage/dialogButtons，多一个
  // dialogTitle。 dialogTitle 仅作为标题 Text 显示，dialogMessage
  // 作正文。三按钮文案注入。
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("dialogTitle",
                                          QString ("Switch interface theme"));
  qw->rootContext ()->setContextProperty (
      "dialogMessage",
      QString (
          "This change requires restarting Mogan STEM to take full effect."));
  QStringList buttons;
  buttons << "Restart"
          << "Later"
          << "Cancel";
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->setSource (QUrl ("qrc:/qml/ConfirmRestart.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
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
  // FontSelector 顶层即调 fontBridge 一批方法取初始值，注入 StubLiveBridge。
  QDialog         host;
  QQuickWidget*   qw  = new QQuickWidget (&host);
  StubLiveBridge* live= new StubLiveBridge (qw);
  StubBridge*     base= new StubBridge (qw);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", base);
  qw->rootContext ()->setContextProperty ("fontBridge", live);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/FontSelector.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_paragraph_format_loads () {
  // ParagraphFormat 顶层调 paraBridge.uiLabels/basicMeta/advancedMeta
  // 取初始值。
  QDialog         host;
  QQuickWidget*   qw  = new QQuickWidget (&host);
  StubLiveBridge* live= new StubLiveBridge (qw);
  StubBridge*     base= new StubBridge (qw);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", base);
  qw->rootContext ()->setContextProperty ("paraBridge", live);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/ParagraphFormat.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_statistics_loads () {
  QVariantList model;
  QVariantMap  row;
  row["label"]= QString ("Page count");
  row["value"]= QString ("1");
  model << row;

  QStringList buttons;
  buttons << "Close";

  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("statsTitle",
                                          QString ("Document statistics"));
  qw->rootContext ()->setContextProperty ("statsItems", model);
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->setSource (QUrl ("qrc:/qml/Statistics.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
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
