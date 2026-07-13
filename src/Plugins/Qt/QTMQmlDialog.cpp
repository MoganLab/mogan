/******************************************************************************
 * MODULE      : QTMQmlDialog.cpp
 * DESCRIPTION : QML 模态对话框底座（见 QTMQmlDialog.hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "QTMQmlDialog.hpp"
#include "FontSelectorBridge.hpp"
#include "QTMQmlDialogBridge.hpp"
#include "QTMQmlDialogInternal.hpp"

#include "analyze.hpp" // occurs
#include "gui.hpp"     // tm_style_sheet
#include "qt_utilities.hpp"
#include "s7_tm.hpp"     // eval_scheme
#include "sys_utils.hpp" // lolly: get_env

#include <moebius/data/scheme.hpp> // tree_to_scheme_tree / scm_unquote

using moebius::data::scm_unquote;
using moebius::data::tree_to_scheme_tree;

#include <QDialog>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

/**
 * @brief QML 加载失败时输出诊断（status / warnings），避免线上 qrc 路径或 QML
 *        语法回归被静默吞成「点了没反应」。仅在 DEBUG 构建写 debug 流。
 */
static void
log_qml_load_failure (QQuickWidget* qw, const char* qml_path) {
#ifdef LIII_DEBUG
  debug_std << "QTMQmlDialog: QML load failed for " << qml_path
            << ", status=" << (int) qw->status () << LF;
  for (const QQmlError& e : qw->errors ())
    debug_std << "  QML error: " << from_qstring (e.toString ()) << LF;
#endif
}

/**
 * @brief 把按钮文案数组统一翻译成 QML 可消费的 QStringList。
 *
 * 两类弹窗（确认型 / form 型）的按钮文案都经此走 qt_translate，避免 QML 硬编码
 * 漏译；缺翻译 key 时原样回退，后续在字典表（如 zh_CN.scm）补条目即可。
 */
QStringList
translate_buttons (array<string> buttons) {
  QStringList out;
  for (int i= 0; i < N (buttons); i++)
    out << qt_translate (buttons[i]);
  return out;
}

/**
 * @brief 统一拼装无边框透明模态 QDialog + 内嵌 QQuickWidget 的宿主。
 *
 * 两类弹窗（确认型 / form 型）共用同一套窗口外观约束：
 * - Qt::Tool + nullptr 父（而非 Qt::Dialog + activeWindow）：后者在 exec() 期间
 *   让 qwindowkitty 把主窗口标签栏 hit-test 误判为 HTBORDER，弹窗后拖动失效。
 * - 透明背景属性须在 show/exec 前设置，避免 macOS 闪屏。
 * - setClearColor（QQuickWidget 专属）而非 WA_TranslucentBackground（对它不完全
 *   生效，默认白色 clear color 会盖住透明、露方角）。
 * - objectName + 样式反制 liii.css 的通用 QDialog 规则，避免圆角外露方块。
 *
 * @param d 由调用方栈分配、生命期覆盖 exec() 的宿主 QDialog。
 * @return 挂到 d 上、待 setSource / 注入 context property 的 QQuickWidget。
 */
static QQuickWidget*
setup_frameless_qml_host (QDialog& d) {
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
  return qw;
}

/**
 * @brief 注入两类弹窗共用的 context property 并返回 bridge。
 *
 * 共用项：closeBridge（按钮 / submit 回流）、dpScale（DPI 缩放）、isDark（跟随
 * tm_style_sheet，liii-night / *-dark 视为深色）。各弹窗特有的 context property
 * （确认型的 dialogMessage/dialogButtons、form 型的 formFields）由调用方在
 * run_qml_dialog 的注入回调里，调用本函数之后自行注入。
 *
 * @return 挂到 QML 的 bridge；调用方持有所有权并负责 delete（bridge 不挂
 * QObject parent，不会被宿主 QDialog 析构带走，以便 form 型 exec 后取
 * results()）。
 */
static QmlDialogBridge*
inject_common_context (QQuickWidget* qw, QDialog& host) {
  bool isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  QmlDialogBridge* bridge= new QmlDialogBridge (&host);
  qw->rootContext ()->setContextProperty ("closeBridge", bridge);
  qw->rootContext ()->setContextProperty ("dpScale", DpiUtils::scaleFactor ());
  qw->rootContext ()->setContextProperty ("isDark", isDark);
  return bridge;
}

/**
 * @brief 把 QQuickWidget 及其宿主 QDialog 锁定为同一固定尺寸。
 *
 * @param logic_w / logic_h 96 DPI 下的逻辑尺寸，内部统一 DpiUtils::scaled ×
 * DPI。 不锁会被 QVBoxLayout 按 sizeHint 压到约 100x30，只渲染 QML 左上角。
 */
