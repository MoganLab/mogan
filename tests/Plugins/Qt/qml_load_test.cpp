/******************************************************************************
 * MODULE     : qml_load_test.cpp
 * DESCRIPTION: 加载真实 QML 弹窗文档，断言 setSource 后 status()==Ready。
 *              安全网：改造成品弹窗（ConfirmClose / ConfirmQuestion /
 *              ConfirmRestart / FormDialog / FontSelector / ParagraphFormat /
 *              Statistics / Version / Preferences / UpdaterProgress）与 atoms/
 *              原子板块后，确保 QML
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

#include "Qt/QTMQmlDialog.hpp"       // cpp_version_dialog
#include "Qt/QTMQmlDialogBridge.hpp" // QmlDialogEscFilter
#include "tree_helper.hpp"           // TUPLE / make_tree_label / get_label

#include <QApplication>
#include <QDialog>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QObject>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest/QtTest>
#include <functional>

// closeBridge 占位：加载测试不点按钮，invokable 桩避免 QML 调用时 TypeError。
class StubBridge : public QObject {
  Q_OBJECT
public:
  explicit StubBridge (QObject* p= nullptr) : QObject (p) {}
  int              cancelCount= 0;
  int              submitCount= 0;
  QVariantMap      lastSubmitted;
  Q_INVOKABLE void choose (int) {}
  Q_INVOKABLE void cancel () { ++cancelCount; }
  Q_INVOKABLE void submit (const QVariantMap& m) {
    ++submitCount;
    lastSubmitted= m;
  }
  Q_INVOKABLE void    startMove () {}
  Q_INVOKABLE QString pickSaveFile (const QString&, const QString&) {
    return QString ();
  }
  Q_INVOKABLE QString browse (const QString&) { return QString (); }
};

// ColorPicker 的 colorBridge 占位：不真取色（pickScreenColor 为 no-op）。
class ColorStubBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (bool canPickScreen READ canPickScreen CONSTANT)
public:
  explicit ColorStubBridge (QObject* p= nullptr) : QObject (p) {}
  bool             canPickScreen () const { return true; }
  Q_INVOKABLE void pickScreenColor () {}
signals:
  void screenColorPicked (const QString& hex);
  void screenPickUnavailable ();
};

class VersionStubBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QString title READ title CONSTANT)
  Q_PROPERTY (QStringList lines READ lines CONSTANT)
  Q_PROPERTY (QStringList buttonLabels READ buttonLabels CONSTANT)
  Q_PROPERTY (bool primaryEnabled READ primaryEnabled CONSTANT)

public:
  explicit VersionStubBridge (QObject* p= nullptr) : QObject (p) {}
  QString     title () const { return QString ("Version"); }
  QStringList lines () const { return m_lines; }
  QStringList buttonLabels () const { return {"OK"}; }
  bool        primaryEnabled () const { return m_primaryEnabled; }
  void        setLines (const QStringList& lines) { m_lines= lines; }
  void        setPrimaryEnabled (bool v) { m_primaryEnabled= v; }

  Q_INVOKABLE void confirm () {}

private:
  QStringList m_lines{"You are using v2026.2.6.",
                      "The latest stable version is v2026.2.6."};
  bool        m_primaryEnabled{true};
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

// Preferences bridge 占位：加载阶段 QML 顶层调 prefBridge.meta()
// 一次性拉字段树。 meta 返回一棵覆盖 group / layout(two-col) / column /
// combo+toggle+info 的最小树， 确保 activeSections / fieldDelegate
// 的所有分支都能实例化（不验证交互语义）。
class PrefStubBridge : public QObject {
  Q_OBJECT
public:
  explicit PrefStubBridge (QObject* p= nullptr) : QObject (p) {}
  static QVariantMap field (const QString& kind, const QString& key,
                            const QString& label) {
    QVariantMap f;
    f["kind"]     = kind;
    f["key"]      = key;
    f["label"]    = label;
    f["value"]    = QString ("default");
    f["options"]  = QStringList ();
    f["optionsTr"]= QStringList ();
    f["editable"] = false;
    return f;
  }
  Q_INVOKABLE QVariantMap meta () {
    QVariantList tabs;
    // general：单列 combo + info（测 group 标题 / info row / single 区段 +
    // action 按钮）。
    QVariantList gf;
    QVariantMap  g0   = field ("combo", "g1", "Look and feel");
    g0["group"]       = QString ("General");
    QVariantMap gb    = field ("combo", "autobackup", "Auto backup");
    gb["buttonLabel"] = QString ("Open backup");
    gb["buttonAction"]= QString ("open-auto-backup-location");
    QVariantMap gi    = field ("info", "ginfo", "Last check");
    gi["value"]       = QString ("Never");
    gf << g0 << field ("combo", "g2", "Language") << gb << gi;
    QVariantMap gtab;
    gtab["key"]   = QString ("general");
    gtab["label"] = QString ("General");
    gtab["fields"]= gf;
    tabs << gtab;
    // keyboard：单列 combo 段 + two-col IR 段（测 layout 区段切分 / 双栏）。
    QVariantList kf;
    QVariantMap  k0= field ("combo", "k1", "Space bar");
    k0["group"]    = QString ("Keyboard behavior");
    kf << k0;
    QVariantList irL, irR;
    QVariantMap  irL0= field ("combo", "ir-left", "Left");
    irL0["group"]    = QString ("Remote controllers");
    irL0["groupSpan"]= true;
    irL0["layout"]   = QString ("two-col");
    irL0["column"]   = 0;
    irL0["editable"] = true;
    QVariantMap irR0 = irL0;
    irR0["key"]      = QString ("ir-center");
    irR0["label"]    = QString ("Center");
    irR0["group"]    = QString ();
    irR0["groupSpan"]= false;
    irR0["column"]   = 1;
    kf << irL0 << irR0;
    QVariantMap ktab;
    ktab["key"]   = QString ("keyboard");
    ktab["label"] = QString ("Keyboard");
    ktab["fields"]= kf;
    tabs << ktab;
    // convert：带子 tab（测 sub-tab 渲染分支）。
    QVariantList cf;
    QVariantMap  c0= field ("toggle", "html-css", "Use CSS");
    c0["group"]    = QString ("TeXmacs to Html");
    cf << c0;
    QVariantMap sub;
    sub["key"]   = QString ("html");
    sub["label"] = QString ("Html");
    sub["fields"]= cf;
    QVariantList subs;
    subs << sub;
    QVariantMap ctab;
    ctab["key"]    = QString ("convert");
    ctab["label"]  = QString ("Convert");
    ctab["fields"] = QVariantList ();
    ctab["subTabs"]= subs;
    tabs << ctab;
    QVariantMap root;
    root["tabs"]= tabs;
    return root;
  }
  Q_INVOKABLE QString submit (const QVariantMap&) {
    return QString ("applied");
  }
  Q_INVOKABLE void cancel () {}
  Q_INVOKABLE void startMove () {}
  Q_INVOKABLE void callAction (const QString&) {}
};

class PageNumberStubBridge : public QObject {
  Q_OBJECT
public:
  explicit PageNumberStubBridge (QObject* p= nullptr) : QObject (p) {}
  Q_INVOKABLE QVariantMap meta () {
    QVariantMap m;
    m["total"] = 10;
    m["rules"] = QVariantList ();
    m["labels"]= QVariantMap ();
    return m;
  }
  Q_INVOKABLE void    submit (const QVariantList&) {}
  Q_INVOKABLE QString formatNumber (int n, const QString&) {
    return QString::number (n);
  }
};

class TestQmlLoad : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_confirm_close_loads ();
  void test_confirm_question_loads ();
  void test_confirm_restart_loads ();
  void test_form_dialog_loads ();
  void test_search_recent_loads ();
  void test_add_package_loads ();
  void test_font_selector_loads ();
  void test_paragraph_format_loads ();
  void test_preferences_loads ();
  void test_page_number_loads ();
  void test_version_loads ();
  void test_version_dialog_focuses_on_open ();
  void test_version_escape_cancels ();
  void test_version_escape_fallback_without_focus ();
  void test_version_long_line_wraps ();
  void test_statistics_loads ();
  void test_print_to_file_loads ();
  void test_export_pdf_loads ();
  void test_export_pdf_home_path_display ();
  void test_export_pdf_path_utf8_roundtrip ();
  void test_updater_progress_loads ();
  void test_color_picker_loads ();
  void test_bibliography_loads ();
  void test_ai_actions_bar_loads ();
  void test_ai_actions_bar_hover ();
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
  qw->rootContext ()->setContextProperty ("homePath", QDir::homePath ());
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl (qrcUrl));
  return qw->status ();
}

// 共用：构造 ExportPdf 对话框（closeBridge/browseBridge/formFields 等最小注入）
// 并加载 qml，返回 QQuickWidget 供属性断言；宿主 QDialog 由调用方持有。
// homePath 非空时注入（家目录缩短为 ~/ 的用例）。
static QQuickWidget*
load_export_pdf_dialog (QDialog& host, const QVariantList& fields,
                        const QString& homePath= QString ()) {
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* close = new StubBridge (qw);
  StubBridge* browse= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("browseBridge", browse);
  if (!homePath.isEmpty ())
    qw->rootContext ()->setContextProperty ("homePath", homePath);
  qw->rootContext ()->setContextProperty ("formFields", fields);
  QStringList buttons;
  buttons << "Export"
          << "Cancel";
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->rootContext ()->setContextProperty ("dialogTitle",
                                          QString ("Export as PDF"));
  qw->rootContext ()->setContextProperty ("browseLabel", QString ("Browse"));
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/ExportPdf.qml"));
  return qw;
}

void
TestQmlLoad::test_confirm_close_loads () {
  QCOMPARE (load_qml ("qrc:/qml/ConfirmClose.qml"), QQuickWidget::Ready);
}

void
TestQmlLoad::test_confirm_question_loads () {
  // ConfirmQuestion 复用 ConfirmClose 的 dialogMessage/dialogButtons，多一个
  // dialogPrimary（默认按钮下标）。按钮按显示顺序注入（左「否」右「是」）。
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty (
      "dialogMessage", QString ("PDF导出完成，是否要打开文件？"));
  QStringList buttons;
  buttons << "否" << "是";
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->rootContext ()->setContextProperty ("dialogPrimary", 1);
  qw->setSource (QUrl ("qrc:/qml/ConfirmQuestion.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  // dialogPrimary 透传：QML 侧 primaryIndex 应与注入一致。
  QCOMPARE (qw->rootObject ()->property ("primaryIndex").toInt (), 1);
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
TestQmlLoad::test_search_recent_loads () {
  QStringList buttons;
  buttons << "OK"
          << "Cancel";
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty (
      "searchLabel", QString ("Search words in recent documents:"));
  qw->rootContext ()->setContextProperty ("searchValue", QString ());
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->setSource (QUrl ("qrc:/qml/SearchRecent.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_add_package_loads () {
  QStringList buttons;
  buttons << "OK"
          << "Cancel";
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("packageLabel",
                                          QString ("Add style package:"));
  qw->rootContext ()->setContextProperty ("packageName", QString ());
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->setSource (QUrl ("qrc:/qml/AddPackage.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);

  host.show ();
  (void) QTest::qWaitForWindowExposed (&host);
  host.activateWindow ();
  qw->setFocus ();
  QTRY_VERIFY (qw->quickWindow () && qw->quickWindow ()->activeFocusItem ());
  QTest::keyClicks (qw, "d");
  QCOMPARE (bridge->submitCount, 0);
  QTest::keyClick (qw, Qt::Key_Return);
  QTRY_COMPARE (bridge->submitCount, 1);
  QCOMPARE (bridge->lastSubmitted.value ("package").toString (), QString ("d"));
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
TestQmlLoad::test_preferences_loads () {
  // Preferences 顶层即调 prefBridge.meta() 拉字段树，注入 PrefStubBridge
  // （覆盖 group / two-col layout / column / combo+toggle+info / sub-tab）。
  QDialog         host;
  QQuickWidget*   qw  = new QQuickWidget (&host);
  PrefStubBridge* pref= new PrefStubBridge (qw);
  StubBridge*     base= new StubBridge (qw);
  QStringList     buttons;
  buttons << "OK" << "Cancel";
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("prefBridge", pref);
  qw->rootContext ()->setContextProperty ("closeBridge", base);
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/Preferences.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_page_number_loads () {
  QDialog               host;
  QQuickWidget*         qw  = new QQuickWidget (&host);
  PageNumberStubBridge* pn  = new PageNumberStubBridge (qw);
  StubBridge*           base= new StubBridge (qw);
  QStringList           buttons;
  buttons << "Apply" << "Cancel";
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("pnBridge", pn);
  qw->rootContext ()->setContextProperty ("closeBridge", base);
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/PageNumber.qml"));
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

void
TestQmlLoad::test_print_to_file_loads () {
  // PrintToFile 需 formFields（含 path/number 两型字段）+ dialogButtons +
  // browseLabel + printBridge（Browse 生效用）。注入最小占位，断言能实例化。
  QVariantList fields;
  QVariantMap  f0;
  f0["type"] = QString ("path");
  f0["label"]= QString ("File name:");
  f0["key"]  = QString ("name");
  f0["value"]= QString ("doc.ps");
  fields << f0;
  QVariantMap f1;
  f1["type"] = QString ("number");
  f1["label"]= QString ("First page:");
  f1["key"]  = QString ("first");
  f1["value"]= QString ("1");
  fields << f1;
  QVariantMap f2;
  f2["type"] = QString ("number");
  f2["label"]= QString ("Last page:");
  f2["key"]  = QString ("last");
  f2["value"]= QString ("3");
  fields << f2;
  QStringList buttons;
  buttons << "OK"
          << "Cancel";

  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* close= new StubBridge (qw);
  StubBridge* print= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("printBridge", print);
  qw->rootContext ()->setContextProperty ("formFields", fields);
  qw->rootContext ()->setContextProperty ("dialogButtons", buttons);
  qw->rootContext ()->setContextProperty ("browseLabel", QString ("Browse"));
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/PrintToFile.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_export_pdf_loads () {
  // ExportPdf 需 formFields（toggle 型选项 + path 型目的地）+ dialogButtons +
  // dialogTitle + browseLabel + browseBridge（Browse 生效用）。注入最小占位，
  // 断言能实例化。
  QVariantList fields;
  QVariantMap  f0;
  f0["type"] = QString ("toggle");
  f0["label"]= QString ("Embed source document");
  f0["key"]  = QString ("embed");
  f0["value"]= QString ("false");
  fields << f0;
  QVariantMap f1;
  f1["type"] = QString ("path");
  f1["label"]= QString ("Export to");
  f1["key"]  = QString ("path");
  f1["value"]= QString ("/tmp/1271/untitled.pdf");
  fields << f1;

  QDialog       host;
  QQuickWidget* qw= load_export_pdf_dialog (host, fields);
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  // path 字段透传：QML 侧 pathKey/pathValue 应取到目的地初值。
  QCOMPARE (qw->rootObject ()->property ("pathKey").toString (),
            QString ("path"));
  QCOMPARE (qw->rootObject ()->property ("pathValue").toString (),
            QString ("/tmp/1271/untitled.pdf"));
  QCOMPARE (qw->rootObject ()->property ("displayPath").toString (),
            QString ("/tmp/1271/untitled.pdf"));

  // 1304: 开启「将源文档作为附件嵌入PDF」时，目的地后缀自动变为 .tmu.pdf
  QMetaObject::invokeMethod (qw->rootObject (), "onToggleChanged",
                             Q_ARG (QVariant, QString ("embed")),
                             Q_ARG (QVariant, true));
  QCOMPARE (qw->rootObject ()->property ("pathValue").toString (),
            QString ("/tmp/1271/untitled.tmu.pdf"));
  QCOMPARE (
      qw->rootObject ()->property ("values").toMap ()["embed"].toString (),
      QString ("true"));
  QCOMPARE (qw->rootObject ()->property ("values").toMap ()["path"].toString (),
            QString ("/tmp/1271/untitled.tmu.pdf"));

  // 关闭「将源文档作为附件嵌入PDF」时，目的地后缀自动切回 .pdf
  QMetaObject::invokeMethod (qw->rootObject (), "onToggleChanged",
                             Q_ARG (QVariant, QString ("embed")),
                             Q_ARG (QVariant, false));
  QCOMPARE (qw->rootObject ()->property ("pathValue").toString (),
            QString ("/tmp/1271/untitled.pdf"));
  QCOMPARE (
      qw->rootObject ()->property ("values").toMap ()["embed"].toString (),
      QString ("false"));
  QCOMPARE (qw->rootObject ()->property ("values").toMap ()["path"].toString (),
            QString ("/tmp/1271/untitled.pdf"));

  // adjustPathSuffix 各边界用例
  auto checkSuffix= [&] (const QString& in, bool embed,
                         const QString& expected) {
    QVariant out;
    QMetaObject::invokeMethod (qw->rootObject (), "adjustPathSuffix",
                               Q_RETURN_ARG (QVariant, out),
                               Q_ARG (QVariant, in), Q_ARG (QVariant, embed));
    QCOMPARE (out.toString (), expected);
  };
  checkSuffix ("/tmp/foo.pdf", true, "/tmp/foo.tmu.pdf");
  checkSuffix ("/tmp/foo.tmu.pdf", true, "/tmp/foo.tmu.pdf");
  checkSuffix ("/tmp/foo", true, "/tmp/foo.tmu.pdf");
  checkSuffix ("/tmp/foo.bar.pdf", true, "/tmp/foo.bar.tmu.pdf");
  checkSuffix ("/tmp/foo.tmu.pdf", false, "/tmp/foo.pdf");
  checkSuffix ("/tmp/foo.pdf", false, "/tmp/foo.pdf");
  checkSuffix ("/tmp/foo", false, "/tmp/foo.pdf");
}

void
TestQmlLoad::test_export_pdf_home_path_display () {
  QVariantList fields;
  QVariantMap  f0;
  f0["type"] = QString ("path");
  f0["label"]= QString ("Export to");
  f0["key"]  = QString ("path");
  f0["value"]= QString ("/home/testuser/Documents/LiiiSTEM/demo.pdf");
  fields << f0;

  QDialog       host;
  QQuickWidget* qw=
      load_export_pdf_dialog (host, fields, QString ("/home/testuser"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);

  // 1. 家目录下常规路径缩短为 ~/
  QCOMPARE (qw->rootObject ()->property ("displayPath").toString (),
            QString ("~/Documents/LiiiSTEM/demo.pdf"));
  // 真实值仍为绝对路径
  QCOMPARE (qw->rootObject ()->property ("pathValue").toString (),
            QString ("/home/testuser/Documents/LiiiSTEM/demo.pdf"));

  // 2. formatDisplayPath 直接调用的各种分支测试
  auto checkDisplay= [&] (const QString& p, const QString& home,
                          const QString& expected) {
    QVariant res;
    QMetaObject::invokeMethod (qw->rootObject (), "formatDisplayPath",
                               Q_RETURN_ARG (QVariant, res),
                               Q_ARG (QVariant, p), Q_ARG (QVariant, home));
    QCOMPARE (res.toString (), expected);
  };
  checkDisplay ("/home/testuser", "/home/testuser", "~");
  checkDisplay ("/var/tmp/demo.pdf", "/home/testuser", "/var/tmp/demo.pdf");
  // 家目录带尾部斜杠
  checkDisplay ("/home/testuser/a.pdf", "/home/testuser/", "~/a.pdf");
  // Windows 反斜杠路径与正斜杠家目录匹配
  checkDisplay ("C:\\Users\\testuser\\Documents\\demo.pdf", "C:/Users/testuser",
                "~/Documents/demo.pdf");

  // 3. 通过 setv 模拟 Browse 换路径后 displayPath 自动联动
  QMetaObject::invokeMethod (
      qw->rootObject (), "setv", Q_ARG (QVariant, QString ("path")),
      Q_ARG (QVariant, QString ("/home/testuser/other.pdf")));
  QCOMPARE (qw->rootObject ()->property ("pathValue").toString (),
            QString ("/home/testuser/other.pdf"));
  QCOMPARE (qw->rootObject ()->property ("displayPath").toString (),
            QString ("~/other.pdf"));
}

void
TestQmlLoad::test_export_pdf_path_utf8_roundtrip () {
  // 1271: path 字段值是文件系统路径（scheme 侧为 UTF-8 字节）。往返不得走
  // to_qstring / from_qstring 的 cork 启发式——utf8_to_cork 会把中文名变成
  // <#XXXX> 逃逸串，导出落盘路径无效。走 "ok" 测试钩子验证往返保真。
  qputenv ("MOGAN_TEST_EXPORT_PDF", "ok");
  // scheme 侧等价表单：
  //   (export-pdf-form (path "Export to" "path" "/tmp/导出/演示.pdf"))
  const string chinesePath= string ("/tmp/导出/演示") * ".pdf";
  tree         form (moebius::TUPLE);
  form << tree (moebius::make_tree_label ("path"), tree ("Export to"),
                tree ("path"), tree (chinesePath));
  tree r= cpp_export_pdf_dialog (form);
  qunsetenv ("MOGAN_TEST_EXPORT_PDF");

  QVERIFY (is_compound (r));
  QCOMPARE (N (r), 1);
  QVERIFY (get_label (r[0][0]) == string ("path"));
  QVERIFY (get_label (r[0][1]) == chinesePath);
}

void
TestQmlLoad::test_updater_progress_loads () {
  // UpdaterProgress 只需 dialogMessage（closeBridge/dpScale/isDark
  // 为共用注入）； 无专用 bridge、无按钮（下载不可中断，ESC 经 DialogShell
  // onCancel 覆盖为 no-op 禁用——加载层面只验证文档实例化，含 RotationAnimation
  // 转圈）。
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty (
      "dialogMessage", QString ("Downloading the update..."));
  qw->setSource (QUrl ("qrc:/qml/UpdaterProgress.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_version_loads () {
  QDialog            host;
  QQuickWidget*      qw    = new QQuickWidget (&host);
  StubBridge*        close = new StubBridge (qw);
  VersionStubBridge* bridge= new VersionStubBridge (qw);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("versionBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/Version.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  QCOMPARE (qw->rootObject ()->implicitWidth (), 560.0);
  QCOMPARE (qw->rootObject ()->implicitHeight (), 220.0);
  QQuickItem* messageLines=
      qw->rootObject ()->findChild<QQuickItem*> ("versionMessageLines");
  QVERIFY (messageLines);
  int messageLineCount= 0;
  for (QQuickItem* item : messageLines->childItems ())
    if (item->objectName () == "versionMessageLine") ++messageLineCount;
  QCOMPARE (messageLineCount, bridge->lines ().size ());
  QCOMPARE (qw->rootObject ()->property ("primaryEnabled").toBool (), true);
}

void
TestQmlLoad::test_version_dialog_focuses_on_open () {
  // 0927：必须经真实 run_qml_dialog + exec() 路径验证。弹窗显示后检查 QML
  // 场景焦点并发 ESC，确认正常 QML 取消链路生效。
  // CI（offscreen、高负载）下焦点事件可能晚到，固定时刻检查一次会误报，
  // 故轮询等待焦点到达后再发 ESC。
  bool   sawDialog = false;
  bool   hadFocus  = false;
  bool   sentEscape= false;
  QTimer focusPoll;
  focusPoll.setInterval (50);
  QObject::connect (&focusPoll, &QTimer::timeout, [&] () {
    for (QWidget* topLevel : QApplication::topLevelWidgets ())
      if (topLevel->objectName () == "QTMQmlDialog") {
        sawDialog       = true;
        QQuickWidget* qw= topLevel->findChild<QQuickWidget*> ();
        if (qw && qw->rootObject () && qw->rootObject ()->hasActiveFocus ()) {
          hadFocus= true;
          QTest::keyClick (qw, Qt::Key_Escape);
          sentEscape= true;
          focusPoll.stop ();
        }
        return;
      }
  });
  // 运行时加载失败或焦点始终未到达，都不能让测试遗留一个阻塞模态窗口。
  QTimer::singleShot (8000, [] () {
    for (QWidget* topLevel : QApplication::topLevelWidgets ())
      if (topLevel->objectName () == "QTMQmlDialog")
        static_cast<QDialog*> (topLevel)->reject ();
  });
  focusPoll.start ();
  QVERIFY (!cpp_version_dialog ("Version", "Version information"));
  focusPoll.stop ();
  QVERIFY (sawDialog);
  QVERIFY (hadFocus);
  QVERIFY (sentEscape);
}

void
TestQmlLoad::test_version_escape_cancels () {
  QDialog            host;
  QQuickWidget*      qw    = new QQuickWidget (&host);
  StubBridge*        close = new StubBridge (qw);
  VersionStubBridge* bridge= new VersionStubBridge (qw);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("versionBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/Version.qml"));
  host.show ();

  QTRY_VERIFY (qw->rootObject ()->hasActiveFocus ());
  QTest::keyClick (qw, Qt::Key_Escape);
  QCOMPARE (close->cancelCount, 1);
}

void
TestQmlLoad::test_version_escape_fallback_without_focus () {
  // 0925/0927：QML 场景无 activeFocusItem 时 ESC 会被 QQuickWidget 静默吞掉
  // （不投递、不传播给 QDialog::reject），引擎侧的 QmlDialogEscFilter 须兜底
  // reject 宿主弹窗，且不走 QML cancel。Windows 下 ESC 常先到达宿主 QDialog，
  // 故过滤器装在宿主上并向宿主发 ESC，覆盖该场景。
  QDialog            host;
  QQuickWidget*      qw    = new QQuickWidget (&host);
  StubBridge*        close = new StubBridge (qw);
  VersionStubBridge* bridge= new VersionStubBridge (qw);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("versionBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/Version.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  // 过滤器装在宿主 QDialog 上：Windows 下 ESC 常先到达宿主而非 QQuickWidget
  host.installEventFilter (new QmlDialogEscFilter (&host, qw, &host));
  host.show ();
  // 强制 QML 场景无焦点项，模拟 ESC 被吞的真实缺陷态
  qw->rootObject ()->setFocus (false);
  QTRY_VERIFY (!qw->rootObject ()->hasActiveFocus ());
  QSignalSpy rejectedSpy (&host, &QDialog::rejected);
  // 直接向宿主发送 ESC，覆盖 Windows 焦点落在宿主的场景
  QTest::keyClick (&host, Qt::Key_Escape);
  QTRY_COMPARE (rejectedSpy.count (), 1);
  QVERIFY (!host.isVisible ());
  QCOMPARE (close->cancelCount, 0);
}

void
TestQmlLoad::test_version_long_line_wraps () {
  // 单行远超弹窗宽度：应自动换行（行高成倍）、弹窗 implicitHeight 超出最小高度
  QDialog            host;
  QQuickWidget*      qw    = new QQuickWidget (&host);
  StubBridge*        close = new StubBridge (qw);
  VersionStubBridge* bridge= new VersionStubBridge (qw);
  bridge->setLines ({QString (400, 'x')});
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->rootContext ()->setContextProperty ("closeBridge", close);
  qw->rootContext ()->setContextProperty ("versionBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->setSource (QUrl ("qrc:/qml/Version.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  // 隐藏状态下 QQuickWidget 不向 root 同步尺寸，show 后视图定宽、root 跟随
  host.show ();
  qw->resize (560, 480);
  QTRY_VERIFY (qw->rootObject ()->implicitHeight () > 220.0);
  QQuickItem* messageLines=
      qw->rootObject ()->findChild<QQuickItem*> ("versionMessageLines");
  QVERIFY (messageLines);
  QQuickItem* line= nullptr;
  for (QQuickItem* item : messageLines->childItems ())
    if (item->objectName () == "versionMessageLine") {
      line= item;
      break;
    }
  QVERIFY (line);
  QTRY_VERIFY (line->height () > 40.0); // 单行 14*1.35≈19，换行后显著更高
}

void
TestQmlLoad::test_color_picker_loads () {
  // ColorPicker 顶层读取 pickerTitleProp / proposalsProp / pickPatternProp /
  // initialColorProp / customColorsProp / labelsProp / dialogButtonsProp /
  // colorBridge，注入最小占位保证文档实例化。
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge*      bridge  = new StubBridge (qw);
  ColorStubBridge* cpBridge= new ColorStubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("colorBridge", cpBridge);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("pickerTitleProp",
                                          QString ("Choose color"));
  qw->rootContext ()->setContextProperty (
      "proposalsProp", QStringList ({"#ff0000", "#00ff00", "#0000ff"}));
  qw->rootContext ()->setContextProperty ("pickPatternProp", false);
  qw->rootContext ()->setContextProperty ("initialColorProp",
                                          QString ("#ff0000"));
  qw->rootContext ()->setContextProperty ("customColorsProp",
                                          QStringList ({"#123456"}));
  QVariantMap labels;
  labels["basicColors"]    = QString ("Basic colors");
  labels["customColors"]   = QString ("Custom colors");
  labels["addToCustom"]    = QString ("Add to custom colors");
  labels["pickScreenColor"]= QString ("Pick screen color");
  qw->rootContext ()->setContextProperty ("labelsProp", labels);
  qw->rootContext ()->setContextProperty ("dialogButtonsProp",
                                          QStringList ({"OK", "Cancel"}));
  qw->setSource (QUrl ("qrc:/qml/ColorPicker.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

// AiActionsBar 用例共用：按生产环境注入主题（dpScale/isDark，Theme 单例
// 读取）与三个按钮文案占位，加载 qrc 内的操作栏
static QQuickWidget*
make_ai_actions_bar (QWidget* host) {
  QQuickWidget* qw= new QQuickWidget (host);
  qw->setResizeMode (QQuickWidget::SizeViewToRootObject);
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);
  qw->rootContext ()->setContextProperty ("labelTranslate",
                                          QString ("Translate"));
  qw->rootContext ()->setContextProperty ("labelPolish", QString ("Polish"));
  qw->rootContext ()->setContextProperty ("labelChat", QString ("Chat"));
  qw->setSource (QUrl ("qrc:/qml/AiActionsBar.qml"));
  return qw;
}

class StubBibBridge : public QObject {
  Q_OBJECT
public:
  explicit StubBibBridge (QObject* p= nullptr) : QObject (p) {}
  Q_INVOKABLE QString     browse (const QString&) { return QString (); }
  Q_INVOKABLE QVariantMap requestPreview (const QString&, const QString&) {
    QVariantMap m;
    m["status"] = QString ("empty");
    m["hint"]   = QString ();
    m["preview"]= QString ();
    return m;
  }
  Q_INVOKABLE QString toRelativePath (const QString& p) { return p; }
};

void
TestQmlLoad::test_bibliography_loads () {
  QDialog       host;
  QQuickWidget* qw= new QQuickWidget (&host);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  StubBridge* bridge= new StubBridge (qw);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("bibBridge", new StubBibBridge (qw));
  qw->rootContext ()->setContextProperty ("dialogTitle",
                                          QString ("Insert bibliography"));
  qw->rootContext ()->setContextProperty ("dialogPrompt", QString (""));
  qw->rootContext ()->setContextProperty ("dialogButtons",
                                          QStringList ({"Insert", "Cancel"}));
  qw->rootContext ()->setContextProperty ("fileLabel", QString ("File:"));
  qw->rootContext ()->setContextProperty ("browseLabel", QString ("Browse"));
  qw->rootContext ()->setContextProperty ("updateLabel",
                                          QString ("Update buffer:"));
  qw->rootContext ()->setContextProperty ("styleLabel", QString ("Style:"));
  qw->rootContext ()->setContextProperty ("initialFile", QString (""));
  qw->rootContext ()->setContextProperty ("initialStyle", QString ("tm-plain"));
  qw->rootContext ()->setContextProperty ("initialUpdate", true);
  qw->rootContext ()->setContextProperty (
      "styleOptions", QStringList ({"tm-plain", "tm-alpha"}));
  qw->rootContext ()->setContextProperty ("dpScale", 1.0);
  qw->rootContext ()->setContextProperty ("isDark", false);

  qw->setSource (QUrl ("qrc:/qml/Bibliography.qml"));
  QCOMPARE (qw->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_ai_actions_bar_loads () {
  // AiActionsBar 是 QTMAiTranslatePopup 内嵌的非模态操作栏（无 closeBridge），
  // 断言能实例化。
  QDialog host;
  QCOMPARE (make_ai_actions_bar (&host)->status (), QQuickWidget::Ready);
}

void
TestQmlLoad::test_ai_actions_bar_hover () {
  // 无按键 mouseMove 必须驱动 QML hover：Quick 由转发到离屏窗口的 move 事件
  // 合成 hover（MouseArea.containsMouse 翻转）。若此处失败说明 QQuickWidget 的
  // hover 链路本身断了，而不是宿主窗口的事件投递问题。
  QDialog       host;
  QQuickWidget* qw= make_ai_actions_bar (&host);
  QCOMPARE (qw->status (), QQuickWidget::Ready);
  host.show ();

  // Repeater delegate 只挂视觉父子（childItems），QObject 父链不保证在根下，
  // 需沿视觉树递归找
  std::function<QList<QQuickItem*> (QQuickItem*)> collect=
      [&] (QQuickItem* item) -> QList<QQuickItem*> {
    QList<QQuickItem*> out;
    if (item->objectName () == "aiActionHoverArea") out << item;
    for (QQuickItem* child : item->childItems ())
      out << collect (child);
    return out;
  };
  QList<QQuickItem*> areas= collect (qw->rootObject ());
  QCOMPARE (areas.size (), 3);
  QQuickItem* ma= areas.first ();
  // SizeViewToRootObject 下 scene 坐标 == widget 坐标
  QPointF center=
      ma->mapToScene (QPointF (ma->width () / 2, ma->height () / 2));

  // 无按键 move 必须直接驱动 QML hover（QQuickWidget 把 move 转发离屏窗口，
  // Quick 侧合成 hover）。若此处失败说明按钮纯悬浮点亮链路断裂
  auto sendMove= [qw] (const QPointF& p) {
    QMouseEvent me (QEvent::MouseMove, p, p,
                    QPointF (qw->mapToGlobal (p.toPoint ())), Qt::NoButton,
                    Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent (qw, &me);
  };
  sendMove (center); // 悬停到按钮点亮
  QVERIFY (ma->property ("containsMouse").toBool ());
  sendMove (QPointF (1, 1)); // 空白处熄灭
  QVERIFY (!ma->property ("containsMouse").toBool ());
  sendMove (center); // 移回按钮重新点亮
  QVERIFY (ma->property ("containsMouse").toBool ());
}

QTEST_MAIN (TestQmlLoad)
#include "qml_load_test.moc"
