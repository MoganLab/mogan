
/******************************************************************************
 * MODULE     : wasm_gui_stubs.cpp
 * DESCRIPTION: No-op GUI stubs for the Qt-free WASM ImGui viewer
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "gui.hpp"
#include "renderer.hpp"
#include "font.hpp"
#include "widget.hpp"
#include "url.hpp"

#include "path.hpp"
#include "object.hpp"

void
gui_open (int& argc, char** argv) {
  (void) argc;
  (void) argv;
}

void
gui_start_loop () {}

void
gui_close () {}

void
gui_root_extents (SI& width, SI& height) {
  width = 1280 * PIXEL;
  height= 800 * PIXEL;
}

void
gui_maximal_extents (SI& width, SI& height) {
  width = 2560 * PIXEL;
  height= 1600 * PIXEL;
}

void
set_default_font (string name) {
  (void) name;
}

font
get_default_font (bool tt, bool mini, bool bold) {
  (void) tt;
  (void) mini;
  (void) bold;
  return font ();
}

void
load_system_font (string family, int size, int dpi, font_metric& fnm,
                  font_glyphs& fng) {
  (void) family;
  (void) size;
  (void) dpi;
  (void) fnm;
  (void) fng;
}

bool
set_selection (string cb, tree t, string s, string sv, string sh,
               string format) {
  (void) cb;
  (void) t;
  (void) s;
  (void) sv;
  (void) sh;
  (void) format;
  return false;
}

bool
get_selection (string cb, tree& t, string& s, string format) {
  (void) cb;
  (void) t;
  (void) s;
  (void) format;
  return false;
}

void
clear_selection (string cb) {
  (void) cb;
}

void
beep () {}

void
needs_update () {}

bool
check_event (int type) {
  (void) type;
  return false;
}

void
image_gc (string name) {
  (void) name;
}

void
show_help_balloon (widget balloon, SI x, SI y) {
  (void) balloon;
  (void) x;
  (void) y;
}

void
show_wait_indicator (widget base, string message, string argument) {
  (void) base;
  (void) message;
  (void) argument;
}

void
external_event (string type, time_t t) {
  (void) type;
  (void) t;
}

url
get_current_buffer_safe () {
  return url ();
}

tree
load_inclusion (url u) {
  (void) u;
  return tree (moebius::ERROR, "unsupported inclusion");
}

tree
with_package_definitions (string package, tree body) {
  (void) package;
  return body;
}

tree
get_subtree (path p) {
  (void) p;
  return tree ();
}

void
exec_delayed (object cmd) {
  (void) cmd;
}

void
exec_delayed_pause (object cmd) {
  (void) cmd;
}

bool
in_presentation_mode () {
  return false;
}
