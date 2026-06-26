
/******************************************************************************
 * MODULE     : view_history_test.cpp
 * DESCRIPTION: view_history 的单元测试
 * COPYRIGHT  : (C) 2025 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "analyze.hpp" // for occurs
#include "base.hpp"
#include "view_history.hpp"

// mock up 一个窗口url
url mock_current_window= url ("tmfs://window/test");

inline url
get_current_window () {
  return mock_current_window;
}

inline url
view_to_window_of_tabpage (url u) {
  // 简单的模拟：如果URL包含"window1"则返回window1，否则返回window2
  // 如果都不包含，则返回默认窗口
  string s= as_string (u);
  if (occurs ("window1", s)) {
    return url ("tmfs://window/window1");
  }
  else if (occurs ("window2", s)) {
    return url ("tmfs://window/window2");
  }
  return url ("tmfs://window/default");
}

class Testview_history : public QObject {
  Q_OBJECT

private slots:
  void init ();
  void test_constructor ();
  void test_add_view ();
  void test_remove_view ();
  void test_size_and_index_access ();
  void test_number_operations ();
  void test_filtered_view_sorting ();
  void test_edge_cases ();

private:
  view_history* history;

  // 辅助函数用于比较URL（Qt默认无法比较url对象）
  void compareUrl (const url& actual, const url& expected,
                   const QString& message) {
    QCOMPARE (as_string (actual), as_string (expected));
  }
};

void
Testview_history::init () {
  init_lolly ();
  history= new view_history ();
}

void
Testview_history::test_constructor () {
  view_history vh;
  QCOMPARE (vh.size (), 0);
}

void
Testview_history::test_add_view () {
  url view1= url ("tmfs://view/1/test1");
  url view2= url ("tmfs://view/2/test2");
  url view3= url ("tmfs://view/3/test3");

  // 添加第一个视图
  history->add_view (view1);
  QCOMPARE (history->size (), 1);
  QCOMPARE (as_string (history->get_view_at_index (0)), as_string (view1));

  // 添加第二个视图
  history->add_view (view2);
  QCOMPARE (history->size (), 2);
  QCOMPARE (as_string (history->get_view_at_index (0)),
            as_string (view2)); // 新视图在history的前面
  QCOMPARE (as_string (history->get_view_at_index (1)),
            as_string (view1)); // 旧视图在history的后面

  // 重新添加第一个视图（应该移到前面）
  history->add_view (view1);
  QCOMPARE (history->size (), 2); // 大小不变
  QCOMPARE (as_string (history->get_view_at_index (0)),
            as_string (view1)); // 重新添加的视图在最前面
  QCOMPARE (as_string (history->get_view_at_index (1)),
            as_string (view2)); // 其他视图相应调整

  // 添加第三个视图
  history->add_view (view3);
  QCOMPARE (history->size (), 3);
  QCOMPARE (as_string (history->get_view_at_index (0)),
            as_string (view3)); // 最新视图在最前面
}

void
Testview_history::test_remove_view () {
  url view1= url ("tmfs://view/1/test1");
  url view2= url ("tmfs://view/2/test2");
  url view3= url ("tmfs://view/3/test3");

  // 先添加一些个视图
  history->add_view (view1);
  history->add_view (view2);
  history->add_view (view3);
  QCOMPARE (history->size (), 3);

  // 删除中间的视图
  history->remove_view (view2);
  QCOMPARE (history->size (), 2); // 大小应减1
  QCOMPARE (as_string (history->get_view_at_index (0)),
            as_string (view3)); // 剩余视图顺序应正确
  QCOMPARE (as_string (history->get_view_at_index (1)), as_string (view1));

  // 删除不存在的视图
  url nonexistent= url ("tmfs://view/999/nonexistent");
  history->remove_view (nonexistent);
  QCOMPARE (history->size (), 2); // 大小不应改变
  QCOMPARE (as_string (history->get_view_at_index (0)),
            as_string (view3)); // 剩余视图顺序应正确
  QCOMPARE (as_string (history->get_view_at_index (1)), as_string (view1));
}

void
Testview_history::test_size_and_index_access () {
  QCOMPARE (history->size (), 0);

  url view1= url ("tmfs://view/1/test1");
  url view2= url ("tmfs://view/2/test2");

  history->add_view (view1);
  history->add_view (view2);

  QCOMPARE (history->size (), 2);

  // 测试有效索引
  QCOMPARE (as_string (history->get_view_at_index (0)), as_string (view2));
  QCOMPARE (as_string (history->get_view_at_index (1)), as_string (view1));

  // 测试无效索引
  QCOMPARE (as_string (history->get_view_at_index (-1)),
            as_string (url_none ()));
  QCOMPARE (as_string (history->get_view_at_index (10)),
            as_string (url_none ()));
}

void
Testview_history::test_number_operations () {
  url view1= url ("tmfs://view/1/test1");
  url view2= url ("tmfs://view/2/test2");

  history->add_view (view1);
  history->add_view (view2);

  // 测试设置排序依据编号
  history->set_number_at_index (0, 100);
  history->set_number_at_index (1, 200);

  // 测试递增和递减
  history->inc_number_at_index (0); // 100 -> 101
  history->dec_number_at_index (1); // 200 -> 199

  // 测试边界外的操作（不应崩溃）
  history->inc_number_at_index (-1);
  history->dec_number_at_index (10);
  history->set_number_at_index (-1, 999);
  history->set_number_at_index (10, 999);

  // 这些操作不应该崩溃，只是验证没有异常
  QVERIFY (true);
}

void
Testview_history::test_filtered_view_sorting () {
  url view1= url ("tmfs://view/1/test1");
  url view2= url ("tmfs://view/2/test2");
  url view3= url ("tmfs://view/3/test3");

  history->add_view (view1);
  history->add_view (view2);
  history->add_view (view3);

  // 手动设置排序依据编号以测试排序
  history->set_number_at_index (
      0, 30); // view3 (最新添加，但是我们把number设置为最大)
  history->set_number_at_index (1, 10); // view2
  history->set_number_at_index (2, 20); // view1

  // 获取未排序的视图
  view_history::FilteredView unsorted=
      history->get_views_for_window (url_none (), false);
  QCOMPARE (N (unsorted.views), 3);

  // 获取排序后的视图
  view_history::FilteredView sorted=
      history->get_views_for_window (url_none (), true);
  QCOMPARE (N (sorted.views), 3);

  // 验证排序顺序（按编号从小到大：10, 20, 30）
  QCOMPARE (sorted.numbers[0], 10);
  QCOMPARE (sorted.numbers[1], 20);
  QCOMPARE (sorted.numbers[2], 30);
}

void
Testview_history::test_edge_cases () {
  // 测试空历史
  view_history empty_history;
  QCOMPARE (empty_history.size (), 0);
  QCOMPARE (as_string (empty_history.get_view_at_index (0)),
            as_string (url_none ()));

  // 删除不存在的视图
  url nonexistent= url ("tmfs://view/999/nonexistent");
  empty_history.remove_view (nonexistent);
  QCOMPARE (empty_history.size (), 0);

  // 获取空历史的过滤视图
  view_history::FilteredView empty_filtered=
      empty_history.get_views_for_window (url_none (), false);
  QCOMPARE (N (empty_filtered.views), 0);

  // 相同视图的重复添加
  url view= url ("tmfs://view/1/test");
  history->add_view (view);
  history->add_view (view);
  history->add_view (view);
  QCOMPARE (history->size (), 1); // 只有一个
  QCOMPARE (as_string (history->get_view_at_index (0)), as_string (view));
}

QTEST_MAIN (Testview_history)
#include "view_history_test.moc"
