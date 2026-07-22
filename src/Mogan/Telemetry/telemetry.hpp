
/******************************************************************************
 * MODULE     : telemetry.hpp
 * DESCRIPTION: C++ glue for Scheme telemetry tracking
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include "scheme.hpp"
#include "string.hpp"

#if !IS_COMMUNITY

// telemetry 相关 scheme 代码已迁移至 (plugin telemetry-*) 模块，由
// telemetry 插件在事件循环启动 ~3s 后懒加载。C++ 调用由 init-research.scm
// 注入 rootlet 的 telemetry-track-or-enqueue 兜底：插件加载前事件入
// *telemetry-pending* 队列，加载后 (init-telemetry) 通过
// telemetry-drain-pending! 一次性补 track。所有 C++ 上报（包括启动期
// OPEN）都不丢失。
inline string
scm_string_literal (const string& s) {
  return "\"" * s * "\"";
}

inline void
telemetry_track (string event_type) {
  try {
    eval_scheme ("(telemetry-track-or-enqueue " *
                 scm_string_literal (event_type) * " '())");
  } catch (...) {
    // telemetry failure should never crash the application
  }
}

inline void
telemetry_track (string event_type, string props) {
  try {
    eval_scheme ("(telemetry-track-or-enqueue " *
                 scm_string_literal (event_type) * " " * props * ")");
  } catch (...) {
    // telemetry failure should never crash the application
  }
}

#else

inline void
telemetry_track (string event_type) {
  (void) event_type;
}

inline void
telemetry_track (string event_type, string props) {
  (void) event_type;
  (void) props;
}

#endif // !IS_COMMUNITY

#endif // TELEMETRY_HPP
