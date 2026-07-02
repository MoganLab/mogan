
/******************************************************************************
 * MODULE     : wasm_tmu_render.cpp
 * DESCRIPTION: Headless .tmu load / typeset / render API for the WASM ImGui viewer
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "wasm_tmu_render.hpp"

#include "convert.hpp"
#include "env.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "mupdf_picture.hpp"
#include "mupdf_renderer.hpp"
#include "new_style.hpp"
#include "preferences.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tree_helper.hpp"
#include "typesetter.hpp"

#include <moebius/drd/drd_info.hpp>
#include <moebius/drd/drd_std.hpp>
#include "tmu.hpp"

using moebius::drd::init_std_drd;
using moebius::drd::std_drd;
using namespace moebius;

static void
wasm_init_env_paths () {
  url style_root= url_unix ("$TEXMACS_HOME_PATH/styles:$TEXMACS_PATH/styles");
  url package_root=
      url_unix ("$TEXMACS_HOME_PATH/packages:$TEXMACS_PATH/packages");
  url all_root= style_root | package_root;
  url style_path= search_sub_dirs (all_root);
  url text_root=
      url_unix ("$TEXMACS_HOME_PATH/texts:$TEXMACS_PATH/texts");
  url text_path= search_sub_dirs (text_root);

  set_env ("TEXMACS_STYLE_PATH", as_string (style_path));
  set_env ("TEXMACS_TEXT_PATH", as_string (text_path));
  set_env ("TEXMACS_FILE_PATH", as_string (text_path | style_path));
  set_env ("TEXMACS_DOC_PATH",
           as_string (url_unix ("$TEXMACS_HOME_PATH/doc:$TEXMACS_PATH/doc")));
  set_env ("TEXMACS_PATTERN_PATH",
           "$TEXMACS_HOME_PATH/misc/patterns:"
           "$TEXMACS_PATH/misc/patterns:"
           "$TEXMACS_PATH/misc/pictures");
  set_env ("TEXMACS_PIXMAP_PATH",
           "$TEXMACS_PATH/misc/pixmaps/modern/32x32/settings:"
           "$TEXMACS_PATH/misc/pixmaps/modern/24x24/main:"
           "$TEXMACS_PATH/misc/pixmaps/modern/20x20/mode:"
           "$TEXMACS_PATH/misc/pixmaps/modern/16x16/focus");
  set_env ("TEXMACS_DIC_PATH",
           "$TEXMACS_HOME_PATH/langs/natural/dic:"
           "$TEXMACS_PATH/langs/natural/dic");
}

static void
wasm_init_home_dirs () {
  make_dir ("$TEXMACS_HOME_PATH");
  make_dir ("$TEXMACS_HOME_PATH/fonts");
  make_dir ("$TEXMACS_HOME_PATH/fonts/error");
  make_dir ("$TEXMACS_HOME_PATH/system");
  make_dir ("$TEXMACS_HOME_PATH/system/tmp");
  make_dir ("$TEXMACS_HOME_PATH/styles");
  make_dir ("$TEXMACS_HOME_PATH/packages");
  make_dir ("$TEXMACS_HOME_PATH/texts");
  make_dir ("$TEXMACS_HOME_PATH/doc");
}

void
wasm_tmu_render_init () {
  cout << "wasm_tmu_render_init" << LF;
  set_env ("PWD", "/");
  set_env ("HOME", "/");
  set_env ("TEXMACS_PATH", "/TeXmacs");
  set_env ("TEXMACS_HOME_PATH", string ("/.") * PREFIX_DIR);
  cout << "after 4 set_env" << LF;

  wasm_init_home_dirs ();
    cout << "wasm_init_home_dirs" << LF;
  wasm_init_env_paths ();
    cout << "wasm_init_env_paths" << LF;
  

  init_std_drd ();
    cout << "init_std_drd" << LF;
  load_user_preferences ();
    cout << "load_user_preferences" << LF;
  font_database_load ();
  //  cout << "font_database_load" << LF;
}

static void
use_modules (tree t) {
  (void) t;
  // No-op for the Qt-free ImGui viewer.  Documents whose styles rely on
  // Scheme module side effects may not render fully.
}

static void
initialize_environment (edit_env& env, tree doc, drd_info& drd) {
  env->write_default_env ();
  bool                  ok;
  tree                  t, style= extract (doc, "style");
  hashmap<string, tree> H;
  style_get_cache (style, H, t, ok);
  if (ok) {
    env->patch_env (H);
    ok= drd->set_locals (t);
    drd->set_environment (H);
  }
  if (!ok) {
    if (!is_tuple (style)) TM_FAILED ("tuple expected as style");
    H  = get_style_env (style);
    drd= get_style_drd (style);
    style_set_cache (style, H, drd->get_locals ());
    env->patch_env (H);
    drd->set_environment (H);
  }
  use_modules (env->read (THE_MODULES));
  tree init= extract (doc, "initial");
  for (int i= 0; i < N (init); i++)
    if (is_func (init[i], ASSOCIATE, 2) && is_atomic (init[i][0]))
      env->write (init[i][0]->label, init[i][1]);
  if (retina_zoom == 2) {
    double mag= 2.0 * env->get_double (MAGNIFICATION);
    env->write (MAGNIFICATION, as_string (mag));
  }
  env->update ();
}

tree
wasm_load_tmu (url u) {
  string s= tm_string_load (u);
  //cout << s << LF;
  //cout << "tm_string_load" << LF;
  tree   doc= tmu_document_to_tree (s);
  cout << "tmu_document_to_tree" << LF;
  return extract_document (doc);
}

box
wasm_typeset_document (tree doc) {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT), h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  edit_env env (drd, "none", h1, h2, h3, h4, h5, h6);
  initialize_environment (env, doc, drd);
  tree body= extract (doc, "body");
  return typeset_as_document (env, body, path ());
}

void
wasm_render_page_to_buffer (box doc_box, double zoom,
                            mupdf_viewer_buffer_rep& buffer) {
  box page= doc_box;
  if (N (page) > 0) page= page[0];
  if (N (page) > 0) page= page[0];

  SI pixel= std_shrinkf * PIXEL;
  SI w    = page->x4 - page->x3;
  SI h    = page->y4 - page->y3;
  SI ww   = (SI) round (zoom * w);
  SI hh   = (SI) round (zoom * h);
  int pxw = (ww + pixel - 1) / pixel;
  int pxh = (hh + pixel - 1) / pixel;
  if (pxw < 1) pxw= 1;
  if (pxh < 1) pxh= 1;

  picture  pic= native_picture (pxw, pxh, 0, 0);
  renderer ren= picture_renderer (pic, zoom);

  rectangles rs;
  page->redraw (ren, path (0), rs, -page->x3, -page->y4);

  mupdf_picture_rep* rep= (mupdf_picture_rep*) pic->get_handle ();
  buffer.copy_from_pixmap (rep->pix);

  tm_delete (ren);
}
