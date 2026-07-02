//
// Copyright (C) 2024 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "goldfish.hpp"
#include <clocale>
#include <string>

int
main (int argc, char** argv) {
#ifdef TB_CONFIG_OS_WINDOWS
  SetConsoleOutputCP (65001);
#endif
  setlocale (LC_ALL, "C.UTF-8");
  std::string gf_lib_dir= goldfish::find_goldfish_library ();
  const char* gf_lib    = gf_lib_dir.c_str ();
  s7_scheme*  sc        = goldfish::init_goldfish_scheme (gf_lib);
  int         ret       = goldfish::repl_for_community_edition (sc, argc, argv);
  s7_pointer  exit_hook = s7_name_to_value (sc, "*exit-hook*");
  s7_pointer  funcs     = s7_hook_functions (sc, exit_hook);
  if (s7_is_pair (funcs)) {
    s7_pointer args= s7_cons (sc, s7_make_integer (sc, ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE), s7_nil (sc));
    s7_apply_function (sc, exit_hook, args);
  }
  return ret;
}
