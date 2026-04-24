-------------------------------------------------------------------------------
--
-- MODULE      : glue_widget.lua
-- DESCRIPTION : Generating glue on widget
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
        initializer_name = "initialize_glue_widget",
        glues = {
            {
                scm_name = "x-gui?",
                cpp_name = "gui_is_x",
                ret_type = "bool"
            },
            {
                scm_name = "qt-gui?",
                cpp_name = "gui_is_qt",
                ret_type = "bool"
            },
            {
                scm_name = "gui-version",
                cpp_name = "gui_version",
                ret_type = "string"
            },
            {
                scm_name = "widget-printer",
                cpp_name = "printer_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "url"
                }
            },
            {
                scm_name = "widget-color-picker",
                cpp_name = "color_picker_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "bool",
                    "array_tree"
                }
            },
            {
                scm_name = "widget-extend",
                cpp_name = "extend_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "array_widget"
                }
            },
            {
                scm_name = "widget-hmenu",
                cpp_name = "horizontal_menu",
                ret_type = "widget",
                arg_list = {
                    "array_widget"
                }
            },
            {
                scm_name = "widget-vmenu",
                cpp_name = "vertical_menu",
                ret_type = "widget",
                arg_list = {
                    "array_widget"
                }
            },
            {
                scm_name = "widget-tmenu",
                cpp_name = "tile_menu",
                ret_type = "widget",
                arg_list = {
                    "array_widget",
                    "int"
                }
            },
            {
                scm_name = "widget-minibar-menu",
                cpp_name = "minibar_menu",
                ret_type = "widget",
                arg_list = {
                    "array_widget"
                }
            },
            {
                scm_name = "widget-separator",
                cpp_name = "menu_separator",
                ret_type = "widget",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "widget-menu-group",
                cpp_name = "menu_group",
                ret_type = "widget",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "widget-pulldown-button",
                cpp_name = "pulldown_button",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "promise_widget"
                }
            },
            {
                scm_name = "widget-pullright-button",
                cpp_name = "pullright_button",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "promise_widget"
                }
            },
            {
                scm_name = "widget-menu-button",
                cpp_name = "menu_button",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "command",
                    "string",
                    "string",
                    "int"
                }
            },
            {
                scm_name = "widget-toggle",
                cpp_name = "toggle_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "bool",
                    "int"
                }
            },
            {
                scm_name = "widget-balloon",
                cpp_name = "balloon_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "widget"
                }
            },
            {
                scm_name = "widget-empty",
                cpp_name = "empty_widget",
                ret_type = "widget"
            },
            {
                scm_name = "widget-text",
                cpp_name = "text_widget",
                ret_type = "widget",
                arg_list = {
                    "string",
                    "int",
                    "int",
                    "bool"
                }
            },
            {
                scm_name = "widget-input",
                cpp_name = "input_text_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "string",
                    "array_string",
                    "int",
                    "string"
                }
            },
            {
                scm_name = "widget-enum",
                cpp_name = "enum_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "array_string",
                    "string",
                    "int",
                    "string"
                }
            },
            {
                scm_name = "widget-choice",
                cpp_name = "choice_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "array_string",
                    "string"
                }
            },
            {
                scm_name = "widget-choices",
                cpp_name = "choice_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "array_string",
                    "array_string"
                }
            },
            {
                scm_name = "widget-filtered-choice",
                cpp_name = "choice_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "array_string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "widget-tree-view",
                cpp_name = "tree_view_widget",
                ret_type = "widget",
                arg_list = {
                    "command",
                    "tree",
                    "tree"
                }
            },
            {
                scm_name = "widget-xpm",
                cpp_name = "xpm_widget",
                ret_type = "widget",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "widget-box",
                cpp_name = "box_widget",
                ret_type = "widget",
                arg_list = {
                    "scheme_tree",
                    "string",
                    "int",
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "widget-glue",
                cpp_name = "glue_widget",
                ret_type = "widget",
                arg_list = {
                    "bool",
                    "bool",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "widget-color",
                cpp_name = "glue_widget",
                ret_type = "widget",
                arg_list = {
                    "content",
                    "bool",
                    "bool",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "widget-hlist",
                cpp_name = "horizontal_list",
                ret_type = "widget",
                arg_list = {
                    "array_widget"
                }
            },
            {
                scm_name = "widget-vlist",
                cpp_name = "vertical_list",
                ret_type = "widget",
                arg_list = {
                    "array_widget"
                }
            },
            {
                scm_name = "widget-division",
                cpp_name = "division_widget",
                ret_type = "widget",
                arg_list = {
                    "string",
                    "widget"
                }
            },
            {
                scm_name = "widget-aligned",
                cpp_name = "aligned_widget",
                ret_type = "widget",
                arg_list = {
                    "array_widget",
                    "array_widget"
                }
            },
            {
                scm_name = "widget-tabs",
                cpp_name = "tabs_widget",
                ret_type = "widget",
                arg_list = {
                    "array_widget",
                    "array_widget"
                }
            },
            {
                scm_name = "widget-icon-tabs",
                cpp_name = "icon_tabs_widget",
                ret_type = "widget",
                arg_list = {
                    "array_url",
                    "array_widget",
                    "array_widget"
                }
            },
            {
                scm_name = "widget-scrollable",
                cpp_name = "user_canvas_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "int"
                }
            },
            {
                scm_name = "widget-resize",
                cpp_name = "resize_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "int",
                    "string",
                    "string",
                    "string",
                    "string",
                    "string",
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "widget-hsplit",
                cpp_name = "hsplit_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "widget"
                }
            },
            {
                scm_name = "widget-vsplit",
                cpp_name = "vsplit_widget",
                ret_type = "widget",
                arg_list = {
                    "widget",
                    "widget"
                }
            },
            {
                scm_name = "widget-texmacs-output",
                cpp_name = "texmacs_output_widget",
                ret_type = "widget",
                arg_list = {
                    "content",
                    "content"
                }
            },
            {
                scm_name = "widget-texmacs-input",
                cpp_name = "texmacs_input_widget",
                ret_type = "widget",
                arg_list = {
                    "content",
                    "content",
                    "url"
                }
            },
            {
                scm_name = "widget-ink",
                cpp_name = "ink_widget",
                ret_type = "widget",
                arg_list = {
                    "command"
                }
            },
            {
                scm_name = "widget-refresh",
                cpp_name = "refresh_widget",
                ret_type = "widget",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "widget-refreshable",
                cpp_name = "refreshable_widget",
                ret_type = "widget",
                arg_list = {
                    "object",
                    "string"
                }
            },
            {
                scm_name = "object->promise-widget",
                cpp_name = "as_promise_widget",
                ret_type = "promise_widget",
                arg_list = {
                    "object"
                }
            },
            {
                scm_name = "tree-bounding-rectangle",
                cpp_name = "get_bounding_rectangle",
                ret_type = "array_int",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "widget-size",
                cpp_name = "get_widget_size",
                ret_type = "array_int",
                arg_list = {
                    "widget"
                }
            },
            {
                scm_name = "texmacs-widget-size",
                cpp_name = "get_texmacs_widget_size",
                ret_type = "array_int",
                arg_list = {
                    "widget"
                }
            },
            {
                scm_name = "widget-tab-page",
                cpp_name = "tab_page_widget",
                ret_type = "widget",
                arg_list = {
                    "url",
                    "widget",
                    "widget",
                    "bool"
                }
            },
            {
                scm_name = "qt-clipboard-format",
                cpp_name = "qt_clipboard_format",
                ret_type = "string"
            },
            {
                scm_name = "qt-clipboard-text",
                cpp_name = "qt_clipboard_text",
                ret_type = "string"
            },
            {
                scm_name = "qt-clipboard-set-html",
                cpp_name = "qt_clipboard_set_html",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "open-pricing-url",
                cpp_name = "open_pricing_url",
                ret_type = "void"
            }
        }
    }
end
