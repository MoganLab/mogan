-------------------------------------------------------------------------------
--
-- MODULE      : glue_xml.lua
-- DESCRIPTION : Generating glue on XML
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_xml",
        binding_object = "",
        initializer_name = "initialize_glue_xml",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "Xml/xml.hpp",
        },
        glues = {
            {
                scm_name = "tm->xml-name",
                cpp_name = "tm_to_xml_name",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "old-tm->xml-cdata",
                cpp_name = "old_tm_to_xml_cdata",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tm->xml-cdata",
                cpp_name = "tm_to_xml_cdata",
                ret_type = "object",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "xml-name->tm",
                cpp_name = "xml_name_to_tm",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "old-xml-cdata->tm",
                cpp_name = "old_xml_cdata_to_tm",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "xml-unspace",
                cpp_name = "xml_unspace",
                ret_type = "string",
                arg_list = {
                    "string",
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "parse-xml",
                cpp_name = "parse_xml",
                ret_type = "scheme_tree",
                arg_list = {
                    "string"
                }
            },
        }
    }
end
