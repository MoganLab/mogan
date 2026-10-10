/******************************************************************************
 * MODULE     : qt_pdf_outline_widget.cpp
 * DESCRIPTION: Dockable outline sidebar widget (PDF bookmarks or document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "qt_pdf_outline_widget.hpp"

#include <QQmlContext>
#include <QTimer>
#include <QVBoxLayout>

#include "converter.hpp" // cork_to_utf8
#include "preferences.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_pdf_reader_widget.hpp"
#include "qt_utilities.hpp" // utf8_to_qstring, qt_inject_theme_context, qt_use_software_scene_graph
#include "s7_tm.hpp"
#include "sys_utils.hpp" // get_env

namespace {
constexpr int kOutlineMinWidth    = 220;
constexpr int kOutlineDefaultWidth= 300;
constexpr int kOutlineMaxWidth    = 550;

static bool
outline_sidebar_enabled () {
  return get_preference ("outline sidebar", "on") == "on";
}

static QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  return QString ();
}

static OutlineItem
parseOutlineNode (tmscm node) {
  OutlineItem item;
  if (!tmscm_is_pair (node)) return item;

  item.title= tmscm_to_qstring (tmscm_car (node));
  tmscm rest= tmscm_cdr (node);
  if (tmscm_is_pair (rest)) {
    item.target   = tmscm_to_qstring (tmscm_car (rest));
    tmscm children= tmscm_cdr (rest);
    if (tmscm_is_pair (children)) {
      tmscm childList= tmscm_car (children);
      for (tmscm cur= childList; tmscm_is_pair (cur); cur= tmscm_cdr (cur)) {
        item.children.append (parseOutlineNode (tmscm_car (cur)));
      }
    }
  }
  return item;
}
} // namespace

OutlineWidget::OutlineWidget (const QString& title, QWidget* parent)
    : QDockWidget (title, parent), bridge_ (new OutlineBridge (this)),
      quick_ (nullptr) {
  setTitleBarWidget (new EmptyTitleBar ());
  setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures (QDockWidget::DockWidgetClosable |
               QDockWidget::DockWidgetMovable |
               QDockWidget::DockWidgetFloatable);

  // 严格设置尺寸边界，避免 Qt 负高警告并约束侧边栏合理宽度
  setMinimumSize (DpiUtils::scaled (kOutlineMinWidth), 0);
  setMaximumSize (DpiUtils::scaled (kOutlineMaxWidth), QWIDGETSIZE_MAX);

  qt_use_software_scene_graph ();

  quick_= new QQuickWidget (this);
  quick_->setResizeMode (QQuickWidget::SizeRootObjectToView);
  quick_->setClearColor (Qt::transparent);
  quick_->setStyleSheet ("background: transparent;");
  qt_inject_theme_context (quick_);
  quick_->rootContext ()->setContextProperty ("outlineBridge", bridge_);
  // setSource 延迟到首次 showEvent：dock 默认隐藏，启动时不应解析 QML

  setWidget (quick_);

  widthSaveTimer_= new QTimer (this);
  widthSaveTimer_->setSingleShot (true);
  widthSaveTimer_->setInterval (300);
  connect (widthSaveTimer_, &QTimer::timeout, this, [this] () {
    if (pendingWidth_ > 0) {
      set_preference ("outline sidebar width",
                      from_qstring (QString::number (pendingWidth_)));
    }
  });

  connect (bridge_, &OutlineBridge::outlineActivated, this,
           &OutlineWidget::outlineActivated);
  connect (bridge_, &OutlineBridge::closeRequested, this, [this] () {
    setVisible (false);
    set_preference ("outline sidebar", "off");
  });
}

QSize
OutlineWidget::sizeHint () const {
  int savedWidth= as_int (get_preference ("outline sidebar width", "300"));
  if (savedWidth <= 0) savedWidth= kOutlineDefaultWidth;
  return QSize (DpiUtils::scaled (savedWidth), DpiUtils::scaled (600));
}

void
OutlineWidget::showEvent (QShowEvent* event) {
  QDockWidget::showEvent (event);
  if (!qmlLoaded_) {
    qmlLoaded_= true;
    quick_->setSource (QUrl ("qrc:/qml/OutlineSidebar.qml"));
  }
}

void
OutlineWidget::resizeEvent (QResizeEvent* event) {
  QDockWidget::resizeEvent (event);
  int w= width ();
  if (w > 0 && isVisible ()) {
    qreal factor  = DpiUtils::scaleFactor ();
    int   unscaled= (factor > 0) ? int (w / factor) : w;
    if (unscaled >= kOutlineMinWidth && unscaled <= kOutlineMaxWidth) {
      // set_preference 每次都会全量写盘，拖动期间防抖合并为一次
      pendingWidth_= unscaled;
      widthSaveTimer_->start ();
    }
  }
}

void
OutlineWidget::setOutline (const QVector<PdfOutlineItem>& outline) {
  if (!outline_sidebar_enabled () || outline.isEmpty ()) {
    bridge_->clear ();
    setVisible (false);
    return;
  }
  bridge_->setOutline (outline);
  setVisible (true);
}

void
OutlineWidget::setOutline (const QVector<OutlineItem>& outline) {
  if (!outline_sidebar_enabled () || outline.isEmpty ()) {
    bridge_->clear ();
    setVisible (false);
    return;
  }
  bridge_->setOutline (outline);
  setVisible (true);
}

bool
OutlineWidget::loadDocumentOutline () {
  if (!outline_sidebar_enabled ()) {
    bridge_->clear ();
    setVisible (false);
    return false;
  }
  if (!eval_scheme ("(defined? 'document-outline)")) {
    string texmacs_path= get_env ("TEXMACS_PATH");
    string file_path   = texmacs_path * "/progs/text/text-outline.scm";
    eval_scheme_file (file_path);
  }
  tmscm result= eval_scheme ("(document-outline)");
  if (!tmscm_is_pair (result)) {
    bridge_->clear ();
    setVisible (false);
    return false;
  }
  QVector<OutlineItem> items;
  for (tmscm cur= result; tmscm_is_pair (cur); cur= tmscm_cdr (cur)) {
    items.append (parseOutlineNode (tmscm_car (cur)));
  }
  if (items.isEmpty ()) {
    bridge_->clear ();
    setVisible (false);
    return false;
  }
  bridge_->setOutline (items);
  setVisible (true);
  return true;
}

void
OutlineWidget::clear () {
  bridge_->clear ();
  setVisible (false);
}

bool
OutlineWidget::hasContent () const {
  return bridge_->hasContent ();
}
