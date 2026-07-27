
/******************************************************************************
 * MODULE     : QTMStartupTabWidget.cpp
 * DESCRIPTION: Startup tab container implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMStartupTabWidget.hpp"
#include "StartupBridge.hpp"

#include "analyze.hpp"
#include "gui.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQuickWidget>
#include <QShowEvent>

namespace {
constexpr int kMinWidth = 600;
constexpr int kMinHeight= 400;
} // namespace

QTMStartupTabWidget::QTMStartupTabWidget (QWidget* parent) : QWidget (parent) {

  setMinimumSize (DpiUtils::scaled (kMinWidth), DpiUtils::scaled (kMinHeight));
  setFocusPolicy (Qt::StrongFocus);

  // 本容器嵌入在 QWK::WidgetWindowAgent 代理的主窗口下。QQuickWidget 会创建
  // 原生渲染窗口（QWindow），若在 QWK 代理窗口下发生 winId 变动，QWindowKit
  // 的 Cocoa winIdChanged 会用已释放的 NSWindowProxy 做消息派发而崩溃
  // （EXC_BAD_ACCESS @ objc_msgSend，见 ~NSWindowProxy）。先让本容器成为稳定的
  // 原生窗口并禁止为祖先创建原生窗口，QQuickWidget 重设父级时就不会触发代理的
  // releaseWindowProxy 路径。
  setAttribute (Qt::WA_DontCreateNativeAncestors);
  setAttribute (Qt::WA_NativeWindow);

  auto* layout= new QHBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);

  // ---- QQuickWidget ----
  Q_INIT_RESOURCE (moganqml);

  quickWidget_= new QQuickWidget ();
  quickWidget_->setAttribute (Qt::WA_DontCreateNativeAncestors);
  quickWidget_->setResizeMode (QQuickWidget::SizeRootObjectToView);
  quickWidget_->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

  // ---- Context properties（与 QTMQmlDialog 共用 dpScale / isDark） ----
  auto* ctx= quickWidget_->rootContext ();

  ctx->setContextProperty ("dpScale", DpiUtils::scaleFactor ());
  refreshTheme ();

  bridge_= new StartupBridge (this);
  ctx->setContextProperty ("startupBridge", bridge_);

  // ---- 加载 QML ----
  quickWidget_->setSource (QUrl ("qrc:/qml/startup/StartupTab.qml"));

#ifdef LIII_DEBUG
  if (quickWidget_->status () != QQuickWidget::Ready) {
    debug_std << "QTMStartupTabWidget: QML load failed, status="
              << (int) quickWidget_->status () << LF;
    for (const auto& e : quickWidget_->errors ())
      debug_std << "  QML error: " << from_qstring (e.toString ()) << LF;
  }
#endif

  layout->addWidget (quickWidget_);
  bridge_->initialize ();
}

QTMStartupTabWidget::~QTMStartupTabWidget ()= default;

void
QTMStartupTabWidget::refreshRecentDocs () {
  if (bridge_) bridge_->refreshRecentDocs ();
}

void
QTMStartupTabWidget::addRecentDoc (const QString& path) {
  if (bridge_) bridge_->addRecentDoc (path);
}

void
QTMStartupTabWidget::refreshTheme () {
  auto* ctx= quickWidget_->rootContext ();
  bool  isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  ctx->setContextProperty ("isDark", isDark);
}

void
QTMStartupTabWidget::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);
  refreshRecentDocs ();
}

void
QTMStartupTabWidget::keyPressEvent (QKeyEvent* event) {
  string key= from_key_press_event (event);
  if (is_empty (key)) return QWidget::keyPressEvent (event);
  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}

void
QTMStartupTabWidget::keyReleaseEvent (QKeyEvent* event) {
  string key= from_key_release_event (event);
  if (is_empty (key)) return QWidget::keyReleaseEvent (event);
  eval_scheme ("(key-press " * qt_scheme_quote (to_qstring (key)) * ")");
  event->accept ();
}
