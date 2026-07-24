
/******************************************************************************
 * MODULE     : QTMStartupTabWidget.cpp
 * DESCRIPTION: Startup tab widget — hosts QQuickWidget + StartupBridge
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMStartupTabWidget.hpp"
#include "StartupBridge.hpp"

#include "analyze.hpp" // occurs
#include "gui.hpp"     // tm_style_sheet
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>

#include <moebius/data/scheme.hpp>

namespace {
constexpr int kMinWidth = 600;
constexpr int kMinHeight= 400;
} // namespace

QTMStartupTabWidget::QTMStartupTabWidget (QWidget* parent)
    : QWidget (parent), currentEntry_ (Entry::Home) {

  setMinimumSize (DpiUtils::scaled (kMinWidth), DpiUtils::scaled (kMinHeight));
  setFocusPolicy (Qt::StrongFocus);

  // 全区域由 QQuickWidget 填充：StartupTab.qml 内部已含侧边栏 + 内容区
  QHBoxLayout* layout= new QHBoxLayout (this);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSpacing (0);

  setupQml ();
}

QTMStartupTabWidget::~QTMStartupTabWidget ()= default;

void
QTMStartupTabWidget::setupQml () {
  // 确保 .qrc 资源已初始化（参照 QTMQmlDialog）
  Q_INIT_RESOURCE (moganqml);

  quickWidget_= new QQuickWidget (this);
  quickWidget_->setResizeMode (QQuickWidget::SizeRootObjectToView);
  quickWidget_->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  quickWidget_->setClearColor (Qt::transparent);

  // 创建 bridge 并注入 context property
  bridge_= new StartupBridge (this);

  // 注入通用的 dpScale / isDark（与 QTMQmlDialog 一致）
  bool isDark=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  quickWidget_->rootContext ()->setContextProperty ("dpScale",
                                                    DpiUtils::scaleFactor ());
  quickWidget_->rootContext ()->setContextProperty ("isDark", isDark);

  // 注入 startup bridge
  quickWidget_->rootContext ()->setContextProperty ("startupBridge", bridge_);

  // 加载 QML
  quickWidget_->setSource (QUrl ("qrc:/qml/startup/StartupTab.qml"));

#ifdef LIII_DEBUG
  if (quickWidget_->status () != QQuickWidget::Ready) {
    debug_std << "QTMStartupTabWidget: QML load failed, status="
              << (int) quickWidget_->status () << LF;
    for (const auto& e : quickWidget_->errors ())
      debug_std << "  QML error: " << from_qstring (e.toString ()) << LF;
  }
#endif

  layout ()->addWidget (quickWidget_);

  // 初始化 bridge（连接 TemplateManager、加载最近文档等）
  bridge_->initialize ();
}

QTMStartupTabWidget::Entry
QTMStartupTabWidget::current_entry () const {
  return currentEntry_;
}

void
QTMStartupTabWidget::set_current_entry (Entry entry) {
  if (currentEntry_ != entry) {
    currentEntry_= entry;
    emit entry_changed (entry);
  }
}

void
QTMStartupTabWidget::refreshRecentDocs () {
  if (bridge_) bridge_->refreshRecentDocs ();
}

void
QTMStartupTabWidget::addRecentDoc (const QString& path) {
  if (bridge_) bridge_->addRecentDoc (path);
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
