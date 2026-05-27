
/******************************************************************************
 * MODULE     : view_type_test.cpp
 * DESCRIPTION: is_tmfs_view_type 的单元测试
 * COPYRIGHT  : (C) 2025 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "analyze.hpp" // for occurs
#include "base.hpp"
#include "new_view.hpp"

class Test_view_type : public QObject {
  Q_OBJECT

private slots:
  void test_normal_cases ();
  void test_aux_cases ();
  void test_live_cases ();
  void test_broken_cases ();
};

void
Test_view_type::test_normal_cases () {
  string normal_url_1=
      "tmfs://view/1/default/Users/texts/scratch/no_name_5.tmu";
  string normal_url_2=
      "tmfs://view/2/default/Users/jimzhou/Downloads/user_guide.tmu";
  string normal_url_114514= "tmfs://view/114514/default/Users/Documents/A.tmu";
  QCOMPARE (is_tmfs_view_type (normal_url_1, "default"), true);
  QCOMPARE (is_tmfs_view_type (normal_url_1, "aux"), false);
  QCOMPARE (is_tmfs_view_type (normal_url_2, "default"), true);
  QCOMPARE (is_tmfs_view_type (normal_url_114514, "default"), true);
  QCOMPARE (is_tmfs_view_type (normal_url_2, "aux"), false);
}

void
Test_view_type::test_aux_cases () {
  string aux_url_1       = "tmfs://view/1/tmfs/aux/edit-strong";
  string aux_url_2       = "tmfs://view/2/tmfs/aux/message-editor";
  string aux_url_11919810= "tmfs://view/1919810/tmfs/aux/edit-strong";
  QCOMPARE (is_tmfs_view_type (aux_url_1, "aux"), true);
  QCOMPARE (is_tmfs_view_type (aux_url_2, "aux"), true);
  QCOMPARE (is_tmfs_view_type (aux_url_11919810, "aux"), true);
  QCOMPARE (is_tmfs_view_type (aux_url_1, "default"), false);
}

void
Test_view_type::test_live_cases () {
  string live_url_1= "tmfs://view/1/tmfs/live/test.tmu";
  string live_url_2= "tmfs://view/2/tmfs/live/no-name.tmu";
  string live_url_3= "tmfs://view/3/tmfs/live/user_guide.tm";
  QCOMPARE (is_tmfs_view_type (live_url_1, "live"), true);
  QCOMPARE (is_tmfs_view_type (live_url_2, "live"), true);
  QCOMPARE (is_tmfs_view_type (live_url_3, "live"), true);
  QCOMPARE (is_tmfs_view_type (live_url_1, "aux"), false);
}

void
Test_view_type::test_broken_cases () {
  string broken_url_1=
      "ntfs://view/1/default/Users/texts/scratch/no_name_5.tmu";
  string broken_url_2= "tmfs://view//tmfs/aux/edit-strong";
  string broken_url_3=
      "tmfs://view/<number>/default/Users/texts/scratch/no_name_5.tmu";
  string broken_url_4= "tmfs://view/abc/tmfs/aux/edit-strong";
  QCOMPARE (is_tmfs_view_type (broken_url_1, "default"), false);
  QCOMPARE (is_tmfs_view_type (broken_url_2, "aux"), false);
  QCOMPARE (is_tmfs_view_type (broken_url_3, "default"), false);
  QCOMPARE (is_tmfs_view_type (broken_url_4, "aux"), false);
}

QTEST_MAIN (Test_view_type)
#include "view_type_test.moc"
