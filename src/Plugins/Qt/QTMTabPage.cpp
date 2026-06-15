
/******************************************************************************
 * MODULE     : QTMTabPage.cpp
 * DESCRIPTION: QT Texmacs tab page classes
 * COPYRIGHT  : (C) 2024 Zhenjun Guo
 *                  2026 Yifan Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTabPage.hpp"
#include "new_view.hpp"
#include "qt_utilities.hpp"
#include "string.hpp"
#include "tm_window.hpp"
#include <QCursor>
#include <QEvent>
#include <QIcon>
#include <QSize>

// Base tab widths
constexpr int MAX_TAB_PAGE_WIDTH_BASE= 150;
constexpr int MIN_TAB_PAGE_WIDTH_BASE= 25;
// Padding used when calculating content-based width for startup/chat tabs
constexpr int SPECIAL_TAB_HORIZONTAL_PADDING= 10;

// The horizontal padding for tab container (in pixels).
#ifdef Q_OS_MAC
const int TAB_CONTAINER_PADDING= 75;
#else
const int TAB_CONTAINER_PADDING= 2;
#endif

constexpr int TAB_CONTENT_VERTICAL_OFFSET   = 0;
constexpr int ADD_TAB_BUTTON_VERTICAL_OFFSET= 0;
constexpr int ADD_BUTTON_SIZE               = 20;
constexpr int CLOSE_BUTTON_SIZE             = 18;
constexpr int TAB_ICON_SIZE                 = 16;
constexpr int TAB_ICON_TEXT_SPACING         = 4;
constexpr int NORMAL_TAB_LEFT_PADDING       = 10;
constexpr int NORMAL_TAB_RIGHT_PADDING      = 10;

// DPI scaling utility functions (使用 DpiUtils)
static double
getDPIScaleFactor () {
  return DpiUtils::scaleFactor ();
}

static int
getScaledMaxTabPageWidth () {
  return DpiUtils::scaled (MAX_TAB_PAGE_WIDTH_BASE);
}

static int
getScaledMinTabPageWidth () {
  return DpiUtils::scaled (MIN_TAB_PAGE_WIDTH_BASE);
}

static int
getScaledSpecialTabHorizontalPadding () {
  return DpiUtils::scaled (SPECIAL_TAB_HORIZONTAL_PADDING);
}

static int
getScaledAddButtonHeight () {
  return DpiUtils::scaled (ADD_BUTTON_SIZE);
}

static int
getScaledCloseButtonHeight () {
  return DpiUtils::scaled (CLOSE_BUTTON_SIZE);
}

static bool
extract_dirty_suffix (const QString& rawTitle, QString& cleanTitle) {
  if (rawTitle.endsWith (" *")) {
    cleanTitle= rawTitle.left (rawTitle.size () - 2);
    return true;
  }
  if (rawTitle.endsWith ('*')) {
    cleanTitle= rawTitle.left (rawTitle.size () - 1).trimmed ();
    return true;
  }
  cleanTitle= rawTitle;
  return false;
}

/**
 * What is g_mostRecentlyClosedTab used for? When we close an ACTIVE(!) tab
 * (let's denote it as T), the tab bar is refreshed twice, meaning that
 * QTMTabPageContainer::replaceTabPages is called twice. Specifically:
 *
 * -- During the first call, tab T has not yet been deleted, so T is still
 *    visible, although it is no longer in the active state.
 * -- During the second call, tab T has been deleted, and at this point, T is no
 *    longer visible.
 *
 * As a result, what the user observes is that when they close an ACTIVE tab, it
 * does not disappear immediately. Therefore, we need it to remember which tab
 * was most recently closed and avoid displaying it during the first update.
 */
int                  g_tabWidth              = -1;
int                  g_pointingIndex         = -1;
url                  g_mostRecentlyClosedTab = url_none ();
url                  g_mostRecentlyDraggedTab= url_none ();
QTMTabPageContainer* g_mostRecentlyDraggedBar= nullptr;
QTMTabPageContainer* g_mostRecentlyEnteredBar= nullptr;

static url
startup_tab_buffer_name () {
  return url ("tmfs://startup-tab");
}

