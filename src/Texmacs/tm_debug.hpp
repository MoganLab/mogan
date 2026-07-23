#ifndef TM_DEBUG_H
#define TM_DEBUG_H

#include "lolly/system/timer.hpp"
#include "string.hpp"
#include "tree.hpp"

/**
 * @brief Enumeration of debugging flags.
 */
enum {
  DEBUG_FLAG_AUTO,
  DEBUG_FLAG_VERBOSE,
  DEBUG_FLAG_EVENTS,
  DEBUG_FLAG_STD,
  DEBUG_FLAG_IO,
  DEBUG_FLAG_BENCH,
  DEBUG_FLAG_HISTORY,
  DEBUG_FLAG_QT,
  DEBUG_FLAG_QT_WIDGETS,
  DEBUG_FLAG_KEYBOARD,
  DEBUG_FLAG_PACKRAT,
  DEBUG_FLAG_FLATTEN,
  DEBUG_FLAG_PARSER,
  DEBUG_FLAG_CORRECT,
  DEBUG_FLAG_CONVERT,
  DEBUG_FLAG_REMOTE,
  DEBUG_FLAG_LIVE,
  DEBUG_FLAG_LORO
};

/**
 * @brief Function used to enable or disable debugging output for a specific
 * debugging flag.
 *
 * @param which The debugging flag to enable or disable.
 * @param write_flag Boolean flag indicating whether to write the flag to the
 * debug output.
 *
 * @return True if debugging is enabled for the specified flag, false otherwise.
 */
bool debug (int which, bool write_flag= false);

/**
 * @brief Function used to disable all debugging output.
 *
 * @return The previous state of the debugging output.
 */
int debug_off ();

/**
 * @brief Function used to enable debugging output for all debugging flags.
 *
 * @param status The new state of the debugging output.
 */
void debug_on (int status);
class string;

/**
 * @brief Function used to set the debugging flag for a given string.
 *
 * @param s The string representing the debugging flag.
 * @param flag The new value of the debugging flag.
 */
void debug_set (string s, bool flag);

/**
 * @brief Function used to get the debugging flag for a given string.
 *
 * @param s The string representing the debugging flag.
 *
 * @return The value of the debugging flag.
 */
bool debug_get (string s);

/**
 * @brief Macro used to enable or disable debugging output for the "auto"
 * debugging flag.
 */
#define DEBUG_AUTO (debug (DEBUG_FLAG_AUTO))

/**
 * @brief Macro used to enable or disable debugging output for the "verbose"
 * debugging flag.
 */
#define DEBUG_VERBOSE (debug (DEBUG_FLAG_VERBOSE))

/**
 * @brief Macro used to enable or disable debugging output for the "events"
 * debugging flag.
 */
#define DEBUG_EVENTS (debug (DEBUG_FLAG_EVENTS))

/**
 * @brief Macro used to enable or disable debugging output for the "std"
 * debugging flag.
 */
#define DEBUG_STD (debug (DEBUG_FLAG_STD))

/**
 * @brief Macro used to enable or disable debugging output for the "io"
 * debugging flag.
 */
#define DEBUG_IO (debug (DEBUG_FLAG_IO))

/**
 * @brief Macro used to enable or disable debugging output for the "bench"
 * debugging flag.
 */
#define DEBUG_BENCH (debug (DEBUG_FLAG_BENCH))

/**
 * @brief Macro used to enable or disable debugging output for the "history"
 * debugging flag.
 */
#define DEBUG_HISTORY (debug (DEBUG_FLAG_HISTORY))

/**
 * @brief Macro used to enable or disable debugging output for the "qt"
 * debugging flag.
 */
#define DEBUG_QT (debug (DEBUG_FLAG_QT))

/**
 * @brief Macro used to enable or disable debugging output for the "qt_widgets"
 * debugging flag.
 */
#define DEBUG_QT_WIDGETS (debug (DEBUG_FLAG_QT_WIDGETS))

