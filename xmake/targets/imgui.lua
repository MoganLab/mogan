-------------------------------------------------------------------------------
--
-- MODULE      : imgui.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

target("imgui") do
    set_kind("static")
    set_basename("imgui")
    set_languages("c++17")
    set_encodings("utf-8")

    add_includedirs({"$(projectdir)/3rdparty/imgui", "$(projectdir)/3rdparty/imgui/backends"}, {public = true})
    if has_config("loro") then
        if is_plat("wasm") then
            -- no WASM support for now
        else
            add_packages("libcurl", {public = true})
        end
    end
    -- The GLFW backend needs glfw; propagate it so libmogan/stem link it too.
    add_packages("glfw", {public = true})

    add_files({
        "$(projectdir)/3rdparty/imgui/imgui.cpp",
        "$(projectdir)/3rdparty/imgui/imgui_draw.cpp",
        "$(projectdir)/3rdparty/imgui/imgui_tables.cpp",
        "$(projectdir)/3rdparty/imgui/imgui_widgets.cpp",
        "$(projectdir)/3rdparty/imgui/backends/imgui_impl_glfw.cpp",
        "$(projectdir)/3rdparty/imgui/backends/imgui_impl_opengl3.cpp",
    })
    on_run(function (target)
        assert(target:check_cxxsnippets([[
            #include <imgui.h>

            int main() {
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGui::DestroyContext();
                return 0;
            }
        ]]))
        print("ImGui ready.")
    end)
end
