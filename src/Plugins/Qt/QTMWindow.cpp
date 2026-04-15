
/******************************************************************************
 * MODULE     : QTMWindow.cpp
 * DESCRIPTION: QT Texmacs window class
 * COPYRIGHT  : (C) 2009 Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMWindow.hpp"
#include "config.h"
#if defined(USE_TUTORIAL)
#include "qt_tutorial.hpp"
#endif
#include "qt_utilities.hpp"
#include "tm_sys_utils.hpp"
#include "tm_url.hpp"
#include "tm_window.hpp"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QSettings>

void
QTMPlainWindow::closeEvent (QCloseEvent* event) {
  if (DEBUG_QT_WIDGETS) debug_widgets << "Close QTMPlainWindow" << LF;
  // Tell QT not to close the window, qt_window_widget_rep will if need be.
  event->ignore ();
  emit closed ();
}

void
QTMPlainWindow::moveEvent (QMoveEvent* event) {
  string name= from_qstring (windowTitle ());
  // FIXME: rather use a slot for this
  coord2 pos= from_qpoint (frameGeometry ().topLeft ());
  notify_window_move (name, pos.x1, pos.x2);
  QWidget::moveEvent (event);
}

void
QTMPlainWindow::resizeEvent (QResizeEvent* event) {
  string name= from_qstring (windowTitle ());
  // FIXME: rather use a slot for this
  coord2 sz= from_qsize (frameSize ());
  notify_window_resize (name, sz.x1, sz.x2);
  QWidget::resizeEvent (event);
}

void
QTMPlainWindow::keyPressEvent (QKeyEvent* event) {
  if (DEBUG_QT_WIDGETS)
    debug_widgets << "QTMPlainWindow key press: " << event->key () << LF;

  if (event->key () == Qt::Key_Escape) {
    // ESC键：关闭对话框
    if (DEBUG_QT_WIDGETS) debug_widgets << "ESC pressed, closing window" << LF;
    event->accept ();
    close ();
  }
  else if (event->key () == Qt::Key_Return || event->key () == Qt::Key_Enter) {
    // Enter键：尝试找到默认按钮并点击
    if (DEBUG_QT_WIDGETS)
      debug_widgets << "Enter pressed, looking for default button" << LF;
    event->accept ();

    // 查找对话框中的按钮
    QList<QPushButton*> buttons= findChildren<QPushButton*> ();
    for (QPushButton* button : buttons) {
      if (button->isDefault () ||
          button->text ().contains ("Ok", Qt::CaseInsensitive)) {
        if (DEBUG_QT_WIDGETS)
          debug_widgets << "Found button: " << from_qstring (button->text ())
                        << LF;
        button->click ();
        return;
      }
    }

    // 如果没有找到默认按钮，尝试点击第一个"Ok"按钮
    for (QPushButton* button : buttons) {
      if (button->text ().contains ("Ok", Qt::CaseInsensitive)) {
        if (DEBUG_QT_WIDGETS)
          debug_widgets << "Found Ok button: " << from_qstring (button->text ())
                        << LF;
        button->click ();
        return;
      }
    }

    // 如果还没有找到，点击第一个按钮
    if (!buttons.isEmpty ()) {
      if (DEBUG_QT_WIDGETS)
        debug_widgets << "Clicking first button: "
                      << from_qstring (buttons.first ()->text ()) << LF;
      buttons.first ()->click ();
    }
  }
  else {
    // 其他按键传递给父类处理
    QWidget::keyPressEvent (event);
  }
}

void
QTMWindow::closeEvent (QCloseEvent* event) {
  widget tmwid= qt_window_widget_rep::widget_from_qwidget (this);
  string name=
      (!is_nil (tmwid) ? concrete (tmwid)->get_nickname () : "QTMWindow");
  if (DEBUG_QT_WIDGETS) debug_widgets << "Close QTMWindow " << name << LF;

  // 保存窗口状态和几何信息
  saveSettings ();
  set_auxiliary_widget_visibility (tmwid, false);
  event->ignore ();
#if defined(OS_MACOS)
  notify_window_destroy (name);
  // this caused bug 61884, closing can still be cancelled
#endif
  emit closed ();
}

void
QTMWindow::moveEvent (QMoveEvent* event) {
  widget tmwid= qt_window_widget_rep::widget_from_qwidget (this);
  string name=
      (!is_nil (tmwid) ? concrete (tmwid)->get_nickname () : "QTMWindow");
  // FIXME: rather use a slot for this
  coord2 pt= from_qpoint (frameGeometry ().topLeft ());
  notify_window_move (name, pt.x1, pt.x2);
  QMainWindow::moveEvent (event);
}

void
QTMWindow::resizeEvent (QResizeEvent* event) {
  widget tmwid= qt_window_widget_rep::widget_from_qwidget (this);
  string name=
      (!is_nil (tmwid) ? concrete (tmwid)->get_nickname () : "QTMWindow");
  // FIXME: rather use a slot for this
  coord2 sz= from_qsize (frameSize ());
  notify_window_resize (name, sz.x1, sz.x2);
  QMainWindow::resizeEvent (event);
}

void
QTMWindow::showEvent (QShowEvent* event) {
  QMainWindow::showEvent (event);
#if defined(USE_TUTORIAL)
  QWK::FirstLaunchTutorialController::instance ()->maybeStartForMainWindow (
      this);
#endif
}

void
QTMWindow::saveSettings () {
  /* 保存工具栏和窗口状态
   * mainWindowState: 保存主窗口的工具栏、菜单栏、状态栏等UI元素的状态
   *                  包括工具栏的位置、是否可见、停靠状态等，以及菜单栏的展开状态
   * mainWindowGeometry: 保存主窗口的几何信息
   *                     包括窗口的位置(x,y坐标)、大小(宽度、高度)、是否最大化/最小化等状态
   * saveState() - 保存工具栏和停靠窗口的状态
   * saveGeometry() - 保存窗口的几何信息 */

  string config_path= concretize (head (get_tm_preference_path ()));
  if (config_path != "") {
    /* 使用用户自定义的配置路径创建QSettings对象
     * 配置文件格式为INI格式，便于用户手动编辑 */
    QSettings settings (to_qstring (config_path * "/qtwindows.ini"),
                        QSettings::IniFormat);
    /* 保存主窗口的UI元素状态 */
    settings.setValue ("mainWindowState", saveState ());
    /* 保存主窗口的几何信息 */
    settings.setValue ("mainWindowGeometry", saveGeometry ());
    /* 立即将设置写入磁盘，确保数据不丢失 */
    settings.sync ();
  }
  else {
    QSettings settings ("LiiiNetwork", STEM_NAME);
    settings.setValue ("mainWindowState", saveState ());
    settings.setValue ("mainWindowGeometry", saveGeometry ());
    settings.sync ();
  }
}

