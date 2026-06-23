--! xmake.lua for s7 (libs7 target)
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
-- Copyright (C) 2023-present, TBOOX Open Source Group.
--
-- @author      jinkaimori, Darcy Shen
-- @file        s7_xmake.lua
--
-- 本文件是 package.tools.xmake.install 使用的 port 构建脚本（见
-- xmake/packages/s/s7/xmake.lua）。它编译的 s7 源文件集合与 goldfish 目标内联编译
-- 的那份完全一致（见 xmake/goldfish.lua），从而保证 STEM/tests 通过 add_packages("s7")
-- 用到的 s7 与 goldfish 二进制内嵌的 s7 完全相同。包脚本里的 on_install 会把源码拷进
-- 包缓存构建目录，因此本文件中 add_files 的相对路径（s7.c 等）会在该缓存目录下解析。

add_rules("mode.release", "mode.debug")

option("gmp", {default = false, defines = "WITH_GMP"})

if has_config("gmp") then
    add_requires("gmp")
end

target("libs7") do
    set_kind("$(kind)")
    -- s7 源码使用 C11 特性，无条件设为 c11，与 xmake/goldfish.lua 编译同一批源码时
    -- 使用的 {languages = "c11"} 保持一致（原 3rdparty/s7 仅在 windows 下设置，这里统一）。
    set_languages("c11")
    add_defines("WITH_SYSTEM_EXTRAS=0")
    if not is_plat("wasm") then
    	add_defines("HAVE_OVERFLOW_CHECKS=0")
    end
    add_defines("WITH_WARNINGS")
    add_defines("WITH_R7RS=1")
    set_basename("s7")
    add_files(
        "s7.c",
        "s7_continuation.c",
        "s7_ctables.c",
        "s7_dtoa.c",
        "s7_module.c",
        "s7_op_names.c",
        "s7_scheme_base.c",
        "s7_scheme_char.c",
        "s7_scheme_complex.c",
        "s7_scheme_format.c",
        "s7_scheme_inexact.c",
        "s7_scheme_predicate.c",
        "s7_scheme_symbol.c",
        "s7_scheme_write.c",
        "s7_liii_bitwise.c",
        "s7_liii_hash_table.c",
        "s7_liii_list.c",
        "s7_liii_string.c",
        "s7_liii_vector.c"
    )
    add_headerfiles("s7.h")
    add_includedirs(".", {public = true})
    add_options("gmp")
    if is_plat("windows") then
        set_optimize("faster")
        add_cxxflags("/fp:precise")
    end
    add_packages("gmp")
    if is_mode("debug") then
        add_defines("S7_DEBUGGING")
    end
end
