
/******************************************************************************
 * MODULE     : rounded_text_bg_test.cpp
 * DESCRIPTION: Tests for rounded text background in concat_box
 ******************************************************************************/

#include "Boxes/construct.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "concater.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "font.hpp"
#include "moebius/drd/drd_std.hpp"
#include "smart_font.hpp"
#include "sys_utils.hpp"
#include "tm_debug.hpp"
#include "tm_sys_utils.hpp"
#include <QtTest/QtTest>

// Minimal mock renderer for testing pre_display/post_display
class mock_renderer_rep : public renderer_rep {
public:
  int       last_pixel;
  color     last_bg;
  pencil    last_pen;
  brush     last_brush;
  int       clear_count;
  int       polygon_count;
  SI        last_cx1, last_cy1, last_cx2, last_cy2;
  array<SI> last_px, last_py;

  mock_renderer_rep ()
      : renderer_rep (false), last_pixel (256), last_bg (black),
        clear_count (0), polygon_count (0), last_cx1 (0), last_cy1 (0),
        last_cx2 (0), last_cy2 (0) {
    pixel= 256;
  }

  pencil get_pencil () { return last_pen; }
  brush  get_background () { return brush (last_bg); }
  void   set_pencil (pencil p) { last_pen= p; }
  void   set_background (brush b) { last_bg= b->get_color (); }
  void   set_brush (brush b) { last_brush= b; }

  void draw (int glyph_index, font_glyphs fn, SI x, SI y, int codepoint= -1) {
    (void) glyph_index;
    (void) fn;
    (void) x;
    (void) y;
    (void) codepoint;
  }
  void line (SI x1, SI y1, SI x2, SI y2) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
  void lines (array<SI> x, array<SI> y) {
    (void) x;
    (void) y;
  }
  void clear (SI x1, SI y1, SI x2, SI y2) {
    last_cx1= x1;
    last_cy1= y1;
    last_cx2= x2;
    last_cy2= y2;
    clear_count++;
  }
  void fill (SI x1, SI y1, SI x2, SI y2) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
  void arc (SI x1, SI y1, SI x2, SI y2, int alpha, int delta) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
    (void) alpha;
    (void) delta;
  }
  void fill_arc (SI x1, SI y1, SI x2, SI y2, int alpha, int delta) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
    (void) alpha;
    (void) delta;
  }
  void polygon (array<SI> x, array<SI> y, bool convex= true) {
    (void) convex;
    last_px= x;
    last_py= y;
    polygon_count++;
  }

  void fetch (SI x1, SI y1, SI x2, SI y2, renderer ren, SI x, SI y) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
    (void) ren;
    (void) x;
    (void) y;
  }
  void new_shadow (renderer& ren) { (void) ren; }
  void delete_shadow (renderer& ren) { (void) ren; }
  void get_shadow (renderer ren, SI x1, SI y1, SI x2, SI y2) {
    (void) ren;
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
  void put_shadow (renderer ren, SI x1, SI y1, SI x2, SI y2) {
    (void) ren;
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
  void apply_shadow (SI x1, SI y1, SI x2, SI y2) {
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
};

static void
rounded_bg (array<SI>& xs, array<SI>& ys, SI x1, SI y1, SI x2, SI y2, SI r) {
  int n= 16;
  for (int i= 0; i <= n; i++) {
    double a= 3.14159265359 + (1.57079632679 * i) / n;
    xs << (SI) (x1 + r + r * cos (a));
    ys << (SI) (y1 + r + r * sin (a));
  }
  for (int i= 0; i <= n; i++) {
    double a= 4.71238898038 + (1.57079632679 * i) / n;
    xs << (SI) (x2 - r + r * cos (a));
    ys << (SI) (y1 + r + r * sin (a));
  }
  for (int i= 0; i <= n; i++) {
    double a= (1.57079632679 * i) / n;
    xs << (SI) (x2 - r + r * cos (a));
    ys << (SI) (y2 - r + r * sin (a));
  }
  for (int i= 0; i <= n; i++) {
    double a= 1.57079632679 + (1.57079632679 * i) / n;
    xs << (SI) (x1 + r + r * cos (a));
    ys << (SI) (y2 - r + r * sin (a));
  }
}

class TestRoundedTextBg : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  void test_text_box_bg_color_api ();
  void test_rounded_bg_polygon ();
  void test_concat_box_pre_display_groups ();
  void test_concat_box_post_display_restores ();
  void test_handle_matching_bracket_bg ();
  void test_matrix_with_brackets_bg ();
  void test_middle_bracket_tracks_tall_item_height ();
  void test_middle_bracket_with_rsub_tracks_tall_item_height ();
  void test_big_op_box_bg_bridge ();
};

void
TestRoundedTextBg::test_text_box_bg_color_api () {
  // Set up font rule
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));

  pencil pen (black);
  color  bg= rgb_color (255, 200, 100, 255);

  box tb= text_box_with_bg (path (), 0, "test", fn, pen, bg, xkerning ());
  QCOMPARE (tb->get_type (), TEXT_BOX);

  color c= tb->get_bg_color ();
  QCOMPARE (c, bg);

  color transparent= rgb_color (0, 0, 0, 0);
  tb->set_bg_color (transparent);
  QCOMPARE (tb->get_bg_color (), transparent);

  tb->set_bg_color (bg);
  QCOMPARE (tb->get_bg_color (), bg);

  // Test that plain text_box initializes bg to transparent
  // and that set_bg_color works on any text_box
  box tb2= text_box (path (), 0, "test", fn, pen);
  QCOMPARE (tb2->get_bg_color (), transparent);
  tb2->set_bg_color (bg);
  QCOMPARE (tb2->get_bg_color (), bg);
  tb2->set_bg_color (transparent);
  QCOMPARE (tb2->get_bg_color (), transparent);
}

