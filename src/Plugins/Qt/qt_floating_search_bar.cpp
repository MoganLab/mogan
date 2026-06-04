
/******************************************************************************
 * MODULE     : qt_floating_search_bar.cpp
 * DESCRIPTION: A VSCode-style floating search bar widget
 * COPYRIGHT  : (C) 2026  Yuki Lu
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_floating_search_bar.hpp"
#include "qt_chat_tab_widget.hpp"
#include "qt_dpi_utils.hpp"
#include "qt_utilities.hpp"
#include "qt_widget.hpp"

#include "s7_tm.hpp"
#include "tm_window.hpp"

#include <moebius/tree_label.hpp>

#include <QAbstractScrollArea>
#include <QDockWidget>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPalette>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

using namespace moebius;

// ---- 尺寸常量（逻辑像素，经 DpiUtils 缩放） ----
constexpr int kBarMinHeight= 64;
constexpr int kBarWidth    = 420;
constexpr int kBarRadius   = 4;
constexpr int kBarMargin   = 6;
constexpr int kBarSpacing  = 4;

constexpr int kBtnSize  = 24;
constexpr int kBtnRadius= 12;

constexpr int kInfoHeight= 24;

constexpr int kShadowBlur   = 8;
constexpr int kShadowOffsetY= 1;
constexpr int kShadowAlpha  = 30;

constexpr int kPosRightPad = 8;
constexpr int kPosTopPad   = 4;
constexpr int kInnerSpacing= 4;

/******************************************************************************
 * QTMFloatingSearchBar 实现
 ******************************************************************************/

QTMFloatingSearchBar::QTMFloatingSearchBar (QWidget* parent)
    : QWidget (parent) {
  setObjectName ("floating_search_bar");
  setWindowFlags (Qt::Widget);
  setAttribute (Qt::WA_StyledBackground);
  setMinimumHeight (DpiUtils::scaled (kBarMinHeight));

  // border-radius 由代码动态计算，支持 DPI 缩放
  setStyleSheet (QString ("#floating_search_bar {"
                          "  border-radius: %1px;"
                          "}")
                     .arg (DpiUtils::scaled (kBarRadius)));

  auto* shadow= new QGraphicsDropShadowEffect (this);
  shadow->setBlurRadius (DpiUtils::scaled (kShadowBlur));
  shadow->setOffset (0, DpiUtils::scaled (kShadowOffsetY));
  shadow->setColor (QColor (0, 0, 0, kShadowAlpha));
  setGraphicsEffect (shadow);

  // 外层水平布局：左边 [输入区]，右边 [按钮 + 匹配信息]
  auto* mainLayout= new QHBoxLayout (this);
  mainLayout->setContentsMargins (
      DpiUtils::scaled (kBarMargin), DpiUtils::scaled (kBarMargin),
      DpiUtils::scaled (kBarMargin), DpiUtils::scaled (kBarMargin));
  mainLayout->setSpacing (DpiUtils::scaled (kBarSpacing));

  // 左侧：输入区（由 setSearchInput 动态插入，占满左侧）
  rowLayout_= new QHBoxLayout ();
  rowLayout_->setSpacing (0);

  // 右侧：垂直布局 [按钮行] + [匹配信息]
  auto* rightLayout= new QVBoxLayout ();
  rightLayout->setSpacing (DpiUtils::scaled (kInnerSpacing));

  // 右侧上层：按钮行
  auto* btnRow= new QHBoxLayout ();
  btnRow->setSpacing (DpiUtils::scaled (kInnerSpacing));

  const QString btnRadiusStyle=
      QString (
          "QToolButton { border-radius: %1px; padding: 0px; margin: 0px; }")
          .arg (DpiUtils::scaled (kBtnRadius));

  modeBtn_= new QToolButton (this);
  modeBtn_->setObjectName ("floating-search-mode-text");
  modeBtn_->setFixedSize (DpiUtils::scaled (kBtnSize),
                          DpiUtils::scaled (kBtnSize));
#ifdef Q_OS_MAC
  modeBtn_->setToolTip (qt_translate ("text mode (Option+Tab)"));
#else
  modeBtn_->setToolTip (qt_translate ("text mode (Ctrl+Tab)"));
#endif
  modeBtn_->setStyleSheet (btnRadiusStyle);
  btnRow->addWidget (modeBtn_);

  auto* prevBtn= new QToolButton (this);
  prevBtn->setObjectName ("floating-search-prev");
  prevBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                         DpiUtils::scaled (kBtnSize));
#ifdef Q_OS_MAC
  prevBtn->setToolTip (qt_translate ("Previous (Cmd+Enter)"));
