
/******************************************************************************
 * MODULE     : init_texmacs.cpp
 * DESCRIPTION: Initialization of TeXmacs
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "boot.hpp"
#include "convert.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "language.hpp"
#include "merge_sort.hpp"
#include "moebius/tree_label.hpp"
#include "new_buffer.hpp"
#include "new_window.hpp"
#include "preferences.hpp"
#include "server.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tm_link.hpp"

#include <signal.h>
#ifdef OS_WIN
#include <process.h>
#else
#include <unistd.h>
#endif
#ifdef OS_MINGW
#include <direct.h>
#include <time.h>
#endif
#ifdef QTTEXMACS
#include "Qt/QTMApplication.hpp"
#include "Qt/QTMOAuth.hpp"
#include "Qt/qt_gui.hpp"
#include "Qt/qt_guide_window.hpp"
#include "Qt/qt_utilities.hpp"
#include "tm_server.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QTimer>
#endif
#include "Metafont/load_tex.hpp"

#include "scheme.hpp"
#include <moebius/data/scheme.hpp>
#include <moebius/drd/drd_std.hpp>

using moebius::data::block_to_scheme_tree;
using moebius::data::scheme_tree_to_block;
using moebius::data::scm_quote;
using moebius::drd::init_std_drd;

extern int    geometry_w, geometry_h;
extern int    geometry_x, geometry_y;
extern url    tm_init_file;
extern url    tm_init_buffer_file;
extern string my_init_cmds;
extern bool   char_clip;
extern bool   texmacs_started;
extern bool   headless_mode;

#ifdef QTTEXMACS
bool g_startup_login_requested= false;
bool g_startup_login_executed = false;
#endif

string extra_init_cmd;
bool   disable_error_recovery= false;
bool   start_server_flag     = false;

int install_status= 0;

#ifdef QTTEXMACS
bool show_startup_login_dialog ();
#endif

void server_start ();

/******************************************************************************
 * Clean exit on fatal signals
 ******************************************************************************/

static void
clean_exit_on_signal (int sig_num) {
  // 首先关闭所有管道（子进程）
  close_all_pipes ();

  // 打印信号信息
  const char* sig_name= "UNKNOWN";
  switch (sig_num) {
  case SIGSEGV:
    sig_name= "SIGSEGV (segmentation fault)";
    break;
#ifndef OS_WIN
  case SIGTRAP:
    sig_name= "SIGTRAP (trace/breakpoint trap)";
    break;
#endif
  case SIGABRT:
    sig_name= "SIGABRT (abort)";
    break;
  case SIGTERM:
    sig_name= "SIGTERM (termination signal)";
    break;
  case SIGINT:
    sig_name= "SIGINT (interrupt)";
    break;
#ifdef SIGQUIT
  case SIGQUIT:
    sig_name= "SIGQUIT (quit)";
    break;
#endif
  default:
    sig_name= "UNKNOWN";
    break;
  }

  cerr << "Process terminated by signal: " << sig_name << " (" << sig_num
       << ")\n";

  // 对于崩溃类信号，打印堆栈跟踪
  if (sig_num == SIGSEGV || sig_num == SIGABRT
#ifndef OS_WIN
      || sig_num == SIGTRAP
#endif
  ) {
    cerr << lolly::get_stacktrace () << LF;
  }

  // 使用默认信号处理器终止程序
  signal (sig_num, SIG_DFL);
  raise (sig_num);
}

/******************************************************************************
 * Texmacs paths
 ******************************************************************************/