void
TestRoundedTextBg::test_rounded_bg_polygon () {
  array<SI> xs, ys;
  rounded_bg (xs, ys, 0, 0, 100, 50, 10);

  // Should have 4 * 17 = 68 points (16 segments per corner + start point)
  QCOMPARE (N (xs), 68);
  QCOMPARE (N (ys), 68);

  // Verify corner arc endpoints with tolerance for floating point
  // Bottom-left arc: from (0,10) to (10,0)
  QVERIFY (xs[0] >= -1 && xs[0] <= 1);
  QVERIFY (ys[0] >= 9 && ys[0] <= 11);
  QVERIFY (xs[16] >= 9 && xs[16] <= 11);
  QVERIFY (ys[16] >= -1 && ys[16] <= 1);

  // Bottom-right arc: from (90,0) to (100,10)
  QVERIFY (xs[17] >= 89 && xs[17] <= 91);
  QVERIFY (ys[17] >= -1 && ys[17] <= 1);
  QVERIFY (xs[33] >= 99 && xs[33] <= 101);
  QVERIFY (ys[33] >= 9 && ys[33] <= 11);

  // Top-right arc: from (100,40) to (90,50)
  QVERIFY (xs[34] >= 99 && xs[34] <= 101);
  QVERIFY (ys[34] >= 39 && ys[34] <= 41);
  QVERIFY (xs[50] >= 89 && xs[50] <= 91);
  QVERIFY (ys[50] >= 49 && ys[50] <= 51);

  // Top-left arc: from (10,50) to (0,40)
  QVERIFY (xs[51] >= 9 && xs[51] <= 11);
  QVERIFY (ys[51] >= 49 && ys[51] <= 51);
  QVERIFY (xs[67] >= -1 && xs[67] <= 1);
  QVERIFY (ys[67] >= 39 && ys[67] <= 41);
}

