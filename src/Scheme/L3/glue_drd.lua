-------------------------------------------------------------------------------
--
-- MODULE      : glue_drd.lua
-- DESCRIPTION : Generating glue on src/Data/Drd
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_drd",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "glue_drd_extra.hpp",
        },
        glues = {
            {
                scm_name = "set-access-mode",
                cpp_name = "set_access_mode",
                ret_type = "int",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "get-access-mode",
                cpp_name = "get_access_mode",
                ret_type = "int"
            },
        }
    }
end
