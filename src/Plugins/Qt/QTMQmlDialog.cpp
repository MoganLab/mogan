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
#include <QQuickItem>
#include <QQuickWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief 确认型弹窗的通用 QML 模态对话框。
 * @param qml_url QML 文档的 qrc 路径。
 * @param message 已翻译的正文。
 * @param buttons 按钮文案 key（英文，cpp 侧 qt_translate 翻译）。
 * @return 用户点选的按钮下标（≥0）；取消 / X / Esc / 加载失败返回 -1。
 *
 * @details 实现要点：
 * - Qt::Tool + nullptr 父（而非 Qt::Dialog + activeWindow）：后者在 exec() 期间
 *   让 qwindowkitty 把主窗口标签栏 hit-test 误判为 HTBORDER，弹窗后拖动失效。
 * - 透明背景属性须在 show/exec 前设置，避免 macOS 闪屏。
 * - setClearColor（QQuickWidget 专属）而非 WA_TranslucentBackground（对它不完全
 *   生效，默认白色 clear color 会盖住透明、露方角）。
 * - objectName + 样式反制 liii.css 的通用 QDialog 规则，避免圆角外露方块。
 * - 三重锁定尺寸：root 无固有 implicit 尺寸，不锁会被 QVBoxLayout 按 sizeHint
 *   压到约 100x30，只渲染左上角。
 */
