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

// GOLDFISH_ENABLE_REPL宏由xmake定义

#include "goldfish.hpp"
#include <mutex>
#include <sstream>
#include <string>

static std::string last_output;
static std::string last_error;
static std::mutex  repl_mutex;
static s7_scheme*  wasm_sc= nullptr;

// 捕获输出的辅助类
class OutputCatcher : public std::streambuf {
public:
  std::ostringstream oss;
  std::streambuf*    old;
  std::ostream&      stream;
  OutputCatcher (std::ostream& s) : stream (s) { old= stream.rdbuf (this); }
  ~OutputCatcher () { stream.rdbuf (old); }
  int overflow (int c) override {
    if (c != EOF) oss.put ((char) c);
    return c;
  }
  std::string str () const { return oss.str (); }
};

extern "C" {
// 供 WASM/JS 调用的接口
int
eval_string (const char* code) {
  std::lock_guard<std::mutex> lock (repl_mutex);
  last_output.clear ();
  last_error.clear ();
  if (!wasm_sc) {
    std::string gf_lib_dir= goldfish::find_goldfish_library ();
    const char* gf_lib    = gf_lib_dir.c_str ();
    wasm_sc               = goldfish::init_goldfish_scheme (gf_lib);
    // 自动加载所有 goldfish/scheme/*.scm
    s7_eval_c_string (wasm_sc, "(load \"scheme/base.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/boot.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/case-lambda.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/char.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/file.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/inexact.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/process-context.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"scheme/time.scm\")");
    // 自动加载所有 goldfish/liii/*.scm
    s7_eval_c_string (wasm_sc, "(load \"liii/alist.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/argparse.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/base.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/base64.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/bitwise.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/case.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/check.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/chez.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/comparator.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/cut.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/either.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/error.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/hash-table.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/list.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/os.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/path.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/string.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/sys.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/uuid.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"liii/vector.scm\")");
    // 自动加载所有 goldfish/srfi/*.scm
    s7_eval_c_string (wasm_sc, "(load \"srfi/sicp.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-1.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-113.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-125.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-128.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-13.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-132.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-133.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-151.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-16.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-2.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-216.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-26.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-39.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-78.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-8.scm\")");
    s7_eval_c_string (wasm_sc, "(load \"srfi/srfi-9.scm\")");
  }
  s7_eval_c_string (wasm_sc, "(import (liii base))");
  s7_eval_c_string (wasm_sc, "(import (liii error))");
  OutputCatcher out_catcher (std::cout);
  OutputCatcher err_catcher (std::cerr);
  int           status= 0;
  try {
    s7_pointer result= s7_eval_c_string (wasm_sc, code);
    if (result) {
      char* result_str= s7_object_to_c_string (wasm_sc, result);
      if (result_str) {
        last_output+= result_str;
        free (result_str);
      }
    }
  } catch (const std::exception& e) {
    last_error= e.what ();
    status    = 1;
  }
  // 捕获 C++ 输出
  last_output+= out_catcher.str ();
  last_error+= err_catcher.str ();
  return status;
}

const char*
get_out () {
  return last_output.c_str ();
}
const char*
get_err () {
  return last_error.c_str ();
}
}
