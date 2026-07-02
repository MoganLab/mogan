/******************************************************************************
 * MODULE     : imgui_viewer.cpp
 * DESCRIPTION: WASM-only ImGui host for RGBA document buffers
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/


#include "imgui_viewer.hpp"

#include <imgui.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#endif

static void
imgui_viewer_sync_texture (imgui_viewer_render_target& target) {
#if defined(__EMSCRIPTEN__)
  if (target.texture_id == 0) glGenTextures (1, &target.texture_id);
  glBindTexture (GL_TEXTURE_2D, target.texture_id);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, target.buffer.width (),
                target.buffer.height (), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                target.buffer.data ());
  target.texture_dirty= false;
#else
  target.texture_dirty= false;
#endif
}

static void
imgui_viewer_apply_zoom (imgui_viewer_render_target& target, float wheel_delta,
                         ImVec2 canvas_min, ImVec2 mouse_pos) {
  if (wheel_delta == 0.0f) return;
  double old_zoom= target.zoom;
  double new_zoom= old_zoom * (wheel_delta > 0.0f ? 1.1 : 0.9);
  if (new_zoom < 0.1) new_zoom= 0.1;
  if (new_zoom > 8.0) new_zoom= 8.0;

  double canvas_x= mouse_pos.x - canvas_min.x;
  double canvas_y= mouse_pos.y - canvas_min.y;
  target.pan_x= (target.pan_x + canvas_x) * (new_zoom / old_zoom) - canvas_x;
  target.pan_y= (target.pan_y + canvas_y) * (new_zoom / old_zoom) - canvas_y;
  target.zoom  = new_zoom;
}

void
imgui_viewer_init (imgui_viewer_render_target& target) {
  target.texture_id  = 0;
  target.texture_dirty= false;
  target.zoom        = 1.0;
  target.pan_x       = 0.0;
  target.pan_y       = 0.0;
  target.dragging    = false;
  target.drag_origin_x = 0.0;
  target.drag_origin_y = 0.0;
  target.drag_pan_x   = 0.0;
  target.drag_pan_y   = 0.0;
}

void
imgui_viewer_render_frame (imgui_viewer_render_target& target,
                           imgui_viewer_render_fn render_fn,
                           void* user_data) {
  if (render_fn != NULL) {
    render_fn (target.buffer, target.zoom, target.pan_x, target.pan_y,
               user_data);
    target.texture_dirty= true;
  }

  if (target.texture_dirty && target.buffer.data () != NULL &&
      target.buffer.width () > 0 && target.buffer.height () > 0)
    imgui_viewer_sync_texture (target);

  ImGui::Begin ("Document Viewer");
  ImGui::TextUnformatted ("Mouse wheel to zoom, drag to pan");
  ImGui::Separator ();

  ImVec2 available= ImGui::GetContentRegionAvail ();
  if (available.x < 1.0f) available.x= 1.0f;
  if (available.y < 1.0f) available.y= 1.0f;

  ImVec2 canvas_min= ImGui::GetCursorScreenPos ();
  ImVec2 canvas_max= ImVec2 (canvas_min.x + available.x,
                             canvas_min.y + available.y);
  ImDrawList* draw_list= ImGui::GetWindowDrawList ();
  draw_list->AddRectFilled (canvas_min, canvas_max, IM_COL32 (18, 18, 20, 255));
  draw_list->AddRect (canvas_min, canvas_max, IM_COL32 (90, 90, 96, 255));

  ImGui::InvisibleButton ("viewer_canvas", available,
                          ImGuiButtonFlags_MouseButtonLeft);
  bool   hovered= ImGui::IsItemHovered ();
  bool   active = ImGui::IsItemActive ();
  ImGuiIO& io   = ImGui::GetIO ();

  if (hovered && io.MouseWheel != 0.0f)
    imgui_viewer_apply_zoom (target, io.MouseWheel, canvas_min, io.MousePos);

  if (active && ImGui::IsMouseClicked (ImGuiMouseButton_Left)) {
    target.dragging      = true;
    target.drag_origin_x = io.MousePos.x;
    target.drag_origin_y = io.MousePos.y;
    target.drag_pan_x    = target.pan_x;
    target.drag_pan_y    = target.pan_y;
  }
  if (target.dragging && ImGui::IsMouseDown (ImGuiMouseButton_Left)) {
    target.pan_x= target.drag_pan_x + (io.MousePos.x - target.drag_origin_x);
    target.pan_y= target.drag_pan_y + (io.MousePos.y - target.drag_origin_y);
  }
  if (target.dragging && ImGui::IsMouseReleased (ImGuiMouseButton_Left))
    target.dragging= false;

  if (target.texture_id != 0 && target.buffer.data () != NULL) {
    ImVec2 image_min= ImVec2 (canvas_min.x + (float) target.pan_x,
                              canvas_min.y + (float) target.pan_y);
    ImVec2 image_max= ImVec2 (
        image_min.x + (float) target.buffer.width () * (float) target.zoom,
        image_min.y + (float) target.buffer.height () * (float) target.zoom);
    draw_list->AddImage ((ImTextureID) (uintptr_t) target.texture_id, image_min,
                         image_max);
  }

  ImGui::End ();
}
