/******************************************************************************
 * MODULE     : parsebib_test.cpp
 * DESCRIPTION: Tests for bib parser
 * COPYRIGHT  : (C) 2026 Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "Bibtex/bibtex.hpp"
#include "Bibtex/bibtex_functions.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "boot.hpp"
#include "converter.hpp"
#include "file.hpp"
#include "server.hpp"
#include "tm_sys_utils.hpp"
#include <s7_tm.hpp>

extern s7_pointer user_env;

class TestParseBib : public QObject {
  Q_OBJECT

private:
  server* sv;

private slots:
  void initTestCase ();
  void test_parse_ascii ();
  void test_parse_1308 ();
  void test_parse_quoted_utf8 ();
  void test_parse_cjk_utf8 ();
};

void
TestParseBib::initTestCase () {
  init_lolly ();
  init_texmacs_home_path ();
  init_texmacs_front ();
  init_tex ();
  int   argc  = 1;
  char* argv[]= {(char*) "parsebib_test", nullptr};
  gui_open (argc, argv);
  if (!tm_s7) {
    tm_s7   = s7_init ();
    user_env= s7_inlet (tm_s7, s7_nil (tm_s7));
    s7_gc_protect (tm_s7, user_env);
  }
  sv= new server (app_type::RESEARCH);
}

void
TestParseBib::test_parse_ascii () {
  string s= "@misc{be1,\n"
            "  title={The Dynamics of Trust},\n"
            "  author={Mohamadali Berahman and Madjid Eshaghi Gordji},\n"
            "  year={2025}\n"
            "}";
  tree   t= parse_bib (s);
  QVERIFY (is_func (t, moebius::DOCUMENT));
  QCOMPARE (N (t), 1);

  tree entry= t[0];
  QVERIFY (is_compound (entry, "bib-entry", 3));
  QCOMPARE (as_charp (entry[0]->label), "misc");
  QCOMPARE (as_charp (entry[1]->label), "be1");

  tree doc   = entry[2];
  tree author= "";
  for (int i= 0; i < N (doc); i++)
    if (doc[i][0] == "author") author= doc[i][1];
  QVERIFY (is_compound (author, "bib-names", 2));
  QCOMPARE (as_charp (author[0][0]->label), "Mohamadali");
  QCOMPARE (as_charp (author[0][2]->label), "Berahman");
  QCOMPARE (as_charp (author[1][0]->label), "Madjid Eshaghi");
  QCOMPARE (as_charp (author[1][2]->label), "Gordji");
}

void
TestParseBib::test_parse_1308 () {
  string s;
  url    u  = url_system (string ("$TEXMACS_PATH/tests/bib/1308.bib"));
  bool   err= load_string (u, s, true);
  QVERIFY2 (!err, "cannot load 1308.bib");

  tree t= parse_bib (s);
  QVERIFY (is_func (t, moebius::DOCUMENT));
  QCOMPARE (N (t), 2);

  // Check entry 0: be1
  tree e1= t[0];
  QVERIFY (is_compound (e1, "bib-entry", 3));
  QCOMPARE (as_charp (e1[0]->label), "misc");
  QCOMPARE (as_charp (e1[1]->label), "be1");

  // Check entry 1: be2 with accent "é" in "Noé"
  tree e2= t[1];
  QVERIFY (is_compound (e2, "bib-entry", 3));
  QCOMPARE (as_charp (e2[0]->label), "misc");
  QCOMPARE (as_charp (e2[1]->label), "be2");

  tree doc2  = e2[2];
  tree author= "";
  for (int i= 0; i < N (doc2); i++)
    if (doc2[i][0] == "author") author= doc2[i][1];
  QVERIFY (is_compound (author, "bib-names", 3));

  // Author 0: Noé Stauffer
  tree   a0        = author[0];
  string first_name= a0[0]->label;
  QCOMPARE (as_charp (cork_to_utf8 (first_name)), "Noé");
  QCOMPARE (as_charp (a0[2]->label), "Stauffer");

  // Author 1: Hossein Gorji
  tree a1= author[1];
  QCOMPARE (as_charp (a1[0]->label), "Hossein");
  QCOMPARE (as_charp (a1[2]->label), "Gorji");

  // Author 2: Ivan Lunati
  tree a2= author[2];
  QCOMPARE (as_charp (a2[0]->label), "Ivan");
  QCOMPARE (as_charp (a2[2]->label), "Lunati");
}

void
TestParseBib::test_parse_quoted_utf8 () {
  string s= "@article{cite1,\n"
            "  author = \"René Descartes\",\n"
            "  title = \"Discours de la méthode\",\n"
            "  year = \"1637\"\n"
            "}";
  tree   t= parse_bib (s);
  QVERIFY (is_func (t, moebius::DOCUMENT));
  QCOMPARE (N (t), 1);

  tree doc   = t[0][2];
  tree author= "";
  for (int i= 0; i < N (doc); i++)
    if (doc[i][0] == "author") author= doc[i][1];
  QVERIFY (is_compound (author, "bib-names", 1));
  QCOMPARE (as_charp (cork_to_utf8 (author[0][0]->label)), "René");
  QCOMPARE (as_charp (cork_to_utf8 (author[0][2]->label)), "Descartes");
}

void
TestParseBib::test_parse_cjk_utf8 () {
  string s= "@article{wang2024,\n"
            "  author = {王青 and 张志学},\n"
            "  title = {基于深度学习的肺癌病理图像分类方法},\n"
            "  year = {2024}\n"
            "}";
  tree   t= parse_bib (s);
  QVERIFY (is_func (t, moebius::DOCUMENT));
  QCOMPARE (N (t), 1);

  tree doc   = t[0][2];
  tree title = "";
  tree author= "";
  for (int i= 0; i < N (doc); i++) {
    if (doc[i][0] == "title") title= doc[i][1];
    if (doc[i][0] == "author") author= doc[i][1];
  }
  QCOMPARE (as_charp (cork_to_utf8 (title->label)),
            "基于深度学习的肺癌病理图像分类方法");
  QVERIFY (is_compound (author, "bib-names", 2));
  QCOMPARE (as_charp (cork_to_utf8 (author[0][2]->label)), "王青");
  QCOMPARE (as_charp (cork_to_utf8 (author[1][2]->label)), "张志学");
}

QTEST_MAIN (TestParseBib)
#include "parsebib_test.moc"