static bool
is_startup_tab_view (url viewUrl) {
  if (is_none (viewUrl)) return false;
  return view_to_buffer (viewUrl) == startup_tab_buffer_name ();
}

static int
startup_tab_index (const QList<QTMTabPage*>& tabs) {
  for (int i= 0; i < tabs.size (); ++i)
    if (tabs[i] != nullptr && is_startup_tab_view (tabs[i]->m_viewUrl))
      return i;
  return -1;
}

/**
 * @brief 返回聊天标签页 buffer 的 URL。
 * @return \c tmfs://chat-tab。
 */
static url
chat_tab_buffer_name () {
  return url ("tmfs://chat-tab");
}

/**
 * @brief 判断视图 URL 是否属于聊天标签页。
 * @param viewUrl 待检测的视图 URL。
 * @return 若视图由聊天标签页 buffer 支撑则返回 true。
 */
static bool
is_chat_tab_view (url viewUrl) {
  if (is_none (viewUrl)) return false;
  return view_to_buffer (viewUrl) == chat_tab_buffer_name ();
}

/**
 * @brief 在给定标签列表中查找聊天标签页的索引。
 * @param tabs 待搜索的标签页列表。
 * @return 聊天标签页的索引，未找到则返回 -1。
 */
static int
chat_tab_index (const QList<QTMTabPage*>& tabs) {
  for (int i= 0; i < tabs.size (); ++i)
    if (tabs[i] != nullptr && is_chat_tab_view (tabs[i]->m_viewUrl)) return i;
  return -1;
}

/******************************************************************************
 * QTMTabPage
 ******************************************************************************/

QTMTabPage::QTMTabPage (url p_url, QAction* p_title, QAction* p_closeBtn,
                        bool p_isActive)
    : m_viewUrl (p_url) {
  p_title->setCheckable (true);
  p_title->setChecked (p_isActive);
  setDefaultAction (p_title);
  applyDisplayTitle (p_title->text ());
  setFocusPolicy (Qt::NoFocus);
  initializeCloseButton (p_closeBtn);
  int pad   = DpiUtils::scaled (8);
  int radius= DpiUtils::scaled (10);
  setStyleSheet (
      QString ("padding: %1px; border-radius: %2px;").arg (pad).arg (radius));
  DpiUtils::applyScaledFont (this, 14);
  setMouseTracking (true);
}

QTMTabPage::QTMTabPage () : m_viewUrl (url_none ()) {
  setFocusPolicy (Qt::NoFocus);
  int pad   = DpiUtils::scaled (8);
  int radius= DpiUtils::scaled (10);
  setStyleSheet (
      QString ("padding: %1px; border-radius: %2px;").arg (pad).arg (radius));
  DpiUtils::applyScaledFont (this, 14);
  setMouseTracking (true);
}

void
QTMTabPage::applyDisplayTitle (const QString& rawTitle) {
  QString cleanTitle;
  m_isDirty= extract_dirty_suffix (rawTitle, cleanTitle);
  setText (cleanTitle);
}

void
QTMTabPage::initializeCloseButton (QAction* closeAction) {
  m_closeBtn= new QWK::WindowButton (this);
  m_closeBtn->setObjectName ("tabpage-close-button");
  m_closeBtn->setFocusPolicy (Qt::NoFocus);
  int closeBtnSize= getScaledCloseButtonHeight ();
  m_closeBtn->setMinimumSize (closeBtnSize, closeBtnSize);
  m_closeBtn->setFixedSize (closeBtnSize, closeBtnSize);
  m_closeBtn->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  int closeBtnRadius= DpiUtils::scaled (6);
  m_closeBtn->setStyleSheet (
      QString ("border-radius: %1px; padding: 0px;").arg (closeBtnRadius));
  m_closeBtn->installEventFilter (this);
  if (closeAction) {
    QPointer<QAction> safeAction (closeAction);
    connect (m_closeBtn, &QPushButton::clicked, this, [=] () {
      if (!safeAction) return;
      g_mostRecentlyClosedTab= m_viewUrl;
      safeAction->trigger ();
    });
  }
  updateCloseButtonVisibility ();
}

bool
QTMTabPage::isPointerOnCloseArea (const QPoint& pos) const {
  if (!m_closeBtn) return false;
  return m_closeBtn->geometry ().contains (pos);
}

