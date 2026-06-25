-------------------------------------------------------------------------------
--
-- MODULE      : glue_moebius.lua
-- DESCRIPTION : Generating glue on routines in moebius
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023-2024  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_moebius",
        glues = {
            {
                scm_name = "string-quote",
                cpp_name = "scm_quote",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-unquote",
                cpp_name = "scm_unquote",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "utf8->cork",
                cpp_name = "utf8_to_cork",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cork->utf8",
                cpp_name = "cork_to_utf8",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "strict-cork->utf8",
                cpp_name = "strict_cork_to_utf8",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
              -- routines for strings in the TeXmacs encoding
            {
                scm_name = "string->tmstring",
                cpp_name = "tm_encode",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring->string",
                cpp_name = "tm_decode",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-length",
                cpp_name = "tm_string_length",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-ref",
                cpp_name = "tm_forward_access",
                ret_type = "string",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring-reverse-ref",
                cpp_name = "tm_backward_access",
                ret_type = "string",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring->list",
                cpp_name = "tm_tokenize",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-next",
                cpp_name = "tm_char_next",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "string-previous",
                cpp_name = "tm_char_previous",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring-split",
                cpp_name = "tm_string_split",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "list->tmstring",
                cpp_name = "tm_recompose",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "path-strip",
                cpp_name = "strip",
                ret_type = "path",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-inf?",
                cpp_name = "path_inf",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-inf-eq?",
                cpp_name = "path_inf_eq",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-less?",
                cpp_name = "path_less",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-less-eq?",
                cpp_name = "path_less_eq",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-start",
                cpp_name = "start",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-end",
                cpp_name = "end",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
        }
    }
end