////////////////////

QTMPopupWidget::QTMPopupWidget (QWidget* contents) {

  QHBoxLayout* l= new QHBoxLayout ();
  l->addWidget (contents);
  l->setContentsMargins (0, 0, 0, 0);
  l->setEnabled (false); // Tell the layout not to adjust itself (!)
  setLayout (l);

  resize (contents->size ());
  setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
  setWindowFlags (Qt::Popup);
  setAttribute (Qt::WA_NoSystemBackground);
  setMouseTracking (true); // Receive mouse events
  //  setFocusPolicy(Qt::StrongFocus);   // Don't! Receive key events
  //  setWindowOpacity(0.9);

  // cout << "QTMPopupWidget created with size: " << size().width()
  //  << " x " << size().height() << LF;
}

/*
 If our contents QWidget is of type QTMWidget it will capture mouse events
 and we won't get called until the pointer exits the contents, so the check
 inside is unnecessary unless the contents are of another kind.

 NOTE that this is intended for popups which appear under the cursor!
 */
void
QTMPopupWidget::mouseMoveEvent (QMouseEvent* event) {

  /* It'd be nice to have something like this...
  if (! drawArea().contains(event->globalPos())) {
    hide();
    emit closed();
  } else {
    move(event->globalPos());
  }
   */

  if (!this->rect ().contains (QCursor::pos ())) {
    hide ();
    emit closed ();
  }

  event->ignore ();
}

void
QTMPopupWidget::keyPressEvent (QKeyEvent* event) {
  (void) event;
  hide ();
  emit closed ();
}

void
QTMPopupWidget::closeEvent (QCloseEvent* event) {
  if (DEBUG_QT_WIDGETS) debug_widgets << "Close QTMPopupWidget" << LF;
  // Tell QT not to close the window, qt_window_widget_rep will if need be.
  event->ignore ();
  emit closed ();
}
