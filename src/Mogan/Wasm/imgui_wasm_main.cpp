/******************************************************************************
 * MODULE     : imgui_wasm_main.cpp
 * DESCRIPTION: WASM entrypoint for the Qt-free ImGui viewer
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "imgui_viewer.hpp"
#include "wasm_tmu_render.hpp"
#include "tm_configure.hpp"
#include "url.hpp"
#include <fcntl.h>
#ifndef OS_WIN
#include <unistd.h>
#endif
#include "locale.hpp"
#include <locale.h> // for setlocale
#include <lolly/system/args.hpp>
#include <lolly/system/timer.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#ifdef STACK_SIZE
#include <sys/resource.h>
#endif

#include "../app_type.hpp"
#include "boot.hpp"
#include "data_cache.hpp"
#include "file.hpp"
#include "observers.hpp"
#include "preferences.hpp"
//#include "server.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tm_ostream.hpp"
#include "tm_timer.hpp"
#include "tm_url.hpp"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <SDL.h>

#if defined(IMGUI_IMPL_OPENGL_ES3)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "../../../imgui/examples/libs/emscripten/emscripten_mainloop_stub.h"
#endif

struct tmu_render_state {
  box   page;
  bool  needs_render;
};

static void
render_tmu_page (mupdf_viewer_buffer_rep& buffer, double zoom, double /*pan_x*/,
                 double /*pan_y*/, void* user_data) {
  tmu_render_state* state= (tmu_render_state*) user_data;
  if (state == NULL || state->page == box ()) return;
  wasm_render_page_to_buffer (state->page, zoom, buffer);
}

int
main (int, char**) {
  cout << "mainentry" << LF;
  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    return 1;

  const char* glsl_version= "#version 300 es";
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK,
                       SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

    bool done= false;
  SDL_WindowFlags flags= (SDL_WindowFlags) (
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window= SDL_CreateWindow (
      "Mogan ImGui Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      1280, 800, flags);
  if (window == NULL) return 1;

  SDL_GLContext gl_context= SDL_GL_CreateContext (window);
  if (gl_context == NULL) return 1;
  SDL_GL_MakeCurrent (window, gl_context);
  SDL_GL_SetSwapInterval (1);

  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  ImGuiIO& io= ImGui::GetIO ();
  io.IniFilename= NULL;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark ();

  ImGui_ImplSDL2_InitForOpenGL (window, gl_context);
  ImGui_ImplOpenGL3_Init (glsl_version);

  cout << "00000000000" << LF;

  // 1.系统初始化
  lolly::init_tbox ();                // 初始化tbox库
  //boot_hacks ();
  //windows_delayed_refresh (1000000000);
  wasm_tmu_render_init ();
  bool enale_logging= true;
  url u= url_system (string ("$TEXMACS_HOME_PATH/system/") *
                     lolly::locale::get_date ("english", "%Y%m%d%H") *
                     string (".log"));
  if (enale_logging) {
    //cout << "Logging into >> " << u << LF;
    //tm_ostream logf (c_string (concretize (u)));
    //if (!logf->is_writable ()) {
    //  cerr << "TeXmacs] Error: could not open " << u << LF;
   // }
    ///else {
    //  cout.redirect (logf);
    //  cerr.redirect (logf);
    //}
  }
  cout << "11111111111" << LF;
  cout << url_system ("$TEXMACS_PATH/doc/about/mogan/stem.en.tmu") << LF;
  tree doc  = wasm_load_tmu (url_system ("$TEXMACS_PATH/doc/about/mogan/stem.en.tmu"));
  cout << "22222222222" << LF;
  box  page = wasm_typeset_document (doc);

  tmu_render_state state;
  state.page        = page;
  state.needs_render= true;

  imgui_viewer_render_target target;
  imgui_viewer_init (target);

#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!done)
#endif
  {
    SDL_Event event;
    while (SDL_PollEvent (&event)) {
      ImGui_ImplSDL2_ProcessEvent (&event);
      if (event.type == SDL_QUIT) done= true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID (window))
        done= true;
    }

    #ifdef __EMSCRIPTEN__
      if (done) emscripten_cancel_main_loop ();
    #else
      if (done) break;
    #endif

    ImGui_ImplOpenGL3_NewFrame ();
    ImGui_ImplSDL2_NewFrame ();
    ImGui::NewFrame ();

    imgui_viewer_render_frame (target, render_tmu_page, &state);

    ImGui::Render ();
    int display_w= 0;
    int display_h= 0;
    SDL_GL_GetDrawableSize (window, &display_w, &display_h);
    glViewport (0, 0, display_w, display_h);
    glClearColor (0.08f, 0.09f, 0.10f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window);
  }

#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  ImGui_ImplOpenGL3_Shutdown ();
  ImGui_ImplSDL2_Shutdown ();
  ImGui::DestroyContext ();
  SDL_GL_DeleteContext (gl_context);
  SDL_DestroyWindow (window);
  SDL_Quit ();
  return 0;
}