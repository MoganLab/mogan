
/******************************************************************************
 * MODULE     : load_tex.cpp
 * DESCRIPTION: simultaneously load pk and tfm file and
 *              generate them if they can't be found.
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "load_tex.hpp"
#include "Freetype/tt_file.hpp"
#include "config.h"
#include "data_cache.hpp"
#include "file.hpp"
#include "hashmap.hpp"
#include "path.hpp"
#include "preferences.hpp"
#include "tex_files.hpp"
#include "tm_debug.hpp"
#include "tm_file.hpp"
#include "tm_timer.hpp"
#include <cmath>

// Memory caches for TFM and PK files to avoid repeated disk loading
static hashmap<string, tex_font_metric> tfm_cache;
static hashmap<string, font_glyphs>     pk_cache;

/*
 For certain LaTeX fonts, it is possible to have non integer font sizes,
 by multiplying the font size by 100. For instance, larm10 would be equivalent
 to larm1000 and larm1050 would stand for a 10.5 point larm font.

 See bug https://savannah.gnu.org/bugs/?37472
 */
static int
mag (double dpi, double size, double dsize) {
  if ((size >= 316) && (dsize < 100)) dsize*= 100;
  if ((dsize >= 316) && (size < 100)) size*= 100;
  return (int) (((size * dpi) / dsize) + 0.5);
}

// 将点值尺寸转换为 TeX 字体尺寸表示
// 规则：整数尺寸（如10）保持不变，0.5倍数尺寸（如10.5）转换为乘以100（1050）
double
to_tex_font_size (double sz) {
  bench_start ("to_tex_font_size_conversion");

  if (sz == 0.0) {
    bench_cumul ("to_tex_font_size_conversion");
    return 0.0;
  }
  if (sz >= 316.0) {
    // 已经可能是乘以100的值，检查是否为错误转换的结果
    double original= sz / 100.0;
    if (original < 316.0) {
      double doubled= original * 2.0;
      if (fabs (original - round (original)) < 0.1) {
        // original是整数，sz可能是错误转换的结果（如1000表示10），返回original
        bench_cumul ("to_tex_font_size_conversion");
        return original;
      }
      if (fabs (doubled - round (doubled)) < 0.1) {
        // original是0.5倍数，sz是正确的乘以100的值
        bench_cumul ("to_tex_font_size_conversion");
        return sz;
      }
    }
    // 其他情况，保持原样
    bench_cumul ("to_tex_font_size_conversion");
    return sz;
  }
  // 检查是否为0.5倍数
  double doubled= sz * 2.0;
  if (fabs (doubled - round (doubled)) < 0.1) {
    // 是0.5倍数，检查是否为整数
    if (fabs (sz - round (sz)) < 0.1) {
      // 是整数（如10.0），直接返回
      bench_cumul ("to_tex_font_size_conversion");
      return sz;
    }
    // 是0.5倍数但不是整数（如10.5），返回乘以100的值
    bench_cumul ("to_tex_font_size_conversion");
    return sz * 100.0;
  }
  // 不是0.5倍数，四舍五入到最近的整数
  bench_cumul ("to_tex_font_size_conversion");
  return round (sz);
}

/******************************************************************************
 * Loading tfm files
 ******************************************************************************/