void
init_texmacs_path (int& argc, char** argv) {
  (void) argc;
  (void) argv;
#ifdef QTTEXMACS
  url exedir= url_system (qt_application_directory ());
#else
  url exedir= url_system (argv[0]) * "..";
  if (!is_rooted (exedir)) {
    exedir= url_pwd () * exedir;
  }
#endif

  string current_texmacs_path= get_env ("TEXMACS_PATH");

#ifdef OS_GNU_LINUX
  if (is_empty (current_texmacs_path) &&
      exists (exedir * "../share/" * PREFIX_DIR)) {
    set_env ("TEXMACS_PATH", as_string (exedir * "../share/" * PREFIX_DIR));
  }
#endif

#ifdef OS_MACOS
  // the following line can inibith external plugin loading
  // QCoreApplication::setLibraryPaths(QStringList());
  // ideally we would like to control the external plugins
  // and add the most useful (gif, jpeg, svg converters)
  // to the bundle package. I still do not have a reliable solution
  // so just allow everything that is reachable.

  // plugins need to be installed in TeXmacs.app/Contents/Plugins
  QCoreApplication::addLibraryPath (QDir::cleanPath (
      QCoreApplication::applicationDirPath ().append ("/../Plugins")));
  // cout << from_qstring ( QCoreApplication::libraryPaths () .join("\n") ) <<
  // LF;
  {
    // ensure that private versions of the Qt frameworks have priority on
    // other instances.
    // in the event that we load qt plugins which could possibly link to
    // other instances of the Qt libraries
    string buf;
    buf= as_string (exedir * "../Frameworks");
    if (get_env ("DYLD_FRAMEWORK_PATH") != "")
      buf= buf * ":" * get_env ("DYLD_FRAMEWORK_PATH");
    set_env ("DYLD_FRAMEWORK_PATH", buf);
    buf= as_string (exedir * "../Resources/lib");
    if (get_env ("DYLD_LIBRARY_PATH") != "")
      buf= buf * ":" * get_env ("DYLD_LIBRARY_PATH");
    set_env ("DYLD_LIBRARY_PATH", buf);
  }
#endif

#if defined(AQUATEXMACS) || defined(OS_MACOS) || defined(MACOSX_EXTENSIONS)
  // Mac bundle environment initialization
  // We set some environment variables when the executable
  // is in a .app bundle on MacOSX
  if (is_empty (current_texmacs_path)) {
    set_env ("TEXMACS_PATH",
             as_string (exedir * "../Resources/share/" * PREFIX_DIR));
  }
#endif

#if defined(OS_MINGW) || defined(OS_WIN)
  // Win bundle environment initialization
  // TEXMACS_PATH is set by assuming that the executable is in TeXmacs/bin/
  // HOME is set to USERPROFILE
  // PWD is set to HOME
  // if PWD is lacking, then the path resolution machinery may not work

  if (is_empty (current_texmacs_path))
    set_env ("TEXMACS_PATH", as_string (exedir * ".."));
  // if (get_env ("HOME") == "") //now set in immediate_options otherwise
  // --setup option fails
  //   set_env ("HOME", get_env("USERPROFILE"));
  // HACK
  // In WINE the variable PWD is already in the outer Unix environment
  // so we need to override it to have a correct behaviour
  if ((get_env ("PWD") == "") || (get_env ("PWD")[0] == '/')) {
    set_env ("PWD", as_string (exedir));
    // set_env ("PWD", get_env("HOME"));
  }
  // system("set");
#endif

#ifdef OS_WASM
  set_env ("PWD", "/");
  set_env ("HOME", "/");
  if (is_empty (current_texmacs_path)) set_env ("TEXMACS_PATH", "/TeXmacs");
#endif

  // check on the latest $TEXMACS_PATH
  current_texmacs_path= get_env ("TEXMACS_PATH");
  if (is_empty (current_texmacs_path) ||
      !exists (url_system (current_texmacs_path))) {
    cout << "The required TEXMACS_PATH(" << current_texmacs_path
         << ") does not exists" << LF;
    exit (1);
  }
}

/******************************************************************************
 * Subroutines for paths
 ******************************************************************************/

static url
get_env_path (string which) {
  return url ("$" * which);
}

static void
set_env_path (string which, url val) {
  // cout << which << " := " << val << "\n";
  if ((!is_none (val)) && (val->t != "")) set_env (which, as_string (val));
}

static url
get_env_path (string which, url def) {
  url val= get_env_path (which);
  if (is_none (val) || (val->t == "")) {
    set_env_path (which, def);
    return def;
  }
  return val;
}

static url
plugin_path (string which) {
  url base  = url_unix ("$TEXMACS_HOME_PATH:$TEXMACS_PATH");
  url search= base * "plugins" * url_wildcard ("*") * which;
  return expand (complete (search, "r"));
}

scheme_tree
plugin_list () {
  bool          flag;
  array<string> a= read_directory ("$TEXMACS_PATH/plugins", flag);
  a << read_directory ("$TEXMACS_HOME_PATH/plugins", flag);
  merge_sort (a);
  int  i, n= N (a);
  tree t (TUPLE);
  for (i= 0; i < n; i++)
    if ((a[i] != ".") && (a[i] != "..") && ((i == 0) || (a[i] != a[i - 1])))
      t << a[i];
  return t;
}

/******************************************************************************
 * Initialize main paths
 ******************************************************************************/

void
init_main_paths () {
  url default_path;
#if defined(OS_MINGW) || defined(OS_WIN)
  default_path= get_env ("APPDATA") * "/" * PREFIX_DIR;
#else
  default_path= "~/.TeXmacs";
#endif
  url home_path= get_env_path ("TEXMACS_HOME_PATH", default_path);
  if (is_none (home_path)) {
    boot_error << "\n";
    boot_error << "Installation problem: please send a bug report.\n";
    boot_error << "'TEXMACS_HOME_PATH' could not be set to '~/.TeXmacs'.\n";
    boot_error << "You may try to set this environment variable manually\n";
    boot_error << "\n";
    TM_FAILED ("installation problem");
    exit (1);
  }
}

/******************************************************************************
 * Directory for temporary files
 ******************************************************************************/

