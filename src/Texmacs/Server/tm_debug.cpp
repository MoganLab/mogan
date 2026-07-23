
/******************************************************************************
 * MODULE     : tm_debug.cpp
 * DESCRIPTION: Debugging facilities
 * COPYRIGHT  : (C) 2011  Joris van der Hoeven
 *              (C) 2008  Timo Bingmann from http://idlebox.net
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "config.h"

#include "analyze.hpp"
#include "string.hpp"
#include "tm_sys_utils.hpp"
#include <moebius/tree_label.hpp>

#ifndef KERNEL_L3
#include "tm_link.hpp"
#include "tm_server.hpp"
#endif
#include "file.hpp"
#include "tm_file.hpp"

#include <cstdio>
#include <ctime>
#include <tbox/platform/time.h>

bool rescue_mode= false;

/******************************************************************************
 * Timestamp formatting
 ******************************************************************************/

static string
get_formatted_timestamp () {
  tb_timeval_t tv= {0};
  if (!tb_gettimeofday (&tv, tb_null)) {
    return "0000-00-00-00.00.00.000";
  }

  time_t seconds     = (time_t) tv.tv_sec;
  int    milliseconds= (int) (tv.tv_usec / 1000);

  struct tm tm_buf;
#ifdef OS_WIN
  localtime_s (&tm_buf, &seconds);
#else
  localtime_r (&seconds, &tm_buf);
#endif

  char buffer[64];
  strftime (buffer, sizeof (buffer), "%Y-%m-%d-%H.%M.%S", &tm_buf);

  char result[80];
  snprintf (result, sizeof (result), "%s.%03d", buffer, milliseconds);

  return result;
}

/******************************************************************************
 * Status reports
 ******************************************************************************/

string
get_system_information () {
  string r;
  r << "System information:\n";
  r << "  TeXmacs version  : " << TEXMACS_VERSION << "\n";
  r << "  Built by         : " << BUILD_USER << "\n";
  r << "  Building date    : " << BUILD_DATE << "\n";
  r << "  Operating system : " << get_pretty_os_name () << "\n";
  r << "  Processor        : " << get_current_cpu_arch () << "\n";
  r << "  Crash date       : " << var_eval_system ("date") << "\n";
  return r;
}

#if !defined(KERNEL_L3)
string
path_as_string (path p) {
  if (is_nil (p)) return "[]";
  string r= "[ ";
  r << as_string (p->item);
  p= p->next;
  while (!is_nil (p)) {
    r << ", " << as_string (p->item);
    p= p->next;
  }
  r << " ]";
  return r;
}

string
get_editor_status_report () {
  string r;

  if (!is_server_started ()) {
    r << "TeXmacs server not yet started";
    return r;
  }

  // If an error happens too early then there is no current view
  // and get_current_editor() will raise an exception leading to
  // an infinite loop. So we stop before.

  if (!has_current_view ()) {
    r << "TeXmacs does not yet have a current view";
    return r;
  }

  server sv= get_server ();
  r << "Editor status:\n";
  editor ed= get_current_editor ();
  path   start_p, end_p;
  ed->get_selection (start_p, end_p);
  r << "  Root path          : " << path_as_string (ed->rp) << "\n"
    << "  Current path       : " << path_as_string (ed->the_path ()) << "\n"
    << "  Shifted path       : " << path_as_string (ed->the_shifted_path ())
    << "\n"
    << "  Physical selection : " << path_as_string (start_p) << " -- "
    << path_as_string (end_p) << "\n";
  if (start_p != end_p) {
    selection sel;
    ed->selection_get (sel);
    r << "  Logical selection  : " << path_as_string (sel->start) << " -- "
      << path_as_string (sel->end) << "\n";
  }
  return r;
}

void
tree_report (string& s, tree t, path p, int indent) {
  for (int i= 0; i < indent; i++)
    s << " ";
  if (is_atomic (t)) {
    s << raw_quote (t->label);
    s << " -- " << path_as_string (p) << "\n";
  }
  else {
    s << as_string (L (t));
    s << " -- " << path_as_string (p) << "\n";
    for (int i= 0; i < N (t); i++)
      tree_report (s, t[i], p * i, indent + 2);
  }
}