bool
try_tfm (string family, double size, double osize, tex_font_metric& tfm,
         bool make) {
  // 转换为 TeX 字体尺寸表示
  double tex_size = to_tex_font_size (size);
  double tex_osize= to_tex_font_size (osize);

  // cout << "Try tfm " << family << tex_size << " (" << tex_osize << ")\n";
  make= make && use_texlive_fonts () &&
        get_user_preference ("texlive.maketfm") != "false";
  string name_tfm= family * as_string ((int) tex_osize) * ".tfm";
  if (tex_font_metric::instances->contains (name_tfm)) {
    tfm= tex_font_metric (name_tfm);
    return true;
  }
  string name= family *
               (tex_size == 0 ? string ("") : as_string ((int) tex_size)) *
               ".tfm";
  if (DEBUG_STD) debug_fonts << "Try tfm " << name << "\n";
  url u= resolve_tex (name);
  if (is_none (u)) {
    if (exists (url ("$TEXMACS_HOME_PATH/fonts/error", name))) {
      if (DEBUG_STD) debug_fonts << "Error during " << name << " loading\n";
      return false;
    }
    if (make) {
      system_wait ("Generating font file", name);
      make_tex_tfm (name);
      system_wait ("");
      u= resolve_tex (name);
      if (is_none (u)) {
        reset_tfm_path ();
        u= resolve_tex (name);
        save_string (url ("$TEXMACS_HOME_PATH/fonts/error", name), "");
      }
    }
    if (is_none (u)) return false;
  }
  // cout << "Tfm " << family << tex_osize << " -> " << family << tex_size <<
  // "\n";
  tfm= load_tfm (u, family, (int) tex_osize);
  if (tex_size != tex_osize)
    cache_set ("font_cache.scm", "tfm:" * family * as_string ((int) tex_osize),
               as_string ((int) tex_size));
  if (tex_size == 0) {
    tex_size= (double) tfm->size;
    if (DEBUG_STD) debug_fonts << "Design size = " << tex_size << "\n";
  }
  if (tex_size != tex_osize)
    tfm->header[1]= mag (tfm->header[1], tex_osize, tex_size);
  return true;
}

bool
load_tex_tfm (string family, double size, int dsize, tex_font_metric& tfm,
              bool make) {
  // Check memory cache first
  string cache_key= family * ":" * as_string (size) * ":" * as_string (dsize);
  if (tfm_cache->contains (cache_key)) {
    tfm= tfm_cache[cache_key];
    return true;
  }

  // cout << "Load TeX tfm " << family << size << " (dsize= " << dsize << ")\n";
  if (try_tfm (family, size, size, tfm, make)) {
    // Store in cache before returning
    tfm_cache (cache_key)= tfm;
    return true;
  }
  if (size > 333.0)
    return load_tex_tfm (family, (size + 50.0) / 100.0, dsize, tfm, make);
  if (false) { // NOTE: only use Type 1 fonts
    if ((size > 14.0) && try_tfm (family, 17.0, size, tfm, make)) return true;
    if ((size > 12.0) && try_tfm (family, 12.0, size, tfm, make)) return true;
    if ((size > 10.0) && try_tfm (family, 10.0, size, tfm, make)) return true;
    if ((size < 5.0) && try_tfm (family, 5.0, size, tfm, make)) return true;
    if ((size < 6.0) && try_tfm (family, 6.0, size, tfm, make)) return true;
    if ((size < 7.0) && try_tfm (family, 7.0, size, tfm, make)) return true;
    if ((size < 8.0) && try_tfm (family, 8.0, size, tfm, make)) return true;
    if ((size < 9.0) && try_tfm (family, 9.0, size, tfm, make)) return true;
    if ((size < 9.0) && try_tfm (family, 7.0, size, tfm, make)) return true;
    if (try_tfm (family, 10.0, size, tfm, make)) return true;
    if ((size > 14.0) && try_tfm (family, 1700.0, size, tfm, make)) return true;
    if ((size > 12.0) && try_tfm (family, 1200.0, size, tfm, make)) return true;
    if ((size < 5.0) && try_tfm (family, 500.0, size, tfm, make)) return true;
    if ((size < 9.0) && try_tfm (family, 700.0, size, tfm, make)) return true;
    if (try_tfm (family, 1000.0, size, tfm, make)) return true;
  }
  if (true) { // NOTE: first search for type 1, then use Metafont
    SI delta= (size < 10.0 ? 1 : -1);
    if (try_tfm (family, size + delta, size, tfm, make)) return true;
    if (try_tfm (family, size - delta, size, tfm, make)) return true;
    if (try_tfm (family, size + 2 * delta, size, tfm, make)) return true;
    if (try_tfm (family, size - 2 * delta, size, tfm, make)) return true;
    if (try_tfm (family, 100 * size, size, tfm, make)) return true;
    if (try_tfm (family, 100 * (size + delta), size, tfm, make)) return true;
    if (try_tfm (family, 100 * (size - delta), size, tfm, make)) return true;
    if (try_tfm (family, 100 * (size + 2 * delta), size, tfm, make))
      return true;
    if (try_tfm (family, 100 * (size - 2 * delta), size, tfm, make))
      return true;
  }
  if (((double) dsize != size) && (dsize != 0))
    if (try_tfm (family, (double) dsize, size, tfm, make)) return true;
  if ((dsize != 10) && (fabs (size - 10.0) > 0.1))
    if (try_tfm (family, 10.0, size, tfm, make)) return true;
  return false;
}

