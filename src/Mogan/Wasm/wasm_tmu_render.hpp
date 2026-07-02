
/******************************************************************************
 * MODULE     : wasm_tmu_render.hpp
 * DESCRIPTION: Headless .tmu load / typeset / render API for the WASM ImGui viewer
 * COPYRIGHT  : (C) 2026 Mogan project
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef WASM_TMU_RENDER_HPP
#define WASM_TMU_RENDER_HPP

#include "boxes.hpp"
#include "mupdf_viewer_buffer.hpp"
#include "tree.hpp"
#include "url.hpp"

void wasm_tmu_render_init ();
tree wasm_load_tmu (url u);
box  wasm_typeset_document (tree doc);
void wasm_render_page_to_buffer (box doc_box, double zoom,
                                 mupdf_viewer_buffer_rep& buffer);

#endif // defined WASM_TMU_RENDER_HPP
