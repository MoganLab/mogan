
/******************************************************************************
 * MODULE     : qt_floating_search_bar.cpp
 * DESCRIPTION: A VSCode-style floating search bar widget
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_floating_search_bar.hpp"
#include "qt_chat_controller.hpp"
#include "qt_chat_tab_widget.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"

#include "s7_tm.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QStyle>

/******************************************************************************
 * QTMFloatingSearchBar
 ******************************************************************************/

QTMFloatingSearchBar::QTMFloatingSearchBar (QWidget* parent)
    : QWidget (parent), edit_ (nullptr), infoLbl_ (nullptr) {
  setObjectName ("floating_search_bar");
  setWindowFlags (Qt::Widget);
  setAttribute (Qt::WA_StyledBackground);
  setFixedHeight (DpiUtils::scaled (32));

  QHBoxLayout* lay= new QHBoxLayout (this);
  lay->setContentsMargins (DpiUtils::scaled (4), 0, DpiUtils::scaled (4), 0);
  lay->setSpacing (DpiUtils::scaled (2));

  // 搜索输入框
  edit_= new QLineEdit (this);
  edit_->setPlaceholderText (tr ("Search"));
  edit_->setFixedHeight (DpiUtils::scaled (24));
  lay->addWidget (edit_, 1);

  // 上一个按钮
  QPushButton* prevBtn= new QPushButton (this);
  prevBtn->setIcon (style ()->standardIcon (QStyle::SP_ArrowUp));
  prevBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  prevBtn->setToolTip (tr ("Previous (Shift+Enter)"));
  lay->addWidget (prevBtn);

  // 下一个按钮
  QPushButton* nextBtn= new QPushButton (this);
  nextBtn->setIcon (style ()->standardIcon (QStyle::SP_ArrowDown));
  nextBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  nextBtn->setToolTip (tr ("Next (Enter)"));
  lay->addWidget (nextBtn);

  // 关闭按钮
  QPushButton* closeBtn= new QPushButton (this);
  closeBtn->setIcon (style ()->standardIcon (QStyle::SP_DialogCloseButton));
  closeBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  closeBtn->setToolTip (tr ("Close (Esc)"));
  lay->addWidget (closeBtn);

  // 匹配计数标签
  infoLbl_= new QLabel (this);
  infoLbl_->setFixedHeight (DpiUtils::scaled (24));
  infoLbl_->setMinimumWidth (DpiUtils::scaled (40));
  lay->addWidget (infoLbl_);

  // 信号连接
  QObject::connect (edit_, &QLineEdit::textChanged, this,
                    &QTMFloatingSearchBar::queryChanged);
  QObject::connect (nextBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::findNextRequested);
  QObject::connect (prevBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::findPreviousRequested);
  QObject::connect (closeBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::closeRequested);

  hide ();
}

void
QTMFloatingSearchBar::activate () {
  show ();
  raise ();
  edit_->clear ();
  infoLbl_->clear ();
  edit_->setFocus ();
}

QString
QTMFloatingSearchBar::queryText () const {
  return edit_->text ();
}

void
QTMFloatingSearchBar::setMatchInfo (const QString& info) {
  infoLbl_->setText (info);
}

void
QTMFloatingSearchBar::keyPressEvent (QKeyEvent* event) {
  if (event->key () == Qt::Key_Escape) {
    emit closeRequested ();
    event->accept ();
  }
  else if (event->key () == Qt::Key_Return || event->key () == Qt::Key_Enter) {
    if (event->modifiers () & Qt::ShiftModifier) emit findPreviousRequested ();
    else emit findNextRequested ();
    event->accept ();
  }
  else {
    QWidget::keyPressEvent (event);
  }
}

/******************************************************************************
 * 浮动搜索栏管理（独立于 QTChatTabWidget）
 ******************************************************************************/

static QTMFloatingSearchBar* g_search_bar       = nullptr;
static QWidget*              g_search_bar_parent= nullptr;

// 事件过滤器：parent resize 时重新定位搜索栏
class SearchBarResizeFilter : public QObject {
public:
  SearchBarResizeFilter (QObject* parent) : QObject (parent) {}

protected:
  bool eventFilter (QObject* watched, QEvent* event) override {
    if (event->type () == QEvent::Resize && g_search_bar &&
        g_search_bar->isVisible ()) {
      QWidget* w= qobject_cast<QWidget*> (watched);
      if (w) {
        int x= w->width () - g_search_bar->width () - DpiUtils::scaled (8);
        int y= DpiUtils::scaled (4);
        g_search_bar->move (x, y);
      }
    }
    return QObject::eventFilter (watched, event);
  }
};

static SearchBarResizeFilter* g_resize_filter= nullptr;

static void
position_search_bar (QWidget* content) {
  int x= content->width () - g_search_bar->width () - DpiUtils::scaled (8);
  int y= DpiUtils::scaled (4);
  g_search_bar->move (x, y);
}

static void
connect_search_bar_signals (QTMFloatingSearchBar* bar) {
  QObject::connect (bar, &QTMFloatingSearchBar::queryChanged, bar,
                    [bar] (const QString& text) {
                      eval_scheme ("(chat-tab-set-query " *
                                   qt_scheme_quote (text) * ")");
                    });
  QObject::connect (bar, &QTMFloatingSearchBar::findNextRequested, bar,
                    [] () { eval_scheme ("(chat-tab-search-next #t)"); });
  QObject::connect (bar, &QTMFloatingSearchBar::findPreviousRequested, bar,
                    [] () { eval_scheme ("(chat-tab-search-next #f)"); });
  QObject::connect (bar, &QTMFloatingSearchBar::closeRequested, bar, [] () {
    eval_scheme ("(chat-tab-search-close)");
    if (g_search_bar) g_search_bar->hide ();
  });
}

/******************************************************************************
 * Scheme 胶水函数
 ******************************************************************************/

void
qt_floating_search (string flag) {
  ChatController* ctrl= get_chat_controller ();
  if (!ctrl) return;
  QTChatTabWidget* view= ctrl->view ();
  if (!view) return;
  QWidget* content= view->contentWidget ();
  if (!content) return;

  if (flag == "true" || flag == "#t") {
    // 按需创建搜索栏
    if (!g_search_bar || g_search_bar_parent != content) {
      // 删除旧实例（parent 变了）
      if (g_search_bar) {
        delete g_search_bar;
        g_search_bar= nullptr;
      }
      if (g_resize_filter) {
        if (g_search_bar_parent)
          g_search_bar_parent->removeEventFilter (g_resize_filter);
        delete g_resize_filter;
        g_resize_filter= nullptr;
      }

      g_search_bar       = new QTMFloatingSearchBar (content);
      g_search_bar_parent= content;
      g_resize_filter    = new SearchBarResizeFilter (content);
      content->installEventFilter (g_resize_filter);

      g_search_bar->setFixedWidth (DpiUtils::scaled (320));
      connect_search_bar_signals (g_search_bar);
    }
    g_search_bar->activate ();
    position_search_bar (content);
  }
  else {
    if (g_search_bar) g_search_bar->hide ();
  }
}