/**
 * @brief Macro usedto enable or disable debugging output for the "keyboard"
 * debugging flag.
 */
#define DEBUG_KEYBOARD (debug (DEBUG_FLAG_KEYBOARD))

/**
 * @brief Macro used to enable or disable debugging output for the "packrat"
 * debugging flag.
 */
#define DEBUG_PACKRAT (debug (DEBUG_FLAG_PACKRAT))

/**
 * @brief Macro used to enable or disable debugging output for the "flatten"
 * debugging flag.
 */
#define DEBUG_FLATTEN (debug (DEBUG_FLAG_FLATTEN))

/**
 * @brief Macro used to enable or disable debugging output for the "parser"
 * debugging flag.
 */
#define DEBUG_PARSER (debug (DEBUG_FLAG_PARSER))

/**
 * @brief Macro used to enable or disable debugging output for the "correct"
 * debugging flag.
 */
#define DEBUG_CORRECT (debug (DEBUG_FLAG_CORRECT))

/**
 * @brief Macro used to enable or disable debugging output for the "convert"
 * debugging flag.
 */
#define DEBUG_CONVERT (debug (DEBUG_FLAG_CONVERT))

/**
 * @brief Macro used to enable or disable debugging output for the "remote"
 * debugging flag.
 */
#define DEBUG_REMOTE (debug (DEBUG_FLAG_REMOTE))

/**
 * @brief Macro used to enable or disable debugging output for the "live"
 * debugging flag.
 */
#define DEBUG_LIVE (debug (DEBUG_FLAG_LIVE))

/**
 * @brief Macro used to enable or disable debugging output for the "loro"
 * debugging flag.
 */
#define DEBUG_LORO (debug (DEBUG_FLAG_LORO))

#define DEBUG_AQUA (debug (DEBUG_FLAG_QT))
#define DEBUG_AQUA_WIDGETS (debug (DEBUG_FLAG_QT_WIDGETS))

void debug_message (string channel, string msg);
void debug_formatted (string channel, tree msg);
tree get_debug_messages (string kind, int max_number);
void clear_debug_messages ();
void clear_debug_messages (string channel);

extern tm_ostream debug_std;
extern tm_ostream debug_qt;
extern tm_ostream debug_aqua;
extern tm_ostream debug_widgets;
extern tm_ostream debug_fonts;
extern tm_ostream debug_convert;
extern tm_ostream debug_typeset;
extern tm_ostream debug_edit;
extern tm_ostream debug_packrat;
extern tm_ostream debug_history;
extern tm_ostream debug_keyboard;
extern tm_ostream debug_automatic;
extern tm_ostream debug_boot;
extern tm_ostream debug_events;
extern tm_ostream debug_shell;
extern tm_ostream debug_io;
extern tm_ostream debug_spell;
extern tm_ostream debug_updater;
extern tm_ostream debug_loro;

extern tm_ostream std_error;
extern tm_ostream failed_error;
extern tm_ostream boot_error;
extern tm_ostream widkit_error;
extern tm_ostream qt_error;
extern tm_ostream font_error;
extern tm_ostream convert_error;
extern tm_ostream io_error;
extern tm_ostream bibtex_error;

extern tm_ostream std_warning;
extern tm_ostream convert_warning;
extern tm_ostream typeset_warning;
extern tm_ostream io_warning;
extern tm_ostream widkit_warning;
extern tm_ostream bibtex_warning;
extern tm_ostream std_bench;

inline void
bench_start (string task) {
  lolly::system::timer_start (task);
}
inline void
bench_cumul (string task) {
  lolly::system::timer_cumul (task);
}
inline void
bench_reset (string task) {
  lolly::system::timer_reset (task);
}
inline void
bench_end (string task, uint32_t threshold= 0, tm_ostream& ostream= std_bench) {
  lolly::system::timer_cumul (task);
  lolly::system::bench_print (ostream, task, threshold);
  lolly::system::timer_reset (task);
}

#endif // defined TM_DEBUG_H
