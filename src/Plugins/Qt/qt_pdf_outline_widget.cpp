/******************************************************************************
 * MODULE     : qt_pdf_outline_widget.cpp
 * DESCRIPTION: Dockable outline sidebar widget (PDF bookmarks or document ToC)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "qt_pdf_outline_widget.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "converter.hpp" // cork_to_utf8
#include "preferences.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_pdf_reader_widget.hpp"
#include "qt_utilities.hpp" // utf8_to_qstring
#include "s7_tm.hpp"
#include "sys_utils.hpp" // get_env

namespace {
/** @brief 大纲侧边栏功能开关，默认开启。 */
static bool
outline_sidebar_enabled () {
  return get_preference ("outline sidebar", "on") == "on";
}

/** @brief scheme 值 → QString，处理 string/symbol 两种类型。 */
static QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_string (v)));
  if (tmscm_is_symbol (v))
    return utf8_to_qstring (cork_to_utf8 (tmscm_to_symbol (v)));
  return QString ();
}

/** @brief 递归解析 Scheme 嵌套树节点 (title target (child1 child2 ...)) →
 * OutlineItem。 */
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
    : QDockWidget (title, parent), tree_ (nullptr), titleLabel_ (nullptr),
      emptyLabel_ (nullptr), container_ (nullptr) {
  // 禁用系统原生标题栏，内部绘制现代工具栏
  setTitleBarWidget (new QWidget ());
  setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures (QDockWidget::DockWidgetClosable |
               QDockWidget::DockWidgetMovable |
               QDockWidget::DockWidgetFloatable);
  setMinimumWidth (DpiUtils::scaled (220));

  container_= new QWidget (this);
  container_->setObjectName ("outlineContainer");
  QVBoxLayout* mainLayout= new QVBoxLayout (container_);
  mainLayout->setContentsMargins (0, 0, 0, 0);
  mainLayout->setSpacing (0);

  // 1. 顶部操作工具栏
  QWidget*     headerBar   = new QWidget (container_);
  QHBoxLayout* headerLayout= new QHBoxLayout (headerBar);
  headerLayout->setContentsMargins (DpiUtils::scaled (12), DpiUtils::scaled (8),
                                    DpiUtils::scaled (12),
                                    DpiUtils::scaled (8));
  headerLayout->setSpacing (DpiUtils::scaled (6));

  titleLabel_= new QLabel (title, headerBar);
  titleLabel_->setObjectName ("outlineTitleLabel");
  titleLabel_->setStyleSheet ("font-weight: bold; font-size: 13px;");
  headerLayout->addWidget (titleLabel_);
  headerLayout->addStretch ();

  // 全部展开按钮
  QPushButton* expandAllBtn= new QPushButton ("+", headerBar);
  expandAllBtn->setObjectName ("outlineExpandBtn");
  expandAllBtn->setToolTip (tr ("Expand All"));
  expandAllBtn->setFixedSize (DpiUtils::scaled (22), DpiUtils::scaled (22));
  expandAllBtn->setFocusPolicy (Qt::NoFocus);
  expandAllBtn->setCursor (Qt::PointingHandCursor);
  connect (expandAllBtn, &QPushButton::clicked, this, [this] () {
    if (tree_) tree_->expandAll ();
  });
  headerLayout->addWidget (expandAllBtn);

  // 全部折叠按钮
  QPushButton* collapseAllBtn= new QPushButton ("-", headerBar);
  collapseAllBtn->setObjectName ("outlineCollapseBtn");
  collapseAllBtn->setToolTip (tr ("Collapse All"));
  collapseAllBtn->setFixedSize (DpiUtils::scaled (22), DpiUtils::scaled (22));
  collapseAllBtn->setFocusPolicy (Qt::NoFocus);
  collapseAllBtn->setCursor (Qt::PointingHandCursor);
  connect (collapseAllBtn, &QPushButton::clicked, this, [this] () {
    if (tree_) {
      tree_->collapseAll ();
      tree_->expandToDepth (0);
    }
  });
  headerLayout->addWidget (collapseAllBtn);

  // 关闭侧边栏按钮
  QPushButton* closeBtn=
      new QPushButton (QString::fromUtf8 ("\xc3\x97"), headerBar);
  closeBtn->setObjectName ("outlineCloseBtn");
  closeBtn->setToolTip (tr ("Close"));
  closeBtn->setFixedSize (DpiUtils::scaled (22), DpiUtils::scaled (22));
  closeBtn->setFocusPolicy (Qt::NoFocus);
  closeBtn->setCursor (Qt::PointingHandCursor);
  connect (closeBtn, &QPushButton::clicked, this, [this] () {
    setVisible (false);
    set_preference ("outline sidebar", "off");
  });
  headerLayout->addWidget (closeBtn);

  mainLayout->addWidget (headerBar);

  // 2. 树形大纲控件
  tree_= new QTreeWidget (container_);
  tree_->header ()->hide ();
  tree_->setUniformRowHeights (true);
  tree_->setExpandsOnDoubleClick (true);
  tree_->setRootIsDecorated (true);
  tree_->setStyleSheet ("QTreeWidget { border: none; }");
  mainLayout->addWidget (tree_, 1);

  // 3. 空大纲占位提示
  emptyLabel_= new QLabel (tr ("No outline available"), container_);
  emptyLabel_->setAlignment (Qt::AlignCenter);
  emptyLabel_->setStyleSheet ("color: #888888; padding: 20px;");
  emptyLabel_->hide ();
  mainLayout->addWidget (emptyLabel_);

  setWidget (container_);

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
  if (!outline_sidebar_enabled () || outline.isEmpty ()) {
    if (emptyLabel_) emptyLabel_->show ();
    setVisible (false);
    return;
  }
  if (emptyLabel_) emptyLabel_->hide ();
  buildTree (outline, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
}

void
OutlineWidget::setOutline (const QVector<OutlineItem>& outline) {
  tree_->clear ();
  if (!outline_sidebar_enabled () || outline.isEmpty ()) {
    if (emptyLabel_) emptyLabel_->show ();
    setVisible (false);
    return;
  }
  if (emptyLabel_) emptyLabel_->hide ();
  buildTree (outline, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
}

bool
OutlineWidget::loadDocumentOutline () {
  tree_->clear ();
  if (!outline_sidebar_enabled ()) {
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
    if (emptyLabel_) emptyLabel_->show ();
    setVisible (false);
    return false;
  }
  QVector<OutlineItem> items;
  for (tmscm cur= result; tmscm_is_pair (cur); cur= tmscm_cdr (cur)) {
    items.append (parseOutlineNode (tmscm_car (cur)));
  }
  if (items.isEmpty ()) {
    if (emptyLabel_) emptyLabel_->show ();
    setVisible (false);
    return false;
  }
  if (emptyLabel_) emptyLabel_->hide ();
  buildTree (items, nullptr);
  tree_->expandToDepth (0);
  setVisible (true);
  return true;
}

void
OutlineWidget::clear () {
  tree_->clear ();
  if (emptyLabel_) emptyLabel_->hide ();
  setVisible (false);
}

bool
OutlineWidget::hasContent () const {
  return tree_->topLevelItemCount () > 0;
}
