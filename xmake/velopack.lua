-- Velopack C/C++ runtime（Windows x64 / macOS arm64）集成。
-- add_velopack_runtime () 仅供 target("stem") / target("libmogan") 块内调用：
-- xmake 的 target 作用域 API（add_includedirs/add_links/...）绑定当前活动 target，
-- 普通 Lua 函数在 target 块内调用即可生效。

function add_velopack_runtime ()
    if is_plat ("windows") and is_arch ("x64") then
        add_includedirs ("$(projectdir)/3rdparty/velopack/include")
        add_linkdirs   ("$(projectdir)/3rdparty/velopack/lib")
        -- 链接导入库：文件名形如 velopack_libc_win_x64_msvc.dll.lib
        add_links ("velopack_libc_win_x64_msvc.dll")
    elseif is_plat ("macosx") and is_arch ("arm64") then
        add_includedirs ("$(projectdir)/3rdparty/velopack/include")
        add_linkdirs   ("$(projectdir)/3rdparty/velopack/lib")
        -- dylib 的 install id 已在 vendor 时写为 @rpath/libvelopack_libc.dylib
        -- （与磁盘文件名一致），链接后 exe/bundle 自带的
        -- @executable_path/Frameworks rpath 即可解析。
        -- 无源码的 shared 依赖不产生链接项，链接由上面的 add_links 显式完成；
        -- add_deps 只是让 qt.widgetapp 部署规则把 dylib 拷进 .app 的
        -- Contents/Frameworks（该机制按 shared 依赖的 targetfile 落位）。
        add_links ("velopack_libc")
        add_deps ("velopack_libc")
    end
end

-- macOS：把 vendored dylib 包成 shared target。qt.widgetapp 的部署规则
-- （xmake rules/qt/deploy/macosx.lua）会把 shared 依赖的 targetfile 自动拷进
-- .app 的 Contents/Frameworks，且时机在 bundle 重建之后——自写 after_build 拷贝
-- 会被规则的 os.tryrm 整包清掉。targetdir 定到 <stem targetdir>/Frameworks，
-- 裸二进制（dev 运行 / CI scheme 测试用 target:targetfile()）靠自带的
-- @executable_path/Frameworks rpath 找到它，.app 由部署规则落位，一处受益两处。
if is_plat ("macosx") and is_arch ("arm64") then
    target ("velopack_libc") do
        set_kind ("shared")
        set_group ("velopack")
        -- 与 stem 同层（build/macosx/$(arch)/$(mode)）下的 Frameworks/
        set_targetdir (path.join ("$(builddir)", "macosx", "$(arch)", "$(mode)", "Frameworks"))
        -- 预编译产物，无源码：on_build 即“编译”，dylib 的 install id 已在
        -- vendor 时写为 @rpath/velopack_libc.dylib（见 3rdparty/velopack/README.md）
        on_build (function (target)
            os.cp (path.join (os.projectdir (), "3rdparty/velopack/lib/libvelopack_libc.dylib"),
                   target:targetfile ())
        end)
        on_install (function (target) end) -- 部署走 qt 规则（Frameworks），不做独立安装
    end
end

-- 最小验证程序：仅验证 Velopack C++ 启动钩子可编译/链接，未安装环境下 Run() 为空操作。
-- 不放在 tests/ 下：根 xmake.lua 会自动发现 tests/**_test.cpp 并链接 libmogan/libmoebius，
-- 会与此处目标重名冲突，且该验证程序不应依赖项目库。
if (is_plat ("windows") and is_arch ("x64")) or
   (is_plat ("macosx") and is_arch ("arm64")) then
    target ("velopack_startup_test") do
        set_kind ("binary")
        set_group ("velopack")
        set_languages ("c++17")
        if is_plat ("windows") then
            set_runtimes ("MT")
        end
        add_velopack_runtime ()
        add_files ("$(projectdir)/tools/velopack/velopack_startup_test.cpp")
        if is_plat ("windows") then
            -- 运行时需在 exe 旁找到 DLL。导入库内嵌的 DLL 名为 velopack_libc.dll，
            -- 故发布时须把 vendored 的 velopack_libc_win_x64_msvc.dll 改名为 velopack_libc.dll。
            -- （after_build 不替代默认编译/链接）
            after_build (function (target)
                os.cp (path.join (os.projectdir (), "3rdparty/velopack/lib/velopack_libc_win_x64_msvc.dll"),
                       path.join (target:targetdir (), "velopack_libc.dll"))
            end)
        else
            -- 裸二进制无 bundle，dylib 放 exe 旁并指 @loader_path 搜索
            add_rpathdirs ("@loader_path")
            after_build (function (target)
                os.cp (path.join (os.projectdir (), "3rdparty/velopack/lib/libvelopack_libc.dylib"),
                       path.join (target:targetdir (), "libvelopack_libc.dylib"))
            end)
        end
    end
end