bool
QTMTabPage::eventFilter (QObject* watched, QEvent* event) {
  if (watched == m_closeBtn) {
    if (event->type () == QEvent::Enter) {
      m_hoverOnCloseArea= true;
      updateCloseButtonVisibility ();
      return false;
    }
    if (event->type () == QEvent::Leave) {
      QPoint pos        = mapFromGlobal (QCursor::pos ());
      m_hoverOnCloseArea= isPointerOnCloseArea (pos);
      updateCloseButtonVisibility ();
      return false;
    }
  }
  return QToolButton::eventFilter (watched, event);
}

/* We can't align the text to the left of the button by QSS or other methods,
 * so for now we achieve it by overriding the paintEvent. */
void
QTMTabPage::paintEvent (QPaintEvent*) {
  QStylePainter          p (this);
  QStyleOptionToolButton opt;
  initStyleOption (&opt);
  opt.text= "";                                      // don't draw the text now
  p.drawComplexControl (QStyle::CC_ToolButton, opt); // base method

  // draw the text now
  QFontMetrics fm (opt.fontMetrics);

  bool isStartup= is_startup_tab_view (m_viewUrl);
  bool isChatTab= is_chat_tab_view (m_viewUrl);

  if (isStartup || isChatTab) {
    static const QIcon startupIcon (":/app/stem.png");
    static const QIcon chatIcon (":/window-bar/ai.svg");
    const QIcon&       icon= isStartup ? startupIcon : chatIcon;

    int     iconSize      = DpiUtils::scaled (TAB_ICON_SIZE);
    int     spacing       = DpiUtils::scaled (TAB_ICON_TEXT_SPACING);
    int     textAvailWidth= width () - iconSize - spacing;
    QString elidedText= fm.elidedText (text (), Qt::ElideRight, textAvailWidth);
    int     textWidth = fm.horizontalAdvance (elidedText);
    int     totalWidth= iconSize + spacing + textWidth;
    int     startX    = (width () - totalWidth) / 2;

    p.drawPixmap (startX, (height () - iconSize) / 2,
                  icon.pixmap (iconSize, iconSize));

    QRect textRect (startX + iconSize + spacing, 0, textWidth, height ());
    p.drawItemText (textRect, Qt::AlignLeft | Qt::AlignVCenter, palette (),
                    isEnabled (), elidedText, QPalette::ButtonText);
  }
  else {
    int leftPadding= DpiUtils::scaled (NORMAL_TAB_LEFT_PADDING);
    int rightPadding=
        m_closeBtn
            ? m_closeBtn->width () + DpiUtils::scaled (NORMAL_TAB_RIGHT_PADDING)
            : DpiUtils::scaled (NORMAL_TAB_RIGHT_PADDING);
    int availableWidth= width () - leftPadding - rightPadding;
    if (availableWidth < 20) {
      availableWidth= 20;
    }
    QString elidedText= fm.elidedText (text (), Qt::ElideRight, availableWidth);
    QRect   textRect (leftPadding, 0, availableWidth, height ());
    p.drawItemText (textRect, Qt::AlignLeft | Qt::AlignVCenter, palette (),
                    isEnabled (), elidedText, QPalette::ButtonText);

    if (m_isDirty && m_closeBtn && !m_closeBtn->isVisible ()) {
      QRect dirtyRect= m_closeBtn->geometry ();
      p.drawItemText (dirtyRect, Qt::AlignCenter, palette (), isEnabled (), "*",
                      QPalette::ButtonText);
    }
  }
}

void
QTMTabPage::resizeEvent (QResizeEvent* e) {
  if (!m_closeBtn) return;
  int w= m_closeBtn->width ();
  int h= m_closeBtn->height ();
  int x= e->size ().width () - w - DpiUtils::scaled (NORMAL_TAB_RIGHT_PADDING);
  int y= (height () - h) / 2;
  y+= TAB_CONTENT_VERTICAL_OFFSET;

  m_closeBtn->setGeometry (x, y, w, h);
}

