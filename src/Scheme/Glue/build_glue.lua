-------------------------------------------------------------------------------
--
-- MODULE      : build_glue.lua
-- DESCRIPTION : Build cpp-scheme glue declaration
-- COPYRIGHT   : (C) 2023       jingkaimori
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

local function translate_name(name)
    if name:endswith("*") then
        name = name:sub(1,-2) .. "_dot"
    end
    name = name:gsub("?", "P")
    name = name:gsub("!", "S")
    name = name:gsub("<", "F")
    name = name:gsub(">", "2")
    name = name:gsub("-", "_")
    name = name:gsub("=", "Q")
    name = "tmg_" .. name
    return name;
end

function main(glue, glue_name)
    local func_list = ""
    local reg_list = ""
    for _,glue_item in ipairs(glue.glues) do
        local scm_name = glue_item.scm_name
        local trans_name = translate_name(scm_name)
        local arg_count = 0;
        local arg_list = ""
        local in_list = ""
        local extraction = ""
        local assertion = ""

        if glue_item.arg_list ~= nil then
            arg_count = #glue_item.arg_list

            local iter,stat,init_pos = ipairs(glue_item.arg_list);
            local first_pos,first_arg = iter(stat,init_pos)
            if first_pos ~= nil then
                arg_list = "tmscm arg1"
                in_list = "in1"
                
                for pos,arg in iter,stat,first_pos do
                    arg_list = arg_list .. ", tmscm arg" .. pos
                    in_list = in_list .. ", in" .. pos
                end
            end

            for pos,arg in ipairs(glue_item.arg_list) do
                assertion = assertion .. "  TMSCM_ASSERT_" .. string.upper(arg) .. " (arg" .. pos .. ", TMSCM_ARG" .. pos .. ", \"" .. scm_name .. "\");\n"
                extraction = extraction .. "  " .. arg .. " in" .. pos .. "= " .. "tmscm_to_" .. arg .. " (arg" .. pos .. ");\n"
            end

            assertion = assertion .. "\n"
            extraction = extraction .. "\n"
        end

        local returned = "TMSCM_UNSPECIFIED"
        local invoke = glue.binding_object .. glue_item.cpp_name .. " (" .. in_list .. ");\n"
        if glue_item.ret_type ~= "void" then
            invoke = glue_item.ret_type .. " out= " .. invoke
            returned = glue_item.ret_type .. "_to_tmscm (out)"
        end

        func_list = func_list .. [[
tmscm
]] .. trans_name .. " (" .. arg_list .. [[) {
]] .. assertion  
.. extraction .. [[
  // TMSCM_DEFER_INTS;
  ]] .. invoke .. [[
  // TMSCM_ALLOW_INTS;

  return ]] .. returned ..[[;
}

]]
        reg_list = reg_list .. [[  tmscm_install_procedure ("]] .. scm_name .. [[",  ]] .. trans_name .. ", " .. arg_count .. [[, 0, 0);
]]
    end
    local res = [[

/******************************************************************************
*
* This file has been generated automatically using build-glue.scm
* from ]] .. glue_name .. [[.lua. Please do not edit its contents.
* Copyright (C) 2000 Joris van der Hoeven
*
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
*
******************************************************************************/

]]

    if glue.standalone then
        for _, inc in ipairs(glue.includes or {}) do
            if inc:startswith("<") then
                res = res .. "#include " .. inc .. "\n"
            else
                res = res .. "#include \"" .. inc .. "\"\n"
            end
        end
        for _, usg in ipairs(glue.usings or {}) do
            res = res .. "using " .. usg .. ";\n"
        end
        res = res .. "\n"
    end

    res = res .. func_list ..
[[void
]] .. glue.initializer_name .. [[ () {
]] .. reg_list
.. [[}
]]
    return res
end
