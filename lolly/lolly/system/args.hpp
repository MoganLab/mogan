
/******************************************************************************
 * MODULE     : args.hpp
 * DESCRIPTION: lolly::system::args like nowide::args
 * COPYRIGHT  : (C) 2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "string.hpp"
#include <vector>

namespace lolly {
namespace system {
#if defined(OS_MINGW) || defined(_WIN32) || defined(_WIN64)
class args {
public:
  // Fix command line agruments
  args (int& argc, char**& argv)
      : old_argc_ (argc), old_argv_ (argv), old_argc_ptr_ (&argc),
        old_argv_ptr_ (&argv) {
    fix_args (argc, argv);
  }

  // Restore original argc, argv if changed
  ~args () {
    if (old_argc_ptr_) *old_argc_ptr_= old_argc_;
    if (old_argv_ptr_) *old_argv_ptr_= old_argv_;
  }

private:
  void fix_args (int& argc, char**& argv);

  std::vector<char*>  args_;
  std::vector<string> arg_values_;

  int    old_argc_;
  char** old_argv_;

  int*    old_argc_ptr_;
  char*** old_argv_ptr_;
};

#else
class args {
public:
  args (int&, char**&) {}
  ~args () {}
};
#endif
} // namespace system
} // namespace lolly