void
QTMTabPage::mousePressEvent (QMouseEvent* e) {
  if (is_startup_tab_view (m_viewUrl) || is_chat_tab_view (m_viewUrl)) {
    // 如果启动页标签或聊天标签已经是当前视图，不处理点击事件，避免取消选中状态
    url currentView= get_current_view_safe ();
    if (!is_none (currentView) && currentView == m_viewUrl) {
      return;
    }
    return QToolButton::mousePressEvent (e);
  }
  if (e->button () == Qt::LeftButton) {
    g_mostRecentlyDraggedTab= this->m_viewUrl;
    g_mostRecentlyDraggedBar=
        qobject_cast<QTMTabPageContainer*> (this->parentWidget ());
    g_mostRecentlyEnteredBar= g_mostRecentlyDraggedBar;
    m_dragStartPos          = e->pos ();
  }
  QToolButton::mousePressEvent (e);
}

void
QTMTabPage::mouseMoveEvent (QMouseEvent* e) {
  m_hoverOnCloseArea= isPointerOnCloseArea (e->pos ());
  updateCloseButtonVisibility ();
  if (is_startup_tab_view (m_viewUrl) || is_chat_tab_view (m_viewUrl)) {
    return QToolButton::mouseMoveEvent (e);
  }
  if (!(e->buttons () & Qt::LeftButton)) return QToolButton::mouseMoveEvent (e);
  if ((e->pos () - m_dragStartPos).manhattanLength () < 3) {
    // avoid treating small movement(more like a click) as dragging
    return QToolButton::mouseMoveEvent (e);
  }
  // TODO: Re-enable tab tear-off (drag out of window to create new window)
  // after stabilizing the drag-and-drop across windows.
  return QToolButton::mouseMoveEvent (e);

  // 创建一个保留 alpha 通道和设备像素比 (devicePixelRatio) 的控件快照。
  // 使用 QWidget::grab() 可以避免生成带有黑色背景的 pixmap，
  // 并防止因额外缩放导致的模糊。
  QPixmap pixmap= this->grab ();
  setDown (false); // to avoid keeping the pressed state

  g_mostRecentlyDraggedTab= this->m_viewUrl;
  g_mostRecentlyDraggedBar=
      qobject_cast<QTMTabPageContainer*> (this->parentWidget ());
  g_mostRecentlyClosedTab= this->m_viewUrl; // hide the tab during dragging
  g_pointingIndex        = -1;
  g_mostRecentlyDraggedBar->arrangeTabPages ();

  QDrag* drag=
      new QDrag (parent ()); // don't point to `this`, it will cause crash
  // 设置热点为鼠标在标签页内的相对位置，这样pixmap会从标签页位置开始显示
  drag->setHotSpot (m_dragStartPos);
  drag->setMimeData (new QMimeData ()); // Qt requires
  drag->setPixmap (pixmap);
  drag->exec (Qt::MoveAction);
  // 没有拖拽到其他窗口，则建立新窗口
  if (!g_mostRecentlyEnteredBar && (g_mostRecentlyDraggedTab != url_none ())) {
    view_set_new_window (g_mostRecentlyDraggedTab);
  }
}

void
QTMTabPage::enterEvent (QEnterEvent* e) {
  m_hoverOnCloseArea= isPointerOnCloseArea (e->position ().toPoint ());
  updateCloseButtonVisibility ();
  QToolButton::enterEvent (e);
}

void
QTMTabPage::leaveEvent (QEvent* e) {
  m_hoverOnCloseArea= false;
  updateCloseButtonVisibility ();
  QToolButton::leaveEvent (e);
}

void
QTMTabPage::updateCloseButtonVisibility () {
  if (!m_closeBtn) return;
  // TODO: 聊天标签页当前不可关闭，后续需支持可删除
  bool shouldShow= !is_startup_tab_view (m_viewUrl) &&
                   !is_chat_tab_view (m_viewUrl) &&
                   ((!m_isDirty && (underMouse () || isChecked ())) ||
                    (m_isDirty && m_hoverOnCloseArea));
  bool wasVisible= m_closeBtn->isVisible ();
  m_closeBtn->setVisible (shouldShow);

  // 如果关闭按钮的可见性发生了变化，需要重新绘制文字区域
  if (wasVisible != shouldShow) {
    update ();
  }
}

