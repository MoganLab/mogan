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
#include <QSize>

static constexpr int kContainerBorderWidth = 2;
static constexpr int kContainerBorderRadius= 14;
static constexpr int kLayoutMargin         = 12; // 外层呼吸边距，防阴影截断
static constexpr int kInnerMarginX         = 10;
static constexpr int kInnerMarginY         = 5;
static constexpr int kInnerSpacing         = 8;
static constexpr int kButtonBorderRadius   = 8;  // 按钮/图标按钮圆角
static constexpr int kButtonFontPx         = 18; // 按钮字号
static constexpr int kActionButtonPadY     = 6;  // 接受/拒绝按钮纵向内边距
static constexpr int kActionButtonPadX     = 16; // 接受/拒绝按钮横向内边距
static constexpr int kIconButtonPad        = 6;  // 点赞/踩按钮内边距
static constexpr int kIconButtonSize       = 18; // 点赞/踩图标边长

// 按钮配色（含 hover/pressed）由主题 CSS 提供，这里只注入随 DPI 缩放的尺寸，
// 避免与主题 CSS 的颜色规则产生控件级/应用级合并冲突。
static void
applyActionButtonGeometry (QPushButton* btn) {
  btn->setStyleSheet (QString ("QPushButton { "
                               "border-radius: %1px; "
                               "font-weight: bold; "
                               "font-size: %2px; "
                               "padding: %3px %4px; "
                               "}")
                          .arg (DpiUtils::scaled (kButtonBorderRadius))
                          .arg (DpiUtils::scaled (kButtonFontPx))
                          .arg (DpiUtils::scaled (kActionButtonPadY))
                          .arg (DpiUtils::scaled (kActionButtonPadX)));
}

static void
applyIconButtonGeometry (QPushButton* btn) {
  const int size= DpiUtils::scaled (kIconButtonSize);
  btn->setIconSize (QSize (size, size));
  btn->setStyleSheet (QString ("QPushButton { "
                               "border-radius: %1px; "
                               "padding: %2px; "
                               "}")
                          .arg (DpiUtils::scaled (kButtonBorderRadius))
                          .arg (DpiUtils::scaled (kIconButtonPad)));
}

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

  layout->addWidget (container);

  QHBoxLayout* innerLayout= new QHBoxLayout (container);
  innerLayout->setContentsMargins (
      DpiUtils::scaled (kInnerMarginX), DpiUtils::scaled (kInnerMarginY),
      DpiUtils::scaled (kInnerMarginX), DpiUtils::scaled (kInnerMarginY));
  innerLayout->setSpacing (DpiUtils::scaled (kInnerSpacing));
  container->setLayout (innerLayout);

  // 1. 接受按钮
  // 按钮配色（含 hover/pressed）由 liii.css / liii-night.css 提供；
  // 尺寸这里以 DpiUtils 注入。QPushButton 基态下控件级样式表与应用样式表
  // 合并对颜色不可靠，故颜色不放进控件级样式表，仅注入尺寸属性
  acceptBtn= new QPushButton (acceptText, container);
  acceptBtn->setObjectName ("accept_btn");
  applyActionButtonGeometry (acceptBtn);
  connect (acceptBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleAccept);
  innerLayout->addWidget (acceptBtn);

  // 2. 拒绝按钮
  rejectBtn= new QPushButton (rejectText, container);
  rejectBtn->setObjectName ("reject_btn");
  applyActionButtonGeometry (rejectBtn);
  connect (rejectBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleReject);
  innerLayout->addWidget (rejectBtn);

  // 3. 点赞按钮
  goodBtn= new QPushButton ("", container);
  goodBtn->setObjectName ("good_btn");
  applyIconButtonGeometry (goodBtn);
  connect (goodBtn, &QPushButton::clicked, this,
           &QTMUserPromptPopup::handleGood);
  innerLayout->addWidget (goodBtn);

  // 4. 踩按钮
  badBtn= new QPushButton ("", container);
  badBtn->setObjectName ("bad_btn");
  applyIconButtonGeometry (badBtn);
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
