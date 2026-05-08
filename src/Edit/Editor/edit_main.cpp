
/******************************************************************************
 * MODULE     : editor.cpp
 * DESCRIPTION: routines for the editor
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "edit_main.hpp"
#include "connect.hpp"
#include "convert.hpp"
#include "converter.hpp"
#include "cork.hpp"
#include "image_files.hpp"
#include "iterator.hpp"
#include "message.hpp"
#include "observers.hpp"
#include "printer.hpp"
#include "sys_utils.hpp"
#include "tm_buffer.hpp"
#include "tm_file.hpp"
#include "tree_observer.hpp"
#include "typesetter.hpp"

#include <moebius/drd/drd_std.hpp>
#include <setjmp.h>

#ifdef USE_PLUGIN_PDF
#include "Pdf/pdf_hummus_make_attachment.hpp"
#endif

#ifdef USE_PLUGIN_GS
#include "Ghostscript/gs_utilities.hpp"
#endif

#ifdef QTTEXMACS
#include "Qt/qt_gui.hpp"
#include "Qt/qt_utilities.hpp"
#endif

using namespace moebius;
using moebius::drd::std_drd;

/******************************************************************************
 * Constructors and destructor
 ******************************************************************************/

editor_rep::editor_rep ()
    : simple_widget_rep (), cvw (NULL), mvw (NULL), drd (std_drd), et (the_et),
      rp () {
  cout << "TeXmacs] warning, this virtual constructor should never be called\n";
}

editor_rep::editor_rep (server_rep* sv2, tm_buffer buf2)
    : simple_widget_rep (), sv (sv2), cvw (NULL), mvw (NULL), buf (buf2),
      drd (buf->buf->title, std_drd), et (the_et), rp (buf2->rp) {}

bool
editor_rep::is_current_editor () {
  editor ed= get_current_editor ();
  return ed.rep == (editor_rep*) this;
}

edit_main_rep::edit_main_rep (server_rep* sv, tm_buffer buf)
    : editor_rep (sv, buf), props (UNKNOWN), ed_obs (edit_observer (this)) {
  attach_observer (subtree (et, rp), ed_obs);
  notify_change (THE_TREE);
  tp= correct_cursor (et, rp * 0);
}

edit_main_rep::~edit_main_rep () { detach_observer (subtree (et, rp), ed_obs); }

editor
new_editor (server_rep* sv, tm_buffer buf) {
  return tm_new<edit_main_rep> (sv, buf);
}

template <>
void
tm_delete<editor_rep> (editor_rep* ptr) {
  void* mem= ptr->derived_this ();
  ptr->~editor_rep ();
  fast_delete (mem);
}

/******************************************************************************
 * Properties
 ******************************************************************************/

void
edit_main_rep::set_property (scheme_tree what, scheme_tree val) {
  props (what)= val;
}

void
edit_main_rep::set_bool_property (string what, bool val) {
  props (what)= (val ? string ("true") : string ("false"));
}

void
edit_main_rep::set_int_property (string what, int val) {
  props (what)= as_tree (val);
}

void
edit_main_rep::set_string_property (string what, string val) {
  props (what)= val;
}

scheme_tree
edit_main_rep::get_property (scheme_tree what) {
  return props[what];
}

bool
edit_main_rep::get_bool_property (string what) {
  return as_bool (props[what]);
}

int
edit_main_rep::get_int_property (string what) {
  return as_int (props[what]);
}

string
edit_main_rep::get_string_property (string what) {
  return as_string (props[what]);
}

/******************************************************************************
 * Global routines
 ******************************************************************************/

void
edit_main_rep::clear_buffer () {
  assign (rp, tree (DOCUMENT, tree ("")));
}

void
edit_main_rep::new_window () {}

void
edit_main_rep::clone_window () {}

void
edit_main_rep::tex_buffer () {}

url
edit_main_rep::get_name () {
  return buf->buf->name;
}

void
edit_main_rep::focus_on_this_editor () {
  focus_on_editor (this);
}

void
edit_main_rep::notify_page_change () {
  if (is_attached (this)) send_invalidate_all (this);
}

string
edit_main_rep::get_metadata (string kind) {
  string var= "global-" * kind;
  string val= cork_to_utf8 (get_init_string (var));
  if (val != "") return val;
  val= cork_to_utf8 (search_metadata (subtree (et, rp), kind));
  if (val != "") return val;
  if (kind == "title") return as_string (tail (get_name ()));
  return "";
}

/******************************************************************************
 * Printing
 ******************************************************************************/

string printing_dpi ("600");
string printing_on ("a4");

int
edit_main_rep::nr_pages () {
  string medium= env->get_string (PAGE_MEDIUM);
  if (medium == "paper") return N (eb[0]);
  typeset_prepare ();
  env->write (PAGE_MEDIUM, "paper");
  box the_box= typeset_as_document (env, subtree (et, rp), reverse (rp));
  env->write (PAGE_MEDIUM, medium);
  return N (the_box[0]);
}

