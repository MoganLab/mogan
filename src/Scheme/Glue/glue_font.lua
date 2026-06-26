-------------------------------------------------------------------------------
--
-- MODULE      : glue_font.lua
-- DESCRIPTION : Generating glue on fonts
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_font",
        binding_object = "",
        initializer_name = "initialize_glue_font",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "object_l3.hpp",
            "object_l5.hpp",
            "scheme.hpp",
            "glue_l5_extra.hpp",
        },
        glues = {
            -- src/Graphics/Font/font.hpp
            {
                scm_name = "default-chinese-font",
                cpp_name = "default_chinese_font_name",
                ret_type = "string"
            },
            {
                scm_name = "default-japanese-font",
                cpp_name = "default_japanese_font_name",
                ret_type = "string"
            },
            {
                scm_name = "default-korean-font",
                cpp_name = "default_korean_font_name",
                ret_type = "string"
            },
            {
                scm_name = "font-database-build",
                cpp_name = "font_database_build",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "font-database-build-local",
                cpp_name = "font_database_build_local",
                ret_type = "void"
            },
            {
                scm_name = "font-database-extend-local",
                cpp_name = "font_database_extend_local",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "font-database-build-global",
                cpp_name = "font_database_build_global",
                ret_type = "void"
            },
            {
                scm_name = "font-database-build-characteristics",
                cpp_name = "font_database_build_characteristics",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "font-database-insert-global",
                cpp_name = "font_database_build_global",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "font-database-save-local-delta",
                cpp_name = "font_database_save_local_delta",
                ret_type = "void"
            },
            {
                scm_name = "font-database-load",
                cpp_name = "font_database_load",
                ret_type = "void"
            },
            {
                scm_name = "font-database-save",
                cpp_name = "font_database_save",
                ret_type = "void"
            },
            {
                scm_name = "font-database-filter",
                cpp_name = "font_database_filter",
                ret_type = "void"
            },
            {
                scm_name = "font-database-families",
                cpp_name = "font_database_families",
                ret_type = "array_string"
            },
            {
                scm_name = "font-database-delta-families",
                cpp_name = "font_database_delta_families",
                ret_type = "array_string"
            },
            {
                scm_name = "font-database-styles",
                cpp_name = "font_database_styles",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-database-search",
                cpp_name = "font_database_search",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "font-database-characteristics",
                cpp_name = "font_database_characteristics",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "font-database-substitutions",
                cpp_name = "font_database_substitutions",
                ret_type = "scheme_tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-family->master",
                cpp_name = "family_to_master",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-master->families",
                cpp_name = "master_to_families",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-master-features",
                cpp_name = "master_features",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-family-features",
                cpp_name = "family_features",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-family-strict-features",
                cpp_name = "family_strict_features",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-style-features",
                cpp_name = "style_features",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-guessed-features",
                cpp_name = "guessed_features",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "font-guessed-distance",
                cpp_name = "guessed_distance",
                ret_type = "double",
                arg_list = {
                    "string",
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "font-master-guessed-distance",
                cpp_name = "guessed_distance",
                ret_type = "double",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "font-family-guessed-features",
                cpp_name = "guessed_features",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "characteristic-distance",
                cpp_name = "characteristic_distance",
                ret_type = "double",
                arg_list = {
                    "array_string",
                    "array_string"
                }
            },
            {
                scm_name = "trace-distance",
                cpp_name = "trace_distance",
                ret_type = "double",
                arg_list = {
                    "string",
                    "string",
                    "double"
                }
            },
            {
                scm_name = "logical-font-public",
                cpp_name = "logical_font",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "logical-font-exact",
                cpp_name = "logical_font_exact",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "logical-font-private",
                cpp_name = "logical_font",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "logical-font-family",
                cpp_name = "get_family",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-variant",
                cpp_name = "get_variant",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-series",
                cpp_name = "get_series",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-shape",
                cpp_name = "get_shape",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-search",
                cpp_name = "search_font",
                ret_type = "array_string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-search-exact",
                cpp_name = "search_font_exact",
                ret_type = "array_string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "search-font-families",
                cpp_name = "search_font_families",
                ret_type = "array_string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "search-font-styles",
                cpp_name = "search_font_styles",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-patch",
                cpp_name = "patch_font",
                ret_type = "array_string",
                arg_list = {
                    "array_string",
                    "array_string"
                }
            },
            {
                scm_name = "logical-font-substitute",
                cpp_name = "apply_substitutions",
                ret_type = "array_string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "font-family-main",
                cpp_name = "main_family",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            -- src/Plugins/Freetype/tt_file.hpp
            {
                scm_name = "tt-exists?",
                cpp_name = "tt_font_exists",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "font-exists-in-tt?",
                cpp_name = "tt_font_exists",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            -- src/Plugins/Freetype/tt_tools.hpp
            {
                scm_name = "tt-dump",
                cpp_name = "tt_dump",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "tt-font-name",
                cpp_name = "tt_font_name",
                ret_type = "scheme_tree",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "tt-analyze",
                cpp_name = "tt_analyze",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            -- src/Plugins/Metafont/tex_files.hpp
            {
                scm_name = "url-exists-in-tex?",
                cpp_name = "exists_in_tex",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "freetype-version",
                cpp_name = "freetype_version",
                ret_type = "string",
            },
        }
    }
end
