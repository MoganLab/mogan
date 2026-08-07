-- Velopack C/C++ runtime（Windows x64）集成。
-- add_velopack_runtime () 仅供 target("stem") / target("libmogan") 块内调用：
-- xmake 的 target 作用域 API（add_includedirs/add_links/...）绑定当前活动 target，
-- 普通 Lua 函数在 target 块内调用即可生效。

function add_velopack_runtime ()
    if not (is_plat ("windows") and is_arch ("x64")) then return end
    add_includedirs ("$(projectdir)/3rdparty/velopack/include")
    add_linkdirs   ("$(projectdir)/3rdparty/velopack/lib")
    -- 链接导入库：文件名形如 velopack_libc_win_x64_msvc.dll.lib
    add_links ("velopack_libc_win_x64_msvc.dll")
end

-- 最小验证程序：仅验证 Velopack C++ 启动钩子可编译/链接，未安装环境下 Run() 为空操作。
-- 不放在 tests/ 下：根 xmake.lua 会自动发现 tests/**_test.cpp 并链接 libmogan/libmoebius，
-- 会与此处目标重名冲突，且该验证程序不应依赖项目库。
if is_plat ("windows") and is_arch ("x64") then
    target ("velopack_startup_test") do
        set_kind ("binary")
        set_group ("velopack")
        set_languages ("c++17")
        set_runtimes ("MT")
        add_velopack_runtime ()
        add_files ("$(projectdir)/tools/velopack/velopack_startup_test.cpp")
        -- 运行时需在 exe 旁找到 DLL。导入库内嵌的 DLL 名为 velopack_libc.dll，
        -- 故发布时须把 vendored 的 velopack_libc_win_x64_msvc.dll 改名为 velopack_libc.dll。
        -- （after_build 不替代默认编译/链接）
        after_build (function (target)
            os.cp (path.join (os.projectdir (), "3rdparty/velopack/lib/velopack_libc_win_x64_msvc.dll"),
                   path.join (target:targetdir (), "velopack_libc.dll"))
        end)
    end
end