void
QTMTabPage::setChecked (bool checked) {
  QToolButton::setChecked (checked);
  updateCloseButtonVisibility ();
}

/******************************************************************************
 * QTMTabPageContainer
 ******************************************************************************/

QTMTabPageContainer::QTMTabPageContainer (QWidget* p_parent)
    : QWidget (p_parent) {
  m_indicator= new QFrame (this);
  m_indicator->setFrameShape (QFrame::VLine);
  m_indicator->setLineWidth (2);
  m_indicator->hide ();
  dummyTabPage= new QTMTabPage ();
  dummyTabPage->setParent (this);
  dummyTabPage->hide ();

  // 创建新增标签页按钮
  m_addTabButton= new QWK::WindowButton (this);
  m_addTabButton->setObjectName ("add-tab-button");
  int addButtonSide= getScaledAddButtonHeight ();
  m_addTabButton->setMinimumSize (addButtonSide, addButtonSide);
  m_addTabButton->setFixedSize (addButtonSide, addButtonSide);
  m_addTabButton->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  int addBtnRadius= DpiUtils::scaled (6);
  m_addTabButton->setStyleSheet (
      QString ("border-radius: %1px; padding: 0px;").arg (addBtnRadius));
  connect (m_addTabButton, &QPushButton::clicked, this,
           &QTMTabPageContainer::onAddTabClicked);
  m_addTabButton->hide ();

  if (parent ()) {
    parent ()->installEventFilter (this);
  }

  setAcceptDrops (true);
  setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QTMTabPageContainer::~QTMTabPageContainer () { removeAllTabPages (); }

void
QTMTabPageContainer::replaceTabPages (QList<QAction*>* p_src) {
  removeAllTabPages ();    // remove  old tabs
  extractTabPages (p_src); // extract new tabs

  arrangeTabPages ();
}

void
QTMTabPageContainer::removeAllTabPages () {
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    // remove from parent first to avoid being freed again
    m_tabPageList[i]->setParent (nullptr);
    m_tabPageList[i]->deleteLater ();
  }
  m_tabPageList.clear ();
}

void
QTMTabPageContainer::extractTabPages (QList<QAction*>* p_src) {
  if (!p_src) return;
  for (int i= 0; i < p_src->size (); ++i) {
    // see the definition of QTMTabPageAction why we're using it
    QTMTabPageAction* carrier= qobject_cast<QTMTabPageAction*> ((*p_src)[i]);
    ASSERT (carrier, "QTMTabPageAction expected")

    QTMTabPage* tab= qobject_cast<QTMTabPage*> (carrier->m_widget);
    if (tab) {
      tab->setParent (this);
      m_tabPageList.append (tab);
    }
    else {
      delete carrier->m_widget; // we don't use it so we should delete it
    }

    // We don't need to manually delete carrier, because it(p_src) is a QAction,
    // which will be deleted by the parent widget (QTMTabPageBar) when it
    // is destroyed (by shedule_destruction).
  }

  int startupIndex= startup_tab_index (m_tabPageList);
  if (startupIndex > 0) {
    QTMTabPage* startupTab= m_tabPageList.takeAt (startupIndex);
    m_tabPageList.prepend (startupTab);
  }

  int chatIndex= chat_tab_index (m_tabPageList);
  if (chatIndex > 1) {
    QTMTabPage* chatTab= m_tabPageList.takeAt (chatIndex);
    m_tabPageList.insert (1, chatTab);
  }
  else if (chatIndex == 0 && m_tabPageList.size () > 1) {
    // Chat tab should be after startup tab, not before
    QTMTabPage* chatTab= m_tabPageList.takeAt (chatIndex);
    m_tabPageList.insert (1, chatTab);
  }
}

