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

static constexpr int kContainerBorderWidth = 2;
static constexpr int kContainerBorderRadius= 14;
static constexpr int kShadowBlurRadius     = 12;
static constexpr int kShadowOffsetY        = 4;
static constexpr int kLayoutMargin         = 12; // 外层呼吸边距，防阴影截断
static constexpr int kInnerMarginX         = 10;
static constexpr int kInnerMarginY         = 5;
static constexpr int kInnerSpacing         = 8;

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

  // 生成主水平布局
  layout= new QHBoxLayout (this);
  layout->setContentsMargins (
      DpiUtils::scaled (kLayoutMargin), DpiUtils::scaled (kLayoutMargin),
      DpiUtils::scaled (kLayoutMargin), DpiUtils::scaled (kLayoutMargin));
  layout->setSizeConstraint (QLayout::SetMinimumSize);
  setLayout (layout);

  QFrame* container= new QFrame (this);
  container->setObjectName ("prompt_container");

  // 气泡卡片圆角样式（配色由 liii.css / liii-night.css 按主题提供）
  container->setStyleSheet (
      QString ("QFrame#prompt_container { "
               "border: %1px solid; "
               "border-radius: %2px; "
               "}")
          .arg (DpiUtils::scaled (kContainerBorderWidth))
          .arg (DpiUtils::scaled (kContainerBorderRadius)));

  // 卡片外部淡出的现代软投影
  QGraphicsDropShadowEffect* shadow= new QGraphicsDropShadowEffect (container);
  shadow->setBlurRadius (DpiUtils::scaled (kShadowBlurRadius));
  shadow->setColor (QColor (0, 0, 0, 30));
  shadow->setOffset (0, DpiUtils::scaled (kShadowOffsetY));
  container->setGraphicsEffect (shadow);

  layout->addWidget (container);

  QHBoxLayout* innerLayout= new QHBoxLayout (container);
  innerLayout->setContentsMargins (
      DpiUtils::scaled (kInnerMarginX), DpiUtils::scaled (kInnerMarginY),
      DpiUtils::scaled (kInnerMarginX), DpiUtils::scaled (kInnerMarginY));
  innerLayout->setSpacing (DpiUtils::scaled (kInnerSpacing));
  container->setLayout (innerLayout);

  // 1. 接受按钮
  // 按钮样式全部由 liii.css / liii-night.css 提供：QPushButton 基态下
  // 控件级样式表与应用样式表合并不可靠，会落回通用 QPushButton 规则
  acceptBtn= new QPushButton (acceptText, container);
  acceptBtn->setObjectName ("accept_btn");
  connect (acceptBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleAccept);
  innerLayout->addWidget (acceptBtn);

  // 2. 拒绝按钮
  rejectBtn= new QPushButton (rejectText, container);
  rejectBtn->setObjectName ("reject_btn");
  connect (rejectBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleReject);
  innerLayout->addWidget (rejectBtn);

  // 3. 点赞按钮
  goodBtn= new QPushButton ("👍", container);
  goodBtn->setObjectName ("good_btn");
  connect (goodBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleGood);
  innerLayout->addWidget (goodBtn);

  // 4. 踩按钮
  badBtn= new QPushButton ("👎", container);
  badBtn->setObjectName ("bad_btn");
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
