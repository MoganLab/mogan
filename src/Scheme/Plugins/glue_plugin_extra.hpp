/******************************************************************************
 * MODULE     : glue_plugin_extra.hpp
 * DESCRIPTION: helper functions used by glue_plugin (generated, standalone).
 *              Extracted so glue_plugin.cpp can be compiled as an
 *              independent translation unit.
 ******************************************************************************/

#ifndef GLUE_PLUGIN_EXTRA_HPP
#define GLUE_PLUGIN_EXTRA_HPP

#include "string.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdlib> // free

inline bool
use_plugin_updater () {
#if defined(USE_PLUGIN_VELOPACK)
  return true;
#else
  return false;
#endif
}

inline bool
use_plugin_tex () {
#ifdef USE_PLUGIN_TEX
  return true;
#else
  return false;
#endif
}

inline bool
use_plugin_bibtex () {
#ifdef USE_PLUGIN_BIBTEX
  return true;
#else
  return false;
#endif
}

inline bool
loro_enabled () {
#ifdef LORO_ENABLED
  return true;
#else
  return false;
#endif
}

static bool s_wasm_prompt_cancelled= false;

// WASM 交互输入：浏览器原生 window.prompt（同步阻塞）。ImGui/WASM 端未实现
// interactive 弹窗与 footer minibuffer，简单文本输入（协作服务器地址、文档
// 显示名）经此桥接。取消（JS 返回 null）时置 s_wasm_prompt_cancelled 并返回
// 空串，scheme 侧据此区分「取消」（no-op）与「确认空串」（如清除服务器偏好）。
// 非 WASM 恒为取消（返回空串 + cancelled=true，调用方按 no-op 处理）。
inline string
wasm_prompt (string title, string def) {
  s_wasm_prompt_cancelled= false;
#ifdef __EMSCRIPTEN__
  c_string t (title), d (def);
  char*    raw= (char*) EM_ASM_PTR (
      {
        var title = UTF8ToString ($0);
        var def   = UTF8ToString ($1);
        var result= window.prompt (title, def);
        if (result === null) return 0;
        var len= lengthBytesUTF8 (result) + 1;
        var buf= _malloc (len);
        stringToUTF8 (result, buf, len);
        return buf;
      },
      (char*) t, (char*) d);
  if (raw == nullptr) {
    s_wasm_prompt_cancelled= true;
    return "";
  }
  string r (raw);
  free (raw);
  return r;
#else
  (void) title;
  (void) def;
  s_wasm_prompt_cancelled= true;
  return "";
#endif
}

inline bool
wasm_prompt_cancelled () {
  return s_wasm_prompt_cancelled;
}

#endif
