--
-- MODULE      : loro.lua
-- DESCRIPTION : Second-party target for the Rust Loro FFI.
--               It invokes Cargo to build libmogan_loro_ffi.a and exports all
--               required link information to dependent targets.
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

target("loro")
    set_kind("phony")
    add_packages("rust")
    local ffi_dir = path.join(os.projectdir(), "3rdparty", "mogan-loro-ffi")
    local profile = (is_mode("release") or is_mode("releasedbg"))
                        and "release"
                        or "dev"
    local subdir = (profile == "release")
                        and "release"
                        or "debug"

    -- 导出库搜索路径与库名
    add_linkdirs(path.join(ffi_dir, "target", subdir), {public = true})
    add_links("mogan_loro_ffi", {public = true})

    -- 跨平台系统库导出
    if is_plat("linux") then
        add_syslinks("pthread", "dl", "m", "util", {public = true})
    elseif is_plat("macosx") then
        add_syslinks("iconv", "resolv", "System", {public = true})
        add_frameworks("Security", "Foundation", {public = true})
    elseif is_plat("windows") then
        -- 补充：Rust 静态库在 Windows 上必需的底层系统链接
        add_syslinks("userenv", "ws2_32", "bcrypt", {public = true})
    end

    before_build(function (target)
        import("core.base.option")
        
        local args = {"build", "--manifest-path", path.join(ffi_dir, "Cargo.toml")}
        table.join2(args, {"--profile", profile})
        
        if option.get("verbose") then
            table.insert(args, "-v")
        end
        
        os.vrunv("cargo", args)
    end)

on_clean(function (target)
    local ffi_dir = path.join(os.projectdir(), "3rdparty", "mogan-loro-ffi")
    local target_dir = path.join(ffi_dir, "target")
    cprint("${yellow}clean cargo target: %s", target_dir)
    os.rm(target_dir)
end)
target_end()
