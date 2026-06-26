-------------------------------------------------------------------------------
--
-- MODULE      : glue_pdf.lua
-- DESCRIPTION : Generating glue on src/Plugins/Pdf
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_pdf",
        binding_object = "",
        initializer_name = "initialize_glue_pdf",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "Pdf/pdf_hummus_extract_attachment.hpp",
            "Pdf/pdf_hummus_make_attachment.hpp",
            "Pdf/pdf_image.hpp",
            "glue_pdf_extra.hpp",
        },
        glues = {
            {
                scm_name = "supports-native-pdf?",
                cpp_name = "supports_native_pdf",
                ret_type = "bool"
            },
            {
                scm_name = "pdfhummus-version",
                cpp_name = "pdfhummus_version",
                ret_type = "string"
            },
            {
                scm_name = "extract-attachments",
                cpp_name = "scm_extract_attachments",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "pdf-make-attachments",
                cpp_name = "pdf_hummus_make_attachments",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "array_url",
                    "url"
                }
            },
            {
                scm_name = "pdf-get-linked-file-paths",
                cpp_name = "get_linked_file_paths",
                ret_type = "array_url",
                arg_list = {
                    "tree",
                    "url"
                }
            },
            {
                scm_name = "pdf-replace-linked-path",
                cpp_name = "replace_with_relative_path",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "url"
                }
            },
            {
                scm_name = "pdf-get-attached-main-tm",
                cpp_name = "get_main_tm",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "pdf-image-size",
                cpp_name = "pdfhummus_image_size",
                ret_type = "array_int",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "array-url-append",
                cpp_name = "append",
                ret_type = "array_url",
                arg_list = {
                    "url",
                    "array_url"
                }
            },
        }
    }
end
