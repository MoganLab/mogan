-------------------------------------------------------------------------------
--
-- MODULE      : stem_packager.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

target("stem_packager") do
    set_enabled(is_plat("macosx") and is_mode("release"))
    set_kind("phony")

    add_deps("stem")

    -- 重新声明变量以解决作用域问题
    local stem_project_name_local = stem_project_name
    local stem_binary_name_local = stem_binary_name
	local stem_dmg_bg_name_local = stem_dmg_bg_image

    set_configvar("XMACS_VERSION", XMACS_VERSION)
    set_configvar("APPCAST", "")
    set_configvar("OSXVERMIN", "")
    set_configvar("STEM_NAME", stem_binary_name_local)
    add_configfiles("$(projectdir)/packages/macos/Info.plist.in", {
        filename = "Info.plist",
        pattern = "@(.-)@",
    })

    set_installdir(path.join("$(builddir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name_local .. ".app/Contents/Resources/"))

    local dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. ".dmg"
    if is_arch("arm64") then
        dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. "-arm.dmg"
    elseif is_arch("x86_64") then
        dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. "-x64.dmg"
    end
	
	-- print("DMG name will be: " .. dmg_name)
	-- print("Build dir is: " .. path.absolute("$(builddir)"))
	-- print("App dir is: " .. path.absolute(path.join("$(builddir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name_local .. ".app")))

    after_install(function (target, opt)
        local app_dir = target:installdir() .. "/../../"
        local build_dir = path.absolute("$(builddir)")
        local project_dir = os.projectdir()
        
        print("Packaging app at: " .. app_dir)
		os.cp(path.join(build_dir, "Info.plist"), app_dir .. "/Contents")
        
        -- 复制图标文件
        local resources_dir = app_dir .. "/Contents/Resources"
        os.cp(path.join(project_dir, "packages", "macos", "stem.icns"), resources_dir)
        os.cp(path.join(project_dir, "packages", "macos", "TeXmacs-document.icns"), resources_dir)
        -- 复制DMG相关的图标到build目录，供create-dmg使用
        os.cp(path.join(project_dir, "packages", "macos", "driver.icns"), build_dir)
        print("Copied icon files to: " .. resources_dir)
        
        os.execv("codesign", {"--force", "--deep", "--sign", "-", app_dir})

        -- 构建DMG路径
        local dmg_path = path.join(build_dir, dmg_name)
        local app_path = path.absolute(app_dir)
        
        -- 清理可能存在的旧DMG文件和临时文件
        if os.isfile(dmg_path) then
            print("Removing existing DMG: " .. dmg_path)
            os.rm(dmg_path)
        end
        os.exec("rm -rf /tmp/create-dmg.* 2>/dev/null || true")
        os.exec("rm -rf /tmp/dmg.* 2>/dev/null || true")

        -- 尝试 create-dmg，使用更安全的参数格式
        -- Helper: run a command with retry on failure
        -- xmake sandbox disables pcall; use try/catch wrapper
        local function retry_execv(cmd, args, attempts, delay_seconds)
            attempts = attempts or 10
            delay_seconds = delay_seconds or 2
            local last_err = nil
            for i = 1, attempts do
                local ok = true
                try {
                    function ()
                        os.execv(cmd, args)
                    end,
                    catch {
                        function (errors)
                            ok = false
                            last_err = errors
                        end
                    }
                }
                if ok then
                    return true
                end
                print(string.format("Attempt %d/%d failed: %s", i, attempts, tostring(last_err)))
                if i < attempts then
                    print(string.format("Retrying in %d seconds...", delay_seconds))
                    os.sleep(delay_seconds * 1000)
                end
            end
            return false
        end

        try {
            function ()
				print("Creating DMG at: " .. dmg_path)
				print("Using app path: " .. app_path)
				
				-- 切换到 build 目录执行 create-dmg
				local old_dir = os.curdir()
				os.cd(build_dir)
				
                -- 检查背景图片
                local background_image = path.join(project_dir, "packages", "macos", stem_dmg_bg_name_local)
                local args_with_bg = {
                    "--volicon", "driver.icns",
                    "--background", background_image,
                    "--volname", stem_project_name_local,
                    "--window-pos", "200", "120",
                    "--window-size", "720", "480",
                    "--icon-size", "120",
                    "--icon", stem_binary_name_local .. ".app", "200", "190",
                    "--app-drop-link", "540", "190",
                    dmg_name,
                    app_path
                }
                local args_no_bg = {
                    "--volicon", "driver.icns",
                    "--volname", stem_project_name_local,
                    "--window-pos", "200", "120",
                    "--window-size", "720", "480",
                    "--icon-size", "120",
                    "--icon", stem_binary_name_local .. ".app", "200", "190",
                    "--app-drop-link", "540", "190",
                    dmg_name,
                    app_path
                }

                local ok
                if os.isfile(background_image) then
                    print("Background image found; using themed DMG background")
                    ok = retry_execv("create-dmg", args_with_bg, 10, 3)
                    if not ok then
                        print("create-dmg failed with background; retrying without background...")
                        ok = retry_execv("create-dmg", args_no_bg, 10, 3)
                    end
                else
                    print("Background image not found; creating DMG without background")
                    ok = retry_execv("create-dmg", args_no_bg, 10, 3)
                end

                if not ok then
                    raise("create-dmg failed after retries")
                end
                
                -- 恢复原目录
                os.cd(old_dir)
            end,
            catch {
                function (errors)
                    print("create-dmg failed: " .. tostring(errors))
                    -- Propagate failure to CI pipeline
                    raise(errors)
                end
            }
        }
    end)
end