string
tree_report (tree t, path p) {
  string s;
  tree_report (s, t, p, 0);
  return s;
}
#endif

/******************************************************************************
 * Crash management
 ******************************************************************************/

string
get_crash_report (const char* msg) {
  string r;
  r << "Error message:\n  " << msg << "\n"
#if defined(KERNEL_L3)
    << "\n"
    << get_system_information ();
#else
    << "\n"
    << get_system_information () << "\n"
    << get_editor_status_report () << "\n"
    << lolly::get_stacktrace ();
#endif
  return r;
}

void
tm_failure (const char* msg) {
  if (rescue_mode) {
    fprintf (stderr, "\nTeXmacs] Fatal unrecoverable error, %s\n", msg);
#ifdef DEBUG_ASSERT
    return;
#endif
    exit (1);
  }
  rescue_mode= true;
  cerr << "\nTeXmacs] Fatal error, " << msg << "\n";

  // cerr << "Saving crash report...\n";
  string report= get_crash_report (msg);
#if defined(KERNEL_L3)
  cerr << "TeXmacs] Dumping report below\n\n" << report << "\n";
#else
  url dir ("$TEXMACS_HOME_PATH/system/crash");
  url err= url_numbered (dir, "crash_report_", "");
  if (!save_string (err, report))
    cerr << "TeXmacs] Crash report saved in " << err << "\n";
  else
    cerr << "TeXmacs] Crash report could not be saved in " << err << "\n"
         << "TeXmacs] Dumping report below\n\n"
         << report << "\n";

  // cerr << "Saving current buffer...\n";
  server sv     = get_server ();
  editor ed     = get_current_editor ();
  string buf    = tree_report (subtree (the_et, ed->rp), ed->rp);
  url    buf_err= glue (err, "_tree");
  if (!save_string (buf_err, buf))
    cerr << "TeXmacs] Current buffer report saved in " << buf_err << "\n";
  else
    cerr << "TeXmacs] Current buffer report could not be saved in " << buf_err
         << "\n"
         << "TeXmacs] Dumping report below\n\n"
         << buf << "\n";

  // cerr << "Autosaving...\n";
  call ("autosave-all");
  // cerr << "Closing pipes...\n";
  close_all_pipes ();
  call ("quit-TeXmacs-scheme");
  clear_pending_commands ();
  // exit (1);
#endif
}

/******************************************************************************
 * debugging messages
 ******************************************************************************/

#if !defined(KERNEL_L3)
tree        debug_messages (TUPLE);
bool        debug_lf_flag= false;
extern bool texmacs_started;

void
debug_message_sub (string channel, string msg) {
  if (occurs ("\n", msg)) {
    int pos= search_forwards ("\n", 0, msg);
    debug_message_sub (channel, msg (0, pos));
    debug_lf_flag= true;
    cout << "\n";
    if (pos + 1 < N (msg)) debug_message_sub (channel, msg (pos + 1, N (msg)));
  }
  else {
    int n= N (debug_messages);
    if (!debug_lf_flag && n > 0 && is_tuple (debug_messages[n - 1], channel)) {
      tree* t= &(debug_messages[n - 1][1]);
      *t     = (*t)->label * msg;
      cout << msg;
    }
    else {
      debug_messages << tuple (channel, msg, "");
      debug_lf_flag= false;
      if (channel != "debug-boot") {
#ifdef DEBUG_WITH_TIMESTAMP
        cout << get_formatted_timestamp () << " ";
#endif
        cout << "TeXmacs] ";
        if (channel != "debug-automatic" && channel != "boot-error")
          cout << channel << ", ";
      }
      cout << msg;
    }
  }
}

void
debug_message (string channel, string msg) {
  debug_message_sub (channel, msg);
  if (texmacs_started && channel != "debug-widgets")
    call ("notify-debug-message", object (channel));
}

void
debug_formatted (string channel, tree msg) {
  int n= N (debug_messages);
  if (n > 0 && is_tuple (debug_messages[n - 1], channel)) {
    debug_messages[n - 1][2]= msg;
    if (texmacs_started && channel != "debug-widgets")
      call ("notify-debug-message", object (channel));
  }
}