void
edit_main_rep::print_doc (url name, bool conform, int first, int last) {
  bool ps  = (suffix (name) == "ps");
  bool pdf = (suffix (name) == "pdf");
  url  orig= resolve (name, "");

  string medium= env->get_string (PAGE_MEDIUM);
  if (conform && (medium != "paper")) conform= false;
  // FIXME: better command for conform printing

  typeset_preamble ();
  // FIXME: when printing several files via aux buffers,
  // it seems that the style can be corrupted.  Why?

  // Set environment variables for printing

  typeset_prepare ();
  env->write (PAGE_SHOW_HF, "true");
  env->write (PAGE_SCREEN_MARGIN, "false");
  env->write (PAGE_BORDER, "none");
  if (is_func (env->read (BG_COLOR), PATTERN))
    env->write (BG_COLOR, env->exec (env->read (BG_COLOR)));
  if (!conform) {
    env->write (PAGE_MEDIUM, "paper");
    env->write (PAGE_PRINTED, "true");
  }

  // Typeset pages for printing
  // NOTE: We inline typeset_as_document here and defer delete_typesetter
  // until after the rendering loop. This is necessary because display_links
  // (called during redraw for non-screen renderers) queries global link
  // tables via get_ids/get_links. These tables are populated by the
  // typesetter during typesetting and cleared when the typesetter is
  // destroyed. If we destroy the typesetter before rendering (as
  // typeset_as_document does), hyperlinks in the PDF become unclickable
  // because the link registrations are already gone. This particularly
  // affects beamer/slideshow export where a temporary buffer copy has no
  // prior link registrations from screen display. (See issue #2843)

  env->style_init_env ();
  env->update ();
  typesetter ttt    = new_typesetter (env, subtree (et, rp), reverse (rp));
  box        the_box= ::typeset (ttt);

  // Determine parameters for printer

  string page_type= env->page_real_type;
  double w        = env->page_real_width;
  double h        = env->page_real_height;
  double cm       = env->as_real_length (string ("1cm"));
  bool   landsc   = env->page_landscape;
  int    dpi      = env->get_int (DPI);
  int    start    = max (0, first - 1);
  int    end      = min (N (the_box[0]), last);
  int    pages    = end - start;
  if (conform) {
    page_type = "user";
    SI     bw = the_box[0][0]->w ();
    SI     bh = the_box[0][0]->h ();
    string bws= as_string (bw) * "tmpt";
    string bhs= as_string (bh) * "tmpt";
    w         = env->as_length (bws);
    h         = env->as_length (bhs);
  }

  // Print pages
  renderer ren= printer (name, dpi, pages, page_type, landsc, w / cm, h / cm);

  if (ren->is_started ()) {
    int i;
    ren->set_metadata ("title", get_metadata ("title"));
    ren->set_metadata ("author", get_metadata ("author"));
    ren->set_metadata ("subject", get_metadata ("subject"));
    ren->set_metadata ("keyword", get_metadata ("keyword"));
    for (i= start; i < end; i++) {
      tree bg= env->read (BG_COLOR);
      ren->set_background (bg);
      if (bg != "white" && bg != "#ffffff")
        ren->clear_pattern (0, (SI) -h, (SI) w, 0);

      rectangles rs;
      the_box[0]->sx (i)= 0;
      the_box[0]->sy (i)= 0;
      the_box[0][i]->redraw (ren, path (0), rs);
      if (i < end - 1) ren->next_page ();
    }
  }
  tm_delete (ren);
  delete_typesetter (ttt);
}

void
edit_main_rep::print_to_file (url name, string first, string last) {
  print_doc (name, false, as_int (first), as_int (last));
  set_message ("Done printing", "print to file");
}

void
edit_main_rep::print_buffer (string first, string last) {
  url target= url_temp ("pdf");
  print_doc (target, false, as_int (first), as_int (last));
  system (get_printing_cmd (), target); // Send the document to the printer
  set_message ("Done printing", "print buffer");
  ::remove (target);
}

#ifdef THISISTHEPREVIOUSCODE_IJUSTLEFTITHEREINCASE
void
edit_main_rep::print_buffer (string first, string last) {
  // in Qt this is the main entry point to the printing subsystem.
  // the other routines (print_to_file, ...) are overriden since all fine tuning
  // is made here via the Qt print dialog
  bool   to_file, landscape;
  url    name= url_none ();
  string printer;
  string paper_type;
  if (qt_print (to_file, landscape, printer, name, first, last, paper_type)) {
    if (!to_file) name= url_temp ("ps");
    print_doc (name, false, as_int (first), as_int (last));
    if (!to_file) {
      string cmd= printing_cmd * " -P" * printer;
      system (cmd, name);
      ::remove (name);
    }
  }
}
#endif

void
edit_main_rep::export_ps (url name, string first, string last) {
  print_doc (name, true, as_int (first), as_int (last));
}