static void
lock_fixed_size (QQuickWidget* qw, QVBoxLayout* vl, QDialog& d, int logic_w,
                 int logic_h) {
  const int w= DpiUtils::scaled (logic_w);
  const int h= DpiUtils::scaled (logic_h);
  qw->setFixedSize (w, h);
  vl->setSizeConstraint (QLayout::SetFixedSize);
  d.setFixedSize (w, h);
}

/**
 * @brief 通用 QML 模态弹窗引擎。
 *
 * 把两类弹窗（确认型 / form 型）共用的 QDialog 拼装 + setSource + 加载检查 +
 * addWidget + 定尺寸 + exec 流程收敛于此，差异点（context property 注入、
 * 尺寸算法、退出码语义）由调用方通过两个回调提供。新增弹窗只需写回调 + 解读
 * exec 返回值，不再重抄宿主拼装。
 *
 * @param qml_url QML 文档的 qrc 路径。
 * @param debug_tag 加载失败写 debug 日志时的标签（如 "confirm dialog"）。
 * @param inject_context 在 setSource 之前调用，负责注入全部 context property：
 *        调用方先调 inject_common_context(qw, host) 注入共用的 closeBridge /
 *        dpScale / isDark（并按需捕获返回的 bridge），再注入弹窗特有项。
 * @param logic_w / logic_h 96 DPI 下的逻辑尺寸（引擎内部统一 DpiUtils::scaled
 *        × DPI 后锁定 QQuickWidget 与 QDialog，调用方不必自己乘 DPI）。
 * @return QDialog::exec 的退出码（即 QML 侧 closeBridge 传给 done() 的值）；
 *         QML 加载失败返回 -1。调用方据此映射结果（确认型 → 按钮下标；form 型
 *         → Accepted/Rejected，表单值另行从 bridge->results() 取）。
 */
