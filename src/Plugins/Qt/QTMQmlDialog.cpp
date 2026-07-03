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

// QML context property：dialogMessage（正文）、dialogButtons（按钮文案数组）、
// dpScale（DPI 缩放）、isDark（深浅色）。按钮下标从 1 起，0 = 取消（Esc）。

int
qt_show_qml_dialog (string qml_url, string message, array<string> buttons) {
  // 激活 qrc（进程内一次性）。
  static const bool resourceInitialized= [] () {
    Q_INIT_RESOURCE (moganqml);
    return true;
  }();
  (void) resourceInitialized;
  // Qt::Tool + nullptr 父（而非 Qt::Dialog + activeWindow）：后者在 exec() 期间
  // 让 qwindowkitty 把主窗口标签栏 hit-test 误判为 HTBORDER，弹窗后拖动失效。
  // 透明背景属性须在 show/exec 前设置，避免 macOS 闪屏。
  QDialog d (nullptr, Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
  d.setAttribute (Qt::WA_TranslucentBackground);
  d.setAttribute (Qt::WA_NativeWindow);
  // objectName + 样式反制 liii.css 的通用 QDialog 规则，避免圆角外露方块。
  d.setObjectName ("QTMQmlDialog");
  d.setStyleSheet ("QDialog#QTMQmlDialog { background: transparent; "
                   "border: none; min-width: 0; min-height: 0; padding: 0; } "
                   "QDialog#QTMQmlDialog QWidget { background: transparent; }");
  QVBoxLayout* vl= new QVBoxLayout (&d);
  vl->setContentsMargins (0, 0, 0, 0);

  QQuickWidget* qw= new QQuickWidget (&d);
  qw->setResizeMode (QQuickWidget::SizeRootObjectToView);
  // setClearColor（QQuickWidget 专属）而非 WA_TranslucentBackground（对它不完全
  // 生效，默认白色 clear color 会盖住透明、露方角）。
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
  // 三重锁定尺寸：root 无固有 implicit 尺寸，不锁会被 QVBoxLayout 按 sizeHint
  // 压到约 100x30，只渲染左上角。
  const int w= DpiUtils::scaled (400);
  const int h= DpiUtils::scaled (150);
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
 * @param fields 字段表 tree：(form <field>...)。
 * @return 用户点 OK 返回 ((key value) ...)；Cancel / 关闭返回空 tree。
 *
 * QDialog 拼装与 qt_show_qml_dialog 同源（无边框 + 透明，避免 macOS 闪屏与
 * 标签栏 hit-test 误判）。尺寸由 QML root 自报（× DPI），动态字段场景需
 * childrenRect 兜底。
 */
tree
cpp_form_dialog (tree fields) {
  // 测试钩子：MOGAN_TEST_FORM_DIALOG=cancel 返回空 tree（模拟 Cancel）；
  // =ok 返回字段表当前值（模拟 OK，不弹窗）。供 TeXmacs/tests/2023.scm 自动化。
  string preset= get_env ("MOGAN_TEST_FORM_DIALOG");
  if (preset == "cancel") return tree (TUPLE);
  if (preset == "ok") {
    tree r (TUPLE);
    if (is_compound (fields)) {
      for (int i= 0; i < N (fields); i++) {
        if (is_compound (fields[i]) && N (fields[i]) >= 4) {
          tree kv (TUPLE);
          kv << fields[i][1] << fields[i][3]; // key, value
          r << kv;
        }
      }
    }
    return r;
  }
  // fields 形如 (form <field>...)；遍历其子节点（字段）。
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
  // QML 加载失败：开发期直接暴露，返回空 tree（scm 不写回）。
  if (qw->status () != QQuickWidget::Ready) return tree (TUPLE);

  vl->addWidget (qw);
  // 读 QML root 自报尺寸（× DPI）；首版字段静态，Ready 后已稳定。
  QQuickItem* root= qw->rootObject ();
  int         w= DpiUtils::scaled (int (root ? root->implicitWidth () : 360));
  int         h= DpiUtils::scaled (int (root ? root->implicitHeight () : 200));
  if (w <= 0) w= DpiUtils::scaled (360);
  if (h <= 0) h= DpiUtils::scaled (200);
  qw->setFixedSize (w, h);
  vl->setSizeConstraint (QLayout::SetFixedSize);
  d.setFixedSize (w, h);

  d.exec ();

  // 收集 submit 暂存的结果，构造 ((key value) ...)；Cancel / 关闭返回空 tree。
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
