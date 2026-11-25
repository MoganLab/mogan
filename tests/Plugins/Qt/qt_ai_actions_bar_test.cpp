/******************************************************************************
 * MODULE     : qt_ai_actions_bar_test.cpp
 * DESCRIPTION: AI 操作栏 Qt/QML 事件边界回归测试
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/QTMAiActionsBar.hpp"
#include "base.hpp"

#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWidget>
#include <QtTest/QtTest>

class ThrowingAiActionsBar : public QTMAiActionsBar {
public:
  explicit ThrowingAiActionsBar (QWidget* parent)
      : QTMAiActionsBar (parent, nullptr) {}

  int actionCount= 0;

protected:
  void performAction (const QString& action) override {
    actionCount++;
    if (action == "chat") throw string ("read-only");
  }
};

class TestAiActionsBar : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_editor_exception_does_not_escape_qml_click () {
    QWidget              host;
    ThrowingAiActionsBar bar (&host);
    QQuickWidget*        quick= bar.findChild<QQuickWidget*> ();
    QVERIFY (quick != nullptr);
    QCOMPARE (quick->status (), QQuickWidget::Ready);
    QQuickItem* root= quick->rootObject ();
    QVERIFY (root != nullptr);
    host.show ();

    auto clickAction= [&] (const QString& action) {
      QList<QQuickItem*>                areas;
      std::function<void (QQuickItem*)> collect= [&] (QQuickItem* item) {
        if (item->objectName () == "aiActionHoverArea") areas << item;
        for (QQuickItem* child : item->childItems ())
          collect (child);
      };
      collect (root);
      for (QQuickItem* area : areas) {
        QQuickItem* cap= area->parentItem ();
        if (cap == nullptr || cap->property ("lit").isValid () == false)
          continue;
        // 鼠标事件经 QQuickWidget 进入真实 MouseArea，再发 triggered 到生产槽。
        if ((action == "chat" && area == areas[2]) ||
            (action == "translate" && area == areas[0])) {
          QPointF center=
              cap->mapToScene (QPointF (cap->width () / 2, cap->height () / 2));
          QPointF     global (quick->mapToGlobal (center.toPoint ()));
          QMouseEvent press (QEvent::MouseButtonPress, center, center, global,
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
          QCoreApplication::sendEvent (quick, &press);
          QMouseEvent release (QEvent::MouseButtonRelease, center, center,
                               global, Qt::LeftButton, Qt::NoButton,
                               Qt::NoModifier);
          QCoreApplication::sendEvent (quick, &release);
          return;
        }
      }
      QFAIL ("AI action area not found");
    };

    clickAction ("chat");
    QCOMPARE (bar.actionCount, 1);
    QVERIFY (N (the_exception) == 0);
    clickAction ("translate");
    QCOMPARE (bar.actionCount, 2);
  }
};

QTEST_MAIN (TestAiActionsBar)
#include "qt_ai_actions_bar_test.moc"