bool
process_running (int pid) {
  string cmd= "ps -p " * as_string (pid);
  string ret= eval_system (cmd);
  return occurs ("texmacs", ret) && occurs (as_string (pid), ret);
}

static void
clean_temp_dirs () {
  bool          err         = false;
  url           main_tmp_dir= url_temp_dir ();
  array<string> a           = read_directory (main_tmp_dir, err);
  for (int i= 0; i < N (a); i++)
    if (is_int (a[i]))
      if (!process_running (as_int (a[i])))
        if (a[i] != as_string ((int) getpid ()))
          rmdir (url (main_tmp_dir) * url (a[i]));
}

/******************************************************************************
 * Make user directories
 ******************************************************************************/

void
init_user_dirs () {
  make_dir ("$TEXMACS_HOME_PATH");
  make_dir ("$TEXMACS_HOME_PATH/bin");
  make_dir ("$TEXMACS_HOME_PATH/doc");
  make_dir ("$TEXMACS_HOME_PATH/doc/about");
  make_dir ("$TEXMACS_HOME_PATH/doc/about/changes");
  make_dir ("$TEXMACS_HOME_PATH/fonts");
  make_dir ("$TEXMACS_HOME_PATH/fonts/enc");
  make_dir ("$TEXMACS_HOME_PATH/fonts/error");
  make_dir ("$TEXMACS_HOME_PATH/fonts/tfm");
  make_dir ("$TEXMACS_HOME_PATH/fonts/truetype");
  make_dir ("$TEXMACS_HOME_PATH/fonts/type1");
  make_dir ("$TEXMACS_HOME_PATH/fonts/virtual");
  make_dir (get_tm_cache_path ());
  make_dir (get_tm_cache_path () * "fonts/pk");
  make_dir ("$TEXMACS_HOME_PATH/langs");
  make_dir ("$TEXMACS_HOME_PATH/langs/mathematical");
  make_dir ("$TEXMACS_HOME_PATH/langs/mathematical/syntax");
  make_dir ("$TEXMACS_HOME_PATH/langs/natural");
  make_dir ("$TEXMACS_HOME_PATH/langs/natural/dic");
  make_dir ("$TEXMACS_HOME_PATH/langs/natural/hyphen");
  make_dir ("$TEXMACS_HOME_PATH/langs/programming");
  make_dir ("$TEXMACS_HOME_PATH/misc");
  make_dir ("$TEXMACS_HOME_PATH/misc/patterns");
  make_dir ("$TEXMACS_HOME_PATH/misc/pixmaps");
  make_dir ("$TEXMACS_HOME_PATH/misc/themes");
  make_dir ("$TEXMACS_HOME_PATH/packages");
  make_dir ("$TEXMACS_HOME_PATH/plugins");
  make_dir ("$TEXMACS_HOME_PATH/progs");
  make_dir ("$TEXMACS_HOME_PATH/server");
  make_dir ("$TEXMACS_HOME_PATH/styles");
  make_dir ("$TEXMACS_HOME_PATH/system");
  make_dir ("$TEXMACS_HOME_PATH/system/bib");
  make_dir ("$TEXMACS_HOME_PATH/system/database");
  make_dir ("$TEXMACS_HOME_PATH/system/database/bib");
  make_dir ("$TEXMACS_HOME_PATH/system/make");
  make_dir ("$TEXMACS_HOME_PATH/system/tmp");
  make_dir ("$TEXMACS_HOME_PATH/texts");
  make_dir ("$TEXMACS_HOME_PATH/users");
  clean_temp_dirs ();
}

/******************************************************************************
 * Boot locks
 ******************************************************************************/

void
acquire_boot_lock () {
  // cout << "Acquire lock\n";
  url lock_file= "$TEXMACS_HOME_PATH/system/boot_lock";
  if (exists (lock_file)) {
    remove (url ("$TEXMACS_HOME_PATH/system/settings.scm"));
    remove (get_tm_cache_path () * url_wildcard ("*"));
    remove (url ("$TEXMACS_HOME_PATH/fonts/error") * url_wildcard ("*"));
  }
  else save_string (lock_file, "", false);
}

void
release_boot_lock () {
  // cout << "Release lock\n";
  url lock_file= "$TEXMACS_HOME_PATH/system/boot_lock";
  remove (lock_file);
}

/******************************************************************************
 * Detection of scheme code
 ******************************************************************************/

void
init_scheme () {
  url guile_path= url_system ("$TEXMACS_PATH/progs");
  guile_path= guile_path | "$TEXMACS_HOME_PATH/progs" | plugin_path ("progs");
  set_env_path ("GUILE_LOAD_PATH", guile_path);
}

/******************************************************************************
 * Set additional environment variables
 ******************************************************************************/

