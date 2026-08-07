-------------------------------------------------------------------------------
--
-- MODULE      : glue_updater.lua
-- DESCRIPTION : Generating glue on src/Plugins/Updater
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_updater",
        binding_object = "",
        initializer_name = "initialize_glue_updater",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "Updater/tm_updater.hpp",
        },
        glues = {
            {
                scm_name = "updater-running?",
                cpp_name = "updater_is_running",
                ret_type = "bool"
            },
            {
                scm_name = "updater-check-background",
                cpp_name = "updater_check_background",
                ret_type = "bool"
            },
            {
                scm_name = "updater-check-foreground",
                cpp_name = "updater_check_foreground",
                ret_type = "bool"
            },
            {
                scm_name = "updater-last-check",
                cpp_name = "updater_last_check",
                ret_type = "long"
            },
            {
                scm_name = "updater-set-interval",
                cpp_name = "updater_set_interval",
                ret_type = "bool",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "updater-state",
                cpp_name = "updater_state",
                ret_type = "int"
            },
            {
                scm_name = "updater-available-version",
                cpp_name = "updater_available_version",
                ret_type = "string"
            },
            {
                scm_name = "updater-release-notes",
                cpp_name = "updater_release_notes",
                ret_type = "string"
            },
            {
                scm_name = "updater-progress",
                cpp_name = "updater_progress",
                ret_type = "int"
            },
            {
                scm_name = "updater-error-code",
                cpp_name = "updater_error_code",
                ret_type = "string"
            },
            {
                scm_name = "updater-download",
                cpp_name = "updater_download",
                ret_type = "bool"
            },
            {
                scm_name = "updater-apply",
                cpp_name = "updater_apply",
                ret_type = "bool"
            },
        }
    }
end
