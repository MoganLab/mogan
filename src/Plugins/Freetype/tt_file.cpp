
/******************************************************************************
 * MODULE     : tt_file.cpp
 * DESCRIPTION: Finding a True Type font
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tt_file.hpp"
#include "Metafont/tex_files.hpp"
#include "analyze.hpp"
#include "data_cache.hpp"
#include "file.hpp"
#include "font.hpp"
#include "hashmap.hpp"
#include "hashset.hpp"
#include "preferences.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tm_timer.hpp"
#include "tt_tools.hpp"

#ifdef USE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#include <lolly/hash/md5.hpp>

// Caching for Font in tuple (filename, path)
// eg. (basename.ttf, url(/path/to/basename.ttf))
static hashmap<string, url> tt_font_locations;
// A hashmap of all basename of the fonts with existence status
static hashmap<string, bool> tt_fonts;
static array<string> tt_allowed_suffixes= array<string> ("ttf", "ttc", "otf");

static void
tt_locate_update_cache (url font_u, bool must_in_db= true) {
  string name     = as_string (tail (font_u));
  string base_name= basename (font_u);

  if (must_in_db && !font_database_exists (base_name)) {
    // the font basename does not exist in font database
    return;
  }
  array<string> suffixes= font_database_suffixes (base_name);
  if (must_in_db && !contains (suffix (font_u), suffixes)) {
    // the font basename does exist in the font database
    // but the font suffix does not exist in the font database
    return;
  }

  tt_font_locations (name)= font_u;
  tt_fonts (base_name)    = true;
  cache_set ("font_basename.scm", name, as_string (font_u));
}

url
add_to_path (url u, url d) {
  if (is_or (d)) return add_to_path (add_to_path (u, d[1]), d[2]);
  if (is_none (u) || u == d) return d;
  if (is_or (u) && u[1] == d) return u;
  if (is_or (u)) return u[1] | add_to_path (u[2], d);
  return u | d;
}

url
tt_add_to_font_path (url u) {
  if (!is_directory (u)) {
    url font_path= get_texmacs_home_path () * url ("fonts") * url ("truetype") *
                   (lolly::hash::md5_hexdigest (u) * "." * suffix (u));
    if (u != font_path) {
      if (exists (font_path)) remove (font_path);
      copy (u, font_path);
    }
    // Sync font info to tt_fonts and tt_font_location
    tt_locate_update_cache (font_path, false);
    return font_path;
  }
  return url_none ();
}

url
tt_font_search_path () {
  // NOTICE: the order of the search path here is very important
  // Please use ret= ret | u to append the url, the former appended url will
  // precede the latter appended url
  url    ret= url_none ();
  string xtt= get_env ("TEXMACS_FONT_PATH");
  if (!is_empty (xtt)) {
    ret= ret | url (xtt);
  }
  if (os_win () || os_mingw ()) {
    ret= ret | url_system ("$windir/Fonts");
  }
  else if (os_macos ()) {
    ret= ret | url ("/Library/Fonts") |
         url ("/Library/Application Support/Apple/Fonts/iLife") |
         url ("/Library/Application Support/Apple/Fonts/iWork") |
         url ("/System/Library/Fonts") |
         url ("/System/Library/PrivateFrameworks/FontServices.framework/"
              "Versions/A/Resources/Fonts") |
         url ("/opt/local/share/texmf-texlive/fonts/opentype") |
         url ("/opt/local/share/texmf-texlive/fonts/truetype") |
         url ("/opt/local/share/texmf-texlive-dist/fonts/opentype") |
         url ("/opt/local/share/texmf-texlive-dist/fonts/truetype") |
         url ("/usr/local/texlive/2020/texmf-dist/fonts/opentype") |
         url ("/usr/local/texlive/2020/texmf-dist/fonts/truetype") |
         url ("/usr/local/texlive/2021/texmf-dist/fonts/opentype") |
         url ("/usr/local/texlive/2021/texmf-dist/fonts/truetype") |
         url ("/usr/local/texlive/2022/texmf-dist/fonts/opentype") |
         url ("/usr/local/texlive/2022/texmf-dist/fonts/truetype");
  }
  else {
#ifdef _FONTCONFIG_H_
    FcConfig*  config  = FcInitLoadConfig ();
    FcStrList* fontdirs= FcConfigGetFontDirs (config);
    FcChar8*   fontdir = NULL;

    FcStrListFirst (fontdirs);
    while (fontdir= FcStrListNext (fontdirs)) {
      ret= ret | url_system ((char*) fontdir);
    }
#else
    ret= ret | url ("/usr/share/fonts/truetype") |
         url ("/usr/share/fonts/opentype");
#endif
    ret= ret | url ("/usr/share/texlive/texmf-dist/fonts/opentype") |
         url ("/usr/share/texlive/texmf-dist/fonts/truetype");
    // special rules for archlinux
    if (exists (url_system ("/usr/share/fonts/noto-cjk"))) {
      ret= ret | url_system ("/usr/share/fonts/noto-cjk");
    }
    // special rules for uos
    if (exists (url_system ("/usr/share/fonts/fonts-cesi"))) {
      ret= ret | url_system ("/usr/share/fonts/fonts-cesi");
    }
  }
  ret= ret | url ("$TEXMACS_HOME_PATH/fonts/truetype") |
       url ("$TEXMACS_PATH/fonts/truetype") |
       url ("$TEXMACS_HOME_PATH/fonts/opentype") |
       url ("$TEXMACS_PATH/fonts/opentype");
  return ret;
}

url
tt_font_path () {
  url xu= search_sub_dirs (tt_font_search_path ());
  return xu;
}

static void
tt_locate_all () {
  if (N (tt_font_locations) > 0) return;

  bench_start ("tt_locate_all");
  url suffixes= url_none ();
  for (int i= 0; i < N (tt_allowed_suffixes); i++) {
    string suffix= tt_allowed_suffixes[i];
    string lo    = string ("*.") * suffix;
    string up    = string ("*.") * upcase_all (suffix);
    suffixes     = suffixes | url (lo) | url (up);
  }
  url all_fonts= expand (complete (tt_font_path () * suffixes));
  url iter     = all_fonts;
  while (is_or (iter)) {
    url font_u= iter[1];
    tt_locate_update_cache (font_u);
    iter= iter[2];
  }
  url font_last= iter;
  tt_locate_update_cache (font_last);
  cache_memorize ();
  bench_end ("tt_locate_all");
}

array<url>
tt_font_paths () {
  tt_locate_all ();

  array<url>       ret;
  iterator<string> it= iterate (tt_font_locations);
  while (it->busy ()) {
    string key= it->next ();
    ret << tt_font_locations[key];
  }
  return ret;
}

static url
tt_locate_pfb (string name) {
  url u= resolve_tex (name * ".pfb");
  return u;
}

static url
tt_fast_locate (string name) {
  array<string> suffixes= tt_allowed_suffixes;
  for (int i= 0; i < N (suffixes); i++) {
    string suffix       = suffixes[i];
    string font_filename= name * "." * suffix;
    if (tt_font_locations->contains (font_filename)) {
      return tt_font_locations[font_filename];
    }
    font_filename= name * "." * upcase_all (suffix);
    if (tt_font_locations->contains (font_filename)) {
      return tt_font_locations[font_filename];
    }
  }
  return url_none ();
}

url
tt_font_find (string font_basename) {
  if (font_database_exists (font_basename)) {
    bool          flag_truetype= true;
    array<string> suffixes     = font_database_suffixes (font_basename);
    for (int i= 0; i < N (suffixes); i++) {
      string suf= suffixes[i];
      if (!contains (locase_all (suf), tt_allowed_suffixes)) {
        flag_truetype= false;
      }
      if (flag_truetype) {
        string filename= font_basename * "." * suf;
        if (is_cached ("font_basename.scm", filename)) {
          url u= url_system (cache_get ("font_basename.scm", filename)->label);
          if (exists (u)) return u;
          cache_reset ("font_basename.scm", filename);
        }
      }
    }
    if (flag_truetype) {
      tt_locate_all ();
      url    u            = tt_fast_locate (font_basename);
      string font_filename= as_string (tail (u));
      string font_fullpath= as_string (u);
      if (exists (u)) {
        cache_set ("font_basename.scm", font_filename, font_fullpath);
        return u;
      }
      else {
        cache_reset ("font_basename.scm", font_filename);
        return url_none ();
      }
    }
  }
  // If a truetype font is not in the font database, it will not be found
  // To support a font, we must first add it to the font database
  // Import font is the way for user to add the font to the local font database
  // For developer, we can add it to the global font database

  return tt_locate_pfb (font_basename);
}

bool
tt_font_exists (string name) {
  if (tt_fonts->contains (name)) return tt_fonts (name);
  bench_start ("tt_font_exists " * name);
  bool yes= !is_none (tt_font_find (name));
  // Cache the result (true if exists, false if not)
  tt_fonts (name)= yes;
  bench_end ("tt_font_exists " * name, 10);
  return yes;
}

string
tt_find_name_sub (string name, int size) {
  if (size == 0) {
    if (tt_font_exists (name)) return name;
    else return "";
  }
  if (tt_font_exists (name * as_string (size))) return name * as_string (size);
  if (size > 333) size= (size + 50) / 100;
  if (tt_font_exists (name * as_string (size))) return name * as_string (size);

  if ((size >= 15) && tt_font_exists (name * "17")) return name * "17";
  if ((size > 12) && tt_font_exists (name * "12")) return name * "12";
  if ((size < 5) && tt_font_exists (name * "5")) return name * "5";
  if ((size < 6) && tt_font_exists (name * "6")) return name * "6";
  if ((size < 7) && tt_font_exists (name * "7")) return name * "7";
  if ((size < 8) && tt_font_exists (name * "8")) return name * "8";
  if ((size < 9) && tt_font_exists (name * "9")) return name * "9";
  if ((size < 9) && tt_font_exists (name * "7")) return name * "7";
  if (tt_font_exists (name * "10")) return name * "10";
  if ((size < 9) && tt_font_exists (name * "700")) return name * "700";
  if ((size >= 15) && tt_font_exists (name * "1700")) return name * "1700";
  if (tt_font_exists (name * "1000")) return name * "1000";
  if (tt_font_exists (name)) return name;
  return "";
}

string
tt_find_name (string name, int size) {
  string s= "tt:" * name * as_string (size);
  if (is_cached ("font_cache.scm", s)) {
    string r= cache_get ("font_cache.scm", s)->label;
    if (tt_font_exists (r)) return r;
    // if the font does not exist, clear the cache in `font_cache.scm`
    cache_reset ("font_cache.scm", s);
  }

  bench_start ("tt_find_name " * name * " " * as_string (size));
  string r= tt_find_name_sub (name, size);
  bench_end ("tt_find_name " * name * " " * as_string (size), 10);

  if (r != "") cache_set ("font_cache.scm", s, r);
  return r;
}