#else
  prevBtn->setToolTip (qt_translate ("Previous (Ctrl+Enter)"));
#endif
  prevBtn->setStyleSheet (btnRadiusStyle);
  btnRow->addWidget (prevBtn);

  auto* nextBtn= new QToolButton (this);
  nextBtn->setObjectName ("floating-search-next");
  nextBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                         DpiUtils::scaled (kBtnSize));
  nextBtn->setToolTip (qt_translate ("Next (Enter)"));
  nextBtn->setStyleSheet (btnRadiusStyle);
  btnRow->addWidget (nextBtn);

  auto* closeBtn= new QToolButton (this);
  closeBtn->setObjectName ("floating-search-close");
  closeBtn->setFixedSize (DpiUtils::scaled (kBtnSize),
                          DpiUtils::scaled (kBtnSize));
  closeBtn->setToolTip (qt_translate ("Close (Esc)"));
  closeBtn->setStyleSheet (btnRadiusStyle);
  btnRow->addWidget (closeBtn);

  rightLayout->addLayout (btnRow);

  // 右侧下层：匹配信息
  infoLbl_= new QLabel (this);
  infoLbl_->setObjectName ("floating-search-info");
  infoLbl_->setFixedHeight (DpiUtils::scaled (kInfoHeight));
  infoLbl_->setAlignment (Qt::AlignCenter);
  infoLbl_->setText (qt_translate ("No matches"));
  rightLayout->addWidget (infoLbl_);

  // 组装：左输入(stretch=1) + 右面板
  mainLayout->addLayout (rowLayout_, 1);
  mainLayout->addLayout (rightLayout);

  connect (nextBtn, &QToolButton::clicked, this,
           &QTMFloatingSearchBar::findNextRequested);
  connect (prevBtn, &QToolButton::clicked, this,
           &QTMFloatingSearchBar::findPreviousRequested);
  connect (closeBtn, &QToolButton::clicked, this,
           &QTMFloatingSearchBar::closeRequested);
  connect (modeBtn_, &QToolButton::clicked, this,
           &QTMFloatingSearchBar::toggleMode);

  if (parent) parent->installEventFilter (this);

  hide ();
}

QTMFloatingSearchBar::~QTMFloatingSearchBar () {
  if (parent ()) parent ()->removeEventFilter (this);
}

void
QTMFloatingSearchBar::setSearchInput (QWidget* input) {
  if (inputQW_) {
    if (inputScrollArea_) {
      inputScrollArea_->removeEventFilter (this);
      inputScrollArea_= nullptr;
    }
    rowLayout_->removeWidget (inputQW_);
    inputQW_->deleteLater ();
  }
  inputQW_= input;
  if (input) {
    input->setObjectName ("floating-search-input");
    rowLayout_->insertWidget (0, input, 1);
    // texmacs_input_widget 内部的 QAbstractScrollArea 可能延迟创建（show 时），
    // 所有 scroll area 设置统一延迟到事件循环处理
    QMetaObject::invokeMethod (
        this,
        [this] {
          if (inputQW_) {
            QAbstractScrollArea* sa=
                inputQW_->findChild<QAbstractScrollArea*> ();
            if (sa) {
              sa->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
              sa->setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
              sa->viewport ()->setBackgroundRole (QPalette::Base);
              sa->installEventFilter (this);
              sa->viewport ()->installEventFilter (this);
              sa->setFocus ();
              inputScrollArea_= sa;
            }
            else {
              inputScrollArea_= nullptr;
              inputQW_->setFocus ();
            }
          }
        },
        Qt::QueuedConnection);
  }
}

void
QTMFloatingSearchBar::activate () {
  show ();
  raise ();
  if (inputQW_) {
    QAbstractScrollArea* sa= inputScrollArea_;
    if (!sa) sa= inputQW_->findChild<QAbstractScrollArea*> ();
    if (sa) sa->setFocus ();
    else inputQW_->setFocus ();
  }
}

void
QTMFloatingSearchBar::setMatchInfo (int current, int total) {
  if (total == 0) infoLbl_->setText (qt_translate ("No matches"));
  else infoLbl_->setText (qt_translate ("%1 of %2").arg (current).arg (total));
}

void
QTMFloatingSearchBar::setSchemeCallbacks (const string& next_cmd,
                                          const string& prev_cmd,
                                          const string& close_cmd) {
  next_cmd_ = next_cmd;
  prev_cmd_ = prev_cmd;
  close_cmd_= close_cmd;
  if (!callbacksConnected_) {
    connectSignals ();
    callbacksConnected_= true;
  }
}

