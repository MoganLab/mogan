
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

#include <moebius/tree_label.hpp>

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QVBoxLayout>

using namespace moebius;

// ---- 尺寸常量（单位：逻辑像素，经 DpiUtils::scaled 缩放） ---- //
constexpr int kBarMinHeight= 64;
constexpr int kBarWidth    = 420;
constexpr int kBarRadius   = 4;
constexpr int kBarMargin   = 6;
constexpr int kBarSpacing  = 4;

constexpr int kBtnSize  = 24;
constexpr int kBtnRadius= 12;

constexpr int kInfoHeight  = 24;
constexpr int kInfoMinWidth= 80;

constexpr int kShadowBlur   = 8;
constexpr int kShadowOffsetY= 1;
constexpr int kShadowAlpha  = 30;

constexpr int kPosRightPad= 8;
constexpr int kPosTopPad  = 4;

/******************************************************************************
 * QTMFloatingSearchBar
 ******************************************************************************/

QTMFloatingSearchBar::QTMFloatingSearchBar (QWidget* parent)
    : QWidget (parent) {
  setObjectName ("floating_search_bar");
  setWindowFlags (Qt::Widget);
  setAttribute (Qt::WA_StyledBackground);
  setMinimumHeight (DpiUtils::scaled (kBarMinHeight));

  setStyleSheet (QString ("#floating_search_bar {"
                          "  background: #f3f3f3;"
                          "  border: none;"
                          "  border-radius: %1px;"
                          "}")
                     .arg (DpiUtils::scaled (kBarRadius)));

  QGraphicsDropShadowEffect* shadow= new QGraphicsDropShadowEffect (this);
  shadow->setBlurRadius (DpiUtils::scaled (kShadowBlur));
  shadow->setOffset (0, DpiUtils::scaled (kShadowOffsetY));
  shadow->setColor (QColor (0, 0, 0, kShadowAlpha));
  setGraphicsEffect (shadow);

  const QString btnStyle= QString ("QPushButton {"
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
                              .arg (DpiUtils::scaled (kBtnRadius));

  // 外层水平布局：左边 [输入区]，右边 [按钮 + 匹配信息]
  QHBoxLayout* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (
      DpiUtils::scaled (kBarMargin), DpiUtils::scaled (kBarMargin),
      DpiUtils::scaled (kBarMargin), DpiUtils::scaled (kBarMargin));
  mainLayout->setSpacing (DpiUtils::scaled (kBarSpacing));

  // 左侧：输入区（由 setSearchInput 动态插入，占满左侧）
  rowLayout_= new QHBoxLayout ();
  rowLayout_->setSpacing (0);

  // 右侧：垂直布局 [按钮行] + [匹配信息]
  QVBoxLayout* rightLayout= new QVBoxLayout ();
  rightLayout->setSpacing (DpiUtils::scaled (4));

  // 右侧上层：按钮行
  QHBoxLayout* btnRow= new QHBoxLayout ();
  btnRow->setSpacing (DpiUtils::scaled (4));

  QPushButton* prevBtn= new QPushButton (this);
  prevBtn->setIcon (QIcon (":floating-search/up.svg"));
  prevBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                         DpiUtils::scaled (kBtnSize));
  prevBtn->setToolTip (tr ("Previous (Shift+Enter)"));
  prevBtn->setStyleSheet (btnStyle);
  btnRow->addWidget (prevBtn);

  QPushButton* nextBtn= new QPushButton (this);
  nextBtn->setIcon (QIcon (":floating-search/down.svg"));
  nextBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                         DpiUtils::scaled (kBtnSize));
  nextBtn->setToolTip (tr ("Next (Enter)"));
  nextBtn->setStyleSheet (btnStyle);
  btnRow->addWidget (nextBtn);

  QPushButton* closeBtn= new QPushButton (this);
  closeBtn->setIcon (QIcon (":tabpage/close.svg"));
  closeBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                          DpiUtils::scaled (kBtnSize));
  closeBtn->setToolTip (tr ("Close (Esc)"));
  closeBtn->setStyleSheet (btnStyle);
  btnRow->addWidget (closeBtn);

  rightLayout->addLayout (btnRow);

  // 右侧下层：匹配信息
  infoLbl_= new QLabel (this);
  infoLbl_->setFixedHeight (DpiUtils::scaled (kInfoHeight));
  infoLbl_->setAlignment (Qt::AlignCenter);
  infoLbl_->setText (QString::fromUtf8 ("无匹配"));
  rightLayout->addWidget (infoLbl_);

  // 组装：左输入(stretch=1) + 右面板
  mainLayout->addLayout (rowLayout_, 1);
  mainLayout->addLayout (rightLayout);

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
    rowLayout_->removeWidget (inputQW_);
    delete inputQW_;
  }
  inputQW_= input;
  rowLayout_->insertWidget (0, input, 1);
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
 * 浮动搜索栏管理
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
        int x= w->width () - g_search_bar->width () -
               DpiUtils::scaled (kPosRightPad);
        int y= DpiUtils::scaled (kPosTopPad);
        g_search_bar->move (x, y);
      }
    }
    return QObject::eventFilter (watched, event);
  }
};

static SearchBarResizeFilter* g_resize_filter= nullptr;

static void
position_search_bar () {
  if (!g_search_bar || !g_search_bar_parent) return;
  int x= g_search_bar_parent->width () - g_search_bar->width () -
         DpiUtils::scaled (kPosRightPad);
  int y= DpiUtils::scaled (kPosTopPad);
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

/// 创建或重建浮动栏容器，确保 attach 到 content。
static void
ensure_search_bar (QWidget* content) {
  if (g_search_bar && g_search_bar_parent == content) return;

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

  // content 被 Qt 销毁时自动清理全局指针，避免悬空
  QObject::connect (content, &QObject::destroyed, [] () {
    g_search_bar       = nullptr;
    g_search_bar_parent= nullptr;
    g_resize_filter    = nullptr;
  });

  g_search_bar->setFixedWidth (DpiUtils::scaled (kBarWidth));
  connect_search_bar_signals (g_search_bar);
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
    ensure_search_bar (content);
    g_search_bar->activate ();
    position_search_bar ();
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

  ensure_search_bar (content);

  // 创建 texmacs_input_widget 绑定到 search-buffer
  url    aux_url= url_system (aux_url_str);
  tree   doc (DOCUMENT, "");
  tree   sty= compound ("style", tree (TUPLE, "generic"));
  widget tw = texmacs_input_widget (doc, sty, aux_url);
  if (is_nil (tw)) return;
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
