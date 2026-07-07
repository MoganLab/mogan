
/******************************************************************************
 * MODULE     : QTMTabPage.hpp
 * DESCRIPTION: QT Texmacs tab page classes
 * COPYRIGHT  : (C) 2024 Zhenjun Guo
 *                  2026 Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/
#ifndef QTMTABPAGE_HPP
#define QTMTABPAGE_HPP

#include "windowbutton.hpp"
#include <QDrag>
#include <QDropEvent>
#include <QFrame>
#include <QMimeData>
#include <QMouseEvent>
#include <QMutex>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolBar>
#include <QToolButton>
#include <QWKWidgets/widgetwindowagent.h>
#include <basic.hpp>
#include <scheme.hpp>

/**
 * @brief 单个标签页控件。
 *
 * 基于 QToolButton，承载标题、关闭按钮、脏标记与拖拽逻辑。
 */
class QTMTabPage : public QToolButton {
  Q_OBJECT
  QWK::WindowButton* m_closeBtn= nullptr;
  QPoint             m_dragStartPos;
  bool               m_isDirty         = false;
  bool               m_hoverOnCloseArea= false;

public:
  const url m_viewUrl;

public:
  explicit QTMTabPage (url p_url, QAction* p_title, QAction* p_closeBtn,
                       bool p_isActive);
  explicit QTMTabPage ();
  virtual void paintEvent (QPaintEvent*) override;
  bool         isDirty () const { return m_isDirty; }
  /**
   * @brief 同步已解析好的干净标题与 dirty 标志。
   *
   * @param cleanTitle 已去除尾部 `*` 后缀的干净标题。
   * @param dirty      是否处于未保存的脏状态。
   *
   * @par 为什么需要它
   * replaceTabPages 复用既有 tab 时调用：srcTab 构造时已 applyDisplayTitle
   * 解析过尾部 `*`，其 text() 是干净标题、isDirty() 是最新脏状态。复用的
   * tab 必须同步这两者，否则 m_isDirty 停留在首次构造的旧值，编辑标脏 /
   * 保存去脏都不会反映到关闭按钮位置的 `*` 上。
   */
  void syncDisplay (const QString& cleanTitle, bool dirty);

public slots:
  void setChecked (bool checked);

protected:
  virtual bool eventFilter (QObject* watched, QEvent* event) override;
  virtual void resizeEvent (QResizeEvent* e) override;
  virtual void mousePressEvent (QMouseEvent* e) override;
  virtual void mouseMoveEvent (QMouseEvent* e) override;
  virtual void enterEvent (QEnterEvent* e) override;
  virtual void leaveEvent (QEvent* e) override;

private:
  void applyDisplayTitle (const QString& rawTitle);
  bool isPointerOnCloseArea (const QPoint& pos) const;
  void updateCloseButtonVisibility ();
  void initializeCloseButton (QAction* closeAction= nullptr);
};

/**
 * @brief QTMTabPage widget 的 QAction 载体。
 *
 * @par 为什么不用 QWidgetAction
 * QWidgetAction 一旦 setDefaultWidget，就无法再取出 defaultWidget——
 * 删除 QWidgetAction 时其 defaultWidget 也会被一同销毁（见 QWidgetAction
 * 源码）。本类仅作指针载体，不持有 widget 生命周期，便于后续从 action
 * 中安全取回 QTMTabPage。
 */
class QTMTabPageAction : public QAction {
  Q_OBJECT

public:
  explicit QTMTabPageAction (QWidget* p_widget) : m_widget (p_widget) {}
  QWidget* const m_widget;
};

/**
 * @brief 标签页容器，承载并排布多个 QTMTabPage。
 *
 * 支持自动多行排布（标签数量多时换行）与拖拽重排序。
 */