void
QTMTabPageContainer::arrangeTabPages () {
  if (!parentWidget ()) return;
  const int windowWidth=
      parentWidget () ? parentWidget ()->width () : this->width ();
  // 动态计算右侧预留空间，防止标签页覆盖系统按钮
  double scale      = getDPIScaleFactor ();
  int    buttonWidth= int (72 * scale); // 按钮宽度
  int    buttonCount= 5;                // pin, min, max, close,login
#ifdef Q_OS_MAC
  buttonCount= 1; // macOS 仅保留 login
#endif
  int reservedRight= buttonCount * buttonWidth;
#ifndef IS_COMMUNITY
  reservedRight+= DpiUtils::scaled (90); // VIP 按钮及间距预留
#endif

  int visibleTabCount= 0;
  // cout << "most recently closed tab:" << g_mostRecentlyClosedTab << LF;
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    QTMTabPage* tab= m_tabPageList[i];
    if (g_mostRecentlyClosedTab != tab->m_viewUrl) {
      visibleTabCount++;
    }
  }

  if (visibleTabCount == 0) {
    g_mostRecentlyClosedTab= url ();
    adjustHeight (0);
    return;
  }

  if (g_pointingIndex != -1) {
    visibleTabCount++; // leave space for the dragged tab
  }
  // cout << "Visible tab count:" << visibleTabCount << LF;

  // Calculate tab dimensions
  int availableWidth= windowWidth - 2 * TAB_CONTAINER_PADDING - reservedRight;

  // Pre-compute special tab widths (startup/chat) so they are excluded from
  // the evenly-divided space and never compressed.
  int specialTabWidth= 0;
  int specialTabCount= 0;
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    QTMTabPage* tab= m_tabPageList[i];
    if (g_mostRecentlyClosedTab == tab->m_viewUrl) continue;
    if (is_startup_tab_view (tab->m_viewUrl) ||
        is_chat_tab_view (tab->m_viewUrl)) {
      QFontMetrics fm (tab->font ());
      int          iconSize    = DpiUtils::scaled (TAB_ICON_SIZE);
      int          spacing     = DpiUtils::scaled (TAB_ICON_TEXT_SPACING);
      int          textWidth   = fm.horizontalAdvance (tab->text ());
      int          contentWidth= iconSize + spacing + textWidth;
      specialTabWidth+=
          contentWidth + 2 * getScaledSpecialTabHorizontalPadding ();
      specialTabCount++;
    }
  }

  int normalTabCount = visibleTabCount - specialTabCount;
  int normalAvailable= availableWidth - specialTabWidth;
  int tabWidth=
      normalTabCount > 0 ? normalAvailable / normalTabCount : availableWidth;
  // Clamp width into a reasonable range: allow longer tabs when count is small
  tabWidth  = std::max (getScaledMinTabPageWidth (),
                        std::min (getScaledMaxTabPageWidth (), tabWidth));
  g_tabWidth= tabWidth; // for external use

  int accumWidth= TAB_CONTAINER_PADDING;

  // Set new positions for all tabs
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    QTMTabPage* tab            = m_tabPageList[i];
    int         currentTabWidth= tabWidth;
    if (is_startup_tab_view (tab->m_viewUrl) ||
        is_chat_tab_view (tab->m_viewUrl)) {
      QFontMetrics fm (tab->font ());
      int          iconSize    = DpiUtils::scaled (TAB_ICON_SIZE);
      int          spacing     = DpiUtils::scaled (TAB_ICON_TEXT_SPACING);
      int          textWidth   = fm.horizontalAdvance (tab->text ());
      int          contentWidth= iconSize + spacing + textWidth;
      currentTabWidth=
          contentWidth + 2 * getScaledSpecialTabHorizontalPadding ();
    }

    if (g_pointingIndex == i) {
      // construct a dummy rectangle widget for indication of the inser place of
      // the dragged tab
      dummyTabPage->setGeometry (accumWidth, 0, currentTabWidth, m_rowHeight);
      dummyTabPage->show ();
      accumWidth+= currentTabWidth;
    }
    if (g_mostRecentlyClosedTab == tab->m_viewUrl) {
      tab->hide ();
      continue;
    }

    tab->setGeometry (accumWidth, 0, currentTabWidth, m_rowHeight);
    accumWidth+= currentTabWidth;
    tab->show ();
  }
  if (g_pointingIndex >= m_tabPageList.size ()) {
    dummyTabPage->setGeometry (accumWidth, 0, tabWidth, m_rowHeight);
    dummyTabPage->show ();
    accumWidth+= tabWidth;
  }

  adjustHeight (0);

  // 设置新增标签页按钮的位置
  if (m_addTabButton) {
    // 将按钮放在最后一个标签页的后面
    int addButtonWidth = m_addTabButton->width ();
    int addButtonHeight= m_addTabButton->height ();
    int buttonX        = accumWidth;
    // 调整按钮垂直位置，与系统按钮对齐
    int buttonY= (m_rowHeight - addButtonHeight) / 2;
    buttonY+= ADD_TAB_BUTTON_VERTICAL_OFFSET;
    m_addTabButton->setGeometry (buttonX, buttonY, addButtonWidth,
                                 addButtonHeight);
    m_addTabButton->show ();
  }

  // if not draggin, clear the memory of most recently closed tab
  if (g_mostRecentlyDraggedTab == url_none ()) g_mostRecentlyClosedTab= url ();
}