void
QTMFloatingSearchBar::connectSignals () {
  if (!is_empty (next_cmd_)) {
    connect (this, &QTMFloatingSearchBar::findNextRequested, this,
             [this] () { eval_scheme (next_cmd_); });
  }
  if (!is_empty (prev_cmd_)) {
    connect (this, &QTMFloatingSearchBar::findPreviousRequested, this,
             [this] () { eval_scheme (prev_cmd_); });
  }
  // close: 始终 hide，callback 非空时才调 eval_scheme
  connect (this, &QTMFloatingSearchBar::closeRequested, this, [this] () {
    if (!is_empty (close_cmd_)) eval_scheme (close_cmd_);
    hide ();
  });
}

void
QTMFloatingSearchBar::setModeIcon (bool mathMode) {
  if (!modeBtn_) return;
  mathMode_= mathMode;
  modeBtn_->setObjectName (mathMode ? "floating-search-mode-math"
                                    : "floating-search-mode-text");
  modeBtn_->style ()->unpolish (modeBtn_);
  modeBtn_->style ()->polish (modeBtn_);
#ifdef Q_OS_MAC
  modeBtn_->setToolTip (mathMode ? qt_translate ("math mode (Option+Tab)")
                                 : qt_translate ("text mode (Option+Tab)"));
#else
  modeBtn_->setToolTip (mathMode ? qt_translate ("math mode (Ctrl+Tab)")
                                 : qt_translate ("text mode (Ctrl+Tab)"));
#endif
}

void
QTMFloatingSearchBar::toggleMode () {
  bool isMath= !mathMode_;
  setModeIcon (isMath);
  eval_scheme ("(floating-search-toggle-mode)");
}