bool
load_tex_tfm (string family, double size, int dsize, tex_font_metric& tfm) {
  string var= "tfm:" * family * as_string (size);
  if (is_cached ("font_cache.scm", var))
    if (try_tfm (family, as_double (cache_get ("font_cache.scm", var)->label),
                 size, tfm, false))
      return true;
  if (get_user_preference ("texlive.maketfm") != "false")
    if (load_tex_tfm (family, size, dsize, tfm, false)) return true;
  return load_tex_tfm (family, size, dsize, tfm, true);
}

/******************************************************************************
 * PK font glyphs with lazy parsing
 ******************************************************************************/

static glyph error_glyph;

struct pk_font_glyphs_rep : public font_glyphs_rep {
  pk_loader* pkl;
  int        bc, ec;
  glyph*     fng; // definitions of the characters

  pk_font_glyphs_rep (string name, pk_loader*);
  glyph& get (int char_code);
};

pk_font_glyphs_rep::pk_font_glyphs_rep (string name, pk_loader* pkl2)
    : font_glyphs_rep (name), pkl (pkl2) {
  if (pkl) {
    fng= pkl->load_pk ();
    bc = pkl->tfm->bc;
    ec = pkl->tfm->ec;
  }
  else {
    fng= 0;
    bc = 0;
    ec = -1;
  }
}

glyph&
pk_font_glyphs_rep::get (int c) {
  if ((c < bc) || (c > ec)) return error_glyph;
  if (pkl && !pkl->unpacked[c - bc]) {
    pkl->input_pos= pkl->char_pos[c - bc];
    pkl->flagbyte = pkl->char_flag[c - bc];
    pkl->unpack (fng[c - bc]);

    pkl->unpacked[c - bc]= true;
  }
  return fng[c - bc];
}

/******************************************************************************
 * Loading pk files
 ******************************************************************************/

bool
try_pk (string family, double size, int dpi, int dsize, tex_font_metric& tfm,
        font_glyphs& pk) {
  // 转换为 TeX 字体尺寸表示
  double tex_size = to_tex_font_size (size);
  double tex_dsize= (double) dsize;
  // cout << "Try pk " << family << tex_size << " at " << dpi << " dpi\n";
#ifdef USE_FREETYPE
  // Substitute by True Type font ?
  int    tt_size= tex_size < 333.0 ? (int) (tex_size + 0.5)
                                   : (int) ((tex_size + 50.0) / 100.0 + 0.5);
  int    tt_dpi = tex_size < 333.0
                      ? dpi
                      : (int) ((tex_size * dpi) / (100.0 * tt_size) + 0.5);
  string tt_name= tt_find_name (family, tt_size);
  if (tt_name != "") {
    if (font_glyphs::instances->contains (tt_name)) pk= font_glyphs (tt_name);
    else pk= tt_font_glyphs (tt_name, tt_size, tt_dpi, tt_dpi);
    return true;
  }
#endif // USE_FREETYPE

  // Open regular pk font
  string name_pk=
      family * as_string ((int) tex_size) * "." * as_string (dpi) * "pk";
  if (font_glyphs::instances->contains (name_pk)) {
    pk= font_glyphs (name_pk);
    return true;
  }
  if (tex_dsize == 0.0) {
    double old_size= tex_size;
    tex_size       = (double) tfm->size;
    dpi            = mag (dpi, old_size, tex_size);
  }
  string size_name (tex_dsize == 0.0 ? string ("")
                                     : as_string ((int) tex_size));
  string name (family * size_name * "." * as_string (dpi) * "pk");
  if (DEBUG_STD) debug_fonts << "Open pk " << name << "\n";
  url u= resolve_tex (name);
  if (is_none (u)) {
    if (exists (url ("$TEXMACS_HOME_PATH/fonts/error", name))) {
      if (DEBUG_STD) debug_fonts << "Error during " << name << " loading\n";
      return false;
    }
    if (use_texlive_fonts () &&
        get_user_preference ("texlive.makepk") != "false") {
      system_wait ("Generating font file", name);
      make_tex_pk (family * size_name, dpi, as_int (get_setting ("DPI")));
      system_wait ("");
      u= resolve_tex (name);
      if (is_none (u)) {
        reset_pk_path ();
        u= resolve_tex (name);
      }
    }
    if (is_none (u)) {
      save_string (url ("$TEXMACS_HOME_PATH/fonts/error", name), "");
      if (DEBUG_STD) debug_fonts << "Error during " << name << " loading\n";
      return false;
    }
  }
  pk= font_glyphs (
      tm_new<pk_font_glyphs_rep> (name_pk, tm_new<pk_loader> (u, tfm, dpi)));
  return true;
}

