/******************************************************************************
 * MODULE     : progressive_typeset_test.cpp
 * DESCRIPTION: Unit tests for progressive typesetting regression
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Bridge/impl_typesetter.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

extern tree the_et;

static edit_env
create_test_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  edit_env              env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->style_init_env ();
  env->write (PAGE_MEDIUM, "paper");
  env->update ();
  return env;
}

static tree
create_long_document (int n) {
  tree doc (DOCUMENT, n);
  for (int i= 0; i < n; i++) {
    doc[i]= string ("Paragraph text line with content ") * as_string (i);
  }
  return doc;
}

class TestProgressiveTypeset : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
    moebius::drd::init_std_drd ();
  }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_initial_chunk ();
  void test_fast_forward_and_no_page_loss_after_edit ();
  void test_offset_alignment_between_frozen_and_unfrozen ();
};

void
TestProgressiveTypeset::test_initial_chunk () {
  edit_env env  = create_test_env ();
  int      n    = 80;
  tree     doc  = create_long_document (n);
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  box b= ttt->typeset ();
  QVERIFY (ttt->progressive_enabled);
  QCOMPARE (ttt->progressive_cur_child, 50);
  QVERIFY (ttt->typeset_in_progress ());

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_fast_forward_and_no_page_loss_after_edit () {
  edit_env env  = create_test_env ();
  int      n    = 100;
  tree     doc  = create_long_document (n);
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  // Initial typeset: 50 paragraphs
  box b1= ttt->typeset ();
  QCOMPARE (ttt->progressive_cur_child, 50);

  // Fast-forward to paragraph 70 (like outline sidebar click)
  ttt->progressive_fast_forward_until (70);
  box b2               = ttt->typeset ();
  int pages_before_edit= ttt->last_page_count;
  QVERIFY (pages_before_edit > 0);
  QVERIFY (ttt->progressive_cur_child >= 70);

  // Simulate editing paragraph 70 (e.g. inserting Return)
  the_et= substitute (doc, 70, tree ("new inserted paragraph"));
  notify_insert (ttt, path (70), tuple (tree ("new inserted paragraph")));
  box b3              = ttt->typeset ();
  int pages_after_edit= ttt->last_page_count;

  // Invariant 1: Total pages after edit must not decrease
  QVERIFY (pages_after_edit >= pages_before_edit);
  // Frontier after edit should cover at least the previous typeset frontier
  QVERIFY (ttt->progressive_cur_child >= 70);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_offset_alignment_between_frozen_and_unfrozen () {
  edit_env env  = create_test_env ();
  int      n    = 120;
  tree     doc  = create_long_document (n);
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  // Advance progressive typesetting in multiple steps
  ttt->typeset ();
  while (ttt->typeset_in_progress ()) {
    int prev_frozen= N (ttt->frozen_page_boxes);
    ttt->progressive_advance (30);
    ttt->typeset ();
    int new_frozen= N (ttt->frozen_page_boxes);
    // Frozen pages must grow monotonically
    QVERIFY (new_frozen >= prev_frozen);
  }

  // Once fully typeset, frontier must reach n
  QCOMPARE (ttt->progressive_cur_child, n);
  QVERIFY (!ttt->typeset_in_progress ());

  delete_typesetter (ttt);
}

#ifdef QTTEXMACS
QTEST_MAIN (TestProgressiveTypeset)
#else
int
main () {
  return 0;
}
#endif
#include "progressive_typeset_test.moc"
