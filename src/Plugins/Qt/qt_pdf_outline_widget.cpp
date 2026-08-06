/******************************************************************************
 * MODULE     : qt_pdf_outline_widget.cpp
 * DESCRIPTION: Dockable outline sidebar widget (PDF bookmarks or document ToC)
 * COPYRIGHT  : (C) 2026
 ******************************************************************************/

#include "qt_pdf_outline_widget.hpp"

#include <QHeaderView>
#include <QTreeWidgetItem>

#include "converter.hpp" // cork_to_utf8
#include "qt_dpi_utils.hpp"
#include "qt_pdf_reader_widget.hpp"
#include "qt_utilities.hpp" // utf8_to_qstring
#include "s7_tm.hpp"
#include "sys_utils.hpp" // get_env

OutlineWidget::OutlineWidget (const QString& title, QWidget* parent)
    : QDockWidget (title, parent), tree_ (new QTreeWidget (this)) {
  // 禁用标题栏，风格与 leftTools 一致
  setTitleBarWidget (new QWidget ());
  setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures (QDockWidget::DockWidgetClosable |
               QDockWidget::DockWidgetMovable |
               QDockWidget::DockWidgetFloatable);
  setMinimumWidth (DpiUtils::scaled (200));

  tree_->header ()->hide ();
  tree_->setUniformRowHeights (true);
  tree_->setExpandsOnDoubleClick (true);
  tree_->setRootIsDecorated (true);
  tree_->setStyleSheet ("QTreeWidget { border: none; }");

  setWidget (tree_);

  connect (tree_, &QTreeWidget::itemClicked, this,
           [this] (QTreeWidgetItem* item) {
             QString target= item->data (0, Qt::UserRole).toString ();
             if (!target.isEmpty ()) emit outlineActivated (target);
           });
}

void
OutlineWidget::buildTree (const QVector<PdfOutlineItem>& items,
                          QTreeWidgetItem*               parent) {
  for (const PdfOutlineItem& item : items) {
    QTreeWidgetItem* treeItem= (parent == nullptr)
                                   ? new QTreeWidgetItem (tree_)
                                   : new QTreeWidgetItem (parent);
    treeItem->setText (0, item.title);
    // PdfOutlineItem::page 是 0-based（fz_resolve_link），goToPage 需要 1-based
    int pageOneBased= (item.page >= 0) ? item.page + 1 : -1;
    treeItem->setData (0, Qt::UserRole, QString::number (pageOneBased));
    if (!item.title.isEmpty ()) {
      treeItem->setToolTip (0, item.title);
    }
    if (!item.children.isEmpty ()) {
      buildTree (item.children, treeItem);
    }
  }
}

void
OutlineWidget::buildTree (const QVector<OutlineItem>& items,
                          QTreeWidgetItem*            parent) {
  for (const OutlineItem& item : items) {
    QTreeWidgetItem* treeItem= (parent == nullptr)
                                   ? new QTreeWidgetItem (tree_)
                                   : new QTreeWidgetItem (parent);
    treeItem->setText (0, item.title);
    treeItem->setData (0, Qt::UserRole, item.target);
    if (!item.title.isEmpty ()) {
      treeItem->setToolTip (0, item.title);
    }
    if (!item.children.isEmpty ()) {
      buildTree (item.children, treeItem);
    }
  }
}

void
OutlineWidget::setOutline (const QVector<PdfOutlineItem>& outline) {
  tree_->clear ();
  if (outline.isEmpty ()) {
    setVisible (false);
    return;
  }
  buildTree (outline, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
}

void
OutlineWidget::setOutline (const QVector<OutlineItem>& outline) {
  tree_->clear ();
  if (outline.isEmpty ()) {
    setVisible (false);
    return;
  }
  buildTree (outline, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
}

namespace {
/** @brief scheme 值 → QString，处理 string/symbol 两种类型。 */
static QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  return QString ();
}

/** @brief 解析 (title path) → OutlineItem。 */
static OutlineItem
parseOutlinePair (tmscm pair) {
  OutlineItem item;
  if (tmscm_is_pair (pair)) {
    item.title= tmscm_to_qstring (tmscm_car (pair));
    if (tmscm_is_pair (tmscm_cdr (pair))) {
      item.target= tmscm_to_qstring (tmscm_cadr (pair));
    }
  }
  return item;
}
} // namespace

bool
OutlineWidget::loadDocumentOutline () {
  tree_->clear ();
  // 确保模块已加载（init-research.scm 中的 use-modules 可能未生效时兜底）
  if (!eval_scheme ("(defined? 'document-outline)")) {
    string texmacs_path= get_env ("TEXMACS_PATH");
    string file_path   = texmacs_path * "/progs/text/text-outline.scm";
    eval_scheme_file (file_path);
  }
  tmscm result= eval_scheme ("(document-outline)");
  if (tmscm_is_null (result)) {
    setVisible (false);
    return false;
  }
  QVector<OutlineItem> items;
  for (tmscm cur= result; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    items.append (parseOutlinePair (tmscm_car (cur)));
  }
  if (items.isEmpty ()) {
    setVisible (false);
    return false;
  }
  buildTree (items, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
  return true;
}

void
OutlineWidget::clear () {
  tree_->clear ();
  setVisible (false);
}

bool
OutlineWidget::hasContent () const {
  return tree_->topLevelItemCount () > 0;
}
