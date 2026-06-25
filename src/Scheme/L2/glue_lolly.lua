-------------------------------------------------------------------------------
--
-- MODULE      : glue_lolly.lua
-- DESCRIPTION : Generating glue to get information about Lolly
-- COPYRIGHT   : (C) 2023       charonxin
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_lolly",
        glues = {
            {
                scm_name = "lolly-version",
                cpp_name = "lolly_version",
                ret_type = "string",
            },
            {
                scm_name = "string-alpha?",
                cpp_name = "is_alpha",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-locase-alpha?",
                cpp_name = "is_locase_alpha",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "integer->hexadecimal",
                cpp_name = "lolly::data::to_Hex",
                ret_type = "string",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "integer->padded-hexadecimal",
                cpp_name = "lolly::data::as_hexadecimal",
                ret_type = "string",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "hexadecimal->integer",
                cpp_name = "lolly::data::from_hex",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-occurs?",
                cpp_name = "occurs",
                ret_type = "bool",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-count-occurrences",
                cpp_name = "count_occurrences",
                ret_type = "int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-search-forwards",
                cpp_name = "search_forwards",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int",
                    "string"
                }
            },
            {
                scm_name = "string-search-backwards",
                cpp_name = "search_backwards",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int",
                    "string"
                }
            },
            {
                scm_name = "string-overlapping",
                cpp_name = "overlapping",
                ret_type = "int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-replace",
                cpp_name = "replace",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-find-non-alpha",
                cpp_name = "find_non_alpha",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int",
                    "bool"
                }
            },
            {
                scm_name = "upcase-first",
                cpp_name = "upcase_first",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "locase-first",
                cpp_name = "locase_first",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "upcase-all",
                cpp_name = "upcase_all",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "locase-all",
                cpp_name = "locase_all",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-union",
                cpp_name = "string_union",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-minus",
                cpp_name = "string_minus",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "escape-generic",
                cpp_name = "escape_generic",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "escape-verbatim",
                cpp_name = "escape_verbatim",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "escape-shell",
                cpp_name = "escape_sh",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "unescape-guile",
                cpp_name = "unescape_guile",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-string-tokenize",
                cpp_name = "tokenize",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "cpp-string-recompose",
                cpp_name = "recompose",
                ret_type = "string",
                arg_list = {
                    "array_string",
                    "string"
                }
            },
            {
                scm_name = "string-trim-spaces-left",
                cpp_name = "trim_spaces_left",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-trim-spaces-right",
                cpp_name = "trim_spaces_right",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-trim-spaces",
                cpp_name = "trim_spaces",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-differences",
                cpp_name = "differences",
                ret_type = "array_int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "string-distance",
                cpp_name = "distance",
                ret_type = "int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "fuzzy-string-match",
                cpp_name = "fuzzy_match_score",
                ret_type = "int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            -- Kernel/Types/string.hpp
            {
                scm_name = "cpp-string-number?",
                cpp_name = "is_double",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            -- System/Misc/sys_utils.hpp
            {
                scm_name = "get-user-login",
                cpp_name = "get_user_login",
                ret_type = "string"
            },
            {
                scm_name = "get-user-name",
                cpp_name = "get_user_name",
                ret_type = "string"
            },
            {
                scm_name = "os-win32?",
                cpp_name = "os_win",
                ret_type = "bool"
            },
            {
                scm_name = "os-mingw?",
                cpp_name = "os_mingw",
                ret_type = "bool"
            },
            {
                scm_name = "os-macos?",
                cpp_name = "os_macos",
                ret_type = "bool"
            },
            {
                scm_name = "evaluate-system",
                cpp_name = "evaluate_system",
                ret_type = "array_string",
                arg_list = {
                    "array_string",
                    "array_int",
                    "array_string",
                    "array_int"
                }
            },
            {
                scm_name = "system-setenv",
                cpp_name = "set_env",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "system-getenv",
                cpp_name = "get_env",
                ret_type = "string",
                arg_list = {
                    "string",
                }
            },
            -- System/Classes/url
            {
                scm_name = "url->url",
                cpp_name = "url",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "root->url",
                cpp_name = "url_root",
                ret_type = "url",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "url->string",
                cpp_name = "as_string",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url->stree",
                cpp_name = "as_tree",
                ret_type = "scheme_tree",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "system->url",
                cpp_name = "url_system",
                ret_type = "url",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "url->system",
                cpp_name = "as_system_string",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url->unix",
                cpp_name = "as_unix_string",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-unix",
                cpp_name = "url_unix",
                ret_type = "url",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "url-none",
                cpp_name = "url_none",
                ret_type = "url"
            },
            {
                scm_name = "url-any",
                cpp_name = "url_wildcard",
                ret_type = "url"
            },
            {
                scm_name = "url-wildcard",
                cpp_name = "url_wildcard",
                ret_type = "url",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "url-pwd",
                cpp_name = "url_pwd",
                ret_type = "url"
            },
            {
                scm_name = "url-parent",
                cpp_name = "url_parent",
                ret_type = "url"
            },
            {
                scm_name = "url-ancestor",
                cpp_name = "url_ancestor",
                ret_type = "url"
            },
            {
                scm_name = "url-append",
                cpp_name = "url_concat",
                ret_type = "url",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-or",
                cpp_name = "url_or",
                ret_type = "url",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-none?",
                cpp_name = "is_none",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-rooted?",
                cpp_name = "is_rooted",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-rooted-protocol?",
                cpp_name = "is_rooted",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-rooted-web?",
                cpp_name = "is_rooted_web",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-root",
                cpp_name = "get_root",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-unroot",
                cpp_name = "unroot",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-atomic?",
                cpp_name = "is_atomic",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-concat?",
                cpp_name = "is_concat",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-or?",
                cpp_name = "is_or",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-ref",
                cpp_name = "url_ref",
                ret_type = "url",
                arg_list = {
                    "url",
                    "int"
                }
            },
            {
                scm_name = "url-head",
                cpp_name = "head",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-tail",
                cpp_name = "tail",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-suffix",
                cpp_name = "suffix",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-basename",
                cpp_name = "basename",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-glue",
                cpp_name = "glue",
                ret_type = "url",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "url-unglue",
                cpp_name = "unglue",
                ret_type = "url",
                arg_list = {
                    "url",
                    "int"
                }
            },
            {
                scm_name = "url-relative",
                cpp_name = "relative",
                ret_type = "url",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-expand",
                cpp_name = "expand",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-factor",
                cpp_name = "factor",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-delta",
                cpp_name = "delta",
                ret_type = "url",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-descends?",
                cpp_name = "descends",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-drive-letter",
                cpp_name = "drive_letter",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-descendants",
                cpp_name = "subdirectories",
                ret_type = "url",
                arg_list = {
                    "url",
                }
            },
            -- System/Files/file
            {
                scm_name = "url-regular?",
                cpp_name = "is_regular",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-directory?",
                cpp_name = "is_directory",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-link?",
                cpp_name = "is_symbolic_link",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-newer?",
                cpp_name = "is_newer",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "url-size",
                cpp_name = "file_size",
                ret_type = "int",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-last-modified",
                cpp_name = "last_modified",
                ret_type = "int",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "url-temp",
                cpp_name = "url_temp",
                ret_type = "url"
            },
            {
                scm_name = "url-temp-dir",
                cpp_name = "url_temp_dir",
                ret_type = "url"
            },
            {
                scm_name = "system-move",
                cpp_name = "move",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "system-copy",
                cpp_name = "copy",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "system-remove",
                cpp_name = "remove",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "system-mkdir",
                cpp_name = "mkdir",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "system-rmdir",
                cpp_name = "rmdir",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "string-save",
                cpp_name = "string_save",
                ret_type = "void",
                arg_list = {
                    "string",
                    "url"
                }
            },
            -- System/Language/locale
            {
                scm_name = "get-locale-language",
                cpp_name = "get_locale_language",
                ret_type = "string"
            },
            {
                scm_name = "get-locale-charset",
                cpp_name = "get_locale_charset",
                ret_type = "string"
            },
            {
                scm_name = "locale-to-language",
                cpp_name = "locale_to_language",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "language-to-locale",
                cpp_name = "language_to_locale",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "http-status-code",
                cpp_name = "http_status_code",
                ret_type = "long",
                arg_list = {
                    "url"
                }
            },
            -- lolly/data
            {
                scm_name = "encode-base64",
                cpp_name = "lolly::data::encode_base64",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "decode-base64",
                cpp_name = "lolly::data::decode_base64",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "uint32->utf8",
                cpp_name = "lolly::data::encode_as_utf8",
                ret_type = "string",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "utf8->herk",
                cpp_name = "lolly::data::utf8_to_herk",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "herk->utf8",
                cpp_name = "lolly::data::herk_to_utf8",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            -- lolly/system
            {
                scm_name = "system",
                cpp_name = "lolly::system::call",
                ret_type = "void",
                arg_list = {
                    "string",
                }
            },
        }
    }
end
