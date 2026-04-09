/******************************************************************************
 * MODULE     : bridge_ornamented_rep_test.cpp
 * DESCRIPTION: Tests for footnote propagation in bridge_ornamented_rep
 * COPYRIGHT   : (C) 2026 Mingshen Chu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Boxes/construct.hpp"
#include "Line/lazy_paragraph.hpp"
#include "Line/lazy_vstream.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

static edit_env
create_test_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  return edit_env (drd, "none", h1, h2, h3, h4, h5, h6);
}

static tree
create_ornament_with_footnote () {
  tree footnote_body (DOCUMENT, 1);
  footnote_body[0]= tree (CONCAT, "footnote body");

  tree paragraph (CONCAT);
  paragraph << "boxed theorem body";
  paragraph << tree (FLOAT, "footnote", "", footnote_body);
  paragraph << " continues";

  tree body (DOCUMENT, 1);
  body[0]= paragraph;

  return tree (ORNAMENT, body);
}

static bool
has_footnote (array<page_item> items) {
  for (int i= 0; i < N (items); ++i)
    for (int j= 0; j < N (items[i]->fl); ++j) {
      lazy_vstream ins= (lazy_vstream) items[i]->fl[j];
      if (is_tuple (ins->channel, "footnote")) return true;
    }
  return false;
}

static array<lazy>
collect_attached_floats_for_test (array<page_item> items) {
  array<lazy> fl;
  for (int i= 0; i < N (items); ++i)
    if (N (items[i]->fl) > 0) fl << items[i]->fl;
  return fl;
}

class TestBridgeOrnamentedRep : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  void keeps_footnote_float ();
};

void
TestBridgeOrnamentedRep::keeps_footnote_float () {
  edit_env env= create_test_env ();
  env->style_init_env ();
  env->update ();

  array<page_item> inner_items (1);
  array<page_item> footnote_lines (1);
  footnote_lines[0]= page_item (empty_box (path (0)));
  array<lazy> fl (1);
  fl[0]         = lazy_vstream (path (0), tuple ("footnote"), footnote_lines,
                                stack_border ());
  inner_items[0]= page_item (empty_box (path (1)), fl);

  array<lazy> ornament_fl= collect_attached_floats_for_test (inner_items);
  QVERIFY (has_footnote (inner_items));

  lazy_paragraph par (env, path ());
  par->a << line_item (STD_ITEM, env->mode_op, empty_box (path (2)),
                       HYPH_INVALID);
  par->format_paragraph ();

  int i= N (par->sss->l) - 1;
  while (i >= 0 && par->sss->l[i]->type == PAGE_CONTROL_ITEM)
    i--;
  QVERIFY (i >= 0);

  par->sss->l[i]= copy (par->sss->l[i]);
  par->sss->l[i]->fl << ornament_fl;

  QVERIFY (has_footnote (par->sss->l));
}

QTEST_MAIN (TestBridgeOrnamentedRep)
#include "bridge_ornamented_rep_test.moc"