bool
load_tex_pk (string family, double size, int dpi, int dsize,
             tex_font_metric& tfm, font_glyphs& pk) {
  // Check memory cache first
  string cache_key= family * ":" * as_string (size) * ":" * as_string (dpi) *
                    ":" * as_string (dsize);
  if (pk_cache->contains (cache_key)) {
    pk= pk_cache[cache_key];
    return true;
  }

  if (try_pk (family, size, dpi, dsize, tfm, pk)) {
    // Store in cache before returning
    pk_cache (cache_key)= pk;
    return true;
  }
  if (((double) dsize != size) && (dsize != 0))
    if (try_pk (family, (double) dsize, mag (dpi, size, (double) dsize), dsize,
                tfm, pk))
      return true;
  if ((dsize != 10) && (fabs (size - 10.0) > 0.1))
    if (try_pk (family, 10.0, mag (dpi, size, 10.0), dsize, tfm, pk))
      return true;
  if (size > 333.0) {
    double sz= (size + 50.0) / 100.0;
    return load_tex_pk (family, sz, mag (dpi, size, sz), dsize, tfm, pk);
  }
  return false;
}

/******************************************************************************
 * Loading tfm and pk files
 ******************************************************************************/

static void
rubber_status (glyph& gl, int st) {
  if (is_nil (gl)) return;
  gl->status|= st;
  gl->yoff= 0;
}

static void
rubber_fix (tex_font_metric tfm, font_glyphs& pk) {
  // This routine is used so as to correct the anti-aliasing of
  // rubber TeX characters (in the vertical direction).
  int c;
  for (c= tfm->bc; c <= tfm->ec; c++) {
    if (tfm->tag (c) == 3) {
      if (tfm->bot (c) != 0) rubber_status (pk->get (tfm->bot (c)), 1);
      if (tfm->top (c) != 0) rubber_status (pk->get (tfm->top (c)), 2);
      if (tfm->mid (c) != 0) rubber_status (pk->get (tfm->mid (c)), 3);
      if (tfm->rep (c) != 0) rubber_status (pk->get (tfm->rep (c)), 3);
    }
  }
}

void
load_tex (string family, double size, int dpi, int dsize, tex_font_metric& tfm,
          font_glyphs& pk) {
  bench_start ("load tex font");
  if (DEBUG_VERBOSE)
    debug_fonts << "Loading " << family << size << " at " << dpi << " dpi\n";
  if (load_tex_tfm (family, size, dsize, tfm) &&
      load_tex_pk (family, size, dpi, dsize, tfm, pk)) {
    bench_cumul ("load tex font");
    rubber_fix (tfm, pk);
    return;
  }
  if (DEBUG_VERBOSE) {
    debug_fonts << "Font " << family << size << " at " << dpi
                << " dpi not found\n";
    debug_fonts << "Loading ecrm" << size << " at " << dpi << " dpi instead\n";
  }
  if (load_tex_tfm ("ecrm", size, 10, tfm) &&
      load_tex_pk ("ecrm", size, dpi, 10, tfm, pk)) {
    bench_cumul ("load tex font");
    return;
  }
#ifdef OS_WIN
  else {
    string name= family * as_string (size) * "@" * as_string (dpi);
    failed_error << "Could not open font " << name << "\nLoading default" << LF;
    cout << "Could not load font...\nLoading default" << LF;
    if (load_tex_tfm ("ecrm", 10.0, 10, tfm) &&
        load_tex_pk ("ecrm", 10.0, 600, 10, tfm, pk)) {
      bench_cumul ("load tex font");
      return;
    }
  }
#endif
  string name= family * as_string (size) * "@" * as_string (dpi);
  failed_error << "Could not open " << name << "\n";
  TM_FAILED ("Tex seems not to be installed properly");
  bench_cumul ("load tex font");
}
