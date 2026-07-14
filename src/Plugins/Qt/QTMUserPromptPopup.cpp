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
#include "qt_dpi_utils.hpp"
#include "server.hpp"
#include <QFrame>
#include <QGraphicsDropShadowEffect>

// =============================================================================
// QTMUserPromptPopup: 用于处理用户与 AI 生成方案的交互，父类是 QTMBasePopup
// =============================================================================
QTMUserPromptPopup::QTMUserPromptPopup (QWidget*              parent,
                                        qt_simple_widget_rep* owner)
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

  // 生成主水平布局，并外留 12px 的呼吸边距以防边缘阴影被截断
  layout= new QHBoxLayout (this);
  layout->setContentsMargins (DpiUtils::scaled (12), DpiUtils::scaled (12),
                              DpiUtils::scaled (12), DpiUtils::scaled (12));
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  setLayout (layout);

  QFrame* container= new QFrame (this);
  container->setObjectName ("prompt_container");

  // 气泡卡片白底圆角样式
  container->setStyleSheet ("QFrame#prompt_container { "
                            "background-color: #ffffff; "
                            "border: 2px solid #94a3b8; "
                            "border-radius: 14px; "
                            "}");

  // 卡片外部淡出的现代软投影
  QGraphicsDropShadowEffect* shadow= new QGraphicsDropShadowEffect (container);
  shadow->setBlurRadius (12);
  shadow->setColor (QColor (0, 0, 0, 30));
  shadow->setOffset (0, 4);
  container->setGraphicsEffect (shadow);

  layout->addWidget (container);

  QHBoxLayout* innerLayout= new QHBoxLayout (container);
  innerLayout->setContentsMargins (DpiUtils::scaled (10), DpiUtils::scaled (5),
                                   DpiUtils::scaled (10), DpiUtils::scaled (5));
  innerLayout->setSpacing (DpiUtils::scaled (8));
  container->setLayout (innerLayout);

  // 1. 接受按钮 (现代祖母绿配色)
  acceptBtn= new QPushButton ("接受  →", container);
  acceptBtn->setObjectName ("accept_btn");
  acceptBtn->setStyleSheet ("QPushButton#accept_btn { "
                            "background-color: #10b981; "
                            "color: #ffffff; "
                            "border: none; "
                            "border-radius: 8px; "
                            "font-weight: bold; "
                            "font-size: 18px; "
                            "padding: 6px 16px; "
                            "} "
                            "QPushButton#accept_btn:hover { "
                            "background-color: #059669; "
                            "} "
                            "QPushButton#accept_btn:pressed { "
                            "background-color: #047857; "
                            "}");
  connect (acceptBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleAccept);
  innerLayout->addWidget (acceptBtn);

  // 2. 拒绝按钮 (轻柔番茄红配色)
  rejectBtn= new QPushButton ("拒绝  Esc", container);
  rejectBtn->setObjectName ("reject_btn");
  rejectBtn->setStyleSheet ("QPushButton#reject_btn { "
                            "background-color: #ef4444; "
                            "color: #ffffff; "
                            "border: none; "
                            "border-radius: 8px; "
                            "font-weight: bold; "
                            "font-size: 18px; "
                            "padding: 6px 16px; "
                            "} "
                            "QPushButton#reject_btn:hover { "
                            "background-color: #dc2626; "
                            "} "
                            "QPushButton#reject_btn:pressed { "
                            "background-color: #b91c1c; "
                            "}");
  connect (rejectBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleReject);
  innerLayout->addWidget (rejectBtn);

  // 3. 点赞按钮
  goodBtn= new QPushButton ("👍", container);
  goodBtn->setObjectName ("good_btn");
  goodBtn->setStyleSheet ("QPushButton#good_btn { "
                          "background-color: transparent; "
                          "color: #4b5563; "
                          "border: none; "
                          "border-radius: 8px; "
                          "font-size: 18px; "
                          "padding: 6px; "
                          "} "
                          "QPushButton#good_btn:hover { "
                          "background-color: #f3f4f6; "
                          "} "
                          "QPushButton#good_btn:pressed { "
                          "background-color: #e5e7eb; "
                          "}");
  connect (goodBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleGood);
  innerLayout->addWidget (goodBtn);

  // 4. 踩按钮
  badBtn= new QPushButton ("👎", container);
  badBtn->setObjectName ("bad_btn");
  badBtn->setStyleSheet ("QPushButton#bad_btn { "
                         "background-color: transparent; "
                         "color: #4b5563; "
                         "border: none; "
                         "border-radius: 8px; "
                         "font-size: 18px; "
                         "padding: 6px; "
                         "} "
                         "QPushButton#bad_btn:hover { "
                         "background-color: #f3f4f6; "
                         "} "
                         "QPushButton#bad_btn:pressed { "
                         "background-color: #e5e7eb; "
                         "}");
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
    : QTMUserPromptPopup (parent, owner) {
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