tree
get_debug_messages (string kind, int max_number) {
  tree m (TUPLE);
  for (int i= N (debug_messages) - 1; i >= 0; i--) {
    tree t= debug_messages[i];
    if (!is_func (t, TUPLE, 3) || !is_atomic (t[0])) continue;
    string s= t[0]->label;
    if (kind == "Debugging console" || ends (s, "-error") ||
        ends (s, "-warning"))
      m << t;
    if (N (m) >= max_number) break;
  }
  tree r (TUPLE);
  for (int i= N (m) - 1; i >= 0; i--)
    r << m[i];
  return r;
}

void
clear_debug_messages (string channel) {
  tree r= tree (TUPLE);
  for (int i= 0; i < N (debug_messages); i++)
    if (is_func (debug_messages[i], TUPLE, 3) &&
        debug_messages[i][0] != channel)
      r << debug_messages[i];
  debug_messages= r;
  debug_lf_flag = false;
}

void
clear_debug_messages () {
  debug_messages= tree (TUPLE);
  debug_lf_flag = false;
}
#endif

/******************************************************************************
 * Streams for debugging purposes
 ******************************************************************************/

class debug_ostream_rep : public tm_ostream_rep {
public:
  string channel;

public:
  debug_ostream_rep (string channel);
  ~debug_ostream_rep ();

  bool is_writable () const;
  void write (const char*);
  void write (tree t);
  void clear ();
};

debug_ostream_rep::debug_ostream_rep (string channel2) : channel (channel2) {}
debug_ostream_rep::~debug_ostream_rep () {}

bool
debug_ostream_rep::is_writable () const {
  return true;
}

void
debug_ostream_rep::clear () {
#if !defined(KERNEL_L3)
  clear_debug_messages (channel);
#endif
}

void
debug_ostream_rep::write (const char* s) {
#if !defined(KERNEL_L3)
  debug_message (channel, s);
#endif
}

void
debug_ostream_rep::write (tree t) {
#if !defined(KERNEL_L3)
  debug_formatted (channel, t);
#endif
}

tm_ostream
debug_ostream (string channel) {
  return (tm_ostream_rep*) tm_new<debug_ostream_rep> (channel);
}

/******************************************************************************
 * debugging
 ******************************************************************************/

static long int debug_status= 0;

bool
debug (int which, bool write_flag) {
  if (write_flag) {
    debug_status= debug_status | (((long int) 1) << which);
    return 0;
  }
  else return (debug_status & (((long int) 1) << which)) > 0;
}

int
debug_off () {
  int status  = debug_status;
  debug_status= 0;
  return status;
}

void
debug_on (int status) {
  debug_status= status;
}

static void
debug_set (int which, bool on) {
  if (on) debug_status= debug_status | (((long int) 1) << which);
  else debug_status= debug_status & (~(((long int) 1) << which));
}

void
debug_set (string s, bool on) {
  if (s == "auto") debug_set (DEBUG_FLAG_AUTO, on);
  else if (s == "verbose") debug_set (DEBUG_FLAG_VERBOSE, on);
  else if (s == "events") debug_set (DEBUG_FLAG_EVENTS, on);
  else if (s == "std") debug_set (DEBUG_FLAG_STD, on);
  else if (s == "io") debug_set (DEBUG_FLAG_IO, on);
  else if (s == "bench") debug_set (DEBUG_FLAG_BENCH, on);
  else if (s == "history") debug_set (DEBUG_FLAG_HISTORY, on);
  else if (s == "qt") debug_set (DEBUG_FLAG_QT, on);
  else if (s == "qt-widgets") debug_set (DEBUG_FLAG_QT_WIDGETS, on);
  else if (s == "keyboard") debug_set (DEBUG_FLAG_KEYBOARD, on);
  else if (s == "packrat") debug_set (DEBUG_FLAG_PACKRAT, on);
  else if (s == "flatten") debug_set (DEBUG_FLAG_FLATTEN, on);
  else if (s == "parser") debug_set (DEBUG_FLAG_PARSER, on);
  else if (s == "correct") debug_set (DEBUG_FLAG_CORRECT, on);
  else if (s == "convert") debug_set (DEBUG_FLAG_CONVERT, on);
  else if (s == "remote") debug_set (DEBUG_FLAG_REMOTE, on);
  else if (s == "live") debug_set (DEBUG_FLAG_LIVE, on);
  else if (s == "loro") debug_set (DEBUG_FLAG_LORO, on);
}

