
-------------------------------------------------------------------------------
--
-- MODULE      : glue_plugins.lua
-- DESCRIPTION : Generating glue on src/Plugins
-- COPYRIGHT   : (C) 2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_plugin",
        binding_object = "",
        initializer_name = "initialize_glue_plugin",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "glue_plugin_extra.hpp",
        },
        glues = {
            {
                scm_name = "use-plugin-bibtex?",
                cpp_name = "use_plugin_bibtex",
                ret_type = "bool"
            },
            {
                scm_name = "use-plugin-tex?",
                cpp_name = "use_plugin_tex",
                ret_type = "bool"
            },
            {
                scm_name = "use-plugin-updater?",
                cpp_name = "use_plugin_updater",
                ret_type = "bool"
            },
            {
                scm_name = "loro-enabled?",
                cpp_name = "loro_enabled",
                ret_type = "bool",
            }
        }
    }
end