void
QTMTabPageContainer::adjustHeight (int p_rowCount) {
  int h= m_rowHeight * (p_rowCount + 1);
  setFixedHeight (h);
}

void
QTMTabPageContainer::setHitTestVisibleForTabPages (
    QWK::WidgetWindowAgent* agent) {
  if (!agent) return;

  // 为每个标签页设置hit test可见性
  for (QTMTabPage* tabPage : m_tabPageList) {
    agent->setHitTestVisible (tabPage, true);
  }

  // 为新增标签页按钮设置hit test可见性
  if (m_addTabButton) {
    agent->setHitTestVisible (m_addTabButton, true);
  }
}

int
QTMTabPageContainer::mapToPointing (QDropEvent* e, QPoint& p_indicatorPos) {
  QPoint pos= e->position ().toPoint ();
  if (m_tabPageList.isEmpty ()) {
    p_indicatorPos= QPoint (0, 0);
    return 0;
  }

  int index= m_tabPageList.size ();
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    QTMTabPage* tab= m_tabPageList[i];
    if (!tab || !tab->isVisible ()) continue;
    QRect rect = tab->geometry ();
    int   x_mid= rect.x () + rect.width () / 2;
    if (pos.x () < x_mid) {
      index         = i;
      p_indicatorPos= rect.topLeft ();
      break;
    }
    index         = i + 1;
    p_indicatorPos= rect.topRight ();
  }

  int startupIndex= startup_tab_index (m_tabPageList);
  if (startupIndex == 0) index= qMax (1, index);
  return std::min (index, static_cast<int> (m_tabPageList.size ()));
}

void
QTMTabPageContainer::dragEnterEvent (QDragEnterEvent* e) {
  g_mostRecentlyEnteredBar= this;
  int index               = -1;
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    if (m_tabPageList[i]->m_viewUrl == g_mostRecentlyDraggedTab) {
      index= i;
      break;
    }
  }
  m_draggingTabIndex= index;
  e->acceptProposedAction ();
}

void
QTMTabPageContainer::dragMoveEvent (QDragMoveEvent* e) {
  if (g_mostRecentlyDraggedTab != url_none ()) {
    e->acceptProposedAction ();
    QPoint pos;
    int    pointingIndex= mapToPointing (e, pos);
    if (g_pointingIndex != pointingIndex) {
      g_pointingIndex= pointingIndex;
      arrangeTabPages ();
    }
    // display a vertical line to tell user where the tab will be inserted
    m_indicator->setGeometry (pos.x (), pos.y (), 2, m_rowHeight);
  }
}

