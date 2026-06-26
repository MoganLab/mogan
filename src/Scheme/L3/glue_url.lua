-------------------------------------------------------------------------------
--
-- MODULE      : glue_url.lua
-- DESCRIPTION : Generating glue on src/System/Classes/url.hpp
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_url",
        binding_object = "",
        initializer_name = "initialize_glue_url",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "s7_tm.hpp",
            "tm_file.hpp",
            "tm_url.hpp",
            "tmfs_url.hpp",
            "url.hpp",
        },
        glues = {
            {
                scm_name = "url-complete",
                cpp_name = "complete",
                ret_type = "url",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-resolve",
                cpp_name = "resolve",
                ret_type = "url",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-resolve-in-path",
                cpp_name = "resolve_in_path",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-exists?",
                cpp_name = "exists",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-exists-in-path?",
                cpp_name = "exists_in_path",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-concretize*",
                cpp_name = "concretize_url",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-concretize",
                cpp_name = "concretize",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-sys-concretize",
                cpp_name = "sys_concretize",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-materialize",
                cpp_name = "materialize",
                ret_type = "string",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-secure?",
                cpp_name = "is_secure",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "string->url",
                cpp_name = "url",
                ret_type = "url",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "url-rooted-tmfs?",
                cpp_name = "is_rooted_tmfs",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-rooted-tmfs-protocol?",
                cpp_name = "is_rooted_tmfs",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "unix->url",
                cpp_name = "url",
                ret_type = "url",
                arg_list = {
                    "string"
                }
            },
        }
    }
end
