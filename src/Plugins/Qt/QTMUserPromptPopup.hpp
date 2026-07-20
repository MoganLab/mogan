/******************************************************************************
 * MODULE     : QTMUserPromptPopup.hpp
 * DESCRIPTION: Base acceptance and rejection popup
 * COPYRIGHT  : (C) 2026 AcceleratorX
 * *****************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_USER_PROMPT_POPUP_HPP
#define QT_USER_PROMPT_POPUP_HPP

#include "qt_simple_widget.hpp"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

// =============================================================================
// QTMUserPromptPopup: 用于处理用户与 AI 生成方案的交互，父类是 QTMBasePopup
// =============================================================================
class QTMUserPromptPopup : public QWidget {
  Q_OBJECT

protected:
  qt_simple_widget_rep* owner;
  QHBoxLayout*          layout;
  QPushButton*          acceptBtn;
  QPushButton*          rejectBtn;
  QPushButton*          goodBtn;
  QPushButton*          badBtn;

public:
  QTMUserPromptPopup (QWidget* parent, qt_simple_widget_rep* owner,
                      const QString& acceptText, const QString& rejectText);
  virtual ~QTMUserPromptPopup ();

  // 显示悬浮框并定位
  void showPopup ();

  // 更新位置和滚动适配
  void updatePosition ();
  void scrollBy (int x, int y);

protected:
  // 计算显示位置
  void getCachedPosition (int& x, int& y);
  void installTopLevelWindowFilter ();

  // 由子类实现的具体点击动作
  virtual void onAcceptClicked ()= 0;
  virtual void onRejectClicked ()= 0;
  virtual void onGoodClicked ()  = 0;
  virtual void onBadClicked ()   = 0;

protected slots:
  void handleAccept ();
  void handleReject ();
  void handleGood ();
  void handleBad ();
  bool eventFilter (QObject* obj, QEvent* event) override;
};

// =============================================================================
// QTMGhostTextPopup: Ghost Text 自动补全的悬浮操作框，父类是 QTMUserPromptPopup
// =============================================================================
class QTMGhostTextPopup : public QTMUserPromptPopup {
  Q_OBJECT

public:
  QTMGhostTextPopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMGhostTextPopup () override;

protected:
  void onAcceptClicked () override;
  void onRejectClicked () override;
  void onGoodClicked () override;
  void onBadClicked () override;
};

// =============================================================================
// QTMDiffTextPopup: Diff Text AI建议的悬浮操作框，父类是 QTMUserPromptPopup
// =============================================================================
class QTMDiffTextPopup : public QTMUserPromptPopup {
  Q_OBJECT

public:
  QTMDiffTextPopup (QWidget* parent, qt_simple_widget_rep* owner);
  ~QTMDiffTextPopup () override;

protected:
  void onAcceptClicked () override;
  void onRejectClicked () override;
  void onGoodClicked () override;
  void onBadClicked () override;
};

#endif // QT_USER_PROMPT_POPUP_HPP