void
TestRoundedTextBg::test_concat_box_pre_display_groups () {
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));

  pencil pen (black);
  color  bg1        = rgb_color (255, 200, 100, 255);
  color  bg2        = rgb_color (100, 200, 255, 255);
  color  transparent= rgb_color (0, 0, 0, 0);

  // Create text boxes: [bg1, bg1, no-bg, bg2, bg2]
  array<box> bs;
  bs << text_box_with_bg (path (), 0, "a", fn, pen, bg1, xkerning ());
  bs << text_box_with_bg (path (), 1, "b", fn, pen, bg1, xkerning ());
  bs << text_box (path (), 2, "c", fn, pen);
  bs << text_box_with_bg (path (), 3, "d", fn, pen, bg2, xkerning ());
  bs << text_box_with_bg (path (), 4, "e", fn, pen, bg2, xkerning ());

  box cb= concat_box (path (), bs);

  mock_renderer_rep mock;
  renderer          ren= &mock;

  // Before pre_display, all bg colors should be original
  QCOMPARE (bs[0]->get_bg_color (), bg1);
  QCOMPARE (bs[1]->get_bg_color (), bg1);
  QCOMPARE (bs[2]->get_bg_color (), transparent);
  QCOMPARE (bs[3]->get_bg_color (), bg2);
  QCOMPARE (bs[4]->get_bg_color (), bg2);

  cb->pre_display (ren);

  // After pre_display:
  // - bg1 group (indices 0,1) should be transparent
  // - no-bg box (index 2) should still be transparent
  // - bg2 group (indices 3,4) should be transparent
  QCOMPARE (bs[0]->get_bg_color (), transparent);
  QCOMPARE (bs[1]->get_bg_color (), transparent);
  QCOMPARE (bs[2]->get_bg_color (), transparent);
  QCOMPARE (bs[3]->get_bg_color (), transparent);
  QCOMPARE (bs[4]->get_bg_color (), transparent);

  // Should have drawn 2 polygons (one per group)
  QCOMPARE (mock.polygon_count, 2);

  cout << "pre_display drew " << mock.polygon_count << " polygons\n";
  cout << "group 1 bounds: x=" << mock.last_cx1 << ".." << mock.last_cx2
       << ", y=" << mock.last_cy1 << ".." << mock.last_cy2 << "\n";

  cb->post_display (ren);

  // After post_display, all should be restored
  QCOMPARE (bs[0]->get_bg_color (), bg1);
  QCOMPARE (bs[1]->get_bg_color (), bg1);
  QCOMPARE (bs[2]->get_bg_color (), transparent);
  QCOMPARE (bs[3]->get_bg_color (), bg2);
  QCOMPARE (bs[4]->get_bg_color (), bg2);
}

void
TestRoundedTextBg::test_concat_box_post_display_restores () {
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));

  pencil pen (black);
  color  bg         = rgb_color (255, 0, 0, 255);
  color  transparent= rgb_color (0, 0, 0, 0);

  // Single text box with bg
  array<box> bs;
  bs << text_box_with_bg (path (), 0, "x", fn, pen, bg, xkerning ());

  box cb= concat_box (path (), bs);

  mock_renderer_rep mock;
  renderer          ren= &mock;

  cb->pre_display (ren);
  QCOMPARE (bs[0]->get_bg_color (), transparent);

  cb->post_display (ren);
  QCOMPARE (bs[0]->get_bg_color (), bg);

  // Test multiple pre/post cycles
  for (int i= 0; i < 3; i++) {
    cb->pre_display (ren);
    QCOMPARE (bs[0]->get_bg_color (), transparent);
    cb->post_display (ren);
    QCOMPARE (bs[0]->get_bg_color (), bg);
  }
}