void
QTMTabPageContainer::dropEvent (QDropEvent* e) {
  e->acceptProposedAction ();
  if (m_draggingTabIndex != -1) {
    QPoint      _; // dummy argument
    int         pointingIndex= mapToPointing (e, _);
    QTMTabPage* draggingTab  = m_tabPageList[m_draggingTabIndex];
    int         oldIndex     = m_draggingTabIndex;
    int newIndex= pointingIndex > oldIndex ? pointingIndex - 1 : pointingIndex;
    int startupIndex= startup_tab_index (m_tabPageList);
    if (startupIndex == oldIndex) {
      g_mostRecentlyClosedTab= url_none ();
      g_pointingIndex        = -1;
      m_draggingTabIndex     = -1;
      arrangeTabPages ();
      m_indicator->hide ();
      dummyTabPage->hide ();
      return;
    }
    if (startupIndex == 0) newIndex= qMax (1, newIndex);
    g_mostRecentlyClosedTab= url_none ();
    g_pointingIndex        = -1;

    // update tab page positions immediately
    if (pointingIndex != oldIndex) {
      m_tabPageList.removeAt (oldIndex);
      m_tabPageList.insert (newIndex, draggingTab);
    }
    arrangeTabPages ();

    // move the tab pages in the view history
    move_tabpage (oldIndex, newIndex);
    m_draggingTabIndex= -1;
  }
  else if (g_mostRecentlyDraggedTab != url_none () &&
           g_mostRecentlyDraggedBar) {
    // Attach当前标签页到其他窗口
    QObject* src= e->source ();
    if (src && src != this) {
      url dragged_view= g_mostRecentlyDraggedTab;
      if (is_startup_tab_view (dragged_view) ||
          is_chat_tab_view (dragged_view)) {
        g_mostRecentlyDraggedTab= url_none ();
        g_mostRecentlyDraggedBar= nullptr;
        g_pointingIndex         = -1;
        m_indicator->hide ();
        dummyTabPage->hide ();
        return;
      }
      tm_window dragged_window= concrete_view (dragged_view)->win_tabpage;
      url target_view= m_tabPageList[0]->m_viewUrl; // 通过view来获取window
      tm_window target_window= concrete_view (target_view)->win_tabpage;
      bool      attached     = (concrete_view (dragged_view)->win != NULL);
      // 注意：dragged_window 有可能被 view_set_window 释放
      if (!view_set_window (dragged_view, abstract_window (target_window),
                            attached)) {
        g_pointingIndex= -1;
        g_mostRecentlyDraggedBar->arrangeTabPages ();
        g_mostRecentlyDraggedBar->dummyTabPage->hide (); // 确保隐藏
        arrangeTabPages ();
        dummyTabPage->hide ();
      }
    }
    g_mostRecentlyDraggedTab= url_none ();
    g_mostRecentlyDraggedBar= nullptr;
  }
  m_draggingTabIndex= -1;
  g_pointingIndex   = -1;
  m_indicator->hide ();
  dummyTabPage->hide ();
}

void
QTMTabPageContainer::dragLeaveEvent (QDragLeaveEvent* e) {
  g_mostRecentlyEnteredBar= nullptr;
  if (g_pointingIndex != -1) {
    g_pointingIndex= -1;
    arrangeTabPages ();
  }
  e->accept ();
  m_indicator->hide ();
  dummyTabPage->hide ();
}

bool
QTMTabPageContainer::eventFilter (QObject* obj, QEvent* event) {
  if (obj == parent () && event->type () == QEvent::Resize) {
    setFixedWidth (parentWidget () ? parentWidget ()->width () : 1000);
    arrangeTabPages ();
  }
  return QWidget::eventFilter (obj, event);
}

/******************************************************************************
 * QTMTabPageBar
 ******************************************************************************/

QTMTabPageBar::QTMTabPageBar (const QString& p_title, QWidget* p_parent,
                              QTMTabPageContainer* m_container)
    : QToolBar (p_title, p_parent), m_container (m_container) {
  // m_container= new QTMTabPageContainer (this);
  if (m_container) {
    addWidget (m_container);
    // 设置大小策略以允许工具栏扩展
    setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed);
  }
}

void
QTMTabPageBar::replaceTabPages (QList<QAction*>* p_src) {
  setUpdatesEnabled (false);
  bool visible= this->isVisible ();
  if (visible) hide (); // TRICK: to avoid flicker of the dest widget

  m_container->replaceTabPages (p_src);

  if (visible) show (); // TRICK: see above
  setUpdatesEnabled (true);
}

void
QTMTabPageBar::resizeEvent (QResizeEvent* e) {
  QSize size= e->size ();
  // 确保容器使用全部可用宽度减去左边的留出的拖拽句柄空间
  int availableWidth= size.width () - 7;
  if (availableWidth > 0 && m_container) {
    m_container->setGeometry (7, 0, availableWidth, size.height ());
  }
}

void
QTMTabPageContainer::onAddTabClicked () {
  emit addTabRequested ();
}