static int
run_qml_dialog (const string& qml_url, const char* debug_tag,
                std::function<void (QQuickWidget*, QDialog&)> inject_context,
                int logic_w, int logic_h) {
  static const bool resourceInitialized= [] () {
    Q_INIT_RESOURCE (moganqml);
    return true;
  }();
  (void) resourceInitialized;
  QDialog       d (nullptr, Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
  QQuickWidget* qw= setup_frameless_qml_host (d);
  QVBoxLayout*  vl= static_cast<QVBoxLayout*> (d.layout ());

  // context 注入须在 setSource 之前。
  inject_context (qw, d);

  qw->setSource (QUrl (to_qstring (qml_url)));
  if (qw->status () != QQuickWidget::Ready) {
    log_qml_load_failure (qw, debug_tag);
    return -1;
  }

  vl->addWidget (qw);
  lock_fixed_size (qw, vl, d, logic_w, logic_h);

  return d.exec ();
}

/**
 * @brief 非阻塞模态引擎（setModal + show）—— live 重绘对话框专用。
 *
 * 区别于 run_qml_dialog 的 exec：show() 不嵌套事件循环，主窗口 paint 照常（live
 * 写回 实时可见）；setModal(true) 让 Qt
 * 拦截其他窗口输入，仍保持模态独占（防切窗口/动光标/ 重复打开）。即「exec
 * 的输入独占」+「show 的不阻塞重绘」兼得。字体选择器等需 live
 * 重绘文档的对话框用本引擎；一次性提交的（ConfirmClose/FormDialog）用
 * run_qml_dialog 即可。
 *
 * 生命期：QDialog 堆分配 + WA_DeleteOnClose，bridge 不挂 parent，靠 host
 * destroyed 信号 deleteLater 自清。
 *
 * @return 恒 0（不阻塞；调用方返回值仅测试钩子路径用）。
 */
static int
run_modal_qml_dialog (
    const string& qml_url, const char* debug_tag,
    std::function<void (QQuickWidget*, QDialog*)> inject_context, int logic_w,
    int logic_h) {
  static const bool resourceInitialized= [] () {
    Q_INIT_RESOURCE (moganqml);
    return true;
  }();
  (void) resourceInitialized;
  QDialog* d=
      new QDialog (nullptr, Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
  d->setAttribute (Qt::WA_DeleteOnClose);
  d->setModal (true);
  QQuickWidget* qw= setup_frameless_qml_host (*d);
  QVBoxLayout*  vl= static_cast<QVBoxLayout*> (d->layout ());
  inject_context (qw, d);
  qw->setSource (QUrl (to_qstring (qml_url)));
  if (qw->status () != QQuickWidget::Ready) {
    log_qml_load_failure (qw, debug_tag);
    delete d;
    return -1;
  }
  vl->addWidget (qw);
  lock_fixed_size (qw, vl, *d, logic_w, logic_h);
  d->show ();
  return 0;
}

/**
 * @brief 「确认关闭」弹窗的 glue 入口。
 * @param message 已翻译的正文。
 * @param scratch 为真时肯定按钮显示「另存为」。
 * @return "Save" / "Don't save" / "Cancel" 之一。
 *
 * @details 按钮顺序为 肯定（Save / Save as）、Don't save、Cancel，对应
 * run_qml_dialog 返回的按钮下标 1/2/3（0 / -1 = Esc / X / 加载失败 = Cancel）。
 * 测试钩子 MOGAN_TEST_CONFIRM_CLOSE 命中时直接返回不弹窗。
 */
string
cpp_confirm_close (string message, bool scratch) {
  string preset= get_env ("MOGAN_TEST_CONFIRM_CLOSE");
  if (preset == "Save" || preset == "Don't save" || preset == "Cancel")
    return preset;
  array<string>    buttons   = {string (scratch ? "Save as" : "Save"),
                                string ("Don't save"), string ("Cancel")};
  QStringList      qmlButtons= translate_buttons (buttons);
  QmlDialogBridge* bridge    = nullptr;
  int              choice    = run_qml_dialog (
      "qrc:/qml/ConfirmClose.qml", "confirm dialog",
      [&] (QQuickWidget* qw, QDialog& host) {
        bridge= inject_common_context (qw, host);
        qw->rootContext ()->setContextProperty ("dialogMessage",
                                                                 to_qstring (message));
        qw->rootContext ()->setContextProperty ("dialogButtons", qmlButtons);
      },
      400, 150);
  delete bridge;
  switch (choice) {
  case 1:
    return "Save";
  case 2:
    return "Don't save";
  default:
    return "Cancel";
  }
}

// ---- form 引擎 --------------------------------------------------------------

// 字段节点下标协议（见 QTMQmlDialog.hpp @par 数据协议）：
// (<type> <label> <key> (<options>...) <value> <live?>)
// label/key/value 的位置在 form 引擎多处使用，集中在此避免魔法下标散落。
const int FIELD_LABEL  = 0;
const int FIELD_KEY    = 1;
const int FIELD_OPTIONS= 2;
const int FIELD_VALUE  = 3;
const int FIELD_LIVE   = 4;

/**
 * @brief 字段节点是否形状合法（compound 且至少含到 value 的位置）。
 */
bool
field_valid (tree f) {
  return is_compound (f) && N (f) > FIELD_VALUE;
}

/**
 * @brief 取字段节点的 key 子树（透传，不拷贝字符串）。
 */
tree
field_key (tree f) {
  return f[FIELD_KEY];
}

/**
 * @brief 取字段节点的 value 子树（透传）。
 */
tree
field_value (tree f) {
  return f[FIELD_VALUE];
}

/**
 * @brief 单个字段节点 tree → QML 可消费的 QVariantMap。
 * @param f 字段节点，形如 (enum <label> <key> (<opt>...) <value> <live?>)。
 * @return 含 type/label/key/options/value/live 的 map；形状不符返回空。
 *
 * label/key/value 纯透传，不做翻译或类型转换（value 在 scm 侧已 string 化）。
 */
QVariantMap
field_tree_to_qml (tree f) {
  QVariantMap m;
  if (!field_valid (f)) return m;
  // 用 get_label 而非 ->label：compound 的 ->label 是被 children 占用的乱码。
  m["type"] = to_qstring (get_label (f));
  m["label"]= to_qstring (get_label (f[FIELD_LABEL]));
  m["key"]  = to_qstring (get_label (field_key (f)));
  if (is_compound (f[FIELD_OPTIONS])) {
    // tree_to_scheme_tree 把 compound label 拼回首位，避免 stree->tree 把列表
    // 首项变 label 而丢项；leaf 经 scm_unquote 去引号。
    tree         opts_tree= tree_to_scheme_tree (f[FIELD_OPTIONS]);
    QVariantList opts;
    for (int i= 0; i < N (opts_tree); i++)
      opts << to_qstring (scm_unquote (get_label (opts_tree[i])));
    m["options"]= opts;
  }
  m["value"]= to_qstring (get_label (field_value (f)));
  // 守卫 is_atomic：compound 的 ->label 是乱码标签，会误判 live=true。
  m["live"]= (N (f) > FIELD_LIVE && is_atomic (f[FIELD_LIVE]) &&
              f[FIELD_LIVE]->label == "true");
  return m;
}

/**
 * @brief 通用 form 弹窗。
 * @param fields 字段表 tree：(form <field>...)，调用方须 stree->tree 转换。
 * @return 用户点 OK 返回 (tuple (tuple key value) ...)；Cancel / 关闭 / 加载
 * 失败返回空 tree。
 *
 * @details 宿主拼装与定尺寸走 run_qml_dialog；尺寸按字段数动态算（与
 * FormDialog.qml 的 implicitHeight 同源）。测试钩子 MOGAN_TEST_FORM_DIALOG=
 * ok/cancel 命中时不弹窗，供自动化测试。
 */
tree
cpp_form_dialog (tree fields) {
  string preset= get_env ("MOGAN_TEST_FORM_DIALOG");
  if (preset == "cancel") return tree (TUPLE);
  if (preset == "ok") {
    // 与真实路径同源判定合法字段，避免 hook 与真实弹窗对畸形字段行为分叉。
    tree r (TUPLE);
    if (is_compound (fields)) {
      for (int i= 0; i < N (fields); i++) {
        QVariantMap m= field_tree_to_qml (fields[i]);
        if (m.isEmpty ()) continue;
        tree kv (TUPLE);
        kv << tree (from_qstring (m.value ("key").toString ()))
           << tree (from_qstring (m.value ("value").toString ()));
        r << kv;
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
  array<string> buttons= {string ("OK"), string ("Cancel")};
  // 与 FormDialog.qml 的 implicitHeight 同源；引擎内部统一 × DPI。
  const int fieldCount= qmlFields.size ();
  const int logicH    = 24 * 2 + fieldCount * (44 + 12) + 64;

  // bridge 须在注入回调里创建并捕获，供事后 results() 取值。
  QmlDialogBridge* bridge= nullptr;
  run_qml_dialog (
      "qrc:/qml/FormDialog.qml", "FormDialog.qml",
      [&] (QQuickWidget* qw, QDialog& host) {
        bridge= inject_common_context (qw, host);
        qw->rootContext ()->setContextProperty ("formFields", qmlFields);
        qw->rootContext ()->setContextProperty ("dialogButtons",
                                                translate_buttons (buttons));
      },
      420, logicH);

  // 退出码对 form 型无意义；Cancel / 加载失败均返回空 tree。
  tree               r (TUPLE);
  const QVariantMap& res= bridge ? bridge->results () : QVariantMap ();
  delete bridge;
  for (auto it= res.begin (); it != res.end (); ++it) {
    tree kv (TUPLE);
    kv << tree (from_qstring (it.key ()));
    kv << tree (from_qstring (it.value ().toString ()));
    r << kv;
  }
  return r;
}

// ---- 字体选择器 ------------------------------------------------------------

/**
 * @brief 字体选择器 QML 对话框（声明见 QTMQmlDialog.hpp）。
 *
 * @details 走 run_modal_qml_dialog（setModal + show，非阻塞模态）——字体选择器需
 * live 写回文档并实时看到效果，exec 的嵌套事件循环会让主窗口不重绘，故用
 * setModal+show 兼得「输入独占」与「live 重绘」。FontSelectorBridge 注入为
 * fontBridge context property 承载 QML↔scheme 交互；字体状态在 scheme（specsKey
 * 句柄），bridge 透传。 Cancel 经打开时快照写回撤销，OK 补齐差异落定。 测试钩子
 * MOGAN_TEST_FONT_SELECTOR=ok|cancel 命中时不弹窗，返回 tree 供测试区分。
 */
tree
cpp_font_selector_dialog (int specs_key) {
  string preset= get_env ("MOGAN_TEST_FONT_SELECTOR");
  if (preset == "cancel") {
    return tree (TUPLE);
  }
  if (preset == "ok") {
    // 测试钩子：走 font-selector-commit（补齐差异落定），返回非空 tuple
    // 供测试区分 OK。
    eval_scheme ("(font-selector-commit " * as_string (specs_key) * ")");
    tree r (TUPLE);
    r << tree ("ok");
    return r;
  }
  // 非阻塞模态（setModal + show）：host 堆分配，destroyed 自清 bridges。
  run_modal_qml_dialog (
      "qrc:/qml/FontSelector.qml", "FontSelector.qml",
      [&] (QQuickWidget* qw, QDialog* host) {
        QmlDialogBridge*    closeBridge= inject_common_context (qw, *host);
        FontSelectorBridge* fontBridge=
            new FontSelectorBridge (host, specs_key);
        qw->rootContext ()->setContextProperty ("fontBridge", fontBridge);
        QObject::connect (host, &QDialog::destroyed, closeBridge,
                          &QObject::deleteLater);
        QObject::connect (host, &QDialog::destroyed, fontBridge,
                          &QObject::deleteLater);
      },
      980, 600);
  // 非阻塞 show 立即返回，用户尚未点 OK/Cancel，无结论——返回空 tree（与 Cancel
  // 一致）。
  return tree (TUPLE);
}