static bool
debug_get (int which) {
  return (debug_status & (((long int) 1) << which)) != 0;
}

bool
debug_get (string s) {
  if (s == "auto") return debug_get (DEBUG_FLAG_AUTO);
  else if (s == "verbose") return debug_get (DEBUG_FLAG_VERBOSE);
  else if (s == "events") return debug_get (DEBUG_FLAG_EVENTS);
  else if (s == "std") return debug_get (DEBUG_FLAG_STD);
  else if (s == "io") return debug_get (DEBUG_FLAG_IO);
  else if (s == "bench") return debug_get (DEBUG_FLAG_BENCH);
  else if (s == "history") return debug_get (DEBUG_FLAG_HISTORY);
  else if (s == "qt") return debug_get (DEBUG_FLAG_QT);
  else if (s == "qt-widgets") return debug_get (DEBUG_FLAG_QT_WIDGETS);
  else if (s == "keyboard") return debug_get (DEBUG_FLAG_KEYBOARD);
  else if (s == "packrat") return debug_get (DEBUG_FLAG_PACKRAT);
  else if (s == "flatten") return debug_get (DEBUG_FLAG_FLATTEN);
  else if (s == "correct") return debug_get (DEBUG_FLAG_CORRECT);
  else if (s == "convert") return debug_get (DEBUG_FLAG_CONVERT);
  else if (s == "remote") return debug_get (DEBUG_FLAG_REMOTE);
  else if (s == "live") return debug_get (DEBUG_FLAG_LIVE);
  else if (s == "loro") return debug_get (DEBUG_FLAG_LORO);
  else return false;
}

tm_ostream std_error    = debug_ostream ("std-error");
tm_ostream failed_error = debug_ostream ("failed-error");
tm_ostream boot_error   = debug_ostream ("boot-error");
tm_ostream qt_error     = debug_ostream ("qt-error");
tm_ostream widkit_error = debug_ostream ("widkit-error");
tm_ostream aqua_error   = debug_ostream ("aqua-error");
tm_ostream font_error   = debug_ostream ("font-error");
tm_ostream convert_error= debug_ostream ("convert-error");
tm_ostream bibtex_error = debug_ostream ("bibtex-error");
tm_ostream io_error     = debug_ostream ("io-error");

tm_ostream std_warning    = debug_ostream ("std-warning");
tm_ostream convert_warning= debug_ostream ("convert-warning");
tm_ostream typeset_warning= debug_ostream ("typeset-warning");
tm_ostream io_warning     = debug_ostream ("io-warning");
tm_ostream widkit_warning = debug_ostream ("widkit-warning");
tm_ostream bibtex_warning = debug_ostream ("bibtex-warning");

tm_ostream debug_std      = debug_ostream ("debug-std");
tm_ostream debug_qt       = debug_ostream ("debug-qt");
tm_ostream debug_aqua     = debug_ostream ("debug-aqua");
tm_ostream debug_widgets  = debug_ostream ("debug-widgets");
tm_ostream debug_fonts    = debug_ostream ("debug-fonts");
tm_ostream debug_convert  = debug_ostream ("debug-convert");
tm_ostream debug_typeset  = debug_ostream ("debug-typeset");
tm_ostream debug_edit     = debug_ostream ("debug-edit");
tm_ostream debug_packrat  = debug_ostream ("debug-packrat");
tm_ostream debug_history  = debug_ostream ("debug-history");
tm_ostream debug_keyboard = debug_ostream ("debug-keyboard");
tm_ostream debug_automatic= debug_ostream ("debug-automatic");
tm_ostream debug_boot     = debug_ostream ("debug-boot");
tm_ostream debug_events   = debug_ostream ("debug-events");
tm_ostream debug_shell    = debug_ostream ("debug-shell");
tm_ostream debug_io       = debug_ostream ("debug-io");
tm_ostream debug_spell    = debug_ostream ("debug-spell");
tm_ostream debug_updater  = debug_ostream ("debug-updater");
tm_ostream debug_loro     = debug_ostream ("debug-loro");

tm_ostream std_bench= debug_ostream ("std-bench");