array<int>
edit_main_rep::print_snippet (url name, tree t, bool conserve_preamble) {
  tree buft= subtree (et, rp);
  if (conserve_preamble)
    if (is_document (buft) && is_compound (buft[0], "hide-preamble"))
      t= tree (SURROUND, buft[0], "", t);
  path temp_root= new_document ();
  temp_root << 0;
  assign (subtree (et, temp_root), copy (t));

  string s= suffix (name);
  bool   bitmap=
      (s == "png" || s == "jpg" || s == "jpeg" || s == "tif" || s == "tiff");
#ifndef QTTEXMACS
  bitmap= false;
#endif
  bool ps= (s == "ps" || s == "eps");

  typeset_prepare ();
  int  dpi          = as_int (printing_dpi);
  tree old_dpi      = env->read (DPI);
  tree old_info_flag= env->read (INFO_FLAG);
  env->write (DPI, printing_dpi);
  if (is_compound (old_info_flag) || !ends (old_info_flag->label, "paper"))
    env->write (INFO_FLAG, "none");
  env->style_init_env ();
  env->update ();
  box b= typeset_as_box (env, t, reverse (temp_root));
  env->write (DPI, old_dpi);
  env->write (INFO_FLAG, old_info_flag);
  env->style_init_env ();
  env->update ();

  if (b->x4 - b->x3 >= 5 * PIXEL && b->y4 - b->y3 >= 5 * PIXEL) {
    if (bitmap) make_raster_image (name, b, 5.0);
    else if (ps) make_eps (name, b, dpi);
    else {
      url temp= url_temp ("pdf");
      make_eps (temp, b, dpi);
      ::remove (name);
      if (!call_scm_converter (temp, name)) {
        if (!exists (name))
          convert_error << "could not convert snippet " << temp
                        << " into :" << name << "\n";
      }
      ::remove (temp);
    }
  }
  array<int> a;
  a << b->x3 << b->y3 << b->x4 << b->y4 << b->x1 << b->y1 << b->x2 << b->y2;
  a << env->get_int (FONT_BASE_SIZE) << dpi;

  delete_document (path_up (temp_root));
  return a;
}

bool
edit_main_rep::graphics_file_to_clipboard (url name) {
#ifdef QTTEXMACS
  the_gui->put_graphics_on_clipboard (name);
  return true;
#else
  return false;
#endif
}

/******************************************************************************
 * Evaluation of expressions
 ******************************************************************************/

void
edit_main_rep::footer_eval (string s) {
  // s= unslash (s); // FIXME: dirty fix; should not be necessary
  s       = tm_decode (s);
  string r= object_to_string (eval (s));
  set_message (verbatim (r), "evaluate expression");
}

tree
edit_main_rep::the_line () {
  path p= search_parent_upwards (DOCUMENT);
  return copy (subtree (et, p));
}

tree
edit_main_rep::the_root () {
  return et;
}

tree
edit_main_rep::the_buffer () {
  return subtree (et, rp);
}

bool
edit_main_rep::test_subtree (path p) {
  return has_subtree (et, p);
}

tree
edit_main_rep::the_subtree (path p) {
  return subtree (et, p);
}

path
edit_main_rep::the_buffer_path () {
  return copy (rp);
}

path
edit_main_rep::the_path () {
  return copy (tp);
}

path
edit_main_rep::the_shifted_path () {
  return shift (et, tp, 1);
}

/******************************************************************************
 * Miscellaneous
 ******************************************************************************/

void
stretched_print (box b) {
  if (N (b) == 0) cout << b << " " << reverse (b->ip) << LF;
  else {
    tree t= (tree) b;
    if (is_tuple (t) && N (t) > 0) t= t[0];
    cout << t << " " << reverse (b->ip) << LF << INDENT;
    for (int i= 0; i < N (b); i++)
      stretched_print (b[i]);
    cout << UNINDENT;
  }
}

void
edit_main_rep::show_tree () {
  stretched_print (et, true);
  // cout << et << "\n";
}

void
edit_main_rep::show_box () {
  stretched_print (eb);
}

void
edit_main_rep::show_env () {
  cout << env << "\n";
}

void
edit_main_rep::show_path () {
  cout << tp << "\n";
}

void
edit_main_rep::show_cursor () {
  cout << "Principal cursor: " << cu->ox << ", " << cu->oy << " [" << cu->delta
       << "], " << cu->y1 << " : " << cu->y2 << ", " << cu->slope << "\n";
  cout << "Ghost cursor    : " << mv->ox << ", " << mv->oy << " [" << mv->delta
       << "], " << mv->y1 << " : " << mv->y2 << ", " << mv->slope << "\n";
}

void
edit_main_rep::show_selection () {
  selection sel;
  selection_get (sel);
  cout << "physical  selection: " << cur_sel << "\n";
  cout << "logical   selection: " << sel->start << " --- " << sel->end << "\n";
}

void
edit_main_rep::show_meminfo () {
  mem_info ();
}

void
edit_main_rep::edit_special () {}
