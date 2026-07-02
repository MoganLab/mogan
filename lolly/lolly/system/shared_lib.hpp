
/** \file shared_lib.hpp
 *  \copyright GPLv3
 *  \details dynamic (shared) library related routines
 *  \author jingkaimori
 *  \date   2024
 */

#pragma once

#include "resource.hpp"
#include "url.hpp"
#include <stdint.h>
#include <tbox/tbox.h>

#ifndef OS_WASM
namespace lolly {
namespace system {

RESOURCE (shared_lib);

struct shared_lib_rep : rep<shared_lib> {
private:
  tb_dynamic_ref_t dynamic_ref;

public:
  shared_lib_rep (string dynamic_name, url path);
  virtual ~shared_lib_rep ();
  template <typename Ret, typename... Args>
  auto get_function (string function_name) -> Ret (*) (Args...) {
    return (Ret (*) (Args...)) tb_dynamic_func (dynamic_ref,
                                                c_string (function_name));
  }
  friend struct shared_lib;
};

shared_lib load_shared_library (string name, url path);

} // namespace system
} // namespace lolly
#endif
