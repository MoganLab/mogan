
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
#include "qt_widget.hpp"

#include "s7_tm.hpp"
#include "tm_window.hpp"
#include "widget.hpp"

#include <moebius/tree_label.hpp>

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStyle>

using namespace moebius;

/******************************************************************************
 * QTMFloatingSearchBar
 ******************************************************************************/

QTMFloatingSearchBar::QTMFloatingSearchBar (QWidget* parent)
    : QWidget (parent) {
  setObjectName ("floating_search_bar");
  setWindowFlags (Qt::Widget);
  setAttribute (Qt::WA_StyledBackground);
  setMinimumHeight (DpiUtils::scaled (44));

  // 圆角、去边框、悬浮阴影
  QString btnStyle= QString ("QPushButton {"
                             "  border: none;"
                             "  border-radius: %1px;"
                             "  background: transparent;"
                             "}"
                             "QPushButton:hover {"
                             "  background: rgba(0,0,0,30);"
                             "}"
                             "QPushButton:pressed {"
                             "  background: rgba(0,0,0,50);"
                             "}")
                        .arg (DpiUtils::scaled (12));
  setStyleSheet (QString ("#floating_search_bar {"
                          "  background: #f3f3f3;"
                          "  border: none;"
                          "  border-radius: %1px;"
                          "}")
                     .arg (DpiUtils::scaled (4)));
  QGraphicsDropShadowEffect* shadow= new QGraphicsDropShadowEffect (this);
  shadow->setBlurRadius (DpiUtils::scaled (8));
  shadow->setOffset (0, DpiUtils::scaled (1));
  shadow->setColor (QColor (0, 0, 0, 30));
  setGraphicsEffect (shadow);

  layout_= new QHBoxLayout (this);
  layout_->setContentsMargins (DpiUtils::scaled (6), DpiUtils::scaled (6),
                               DpiUtils::scaled (6), DpiUtils::scaled (6));
  layout_->setSpacing (DpiUtils::scaled (4));

  // 布局顺序: [输入区] [匹配计数] [上一个] [下一个] [关闭]
  // 输入区由 setSearchInput 动态插入 index 0

  // 匹配计数标签
  infoLbl_= new QLabel (this);
  infoLbl_->setFixedHeight (DpiUtils::scaled (24));
  infoLbl_->setMinimumWidth (DpiUtils::scaled (80));
  infoLbl_->setAlignment (Qt::AlignCenter);
  infoLbl_->setText (QString::fromUtf8 ("无匹配"));
  layout_->addWidget (infoLbl_);

  // 上一个按钮
  QPushButton* prevBtn= new QPushButton (this);
  prevBtn->setIcon (QIcon (":floating-search/up.svg"));
  prevBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  prevBtn->setToolTip (tr ("Previous (Shift+Enter)"));
  prevBtn->setStyleSheet (btnStyle);
  layout_->addWidget (prevBtn);

  // 下一个按钮
  QPushButton* nextBtn= new QPushButton (this);
  nextBtn->setIcon (QIcon (":floating-search/down.svg"));
  nextBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  nextBtn->setToolTip (tr ("Next (Enter)"));
  nextBtn->setStyleSheet (btnStyle);
  layout_->addWidget (nextBtn);

  // 关闭按钮
  QPushButton* closeBtn= new QPushButton (this);
  closeBtn->setIcon (QIcon (":tabpage/close.svg"));
  closeBtn->setFixedSize (DpiUtils::scaled (24), DpiUtils::scaled (24));
  closeBtn->setToolTip (tr ("Close (Esc)"));
  closeBtn->setStyleSheet (btnStyle);
  layout_->addWidget (closeBtn);

  QObject::connect (nextBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::findNextRequested);
  QObject::connect (prevBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::findPreviousRequested);
  QObject::connect (closeBtn, &QPushButton::clicked, this,
                    &QTMFloatingSearchBar::closeRequested);

  hide ();
}

void
QTMFloatingSearchBar::setSearchInput (QWidget* input) {
  if (inputQW_) {
    layout_->removeWidget (inputQW_);
    delete inputQW_;
  }
  inputQW_= input;
  layout_->insertWidget (0, input, 1);
}

void
QTMFloatingSearchBar::activate () {
  show ();
  raise ();
  if (inputQW_) inputQW_->setFocus ();
}

void
QTMFloatingSearchBar::setMatchInfo (int current, int total) {
  if (total == 0) infoLbl_->setText (tr ("No matches"));
  else infoLbl_->setText (tr ("%1 of %2").arg (current).arg (total));
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
    // 按需创建浮动栏容器
    if (!g_search_bar || g_search_bar_parent != content) {
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

      g_search_bar->setFixedWidth (DpiUtils::scaled (360));
      connect_search_bar_signals (g_search_bar);
    }

    g_search_bar->activate ();
    position_search_bar (content);
  }
  else {
    if (g_search_bar) g_search_bar->hide ();
  }
}

void
qt_floating_search_init (string aux_url_str) {
  ChatController* ctrl= get_chat_controller ();
  if (!ctrl) return;
  QTChatTabWidget* view= ctrl->view ();
  if (!view) return;
  QWidget* content= view->contentWidget ();
  if (!content) return;

  // 确保浮动栏容器存在
  if (!g_search_bar || g_search_bar_parent != content) {
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

    g_search_bar->setFixedWidth (DpiUtils::scaled (420));
    connect_search_bar_signals (g_search_bar);
  }

  // 创建 texmacs_input_widget 绑定到 search-buffer
  url      aux_url= url_system (aux_url_str);
  tree     doc (DOCUMENT, "");
  tree     sty   = compound ("style", tree (TUPLE, "generic"));
  widget   tw    = texmacs_input_widget (doc, sty, aux_url);
  QWidget* inputW= concrete (tw)->as_qwidget ();
  if (inputW) {
    inputW->setStyleSheet ("QWidget {"
                           "  background: white;"
                           "  border: 1px solid #d0d0d0;"
                           "}"
                           "QWidget:focus {"
                           "  border: 1px solid #215a6a;"
                           "}");
    g_search_bar->setSearchInput (inputW);
  }
}
