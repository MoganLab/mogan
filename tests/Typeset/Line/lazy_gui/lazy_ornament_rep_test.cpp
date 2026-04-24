/******************************************************************************
 * MODULE     : lazy_ornament_rep_test.cpp
 * DESCRIPTION: Tests for footnote propagation in lazy_ornament_rep
 * COPYRIGHT   : (C) 2026 Mingshen Chu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Format/format.hpp"
#include "Line/lazy_vstream.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "formatter.hpp"
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

static int
count_footnotes (array<page_item> items) {
  int total= 0;
  for (int i= 0; i < N (items); ++i)
    for (int j= 0; j < N (items[i]->fl); ++j) {
      lazy_vstream ins= (lazy_vstream) items[i]->fl[j];
      if (is_tuple (ins->channel, "footnote")) total++;
    }
  return total;
}

static bool
has_footnote (array<page_item> items) {
  return count_footnotes (items) > 0;
}

class TestLazyOrnamentRep : public QObject {
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
TestLazyOrnamentRep::keeps_footnote_float () {
  edit_env env     = create_test_env ();
  tree     ornament= create_ornament_with_footnote ();

  lazy lz= make_lazy (env, ornament, path ());
  lazy produced=
      lz->produce (LAZY_VSTREAM, make_format_vstream (600 * PIXEL, 0, 0));
  lazy_vstream vs= (lazy_vstream) produced;

  QVERIFY (has_footnote (vs->l));
}

QTEST_MAIN (TestLazyOrnamentRep)
#include "lazy_ornament_rep_test.moc"
