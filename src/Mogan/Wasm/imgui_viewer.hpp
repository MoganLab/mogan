/******************************************************************************
 * MODULE     : imgui_viewer.hpp
 * DESCRIPTION: WASM-only ImGui host for RGBA document buffers
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef MOGAN_WASM_IMGUI_VIEWER_HPP
#define MOGAN_WASM_IMGUI_VIEWER_HPP

#include "mupdf_viewer_buffer.hpp"

struct imgui_viewer_render_target {
  mupdf_viewer_buffer_rep buffer;
  unsigned int            texture_id;
  bool                    texture_dirty;
  double                  zoom;
  double                  pan_x;
  double                  pan_y;
  bool                    dragging;
  double                  drag_origin_x;
  double                  drag_origin_y;
  double                  drag_pan_x;
  double                  drag_pan_y;
};

typedef void (*imgui_viewer_render_fn) (mupdf_viewer_buffer_rep& buffer,
                                        double zoom, double pan_x,
                                        double pan_y, void* user_data);

void imgui_viewer_init (imgui_viewer_render_target& target);
void imgui_viewer_render_frame (imgui_viewer_render_target& target,
                                imgui_viewer_render_fn render_fn,
                                void* user_data);

#endif // defined MOGAN_WASM_IMGUI_VIEWER_HPP