void
init_env_vars () {
  // Handle binary, library and guile paths for plugins
  url bin_path= get_env_path ("PATH") | plugin_path ("bin");
#if defined(OS_MINGW) || defined(OS_WIN)
  bin_path= bin_path | url ("$TEXMACS_PATH/bin");
  if (has_user_preference ("manual path"))
    bin_path= url_system (get_user_preference ("manual path")) | bin_path;
#endif

  set_env_path ("PATH", bin_path);
  url lib_path= get_env_path ("LD_LIBRARY_PATH") | plugin_path ("lib");
  set_env_path ("LD_LIBRARY_PATH", lib_path);

  // Get TeXmacs style and package paths
  url style_root= get_env_path (
      "TEXMACS_STYLE_ROOT",
      url_unix ("$TEXMACS_HOME_PATH/styles:$TEXMACS_PATH/styles") |
          plugin_path ("styles"));
  url package_root= get_env_path (
      "TEXMACS_PACKAGE_ROOT",
      url_unix ("$TEXMACS_HOME_PATH/packages:$TEXMACS_PATH/packages") |
          plugin_path ("packages"));
  url all_root= style_root | package_root;
  url style_path=
      get_env_path ("TEXMACS_STYLE_PATH", search_sub_dirs (all_root));
  url text_root= get_env_path ("TEXMACS_TEXT_ROOT",
                               "$TEXMACS_HOME_PATH/texts:$TEXMACS_PATH/texts" |
                                   plugin_path ("texts"));
  url text_path=
      get_env_path ("TEXMACS_TEXT_PATH", search_sub_dirs (text_root));

  // Get other data paths
  (void) get_env_path ("TEXMACS_FILE_PATH", text_path | style_path);
  (void) set_env_path (
      "TEXMACS_DOC_PATH",
      get_env_path ("TEXMACS_DOC_PATH") |
          url_unix ("$TEXMACS_HOME_PATH/doc:$TEXMACS_PATH/doc") |
          plugin_path ("doc"));
  (void) set_env_path ("TEXMACS_SECURE_PATH",
                       get_env_path ("TEXMACS_SECURE_PATH") |
                           "$TEXMACS_PATH:$TEXMACS_HOME_PATH");
  (void) get_env_path (
      "TEXMACS_PATTERN_PATH",
      "$TEXMACS_HOME_PATH/misc/patterns" | url ("$TEXMACS_PATH/misc/patterns") |
          url ("$TEXMACS_PATH/misc/pictures") | plugin_path ("misc/patterns"));
  (void) set_env_path (
      "TEXMACS_PIXMAP_PATH",
      ((get_user_preference ("gui theme", "default") == "liii" ||
        get_user_preference ("gui theme", "default") == "default")
           ? url ("$TEXMACS_PATH/misc/pixmaps/liii/32x32/settings") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii/24x24/main") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii/20x20/mode") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii/16x16/"
                      "focus") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/32x32/settings") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/24x24/main") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/20x20/mode") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/16x16/focus")
       : (get_user_preference ("gui theme", "default") == "liii-night")
           ? url ("$TEXMACS_PATH/misc/pixmaps/liii-night/32x32/settings") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii-night/24x24/main") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii-night/20x20/mode") |
                 url ("$TEXMACS_PATH/misc/pixmaps/liii-night/16x16/"
                      "focus") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/32x32/settings") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/24x24/main") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/20x20/mode") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/16x16/focus")
           : url ("$TEXMACS_PATH/misc/pixmaps/modern/32x32/settings") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/24x24/main") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/20x20/mode") |
                 url ("$TEXMACS_PATH/misc/pixmaps/modern/16x16/focus")) |
          plugin_path ("misc/pixmaps"));
  (void) get_env_path ("TEXMACS_DIC_PATH",
                       "$TEXMACS_HOME_PATH/langs/natural/dic" |
                           url ("$TEXMACS_PATH/langs/natural/dic"));
  (void) get_env_path ("TEXMACS_THEME_PATH",
                       url ("$TEXMACS_PATH/misc/themes") |
                           url ("$TEXMACS_HOME_PATH/misc/themes") |
                           plugin_path ("misc/themes"));
#ifdef OS_WIN
  set_env ("TEXMACS_SOURCE_PATH", "");
#else
  set_env ("TEXMACS_SOURCE_PATH", TEXMACS_SOURCES);
#endif
}

/******************************************************************************
 * Miscellaneous initializations
 ******************************************************************************/

void
init_misc () {
  // Set extra environment variables for Cygwin
#ifdef OS_CYGWIN
  set_env ("CYGWIN", "check_case:strict");
  set_env ("COMSPEC", "");
  set_env ("ComSpec", "");
#endif
}

/******************************************************************************
 * First installation
 ******************************************************************************/

