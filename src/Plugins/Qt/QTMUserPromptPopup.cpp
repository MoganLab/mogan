/******************************************************************************
 * MODULE     : QTMUserPromptPopup.cpp
 * DESCRIPTION: Base acceptance and rejection popup
 * COPYRIGHT  : (C) 2026 AcceleratorX
 * *****************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMUserPromptPopup.hpp"
#include "analyze.hpp"
#include "gui.hpp"
#include "qt_dpi_utils.hpp"
#include "server.hpp"
#include <QFrame>
#include <QGraphicsDropShadowEffect>

// =============================================================================
// QTMUserPromptPopup: 用于处理用户与 AI 生成方案的交互，父类是 QTMBasePopup
// =============================================================================
QTMUserPromptPopup::QTMUserPromptPopup (QWidget*              parent,
                                        qt_simple_widget_rep* owner,
                                        const QString&        acceptText,
                                        const QString&        rejectText)
    : QWidget (parent), owner (owner), layout (nullptr) {
  setObjectName ("user_prompt_popup");

  // 必须在 native 句柄生成前直接一步到位地设置 ToolTip 以及透明通道
  setWindowFlags (Qt::ToolTip | Qt::FramelessWindowHint |
                  Qt::WindowStaysOnTopHint);
  setAttribute (Qt::WA_ShowWithoutActivating);
  setAttribute (Qt::WA_DeleteOnClose, false);
  setAttribute (Qt::WA_TranslucentBackground, true);
  setMouseTracking (true);
  setFocusPolicy (Qt::NoFocus);

  // 确保顶层容器本身不填涂任何背景色和边框
  setStyleSheet (
      "QWidget#user_prompt_popup { background: transparent; border: none; }");

  installTopLevelWindowFilter ();

  // 主题切换需重启后生效，构造时判定一次即可
  const bool dark_mode=
      occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet);
  const char* container_bg    = dark_mode ? "#2c2c2c" : "#ffffff";
  const char* container_border= dark_mode ? "#555555" : "#94a3b8";
  const char* accept_bg       = dark_mode ? "#10b981" : "#10b981";
  const char* accept_hover    = dark_mode ? "#34d399" : "#059669";
  const char* accept_pressed  = dark_mode ? "#059669" : "#047857";
  const char* reject_bg       = dark_mode ? "#ef4444" : "#ef4444";
  const char* reject_hover    = dark_mode ? "#f87171" : "#dc2626";
  const char* reject_pressed  = dark_mode ? "#dc2626" : "#b91c1c";
  const char* feedback_fg     = dark_mode ? "#d1d5db" : "#4b5563";
  const char* feedback_hover=
      dark_mode ? "rgba(255, 255, 255, 0.1)" : "#f3f4f6";
  const char* feedback_pressed=
      dark_mode ? "rgba(255, 255, 255, 0.2)" : "#e5e7eb";

  // 生成主水平布局，并外留 12px 的呼吸边距以防边缘阴影被截断
  layout= new QHBoxLayout (this);
  layout->setContentsMargins (DpiUtils::scaled (12), DpiUtils::scaled (12),
                              DpiUtils::scaled (12), DpiUtils::scaled (12));
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  setLayout (layout);

  QFrame* container= new QFrame (this);
  container->setObjectName ("prompt_container");

  // 气泡卡片圆角样式
  container->setStyleSheet (QString ("QFrame#prompt_container { "
                                     "background-color: %1; "
                                     "border: 2px solid %2; "
                                     "border-radius: 14px; "
                                     "}")
                                .arg (container_bg)
                                .arg (container_border));

  // 卡片外部淡出的现代软投影
  QGraphicsDropShadowEffect* shadow= new QGraphicsDropShadowEffect (container);
  shadow->setBlurRadius (12);
  shadow->setColor (dark_mode ? QColor (0, 0, 0, 90) : QColor (0, 0, 0, 30));
  shadow->setOffset (0, 4);
  container->setGraphicsEffect (shadow);

  layout->addWidget (container);

  QHBoxLayout* innerLayout= new QHBoxLayout (container);
  innerLayout->setContentsMargins (DpiUtils::scaled (10), DpiUtils::scaled (5),
                                   DpiUtils::scaled (10), DpiUtils::scaled (5));
  innerLayout->setSpacing (DpiUtils::scaled (8));
  container->setLayout (innerLayout);

  // 1. 接受按钮 (现代祖母绿配色)
  acceptBtn= new QPushButton (acceptText, container);
  acceptBtn->setObjectName ("accept_btn");
  acceptBtn->setStyleSheet (QString ("QPushButton#accept_btn { "
                                     "background-color: %1; "
                                     "color: #ffffff; "
                                     "border: none; "
                                     "border-radius: 8px; "
                                     "font-weight: bold; "
                                     "font-size: 18px; "
                                     "padding: 6px 16px; "
                                     "} "
                                     "QPushButton#accept_btn:hover { "
                                     "background-color: %2; "
                                     "} "
                                     "QPushButton#accept_btn:pressed { "
                                     "background-color: %3; "
                                     "}")
                                .arg (accept_bg)
                                .arg (accept_hover)
                                .arg (accept_pressed));
  connect (acceptBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleAccept);
  innerLayout->addWidget (acceptBtn);

  // 2. 拒绝按钮 (轻柔番茄红配色)
  rejectBtn= new QPushButton (rejectText, container);
  rejectBtn->setObjectName ("reject_btn");
  rejectBtn->setStyleSheet (QString ("QPushButton#reject_btn { "
                                     "background-color: %1; "
                                     "color: #ffffff; "
                                     "border: none; "
                                     "border-radius: 8px; "
                                     "font-weight: bold; "
                                     "font-size: 18px; "
                                     "padding: 6px 16px; "
                                     "} "
                                     "QPushButton#reject_btn:hover { "
                                     "background-color: %2; "
                                     "} "
                                     "QPushButton#reject_btn:pressed { "
                                     "background-color: %3; "
                                     "}")
                                .arg (reject_bg)
                                .arg (reject_hover)
                                .arg (reject_pressed));
  connect (rejectBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleReject);
  innerLayout->addWidget (rejectBtn);

  // 3. 点赞按钮
  goodBtn= new QPushButton ("👍", container);
  goodBtn->setObjectName ("good_btn");
  goodBtn->setStyleSheet (QString ("QPushButton#good_btn { "
                                   "background-color: transparent; "
                                   "color: %1; "
                                   "border: none; "
                                   "border-radius: 8px; "
                                   "font-size: 18px; "
                                   "padding: 6px; "
                                   "} "
                                   "QPushButton#good_btn:hover { "
                                   "background-color: %2; "
                                   "} "
                                   "QPushButton#good_btn:pressed { "
                                   "background-color: %3; "
                                   "}")
                              .arg (feedback_fg)
                              .arg (feedback_hover)
                              .arg (feedback_pressed));
  connect (goodBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleGood);
  innerLayout->addWidget (goodBtn);

  // 4. 踩按钮
  badBtn= new QPushButton ("👎", container);
  badBtn->setObjectName ("bad_btn");
  badBtn->setStyleSheet (QString ("QPushButton#bad_btn { "
                                  "background-color: transparent; "
                                  "color: %1; "
                                  "border: none; "
                                  "border-radius: 8px; "
                                  "font-size: 18px; "
                                  "padding: 6px; "
                                  "} "
                                  "QPushButton#bad_btn:hover { "
                                  "background-color: %2; "
                                  "} "
                                  "QPushButton#bad_btn:pressed { "
                                  "background-color: %3; "
                                  "}")
                             .arg (feedback_fg)
                             .arg (feedback_hover)
                             .arg (feedback_pressed));
  connect (badBtn, &QPushButton::clicked, this, &QTMUserPromptPopup::handleBad);
  innerLayout->addWidget (badBtn);

  adjustSize ();
}

QTMUserPromptPopup::~QTMUserPromptPopup () {}

void
QTMUserPromptPopup::showPopup () {
  updatePosition ();
  if (!isVisible ()) {
    show ();
  }
  raise ();
}

void
QTMUserPromptPopup::updatePosition () {
  int pos_x, pos_y;
  getCachedPosition (pos_x, pos_y);
  move (pos_x, pos_y);
}

void
QTMUserPromptPopup::scrollBy (int x, int y) {
  (void) x;
  (void) y;
  updatePosition ();
}

void
QTMUserPromptPopup::getCachedPosition (int& x, int& y) {
  QTMWidget* canvas= owner ? owner->canvas () : nullptr;
  if (canvas && canvas->surface ()) {
    QPoint cursor_pos      = canvas->cursorPos ();
    QPoint origin          = canvas->origin ();
    QPoint surface_top_left= canvas->surface ()->geometry ().topLeft ();
    QPoint local_pos (cursor_pos.x () - origin.x () + surface_top_left.x (),
                      cursor_pos.y () - origin.y () + surface_top_left.y ());
    QPoint global_pos= canvas->viewport ()->mapToGlobal (local_pos);
    x                = global_pos.x () + DpiUtils::scaled (8);
    y                = global_pos.y () + DpiUtils::scaled (0);
  }
  else {
    x= 0;
    y= 0;
  }
}

void
QTMUserPromptPopup::handleAccept () {
  onAcceptClicked ();
}

void
QTMUserPromptPopup::handleReject () {
  onRejectClicked ();
}

void
QTMUserPromptPopup::handleGood () {
  onGoodClicked ();
}

void
QTMUserPromptPopup::handleBad () {
  onBadClicked ();
}

void
QTMUserPromptPopup::installTopLevelWindowFilter () {
  QWidget* w= parentWidget ();
  while (w) {
    if (w->isWindow ()) {
      w->installEventFilter (this);
      break;
    }
    w= w->parentWidget ();
  }
}

bool
QTMUserPromptPopup::eventFilter (QObject* obj, QEvent* event) {
  if (event->type () == QEvent::WindowStateChange) {
    QWidget* tlw= qobject_cast<QWidget*> (obj);
    if (tlw && tlw->windowState () & Qt::WindowMinimized) {
      hide ();
    }
  }
  else if (event->type () == QEvent::Hide) {
    hide ();
  }
  else if (event->type () == QEvent::WindowDeactivate) {
    hide ();
  }
  return QWidget::eventFilter (obj, event);
}

// =============================================================================
// QTMGhostTextPopup: Ghost Text 自动补全的悬浮操作框，父类是 QTMUserPromptPopup
// =============================================================================
QTMGhostTextPopup::QTMGhostTextPopup (QWidget*              parent,
                                      qt_simple_widget_rep* owner)
    : QTMUserPromptPopup (parent, owner, "接受  →", "拒绝  Esc") {
  setObjectName ("ghost_text_popup");
}

QTMGhostTextPopup::~QTMGhostTextPopup () {}

void
QTMGhostTextPopup::onAcceptClicked () {
  hide ();
  call ("accept-ghost");
}

void
QTMGhostTextPopup::onRejectClicked () {
  hide ();
  call ("reject-ghost");
}

void
QTMGhostTextPopup::onGoodClicked () {
  call ("ghost-feedback", "good");
}

void
QTMGhostTextPopup::onBadClicked () {
  call ("ghost-feedback", "bad");
}

// =============================================================================
// QTMDiffTextPopup: Diff Text AI建议的悬浮操作框，父类是 QTMUserPromptPopup
// =============================================================================
QTMDiffTextPopup::QTMDiffTextPopup (QWidget*              parent,
                                    qt_simple_widget_rep* owner)
    : QTMUserPromptPopup (parent, owner, "接受  Enter", "拒绝  Backspace") {
  setObjectName ("diff_text_popup");
}

QTMDiffTextPopup::~QTMDiffTextPopup () {}

void
QTMDiffTextPopup::onAcceptClicked () {
  hide ();
  call ("accept-diff");
}

void
QTMDiffTextPopup::onRejectClicked () {
  hide ();
  call ("reject-diff");
}

void
QTMDiffTextPopup::onGoodClicked () {
  call ("diff-feedback", "good");
}

void
QTMDiffTextPopup::onBadClicked () {
  call ("diff-feedback", "bad");
}
