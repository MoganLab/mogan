set_xmakever("2.8.7")

set_allowedmodes("releasedbg", "release", "debug")
add_rules("mode.debug")

local moe_root = os.scriptdir()
local moe_files = {
    path.join(moe_root, "Data/Convert/**.cpp"),
    path.join(moe_root, "Data/History/**.cpp"),
    path.join(moe_root, "Data/Tree/**.cpp"),
    path.join(moe_root, "Kernel/Types/**.cpp"),
    path.join(moe_root, "Kernel/Abstractions/**.cpp"),
    path.join(moe_root, "Scheme/**.cpp"),
    path.join(moe_root, "moebius/**.cpp"),
}
local moe_includedirs = {
    path.join(moe_root, "Data/Convert"),
    path.join(moe_root, "Data/History"),
    path.join(moe_root, "Data/Tree"),
    path.join(moe_root, "Kernel/Types"),
    path.join(moe_root, "Kernel/Abstractions"),
    path.join(moe_root, "Scheme"),
    path.join(moe_root, "Scheme/L1"),
    path.join(moe_root, "Scheme/L2"),
    path.join(moe_root, "Scheme/L3"),
    path.join(moe_root, "Scheme/S7"),
    path.join(moe_root, "Scheme/Scheme"),
    moe_root,
}

add_requires("liii-doctest", {system=false})
add_requires("nanobench", {system=false})
add_requires("s7", {system=false})


target("libmoebius") do
    set_kind ("static")
    set_languages("c++17")
    set_encodings("utf-8")
    set_basename("moebius")

    add_includedirs(moe_includedirs, {public = true})
    add_files(moe_files)

    add_deps("liblolly")
    add_packages("s7")

    add_headerfiles("Data/Convert/(*.hpp)")
    add_headerfiles("Data/History/(*.hpp)")
    add_headerfiles("Data/Tree/(*.hpp)")
    add_headerfiles("Kernel/Types/(*.hpp)")
    add_headerfiles("Kernel/Abstractions/(*.hpp)")
    add_headerfiles("Scheme/(*.hpp)")
    add_headerfiles("Scheme/L1/(*.hpp)")
    add_headerfiles("Scheme/L2/(*.hpp)")
    add_headerfiles("Scheme/L3/(*.hpp)")
    add_headerfiles("Scheme/S7/(*.hpp)")
    add_headerfiles("Scheme/Scheme/(*.hpp)")
    add_headerfiles("moebius/(data/*.hpp)", {prefixdir="moebius"})
    add_headerfiles("moebius/(drd/*.hpp)", {prefixdir="moebius"})
    add_headerfiles("moebius/(*.hpp)", {prefixdir="moebius"})
end

target("moebius_tests") do
    set_kind ("binary")
    set_languages("c++17")
    set_default (false)

    add_deps("libmoebius")

    add_includedirs(moe_includedirs)
    add_includedirs("tests")

    cpp_tests_on_all_plat = os.files("tests/**_test.cpp")
    for _, testfile in ipairs(cpp_tests_on_all_plat) do
        add_tests(path.basename(testfile), {
            kind = "binary",
            files = testfile,
            packages = {"liii-doctest"},
            defines = "DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN"})
    end
end

target("moebius_bench_base")do
    set_kind("object")
    set_languages("c++17")
    set_default (false)
    set_policy("check.auto_ignore_flags", false)
    add_packages("nanobench")

    if is_plat("windows") then
        set_encodings("utf-8")
    end
    add_files("bench/nanobench.cpp")
end

function add_bench_target(filepath)
    local benchname = path.basename(filepath)
    target(benchname) do
        set_group("bench")
        set_languages("c++17")
        set_default(false)
        set_policy("check.auto_ignore_flags", false)
        set_rundir("$(projectdir)")
        add_deps({"libmoebius", "moebius_bench_base"})
        add_packages("nanobench")

        if is_plat("linux") then
            add_syslinks("stdc++", "m")
        end

        if is_plat("windows") then
            set_encodings("utf-8")
            add_syslinks("secur32", "shell32")
        end

        add_includedirs(moe_includedirs)
        add_files(filepath)
    end
end

cpp_bench_on_all_plat = os.files("bench/**_bench.cpp")
for _, filepath in ipairs(cpp_bench_on_all_plat) do
    add_bench_target (filepath)
end