void
TestRoundedTextBg::test_handle_matching_bracket_bg () {
  // Create environment with text-bg-color set
  drd_info              drd ("none", moebius::drd::std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  edit_env              env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->write ("text-bg-color", "#ffe47f");

  // Typeset a simple expression with brackets
  tree t (moebius::CONCAT);
  t << tree (moebius::LEFT, "(");
  t << "a";
  t << tree (moebius::RIGHT, ")");

  box b= typeset_as_concat (env, t, path ());

  // Actually typeset_as_concat returns a concat_box
  int n= b->subnr ();
  cout << "concat_box subnr=" << n << " type=" << b->get_type () << "\n";

  bool found_bracket_bg= false;
  for (int i= 0; i < n; i++) {
    box   sb= b->subbox (i);
    color c = sb->get_bg_color ();
    int   r, g, bl, a;
    get_rgb_color (c, r, g, bl, a);
    cout << "subbox i=" << i << " type=" << sb->get_type () << " a=" << a
         << "\n";
    if (a > 0) found_bracket_bg= true;
  }

  mock_renderer_rep mock;
  renderer          ren= &mock;
  b->pre_display (ren);
  cout << "pre_display polygon_count=" << mock.polygon_count << "\n";
  QVERIFY (found_bracket_bg);
  QVERIFY (mock.polygon_count > 0);
}

void
TestRoundedTextBg::test_matrix_with_brackets_bg () {
  // Simulate <marked|<matrix|...>>+1 where only the matrix has bg
  drd_info              drd ("none", moebius::drd::std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);

  edit_env env_bg (drd, "none", h1, h2, h3, h4, h5, h6);
  env_bg->write ("text-bg-color", "#ffe47f");
  env_bg->table_max= MAX_SI;

  edit_env env_plain (drd, "none", h1, h2, h3, h4, h5, h6);
  env_plain->table_max= MAX_SI;

  // Typeset matrix with brackets (marked part)
  tree t_matrix (moebius::CONCAT);
  t_matrix << tree (moebius::LEFT, "(");

  tree tab (moebius::TABLE);
  tree row1 (moebius::ROW);
  row1 << tree (moebius::CELL, "1");
  row1 << tree (moebius::CELL, "2");
  tab << row1;
  tree row2 (moebius::ROW);
  row2 << tree (moebius::CELL, "3");
  row2 << tree (moebius::CELL, "4");
  tab << row2;
  t_matrix << tab;

  t_matrix << tree (moebius::RIGHT, ")");

  box matrix_box= typeset_as_concat (env_bg, t_matrix, path ());

  // Typeset trailing +1 (non-marked part)
  tree t_tail (moebius::CONCAT);
  t_tail << "+";
  t_tail << "1";
  box tail_box= typeset_as_concat (env_plain, t_tail, path ());

  // Combine into outer concat
  array<box> bs;
  bs << matrix_box;
  bs << tail_box;
  box outer= concat_box (path (), bs);

  mock_renderer_rep mock;
  renderer          ren= &mock;
  outer->pre_display (ren);

  // Should draw exactly 1 polygon for the matrix+brackets group
  QCOMPARE (mock.polygon_count, 1);

  // Post-display should restore bg colors
  outer->post_display (ren);
}

void
TestRoundedTextBg::test_middle_bracket_tracks_tall_item_height () {
  drd_info              drd ("none", moebius::drd::std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);

  edit_env env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->table_max= MAX_SI;

  tree tab (moebius::TABLE);
  tree row1 (moebius::ROW);
  row1 << tree (moebius::CELL, "1");
  row1 << tree (moebius::CELL, "2");
  tab << row1;
  tree row2 (moebius::ROW);
  row2 << tree (moebius::CELL, "3");
  row2 << tree (moebius::CELL, "4");
  tab << row2;

  tree t_mid (moebius::CONCAT);
  t_mid << tab;
  t_mid << tree (moebius::MID, "|");

  array<line_item> items_mid= typeset_concat (env, t_mid, path ());

  tree t_tail (moebius::CONCAT);
  t_tail << copy (tab);
  t_tail << tree (moebius::MID, "|");
  t_tail << "p";

  array<line_item> items_tail= typeset_concat (env, t_tail, path ());

  QCOMPARE (N (items_mid), 2);
  QCOMPARE (N (items_tail), 3);

  box tall_ref= items_tail[0]->b;
  box mid_ref = items_mid[1]->b;
  box mid_tail= items_tail[1]->b;

  QCOMPARE (mid_tail->y1, mid_ref->y1);
  QCOMPARE (mid_tail->y2, mid_ref->y2);
  QVERIFY (mid_tail->y1 <= tall_ref->y1);
  QVERIFY (mid_tail->y2 >= tall_ref->y2);
}

void
TestRoundedTextBg::test_middle_bracket_with_rsub_tracks_tall_item_height () {
  drd_info              drd ("none", moebius::drd::std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);

  edit_env env (drd, "none", h1, h2, h3, h4, h5, h6);
  env->table_max= MAX_SI;

  tree tab (moebius::TABLE);
  tree row1 (moebius::ROW);
  row1 << tree (moebius::CELL, "1");
  row1 << tree (moebius::CELL, "2");
  tab << row1;
  tree row2 (moebius::ROW);
  row2 << tree (moebius::CELL, "3");
  row2 << tree (moebius::CELL, "4");
  tab << row2;

  tree t_mid (moebius::CONCAT);
  t_mid << tab;
  t_mid << tree (moebius::MID, "|");

  array<line_item> items_mid= typeset_concat (env, t_mid, path ());

  tree t_rsub (moebius::CONCAT);
  t_rsub << copy (tab);
  t_rsub << tree (moebius::MID, "|");
  t_rsub << tree (moebius::RSUB, "p");

  array<line_item> items_rsub= typeset_concat (env, t_rsub, path ());

  QCOMPARE (N (items_mid), 2);
  QCOMPARE (N (items_rsub), 2);

  box mid_plain    = items_mid[1]->b;
  box mid_with_rsub= items_rsub[1]->b;
  QVERIFY (mid_with_rsub->subnr () >= 1);
  box mid_script_ref= mid_with_rsub[0];

  QCOMPARE (mid_script_ref->y1, mid_plain->y1);
  QCOMPARE (mid_script_ref->y2, mid_plain->y2);
}

void
TestRoundedTextBg::test_big_op_box_bg_bridge () {
  tree which= tree (TUPLE, "roman", "rm", "medium", "right", "$s", "$d");
  tree by   = tree (TUPLE, "ec", "ecrm", "$s", "$d");
  font_rule (which, by);

  font fn= smart_font ("sys-chinese", "rm", "medium", "right", 10, 600);
  QVERIFY (!is_nil (fn));

  pencil pen (black);
  color  bg         = rgb_color (255, 200, 100, 255);
  color  transparent= rgb_color (0, 0, 0, 0);

  // Simulate <marked|<big|int>x> where the BIG_OP_BOX has no bg_color
  box inner = text_box (path (), 0, "I", fn, pen);
  box big_op= macro_box (path (), inner, font (), BIG_OP_BOX);
  box txt   = text_box_with_bg (path (), 1, "x", fn, pen, bg, xkerning ());

  array<box> bs;
  bs << big_op;
  bs << txt;

  box cb= concat_box (path (), bs);

  mock_renderer_rep mock;
  renderer          ren= &mock;

  cb->pre_display (ren);

  // Should draw exactly 1 polygon covering both boxes
  QCOMPARE (mock.polygon_count, 1);

  // Verify the polygon starts left of txt, i.e. it covers big_op too
  SI min_x= MAX_SI;
  for (int i= 0; i < N (mock.last_px); i++) {
    min_x= min (min_x, mock.last_px[i]);
  }
  SI txt_x1_in_cb= cb->sx1 (1);
  cout << "big_op x1=" << big_op->x1 << " txt_x1=" << txt_x1_in_cb
       << " polygon min_x=" << min_x << "\n";
  QVERIFY (min_x < txt_x1_in_cb);

  cb->post_display (ren);
}

QTEST_MAIN (TestRoundedTextBg)
#include "rounded_text_bg_test.moc"
