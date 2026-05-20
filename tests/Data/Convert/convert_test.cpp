/******************************************************************************
 * MODULE     : converter_test.cpp
 * DESCRIPTION: Properties of characters and strings
 * COPYRIGHT  : (C) 2023 jingkaimori
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "convert.hpp"
#include "file.hpp"
#include "tm_ostream.hpp"
#include "tree_helper.hpp"

using namespace moebius;

Q_DECLARE_METATYPE (tree)
Q_DECLARE_METATYPE (string)

class TestConverter : public QObject {
  Q_OBJECT

private slots:
  void test_search_metadata_data ();
  void test_search_metadata ();
  void test_tmu_raw_data ();
  void test_tmu_plain_text ();
  void test_tmu_escape_sequences ();
  void test_tmu_document ();
  void test_tmu_compound ();
  void test_tmu_nested_compound ();
  void test_tmu_raw_data_performance ();
  void test_tmu_text_performance ();
};

void
TestConverter::test_search_metadata_data () {
  QTest::addColumn<tree> ("input_tree");
  QTest::addColumn<string> ("title");
  QTest::addColumn<string> ("author");
  QTest::addColumn<string> ("keyword");
  QTest::addColumn<string> ("invalid");

  string empty ("");
  QTest::newRow ("regular document")
      << tree (DOCUMENT,
               compound (
                   "doc-data", compound ("doc-title", "Test of Metadata"),
                   compound ("doc-author",
                             compound ("author-data",
                                       compound ("author-name", "author1"))),
                   compound ("doc-author",
                             compound ("author-data",
                                       compound ("author-name", "author2"))),
                   compound ("doc-author",
                             compound ("author-data",
                                       compound ("author-name", "author3")))),
               compound ("abstract-data",
                         compound ("abstract-keywords", "keyword 1",
                                   "keyword 2", "keyword 3")))
      << string ("Test of Metadata") << string ("author1, author2, author3")
      << string ("keyword 1, keyword 2, keyword 3") << empty;
  QTest::newRow ("texmacs document")
      << tree (DOCUMENT, compound ("tmdoc-title", "Test of manual title"))
      << string ("Test of manual title") << empty << empty << empty;
}

void
TestConverter::test_search_metadata () {
  QFETCH (tree, input_tree);
  QFETCH (string, title);
  QFETCH (string, author);
  QFETCH (string, keyword);
  QFETCH (string, invalid);
  qcompare (search_metadata (input_tree, "title"), title);
  qcompare (search_metadata (input_tree, "author"), author);
  qcompare (search_metadata (input_tree, "keyword"), keyword);
  qcompare (search_metadata (input_tree, "invalid"), invalid);
}

void
TestConverter::test_tmu_raw_data () {
  string s= "<#41424344>";
  tree   t= tmu_to_tree (s);
  QVERIFY (is_func (t, RAW_DATA));
  QCOMPARE (N (t), 1);
  qcompare (as_string (t[0]), string ("ABCD"));
}

void
TestConverter::test_tmu_plain_text () {
  tree t= tmu_to_tree ("hello");
  QVERIFY (is_func (t, DOCUMENT));
  QCOMPARE (N (t), 1);
  QVERIFY (t[0] == tree ("hello"));

  tree t2= tmu_to_tree ("hello world");
  QVERIFY (is_func (t2, DOCUMENT));
  QCOMPARE (N (t2), 1);
  QVERIFY (t2[0] == tree ("hello world"));
}

void
TestConverter::test_tmu_escape_sequences () {
  tree t1= tmu_to_tree ("a\\;b");
  QVERIFY (is_func (t1, DOCUMENT));
  QVERIFY (t1[0] == tree ("ab"));

  tree t2= tmu_to_tree ("a\\\\b");
  QVERIFY (is_func (t2, DOCUMENT));
  QVERIFY (t2[0] == tree ("a\\b"));

  tree t3= tmu_to_tree ("a\\|b");
  QVERIFY (is_func (t3, DOCUMENT));
  QVERIFY (t3[0] == tree ("a|b"));

  tree t4= tmu_to_tree ("a\\>b");
  QVERIFY (is_func (t4, DOCUMENT));
  QVERIFY (t4[0] == tree ("a>b"));

  QVERIFY (tmu_to_tree ("a\\<b")[0] == tree ("a<b"));
  QVERIFY (tmu_to_tree ("\\;")[0] == tree (""));
  QVERIFY (tmu_to_tree ("no escape")[0] == tree ("no escape"));
}

void
TestConverter::test_tmu_document () {
  string s= "line1\n\nline2\n\nline3";
  tree   t= tmu_to_tree (s);
  QVERIFY (is_func (t, DOCUMENT));
  QCOMPARE (N (t), 3);
  QVERIFY (t[0] == tree ("line1"));
  QVERIFY (t[1] == tree ("line2"));
  QVERIFY (t[2] == tree ("line3"));
}

void
TestConverter::test_tmu_compound () {
  tree t= tmu_to_tree ("<bold|text>");
  QVERIFY (is_func (t, DOCUMENT));
  QCOMPARE (N (t), 1);
  QVERIFY (is_compound (t[0]));
  QVERIFY (t[0][0] == tree ("text"));
}

void
TestConverter::test_tmu_nested_compound () {
  tree t= tmu_to_tree ("<with|color|red|hello>");
  QVERIFY (is_func (t, DOCUMENT));
  QCOMPARE (N (t), 1);
  QVERIFY (is_compound (t[0]));
  QCOMPARE (N (t[0]), 3);
  QVERIFY (t[0][0] == tree ("color"));
  QVERIFY (t[0][1] == tree ("red"));
  QVERIFY (t[0][2] == tree ("hello"));
}

void
TestConverter::test_tmu_raw_data_performance () {
  string hex_data;
  for (int i= 0; i < 1000000; i++) {
    hex_data << '4' << '1';
  }
  string s= "<TMU|<tuple|1.1.0|2025.1.5>>\n<#";
  s << hex_data;
  s << ">";

  QElapsedTimer timer;
  timer.start ();
  tree   t      = tmu_document_to_tree (s);
  qint64 elapsed= timer.elapsed ();

  cout << "Performance: parsed 1M hex bytes in " << (int) elapsed << " ms\n";
  QVERIFY (!is_compound (t, "error"));
}

void
TestConverter::test_tmu_text_performance () {
  string text;
  for (int i= 0; i < 1000000; i++) {
    text << 'a';
  }
  string s= "<TMU|<tuple|1.1.0|2025.1.5>>\n<text|";
  s << text;
  s << "|>";

  QElapsedTimer timer;
  timer.start ();
  tree   t      = tmu_document_to_tree (s);
  qint64 elapsed= timer.elapsed ();

  cout << "Performance: parsed 1M text chars in " << (int) elapsed << " ms\n";
  QVERIFY (!is_compound (t, "error"));
}

QTEST_MAIN (TestConverter)
#include "convert_test.moc"