class QTMTabPageContainer : public QWidget {
  Q_OBJECT
  QList<QTMTabPage*> m_tabPageList;
  int                m_rowHeight       = 0;
  int                m_draggingTabIndex= -1;
  QFrame*            m_indicator;
  int                m_width= 0;
  bool               dragging;
  QPoint             dragPosition;
  QWK::WindowButton* m_addTabButton;
  bool               m_vipButtonReserved= false;

public:
  QTMTabPage* dummyTabPage;
  explicit QTMTabPageContainer (QWidget* p_parent);
  ~QTMTabPageContainer ();

#ifdef LIII_DEBUG
  /// @brief replaceTabPages 增量计数器：从 carrier 摄取 / deleteLater 移除
  ///        的 tab 数，以及 updateActiveTab 命中次数。仅 debug 模式可用。
  int debug_added_count  = 0;
  int debug_removed_count= 0;
  int debug_active_count = 0;
  /// @brief 按 view-url 取现有 tab 指针，用于断言增量 diff 复用了同一对象
  ///        而非全量重建换新指针。找不到返回 nullptr。
  QTMTabPage* debug_findTab (const url& viewUrl) const;
#endif

  inline void setRowHeight (int p_height) { m_rowHeight= p_height; }
  /**
   * @brief 切换右侧是否为 VIP 按钮预留宽度，状态变化时立即重排标签页。
   *
   * @param reserved true 时 arrangeTabPages 会在右侧预留 VIP 按钮宽度，
   *                 false 时不预留，标签页可扩展利用该空间。
   */
  void setVipButtonReserved (bool reserved) {
    if (m_vipButtonReserved != reserved) {
      m_vipButtonReserved= reserved;
      arrangeTabPages ();
    }
  }

  /// @brief 按 view-url 做增量 diff，复用已有 tab、摄取新增、移除多余。
  void replaceTabPages (QList<QAction*>* p_src);
  /// @brief 按 currentView 切换 active 高亮，仅遍历 setChecked，不重建 widget。
  void updateActiveTab (const url& currentView);
  /// @brief 重新计算并排布所有标签页的位置与尺寸。
  void arrangeTabPages ();
  void setHitTestVisibleForTabPages (QWK::WidgetWindowAgent* agent);

signals:
  void addTabRequested ();

private:
  void removeAllTabPages ();
  void adjustHeight (int p_rowCount);
  void onAddTabClicked ();

  int          mapToPointing (QDropEvent* e, QPoint& m_indicator);
  virtual void dragEnterEvent (QDragEnterEvent* e) override;
  virtual void dragMoveEvent (QDragMoveEvent* e) override;
  virtual void dropEvent (QDropEvent* e) override;
  virtual void dragLeaveEvent (QDragLeaveEvent* e) override;

protected:
  bool eventFilter (QObject* obj, QEvent* event) override;
};

/**
 * @brief 把 QTMTabPageContainer 包装成 QToolBar。
 *
 * 用于挂到 QMainWindow 上作为可拖拽停靠的工具栏。
 */
class QTMTabPageBar : public QToolBar {
  QTMTabPageContainer* m_container;

public:
  explicit QTMTabPageBar (const QString& p_title, QWidget* p_parent,
                          QTMTabPageContainer* m_container);

  inline void setRowHeight (int p_height) {
    m_container->setRowHeight (p_height);
  }

  void replaceTabPages (QList<QAction*>* p_src);

protected:
  virtual void resizeEvent (QResizeEvent* e) override;
};

/// @brief 标签页管理相关的全局状态。
///@{
extern int                  g_tabWidth;
extern int                  g_pointingIndex;
extern url                  g_mostRecentlyClosedTab;
extern url                  g_mostRecentlyDraggedTab;
extern QTMTabPageContainer* g_mostRecentlyDraggedBar;
extern QTMTabPageContainer* g_mostRecentlyEnteredBar;
///@}

/**
 * @brief Qt 侧关闭标签页入口：先标记最近关闭的 view，再执行 kill_tabpage。
 *
 * @par 为什么不能在关闭按钮点击时设置标记
 * 关闭流程会弹出确认对话框，若用户点「取消」，提前设置的隐藏标记会让
 * tab 持续隐藏。所以标记必须在真正执行 kill 的这一步设置。
 */
void cpp_kill_tabpage (url p_win, url p_view);

#endif // QTMTABPAGE_HPP
