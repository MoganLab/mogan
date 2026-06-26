-------------------------------------------------------------------------------
--
-- MODULE      : glue_file.lua
-- DESCRIPTION : Generating glue on src/System/Files/file.hpp
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_file",
        binding_object = "",
        initializer_name = "initialize_glue_file",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "s7_tm.hpp",
            "tm_file.hpp",
            "tm_url.hpp",
            "url.hpp",
        },
        glues = {
            {
                scm_name = "string-append-to-file",
                cpp_name = "string_append_to_file",
                ret_type = "void",
                arg_list = {
                    "string",
                    "url"
                }
            },
            {
                scm_name = "url-test?",
                cpp_name = "url_test",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-scratch",
                cpp_name = "url_scratch",
                ret_type = "url",
                arg_list = {
                    "string",
                    "string",
                    "int"
                }
            },
            {
                scm_name = "url-scratch?",
                cpp_name = "is_scratch",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-format",
                cpp_name = "file_format",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "system-url->string",
                cpp_name = "sys_concretize",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "system-search-score",
                cpp_name = "search_score",
                ret_type = "int",
                arg_list = {
                    "url",
                    "array_string"
                }
            },
            {
                scm_name = "url-grep",
                cpp_name = "grep",
                ret_type = "url",
                arg_list = {
                    "string",
                    "url"
                }
            },
            {
                scm_name = "url-search-upwards",
                cpp_name = "search_file_upwards",
                ret_type = "url",
                arg_list = {
                    "url",
                    "string",
                    "array_string"
                }
            },
            {
                scm_name = "string-load",
                cpp_name = "tm_string_load",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
        }
    }
end
