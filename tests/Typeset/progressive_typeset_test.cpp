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
  static drd_info              drd ("none", std_drd);
  static hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  static hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  static hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  h1= hashmap<string, tree> (UNINIT);
  h2= hashmap<string, tree> (UNINIT);
  h3= hashmap<string, tree> (UNINIT);
  h4= hashmap<string, tree> (UNINIT);
  h5= hashmap<string, tree> (UNINIT);
  h6= hashmap<string, tree> (UNINIT);
  edit_env env (drd, "none", h1, h2, h3, h4, h5, h6);
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
  void test_nested_document_in_later_chunk ();
  void test_append_at_end_after_full_typeset ();
  void test_progressive_preserves_local_aux_across_chunks ();
  void test_blank_pages_at_freeze_boundary ();
  void test_retypeset_after_full_typeset_without_reset ();
  void test_progressive_preserves_env_counters_across_chunks ();
  void test_remove_at_end_legal_page_shrink_no_duplicate ();
  void test_remove_relative_to_progressive_frontier ();
};

void
TestProgressiveTypeset::test_initial_chunk () {
  edit_env env  = create_test_env ();
  int      n    = 80;
  tree     doc  = create_long_document (n);
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  ttt->typeset ();
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
  ttt->typeset ();
  QCOMPARE (ttt->progressive_cur_child, 50);

  // Fast-forward to paragraph 70 (like outline sidebar click)
  ttt->progressive_fast_forward_until (70);
  box b2               = ttt->typeset ();
  int pages_before_edit= N (b2[0]);
  QVERIFY (pages_before_edit > 0);
  QVERIFY (ttt->progressive_cur_child >= 70);

  // Simulate editing paragraph 70 (e.g. inserting Return)
  the_et= substitute (doc, 70, tree ("new inserted paragraph"));
  notify_insert (ttt, path (70), tuple (tree ("new inserted paragraph")));
  box b3              = ttt->typeset ();
  int pages_after_edit= N (b3[0]);

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

void
TestProgressiveTypeset::test_nested_document_in_later_chunk () {
  edit_env env= create_test_env ();
  int      n  = 80;
  tree     doc (DOCUMENT, n);
  for (int i= 0; i < n; i++) {
    if (i == 50) {
      // 嵌套 document 结构（模拟 equation*、equations 或表格单元格）
      doc[i]= tree (DOCUMENT, tree ("Formula text in second chunk"));
    }
    else {
      doc[i]= string ("Paragraph text line ") * as_string (i);
    }
  }
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  // 1. 初始排版：首块覆盖 0..50
  ttt->typeset ();
  QCOMPARE (ttt->progressive_cur_child, 50);

  // 2. 推进排版前沿至第二块（50..80）
  ttt->progressive_advance (30);
  box  b    = ttt->typeset ();
  bool found= false;
  b->find_box_path (path (50, 0, 0), found);
  QVERIFY (found);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_append_at_end_after_full_typeset () {
  edit_env env  = create_test_env ();
  int      n    = 120;
  tree     doc  = create_long_document (n);
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  // 1. 初始排版并快进至文末，使全文档排版完成
  ttt->typeset ();
  ttt->progressive_fast_forward_until (n - 1);
  box b1          = ttt->typeset ();
  int pages_before= N (b1[0]);
  QVERIFY (!ttt->typeset_in_progress ());
  QCOMPARE (ttt->progressive_cur_child, n);

  // 2. 在文末插入新段落（模拟文末按回车）
  tree new_para ("");
  the_et= doc * tuple (new_para);
  notify_insert (ttt, path (n), tuple (new_para));
  box b2         = ttt->typeset ();
  int pages_after= N (b2[0]);

  // 3. 验证首轮重排即完整覆盖新插入节点，不落入未完成分块状态
  QVERIFY (!ttt->typeset_in_progress ());
  QCOMPARE (ttt->progressive_cur_child, n + 1);
  QVERIFY (pages_after >= pages_before);

  bool found= false;
  b2->find_box_path (path (n, 0), found);
  QVERIFY (found);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_progressive_preserves_local_aux_across_chunks () {
  edit_env env= create_test_env ();
  int      n  = 80;
  tree     doc (DOCUMENT, n);
  for (int i= 0; i < n; i++) {
    if (i == 5) {
      doc[i]= tree (CONCAT, tree (WRITE, "toc", "Section in chunk 0"));
    }
    else if (i == 60) {
      doc[i]= tree (CONCAT, tree (WRITE, "toc", "Section in chunk 1"));
    }
    else {
      doc[i]= tree ("Paragraph " * as_string (i));
    }
  }
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  // 1. 首块 [0, 50)：尚未抵达文末，处于排版中状态，同时收集 chunk 0 的 toc 条目
  ttt->typeset ();
  QVERIFY (ttt->typeset_in_progress ());
  QVERIFY (env->local_aux->contains ("toc"));
  QCOMPARE (N (env->local_aux["toc"]), 1);

  // 2. 推进至文末 [50, 80)：完成全量排版，且累计保留全部分块的 local_aux
  ttt->progressive_advance (30);
  ttt->typeset ();
  QVERIFY (!ttt->typeset_in_progress ());
  QVERIFY (env->local_aux->contains ("toc"));
  QCOMPARE (N (env->local_aux["toc"]), 2);
  QCOMPARE (env->local_aux["toc"][0], tree ("Section in chunk 0"));
  QCOMPARE (env->local_aux["toc"][1], tree ("Section in chunk 1"));

  delete_typesetter (ttt);
}

static bool
box_contains_text (box b, const string& needle) {
  if (b->get_type () == TEXT_BOX) {
    string s= b->get_leaf_string ();
    if (occurs (needle, s)) return true;
  }
  for (int i= 0; i < b->subnr (); i++) {
    if (box_contains_text (b->subbox (i), needle)) return true;
  }
  return false;
}

void
TestProgressiveTypeset::test_blank_pages_at_freeze_boundary () {
  edit_env env= create_test_env ();
  int      n  = 160;
  tree     doc (DOCUMENT, n);
  doc[0]= tree ("UniqueCoverTitle");
  doc[1]= tree (NEW_PAGE);
  doc[2]= tree ("VersoNote");
  doc[3]= tree (NEW_PAGE);
  doc[4]= tree (NEW_DPAGE);
  for (int i= 5; i < n; i++) {
    doc[i]= string ("Chapter body paragraph line with enough words ") *
            as_string (i);
  }
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  box last_box= ttt->typeset ();
  while (ttt->typeset_in_progress ()) {
    ttt->progressive_advance (30);
    last_box= ttt->typeset ();
  }

  QVERIFY (last_box->subnr () == 1);
  box pages_box = last_box->subbox (0);
  int title_hits= 0;
  for (int pi= 0; pi < pages_box->subnr (); pi++) {
    if (box_contains_text (pages_box->subbox (pi), "UniqueCoverTitle")) {
      title_hits++;
    }
  }
  QCOMPARE (title_hits, 1);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_retypeset_after_full_typeset_without_reset () {
  edit_env env= create_test_env ();
  int      n  = 120;
  tree     doc (DOCUMENT, n);
  doc[0]= tree ("UniqueCoverTitle");
  for (int i= 1; i < n; i++) {
    doc[i]= string ("Chapter body paragraph line with enough words ") *
            as_string (i);
  }
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  box b1= ttt->typeset ();
  while (ttt->typeset_in_progress ()) {
    ttt->progressive_advance (30);
    b1= ttt->typeset ();
  }
  QCOMPARE (ttt->progressive_cur_child, n);
  int pages_first= N (b1[0]);
  QVERIFY (pages_first > 0);

  // 1. 渐进排版完成后，未调用 progressive_reset() 直接再次触发 typeset()
  box b2= ttt->typeset ();
  QCOMPARE (N (b2[0]), pages_first);
  QCOMPARE (ttt->progressive_cur_child, n);

  // 2. 经过一次 reset + 全量 typeset()（使 chunk_start=0, chunk_end=n）后，
  //    再次直接触发 typeset()，仍不得将整书页面叠加到 frozen_page_boxes 之后
  ttt->progressive_reset ();
  box b3= ttt->typeset ();
  QCOMPARE (N (b3[0]), pages_first);
  box b4= ttt->typeset ();
  QCOMPARE (N (b4[0]), pages_first);

  QVERIFY (b4->subnr () == 1);
  box pages_box = b4->subbox (0);
  int title_hits= 0;
  for (int pi= 0; pi < pages_box->subnr (); pi++) {
    if (box_contains_text (pages_box->subbox (pi), "UniqueCoverTitle")) {
      title_hits++;
    }
  }
  QCOMPARE (title_hits, 1);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::
    test_progressive_preserves_env_counters_across_chunks () {
  edit_env env= create_test_env ();
  int      n  = 100;
  tree     doc (DOCUMENT, n);
  for (int i= 0; i < n; i++) {
    doc[i]= string ("Paragraph ") * as_string (i);
  }
  doc[10]       = tree (ASSIGN, "section-nr", "5");
  doc[60]       = tree (ASSIGN, "equation-nr",
                        tree (PLUS, tree (VALUE, "section-nr"), "1"));
  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  ttt->typeset ();
  QCOMPARE (ttt->progressive_cur_child, 50);
  QCOMPARE (env->read ("section-nr"), tree ("5"));

  // 模拟 edit_typeset_rep::typeset_prepare() 在分块间重置 env
  env->write_default_env ();
  env->style_init_env ();
  env->write (PAGE_MEDIUM, "paper");
  env->update ();

  ttt->progressive_fast_forward_until (60);
  ttt->typeset ();

  QCOMPARE (env->read ("section-nr"), tree ("5"));
  QCOMPARE (env->read ("equation-nr"), tree ("6"));

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_remove_at_end_legal_page_shrink_no_duplicate () {
  edit_env env= create_test_env ();
  int      n  = 80;
  tree     doc (DOCUMENT, n);
  doc[0]= tree ("UniqueShrinkDocTitle");
  for (int i= 1; i < 77; i++) {
    doc[i]= string ("Paragraph body content with enough text line ") *
            as_string (i);
  }
  doc[77]= tree (NEW_PAGE);
  doc[78]= tree ("Extra trailing page paragraph 1");
  doc[79]= tree ("Extra trailing page paragraph 2");

  the_et        = doc;
  typesetter ttt= new_typesetter (env, doc, path ());

  box b1= ttt->typeset ();
  while (ttt->typeset_in_progress ()) {
    ttt->progressive_advance (30);
    b1= ttt->typeset ();
  }
  QCOMPARE (ttt->progressive_cur_child, n);
  QVERIFY (!ttt->typeset_in_progress ());
  int pages_before= N (b1[0]);
  QVERIFY (pages_before > 1);

  // 删除末尾 3 个节点（包含强制分页符和尾页段落），使页数严格减少
  the_et= doc (0, 77);
  notify_remove (ttt, path (77), 3);

  box b2         = ttt->typeset ();
  int pages_after= N (b2[0]);

  // 1. 页数严格减少，不落入错误兜底二次排版翻倍
  QVERIFY (pages_after < pages_before);
  // 2. 首轮重排即覆盖全部剩余节点，不残留 pending
  QCOMPARE (ttt->progressive_cur_child, 77);
  QVERIFY (!ttt->typeset_in_progress ());

  // 3. 唯一标题只出现一次
  QVERIFY (b2->subnr () == 1);
  box pages_box = b2->subbox (0);
  int title_hits= 0;
  for (int pi= 0; pi < pages_box->subnr (); pi++) {
    if (box_contains_text (pages_box->subbox (pi), "UniqueShrinkDocTitle")) {
      title_hits++;
    }
  }
  QCOMPARE (title_hits, 1);

  delete_typesetter (ttt);
}

void
TestProgressiveTypeset::test_remove_relative_to_progressive_frontier () {
  edit_env env= create_test_env ();

  // Case 1: 在排版前沿之前删除节点 [10, 15)
  {
    int  n        = 100;
    tree doc      = create_long_document (n);
    the_et        = doc;
    typesetter ttt= new_typesetter (env, doc, path ());

    ttt->typeset ();
    QCOMPARE (ttt->progressive_cur_child, 50);
    QCOMPARE (ttt->max_typeset_child, 50);

    the_et= doc (0, 10) * doc (15, n);
    notify_remove (ttt, path (10), 5);
    QCOMPARE (ttt->max_typeset_child, 45);

    box b= ttt->typeset ();
    QCOMPARE (ttt->chunk_start, 0);
    QCOMPARE (ttt->chunk_end, 45);
    QCOMPARE (ttt->progressive_cur_child, 45);
    QVERIFY (ttt->typeset_in_progress ());

    // 检查内容完整性：原 15 号段落前移至 10 号
    bool found_10= false;
    b->find_box_path (path (10, 0), found_10);
    QVERIFY (found_10);

    delete_typesetter (ttt);
  }

  // Case 2: 跨越排版前沿删除节点 [45, 55)
  {
    int  n        = 100;
    tree doc      = create_long_document (n);
    the_et        = doc;
    typesetter ttt= new_typesetter (env, doc, path ());

    ttt->typeset ();
    QCOMPARE (ttt->progressive_cur_child, 50);
    QCOMPARE (ttt->max_typeset_child, 50);

    the_et= doc (0, 45) * doc (55, n);
    notify_remove (ttt, path (45), 10);
    QCOMPARE (ttt->max_typeset_child, 45);

    box b= ttt->typeset ();
    QCOMPARE (ttt->chunk_start, 0);
    QCOMPARE (ttt->chunk_end, 45);
    QCOMPARE (ttt->progressive_cur_child, 45);
    QVERIFY (ttt->typeset_in_progress ());

    bool found_44= false;
    b->find_box_path (path (44, 0), found_44);
    QVERIFY (found_44);

    delete_typesetter (ttt);
  }

  // Case 3: 在排版前沿之后删除节点 [70, 75)
  {
    int  n        = 100;
    tree doc      = create_long_document (n);
    the_et        = doc;
    typesetter ttt= new_typesetter (env, doc, path ());

    ttt->typeset ();
    QCOMPARE (ttt->progressive_cur_child, 50);
    QCOMPARE (ttt->max_typeset_child, 50);

    the_et= doc (0, 70) * doc (75, n);
    notify_remove (ttt, path (70), 5);
    QCOMPARE (ttt->max_typeset_child, 50);

    box b= ttt->typeset ();
    QCOMPARE (ttt->chunk_start, 0);
    QCOMPARE (ttt->chunk_end, 50);
    QCOMPARE (ttt->progressive_cur_child, 50);
    QVERIFY (ttt->typeset_in_progress ());

    while (ttt->typeset_in_progress ()) {
      ttt->progressive_advance (30);
      b= ttt->typeset ();
    }
    QCOMPARE (ttt->progressive_cur_child, 95);
    QVERIFY (!ttt->typeset_in_progress ());

    bool found_69= false;
    b->find_box_path (path (69, 0), found_69);
    QVERIFY (found_69);

    delete_typesetter (ttt);
  }
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