int
qt_show_qml_dialog (string qml_url, string message, array<string> buttons) {
  static const bool resourceInitialized= [] () {
    Q_INIT_RESOURCE (moganqml);
    return true;
  }();
  (void) resourceInitialized;
  QDialog d (nullptr, Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
  d.setAttribute (Qt::WA_TranslucentBackground);
  d.setAttribute (Qt::WA_NativeWindow);
  d.setObjectName ("QTMQmlDialog");
  d.setStyleSheet ("QDialog#QTMQmlDialog { background: transparent; "
                   "border: none; min-width: 0; min-height: 0; padding: 0; } "
                   "QDialog#QTMQmlDialog QWidget { background: transparent; }");
  QVBoxLayout* vl= new QVBoxLayout (&d);
  vl->setContentsMargins (0, 0, 0, 0);

  QQuickWidget* qw= new QQuickWidget (&d);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->setClearColor (Qt::transparent);
  qw->setStyleSheet ("background: transparent;");

  QStringList qmlButtons;
  for (int i= 0; i < N (buttons); i++) {
    qmlButtons << qt_translate (buttons[i]);
  }

  // context property 须在 setSource 之前设置。isDark 跟随 tm_style_sheet。
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
  if (qw->status () != QQuickWidget::Ready) return -1;

  vl->addWidget (qw);
  const int w= DpiUtils::scaled (400);
  const int h= DpiUtils::scaled (150);
  qw->setFixedSize (w, h);
  vl->setSizeConstraint (QLayout::SetFixedSize);
  d.setFixedSize (w, h);

  int result= d.exec ();
  return result > 0 ? result : -1;
}

/**
 * @brief 「确认关闭」弹窗的 glue 入口。
 * @param message 已翻译的正文。
 * @param scratch 为真时肯定按钮显示「另存为」。
 * @return "Save" / "Don't save" / "Cancel" 之一。
 *
 * @details 按钮顺序为 肯定（Save / Save as）、Don't save、Cancel，对应
 * qt_show_qml_dialog 返回下标 1/2/3。测试钩子 MOGAN_TEST_CONFIRM_CLOSE 命中时
 * 直接返回不弹窗。
 */
string
cpp_confirm_close (string message, bool scratch) {
  string preset= get_env ("MOGAN_TEST_CONFIRM_CLOSE");
  if (preset == "Save" || preset == "Don't save" || preset == "Cancel")
    return preset;
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

// ---- form 引擎 --------------------------------------------------------------

/**
 * @brief 单个字段节点 tree → QML 可消费的 QVariantMap。
 * @param f 字段节点，形如 (enum <label> <key> (<opt>...) <value> <live?>)。
 * @return 含 type/label/key/options/value/live 的 map；形状不符返回空。
 *
 * label/key/value 纯透传，不做翻译或类型转换（value 在 scm 侧已 string 化）。
 */
static QVariantMap
field_tree_to_qml (tree f) {
  QVariantMap m;
  if (!is_compound (f) || N (f) < 4) return m;
  m["type"] = to_qstring (f->label);
  m["label"]= to_qstring (f[0]->label);
  m["key"]  = to_qstring (f[1]->label);
  if (is_compound (f[2])) {
    QVariantList opts;
    for (int i= 0; i < N (f[2]); i++)
      opts << to_qstring (f[2][i]->label);
    m["options"]= opts;
  }
  m["value"]= to_qstring (f[3]->label);
  m["live"] = (N (f) >= 5 && f[4]->label == "true");
  return m;
}

/**
 * @brief 通用 form 弹窗引擎。
 * @param fields 字段表 tree：(form <field>...)，调用方须 stree->tree 转换。
 * @return 用户点 OK 返回 (tuple (tuple key value) ...)；Cancel / 关闭返回空 tree。
 *
 * @details 实现要点：
 * - 测试钩子 MOGAN_TEST_FORM_DIALOG=ok/cancel 命中时不弹窗，供自动化测试。
 * - QDialog 拼装与 qt_show_qml_dialog 同源（无边框 + 透明，避免 macOS 闪屏与
 *   标签栏 hit-test 误判）。
 * - 尺寸由 QML root 自报（× DPI），动态字段场景需 childrenRect 兜底（首案
 *   字段静态，Ready 后已稳定）。QML 加载失败返回空 tree，开发期直接暴露。
 */
tree
cpp_form_dialog (tree fields) {
  string preset= get_env ("MOGAN_TEST_FORM_DIALOG");
  if (preset == "cancel") return tree (TUPLE);
  if (preset == "ok") {
    tree r (TUPLE);
    if (is_compound (fields)) {
      for (int i= 0; i < N (fields); i++) {
        if (is_compound (fields[i]) && N (fields[i]) >= 4) {
          tree kv (TUPLE);
          kv << fields[i][1] << fields[i][3];
          r << kv;
        }
      }
    }
    return r;
  }
  QVariantList qmlFields;
  if (is_compound (fields)) {
    for (int i= 0; i < N (fields); i++) {
      if (is_compound (fields[i])) {
        QVariantMap m= field_tree_to_qml (fields[i]);
        if (!m.isEmpty ()) qmlFields << m;
      }
    }
  }

  static const bool resourceInitialized= [] () {
    Q_INIT_RESOURCE (moganqml);
    return true;
  }();
  (void) resourceInitialized;

  QDialog d (nullptr, Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
  d.setAttribute (Qt::WA_TranslucentBackground);
  d.setAttribute (Qt::WA_NativeWindow);
  d.setObjectName ("QTMQmlDialog");
  d.setStyleSheet ("QDialog#QTMQmlDialog { background: transparent; "
                   "border: none; min-width: 0; min-height: 0; padding: 0; } "
                   "QDialog#QTMQmlDialog QWidget { background: transparent; }");
  QVBoxLayout* vl= new QVBoxLayout (&d);
  vl->setContentsMargins (0, 0, 0, 0);

  QQuickWidget* qw= new QQuickWidget (&d);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  qw->setClearColor (Qt::transparent);
  qw->setStyleSheet ("background: transparent;");

  bool isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  QmlDialogBridge* bridge= new QmlDialogBridge (&d);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("formFields", qmlFields);
  qw->rootContext ()->setContextProperty ("dpScale", DpiUtils::scaleFactor ());
  qw->rootContext ()->setContextProperty ("isDark", isDark);

  qw->setSource (QUrl ("qrc:/qml/FormDialog.qml"));
  if (qw->status () != QQuickWidget::Ready) return tree (TUPLE);

  vl->addWidget (qw);
  QQuickItem* root= qw->rootObject ();
  int         w= DpiUtils::scaled (int (root ? root->implicitWidth () : 360));
  int         h= DpiUtils::scaled (int (root ? root->implicitHeight () : 200));
  if (w <= 0) w= DpiUtils::scaled (360);
  if (h <= 0) h= DpiUtils::scaled (200);
  qw->setFixedSize (w, h);
  vl->setSizeConstraint (QLayout::SetFixedSize);
  d.setFixedSize (w, h);

  d.exec ();

  tree               r (TUPLE);
  const QVariantMap& res= bridge->results ();
  for (auto it= res.begin (); it != res.end (); ++it) {
    tree kv (TUPLE);
    kv << tree (from_qstring (it.key ()));
    kv << tree (from_qstring (it.value ().toString ()));
    r << kv;
  }
  return r;
}