void
setup_texmacs () {
  url settings_file= "$TEXMACS_HOME_PATH/system/settings.scm";
  debug_boot << "Welcome to Mogan STEM " << XMACS_VERSION << "\n";
  debug_boot << HRULE;

  set_setting ("VERSION", XMACS_VERSION);
  set_setting ("DPI", "600");

  string s= scheme_tree_to_block (texmacs_settings);
  // cout << "settings_t= " << texmacs_settings << "\n";
  // cout << "settings_s= " << s << "\n";
  if (save_string (settings_file, s) || load_string (settings_file, s, false)) {
    failed_error << HRULE;
    failed_error << "I could not save or reload the file\n\n";
    failed_error << "\t" << settings_file << "\n\n";
    failed_error << "Please give me full access control over this file and\n";
    failed_error << "rerun 'TeXmacs'.\n";
    failed_error << HRULE;
    TM_FAILED ("unable to write settings");
  }

  debug_boot << HRULE;
  debug_boot << "Installation completed successfully !\n";
  debug_boot << "I will now start up the editor\n";
  debug_boot << HRULE;
}

/******************************************************************************
 * Initialization of TeXmacs
 ******************************************************************************/

// init_texmacs前置方法，抽出来是因为依赖这个前置方法
void
init_texmacs_front () {
  init_main_paths ();
  init_user_dirs ();
  init_scheme ();
  init_env_vars ();
  init_misc ();
}

void
init_texmacs () {
  if (g_startup_login_executed == true) {
    return;
  }

  // cout << "Initialize -- Boot lock\n";
  acquire_boot_lock ();
  // cout << "Initialize -- Succession status table\n";
  init_succession_status_table ();
  // cout << "Initialize -- Succession standard DRD\n";
  init_std_drd ();
  // cout << "Initialize -- User preferences\n";
  load_user_preferences ();

  // cout << "Initialize -- font_database_load\n";
  font_database_load ();
  // cout << "Initialize -- font_database_load end\n";
}

void
load_welcome_doc () {
  if (DEBUG_STD) debug_boot << "Loading welcome message...\n";
  string cmd= "(mogan-welcome)";
  exec_delayed (scheme_cmd (cmd));
}

/******************************************************************************
 * Load settings and check version
 ******************************************************************************/

int
load_settings_and_check_version () {
  url settings_path= "$TEXMACS_HOME_PATH/system/settings.scm";

  install_status= 0;
  if (exists (settings_path)) {
    string s        = string_load (settings_path);
    texmacs_settings= block_to_scheme_tree (s);
  }
  else {
    setup_texmacs ();
    install_status= 1;
  }

  if (get_setting ("VERSION") != XMACS_VERSION) {
    init_upgrade ();
    install_status= 2;
  }

  return install_status;
}

/******************************************************************************
 * Initialization of built-in plug-ins
 ******************************************************************************/

void
init_plugins () {
  setup_tex ();
  init_tex ();
}

