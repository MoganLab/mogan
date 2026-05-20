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
  void test_tmu_raw_data_performance ();
  void test_tmu_text_performance ();
  void test_tmu_real_file_performance ();
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
TestConverter::test_tmu_raw_data_performance () {
  string hex_data;
  for (int i= 0; i < 1000000; i++) {
    hex_data << string ("41");
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

void
TestConverter::test_tmu_real_file_performance () {
  url    u= url_system ("/home/da/DevTeam/chapter-4.tmu");
  string doc_s;
  if (load_string (u, doc_s, false)) {
    QSKIP ("chapter-4.tmu not found");
  }

  QElapsedTimer timer;
  timer.start ();
  tree   t      = tmu_document_to_tree (doc_s);
  qint64 elapsed= timer.elapsed ();

  cout << "Performance: parsed chapter-4.tmu (" << N (doc_s) << " bytes) in "
       << (int) elapsed << " ms\n";
  QVERIFY (!is_compound (t, "error"));
}

QTEST_MAIN (TestConverter)
#include "convert_test.moc"