bool
QTMFloatingSearchBar::eventFilter (QObject* watched, QEvent* event) {
  if (event->type () == QEvent::Resize && watched == parent () &&
      isVisible ()) {
    reposition ();
  }
  if (event->type () == QEvent::KeyPress && isVisible () && inputQW_) {
    auto*                ke= static_cast<QKeyEvent*> (event);
    QAbstractScrollArea* sa= inputScrollArea_;

    if (ke->key () == Qt::Key_Escape) {
      if (sa && watched == sa) {
        emit closeRequested ();
        return true;
      }
    }
    // Ctrl/Cmd+J：切换 AI 聊天侧边栏
    if (ke->key () == Qt::Key_J &&
        (ke->modifiers () & (Qt::ControlModifier | Qt::MetaModifier))) {
      QMainWindow* mw= qobject_cast<QMainWindow*> (window ());
      if (mw) {
        QDockWidget* dock= mw->findChild<QDockWidget*> ("chatSideDock");
        if (dock) {
          if (dock->isVisible ()) {
            QTChatTabWidget* chatWidget=
                qobject_cast<QTChatTabWidget*> (dock->widget ());
            if (chatWidget) emit chatWidget->closeSidebarRequested ();
          }
          else {
            QPushButton* toggleBtn=
                mw->findChild<QPushButton*> ("chat-tab-collapse-btn");
            if (toggleBtn) toggleBtn->click ();
          }
        }
      }
      return true;
    }
#ifdef Q_OS_MAC
    if (ke->key () == Qt::Key_Tab && (ke->modifiers () & Qt::AltModifier)) {
#else
    if (ke->key () == Qt::Key_Tab && (ke->modifiers () & Qt::ControlModifier)) {
#endif
      toggleMode ();
      return true;
    }

    // 输入区内非修饰键/非导航键按键：触发实时搜索更新
    if (sa && watched == sa) {
      switch (ke->key ()) {
      case Qt::Key_Shift:
      case Qt::Key_Control:
      case Qt::Key_Meta:
      case Qt::Key_Alt:
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Up:
      case Qt::Key_Down:
      case Qt::Key_Home:
      case Qt::Key_End:
      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
        break;
      default:
        QMetaObject::invokeMethod (
            this, [this] () { eval_scheme ("(floating-search-on-input)"); },
            Qt::QueuedConnection);
        break;
      }
    }
  }
  return QWidget::eventFilter (watched, event);
}

void
QTMFloatingSearchBar::showEvent (QShowEvent* event) {
  QWidget::showEvent (event);
  reposition ();
}

void
QTMFloatingSearchBar::reposition () {
  QWidget* p= qobject_cast<QWidget*> (parent ());
  if (!p) return;
  int x= p->width () - width () - DpiUtils::scaled (kPosRightPad);
  int y= DpiUtils::scaled (kPosTopPad);
  move (x, y);
}

/******************************************************************************
 * 搜索栏管理器
 ******************************************************************************/

static QHash<QWidget*, QTMFloatingSearchBar*>&
searchBars () {
  static QHash<QWidget*, QTMFloatingSearchBar*> bars;
  return bars;
}

static QTMFloatingSearchBar*
get_or_create_bar (QWidget* parent) {
  if (!parent) return nullptr;
  auto& bars= searchBars ();
  auto  it  = bars.find (parent);
  if (it != bars.end () && *it) return *it;

  auto* bar   = new QTMFloatingSearchBar (parent);
  bars[parent]= bar;

  QWidget* pw= parent;
  QObject::connect (parent, &QObject::destroyed,
                    [pw] () { searchBars ().remove (pw); });

  bar->setFixedWidth (DpiUtils::scaled (kBarWidth));
  return bar;
}

void
qt_floating_search_bar_show (QWidget* parent, bool show) {
  if (!parent) return;
  auto* bar= get_or_create_bar (parent);
  if (!bar) return;
  if (show) bar->show ();
  else bar->hide ();
}

bool
qt_floating_search_bar_init (QWidget* parent, const string& aux_url_str,
                             const string& mode) {
  if (!parent) return false;
  auto* bar= get_or_create_bar (parent);

  url   aux_url   = url_system (aux_url_str);
  qreal searchZoom= DpiUtils::scaled (100) / 100.0;
  tree  doc;
  if (mode == "math") {
    doc= tree (WITH, "font", "sys-chinese", "zoom-factor",
               as_string (searchZoom), "mode", "math", tree (DOCUMENT, ""));
  }
  else {
    doc= tree (WITH, "font", "sys-chinese", "zoom-factor",
               as_string (searchZoom), tree (DOCUMENT, ""));
  }
  tree   sty= compound ("style", tree (TUPLE, "generic"));
  widget tw = texmacs_input_widget (doc, sty, aux_url);
  set_zoom_factor (tw, searchZoom);
  if (is_nil (tw)) {
    bar->hide ();
    return false;
  }
  QWidget* inputW= concrete (tw)->as_qwidget ();
  if (!inputW) {
    bar->hide ();
    return false;
  }
  bar->setSearchInput (inputW);
  return true;
}

void
qt_floating_search_bar_set_match_info (QWidget* parent, int current,
                                       int total) {
  if (!parent) return;
  auto* bar= searchBars ().value (parent);
  if (bar) bar->setMatchInfo (current, total);
}

void
qt_floating_search_bar_set_callbacks (QWidget* parent, const string& next_cmd,
                                      const string& prev_cmd,
                                      const string& close_cmd) {
  if (!parent) return;
  auto* bar= get_or_create_bar (parent);
  bar->setSchemeCallbacks (next_cmd, prev_cmd, close_cmd);
}

void
qt_floating_search_bar_destroy (QWidget* parent) {
  if (!parent) return;
  auto& bars= searchBars ();
  auto  it  = bars.find (parent);
  if (it != bars.end ()) {
    (*it)->setParent (nullptr);
    delete *it;
    bars.erase (it);
  }
}

/******************************************************************************
 * 兼容层胶水函数（通过 provider 代理）
 ******************************************************************************/

static qt_floating_search_parent_provider g_parent_provider;

void
qt_floating_search_set_parent_provider (
    qt_floating_search_parent_provider provider) {
  g_parent_provider= provider;
}

static QWidget*
get_provider_parent () {
  return g_parent_provider ? g_parent_provider () : nullptr;
}

void
qt_floating_search (string flag) {
  QWidget* parent= get_provider_parent ();
  if (!parent) return;
  bool show= (flag == "true" || flag == "#t");
  if (show) {
    auto* bar= get_or_create_bar (parent);
    if (bar) bar->activate ();
  }
  else {
    qt_floating_search_bar_show (parent, false);
  }
}

void
qt_floating_search_init (string aux_url_str, string mode) {
  QWidget* parent= get_provider_parent ();
  if (!parent) return;
  qt_floating_search_bar_init (parent, aux_url_str, mode);
}

void
qt_floating_search_set_match_info (int current, int total) {
  QWidget* parent= get_provider_parent ();
  if (!parent) return;
  qt_floating_search_bar_set_match_info (parent, current, total);
}

void
qt_floating_search_set_callbacks (string next_cmd, string prev_cmd,
                                  string close_cmd) {
  QWidget* parent= get_provider_parent ();
  if (!parent) return;
  qt_floating_search_bar_set_callbacks (parent, next_cmd, prev_cmd, close_cmd);
}