void
TeXmacs_main (int argc, char** argv) {
  int    i;
  bool   flag= true;
  string the_default_font;
  for (i= 1; i < argc; i++)
    if (argv[i][0] == '\0') argc= i;
    else if (((argv[i][0] == '-') || (argv[i][0] == '+')) &&
             (argv[i][1] != '\0')) {
      string s= argv[i];
      if ((N (s) >= 2) && (s (0, 2) == "--")) s= s (1, N (s));
      if ((s == "-s") || (s == "-silent")) flag= false;
      else if ((s == "-V") || (s == "-verbose"))
        debug (DEBUG_FLAG_VERBOSE, true);
      else if ((s == "-d") || (s == "-debug")) debug (DEBUG_FLAG_STD, true);
      else if (s == "-debug-events") debug (DEBUG_FLAG_EVENTS, true);
      else if (s == "-debug-io") debug (DEBUG_FLAG_IO, true);
      else if (s == "-debug-bench") debug (DEBUG_FLAG_BENCH, true);
      else if (s == "-debug-history") debug (DEBUG_FLAG_HISTORY, true);
      else if (s == "-debug-qt") debug (DEBUG_FLAG_QT, true);
      else if (s == "-debug-qt-widgets") debug (DEBUG_FLAG_QT_WIDGETS, true);
      else if (s == "-debug-keyboard") debug (DEBUG_FLAG_KEYBOARD, true);
      else if (s == "-debug-packrat") debug (DEBUG_FLAG_PACKRAT, true);
      else if (s == "-debug-flatten") debug (DEBUG_FLAG_FLATTEN, true);
      else if (s == "-debug-parser") debug (DEBUG_FLAG_PARSER, true);
      else if (s == "-debug-correct") debug (DEBUG_FLAG_CORRECT, true);
      else if (s == "-debug-convert") debug (DEBUG_FLAG_CONVERT, true);
      else if (s == "-debug-remote") debug (DEBUG_FLAG_REMOTE, true);
      else if (s == "-debug-live") debug (DEBUG_FLAG_LIVE, true);
      else if (s == "-debug-all") {
        debug (DEBUG_FLAG_EVENTS, true);
        debug (DEBUG_FLAG_STD, true);
        debug (DEBUG_FLAG_IO, true);
        debug (DEBUG_FLAG_HISTORY, true);
        debug (DEBUG_FLAG_BENCH, true);
        debug (DEBUG_FLAG_QT, true);
        debug (DEBUG_FLAG_QT_WIDGETS, true);
      }
      else if (s == "-disable-error-recovery") disable_error_recovery= true;
      else if ((s == "-fn") || (s == "-font")) {
        i++;
        if (i < argc) the_default_font= argv[i];
      }
      else if ((s == "-g") || (s == "-geometry")) {
        i++;
        if (i < argc) {
          string g= argv[i];
          int    j= 0, j1, j2, j3;
          for (j= 0; j < N (g); j++)
            if (g[j] == 'x') break;
          j1= j;
          if (j < N (g)) j++;
          for (; j < N (g); j++)
            if ((g[j] == '+') || (g[j] == '-')) break;
          j2= j;
          if (j < N (g)) j++;
          for (; j < N (g); j++)
            if ((g[j] == '+') || (g[j] == '-')) break;
          j3= j;
          if (j1 < N (g)) {
            geometry_w= max (as_int (g (0, j1)), 320);
            geometry_h= max (as_int (g (j1 + 1, j2)), 200);
          }
          if (j3 < N (g)) {
            if (g[j2] == '-') geometry_x= as_int (g (j2, j3)) - 1;
            else geometry_x= as_int (g (j2 + 1, j3));
            if (g[j3] == '-') geometry_y= as_int (g (j3, N (g))) - 1;
            else geometry_y= as_int (g (j3 + 1, N (g)));
          }
        }
      }
      else if ((s == "-b") || (s == "-initialize-buffer")) {
        i++;
        if (i < argc) tm_init_buffer_file= url_system (argv[i]);
      }
      else if ((s == "-i") || (s == "-initialize")) {
        i++;
        if (i < argc) tm_init_file= url_system (argv[i]);
      }
      else if ((s == "-v") || (s == "-version")) {
        cout << "\n";
        cout << "TeXmacs version " << TEXMACS_VERSION << LF;
        cout << TEXMACS_COPYRIGHT << LF << LF;
        cout << "Mogan Research version " << XMACS_VERSION << LF;
        cout << "(c) 2022-2023 by Mogan Developers" << LF << LF;
        exit (0);
      }
      else if ((s == "-p") || (s == "-path")) {
        cout << get_env ("TEXMACS_PATH") << "\n";
        exit (0);
      }
      else if ((s == "-bp") || (s == "-binpath")) {
        cout << get_env ("TEXMACS_BIN_PATH") << "\n";
        exit (0);
      }
      else if ((s == "-q") || (s == "-quit"))
        my_init_cmds= my_init_cmds * " (quit-TeXmacs)";
      else if ((s == "-r") || (s == "-reverse")) set_reverse_colors (true);
      else if (s == "-no-retina") {
        retina_manual= true;
        retina_factor= 1;
        retina_zoom  = 1;
        retina_icons = 1;
        retina_scale = 1.0;
      }
      else if ((s == "-R") || (s == "-retina")) {
        retina_manual= true;
#ifdef MACOSX_EXTENSIONS
        retina_factor= 2;
        retina_zoom  = 1;
        retina_scale = 1.4;
#elif defined(OS_WASM)
        retina_factor= 2;
        retina_zoom  = 2;
        retina_scale = (tm_style_sheet == "" ? 1.0 : 1.6666);
#else
        retina_factor= 1;
        retina_zoom  = 2;
        retina_scale = (tm_style_sheet == "" ? 1.0 : 1.6666);
#endif
        retina_icons= 2;
      }
      else if (s == "-no-retina-icons") {
        retina_iman = true;
        retina_icons= 1;
      }
      else if (s == "-retina-icons") {
        retina_iman = true;
        retina_icons= 2;
      }
      else if ((s == "-c") || (s == "-convert")) {
        i+= 2;
        if (i < argc) {
          url    in ("$PWD", argv[i - 1]);
          url    out ("$PWD", argv[i]);
          string in_suffix = suffix (in);
          string out_suffix= suffix (out);

          // Special handling for .tex to .tmu conversion
          if (in_suffix == "tex" && out_suffix == "tmu") {
            my_init_cmds= my_init_cmds * " " *
                          "(let* ((latex-content (string-load " *
                          scm_quote (as_string (in)) * "))" *
                          "       (texmacs-tree (cpp-latex-document->texmacs "
                          "latex-content #f))" *
                          "       (tmu-content (serialize-tmu texmacs-tree)))" *
                          "  (string-save tmu-content " *
                          scm_quote (as_string (out)) * "))";
          }
          else {
            // Default conversion method for other formats
            my_init_cmds= my_init_cmds * " " * "(load-buffer " *
                          scm_quote (as_string (in)) * " :strict) " *
                          "(export-buffer " * scm_quote (as_string (out)) * ")";
          }
        }
      }
      else if ((s == "-x") || (s == "-execute")) {
        i++;
        if (i < argc) my_init_cmds= (my_init_cmds * " ") * argv[i];
      }
      else if (s == "-server") start_server_flag= true;
      else if (s == "-log-file") i++;
      else if ((s == "-Oc") || (s == "-no-char-clipping")) char_clip= false;
      else if ((s == "+Oc") || (s == "-char-clipping")) char_clip= true;
      else if ((s == "-S") || (s == "-setup") || (s == "-delete-cache") ||
               (s == "-delete-font-cache") || (s == "-delete-style-cache") ||
               (s == "-delete-file-cache") || (s == "-delete-doc-cache") ||
               (s == "-delete-plugin-cache") || (s == "-delete-server-data") ||
               (s == "-delete-databases") || (s == "-headless"))
        ;
      else if (s == "-build-manual") {
        if ((++i) < argc)
          extra_init_cmd << "(build-manual " << scm_quote (argv[i])
                         << " delayed-quit)";
      }
      else if (s == "-reference-suite") {
        if ((++i) < argc)
          extra_init_cmd << "(build-ref-suite " << scm_quote (argv[i])
                         << " delayed-quit)";
      }
      else if (s == "-test-suite") {
        if ((++i) < argc)
          extra_init_cmd << "(run-test-suite " << scm_quote (argv[i])
                         << "delayed-quit)";
      }
      else if (starts (s, "-psn"))
        ;
      else {
        cout << "\n";
        cout << "Options for TeXmacs:\n\n";
        cout << "  -b [file]  Specify scheme buffers initialization file\n";
        cout << "  -c [i] [o] Convert file 'i' into file 'o'\n";
        cout << "  -d         For debugging purposes\n";
        cout << "  -fn [font] Set the default TeX font\n";
        cout << "  -g [geom]  Set geometry of window in pixels\n";
        cout << "  -h         Display this help message\n";
        cout << "  -i [file]  Specify scheme initialization file\n";
        cout << "  -p         Get the TeXmacs path\n";
        cout << "  -q         Shortcut for -x \"(quit-TeXmacs)\"\n";
        cout << "  -r         Reverse video mode\n";
        cout << "  -s         Suppress information messages\n";
        cout << "  -S         Rerun TeXmacs setup program before starting\n";
        cout << "  -v         Display current TeXmacs version\n";
        cout << "  -V         Show some informative messages\n";
        cout << "  -x [cmd]   Execute scheme command\n";
        cout << "  -Oc        TeX characters bitmap clipping off\n";
        cout << "  +Oc        TeX characters bitmap clipping on (default)\n";
        cout << "\nPlease report bugs to <bugs@texmacs.org>\n";
        cout << "\n";
        exit (0);
      }
    }
  if (flag) debug (DEBUG_FLAG_AUTO, true);

  // Further options via environment variables
  if (get_env ("TEXMACS_RETINA") == "off") {
    retina_manual= true;
    retina_factor= 1;
    retina_icons = 1;
    retina_scale = 1.0;
  }
  if (get_env ("TEXMACS_RETINA") == "on") {
    retina_manual= true;
#ifdef MACOSX_EXTENSIONS
    retina_factor= 2;
    retina_zoom  = 1;
    retina_scale = 1.4;
#else
    retina_factor= 1;
    retina_zoom  = 2;
    retina_scale = (tm_style_sheet == "" ? 1.0 : 1.6666);
#endif
    retina_icons= 2;
  }
  if (get_env ("TEXMACS_RETINA_ICONS") == "off") {
    retina_iman = true;
    retina_icons= 1;
  }
  if (get_env ("TEXMACS_RETINA_ICONS") == "on") {
    retina_iman = true;
    retina_icons= 2;
  }
  // End options via environment variables

  // Further user preferences
  string native= (gui_version () == "qt4" ? string ("on") : string ("off"));
  string unify = (gui_version () == "qt4" ? string ("on") : string ("off"));
  string mini  = (os_macos () ? string ("off") : string ("on"));
  if (tm_style_sheet != "") mini= "off";
  use_native_menubar = get_preference ("use native menubar", native) == "on";
  use_unified_toolbar= get_preference ("use unified toolbar", unify) == "on";
  use_mini_bars      = get_preference ("use minibars", mini) == "on";
  if (!use_native_menubar) use_unified_toolbar= false;
  // End user preferences

  if (DEBUG_STD) debug_boot << "Installing internal plug-ins...\n";
  bench_start ("initialize plugins");
  bench_cumul ("initialize plugins");
  if (DEBUG_STD) debug_boot << "Opening display...\n";

  gui_open (argc, argv);
  set_default_font (the_default_font);
  if (DEBUG_STD) debug_boot << "Starting server...\n";
  { // opening scope for server sv
    server sv (app_type::RESEARCH);
    string where           = "";
    bool   has_initial_file= false;
    bool   first_file      = true;
    for (i= 1; i < argc; i++) {
      if (argv[i] == NULL) break;
      string s= argv[i];
      if ((N (s) >= 2) && (s (0, 2) == "--")) s= s (1, N (s));
      if ((s[0] != '-') && (s[0] != '+')) {
        has_initial_file= true;
        if (DEBUG_STD) debug_boot << "Loading " << s << "...\n";
        url u= url_system (s);
        if (!is_rooted (u)) u= resolve (url_pwd (), "") * u;
        string b= scm_quote (as_string (u));
        string cmd;
        // only open window once
        if (first_file) {
          buffer_load (u);
          new_buffer_in_new_window (u, tree (moebius::DOCUMENT));
          first_file= false;
        }
        else {
          cmd= "(switch-to-buffer " * b * ")";
          exec_delayed (scheme_cmd (cmd));
        }
      }
      if ((s == "-c") || (s == "-convert")) i+= 2;
      else if ((s == "-b") || (s == "-initialize-buffer") || (s == "-fn") ||
               (s == "-font") || (s == "-i") || (s == "-initialize") ||
               (s == "-g") || (s == "-geometry") || (s == "-x") ||
               (s == "-execute") || (s == "-log-file") ||
               (s == "-build-manual") || (s == "-reference-suite") ||
               (s == "-test-suite")) {
        i++;
      }
    }

    bool has_initial_welcome= false;
    if (install_status == 1 || install_status == 2) {
      has_initial_welcome= true;
      load_welcome_doc ();
    }

    if (!has_initial_file || !has_initial_welcome) ensure_window ();

    if (DEBUG_BENCH) lolly::system::bench_print (std_bench);
    bench_reset ("initialize texmacs");
    bench_reset ("initialize plugins");
    bench_reset ("initialize scheme");

    if (DEBUG_STD) debug_boot << "Starting event loop...\n";
    texmacs_started= true;
    if (!disable_error_recovery) {
      // 注册信号处理器，确保子进程被正确清理
      // 包括崩溃类信号和用户中断信号
      signal (SIGSEGV, clean_exit_on_signal); // 段错误
#ifndef OS_WIN
      signal (SIGTRAP, clean_exit_on_signal); // 断点陷阱/跟踪
#endif
      signal (SIGABRT, clean_exit_on_signal); // abort
      signal (SIGTERM, clean_exit_on_signal); // 终止信号 (kill 命令)
      signal (SIGINT, clean_exit_on_signal);  // 中断 (Ctrl+C)
#ifdef SIGQUIT
      signal (SIGQUIT, clean_exit_on_signal); // 退出 (Ctrl+\)
#endif
    }
    if (start_server_flag) server_start ();
    release_boot_lock ();

#ifdef QTTEXMACS
    // Trigger login if requested from startup dialog
    // Use timer to execute after event loop starts
    if (g_startup_login_requested) {
      QTimer::singleShot (0, [] () {
        if (is_server_started ()) {
          tm_server_rep* server=
              dynamic_cast<tm_server_rep*> (get_server ().operator->());
          if (server && server->getAccount ()) {
            server->getAccount ()->login ();
          }
        }
        g_startup_login_requested= false;
      });
    }
#endif

    if (N (extra_init_cmd) > 0) exec_delayed (scheme_cmd (extra_init_cmd));
    gui_start_loop ();

    if (DEBUG_STD) debug_boot << "Stopping server...\n";
  } // ending scope for server sv

  if (DEBUG_STD) debug_boot << "Closing display...\n";
  gui_close ();

  if (DEBUG_STD) debug_boot << "Good bye...\n";
}

#ifdef QTTEXMACS
#include <QEventLoop>

bool
show_startup_login_dialog () {
  // Don't show dialog in headless mode
  if (headless_mode) {
    return true;
  }

  if (install_status != 1 && install_status != 2) {
    // Normal startup, no need to show login dialog
    return true;
  }

  // is_community= true,return true;
  if (is_community_stem ()) {
    return true;
  }

  // Create non-modal dialog
  QWK::StartupLoginDialog* dialog= new QWK::StartupLoginDialog ();
  dialog->setModal (false);
  dialog->setAttribute (Qt::WA_DeleteOnClose);

  bool userDecisionMade= false;

  // Connect dialog signals
  QObject::connect (dialog, &QWK::StartupLoginDialog::loginRequested, [&] () {
    g_startup_login_requested= true;
    userDecisionMade         = true;
  });

  QObject::connect (dialog, &QWK::StartupLoginDialog::skipRequested,
                    [&] () { userDecisionMade= true; });

  // Show the dialog (non-blocking)
  dialog->show ();

  // Start background initialization if dialog supports it
  dialog->startInitialization ();

  // Enter local event loop to wait for user decision
  dialog->exec ();

  return userDecisionMade;
}
#endif
