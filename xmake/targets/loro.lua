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
    if is_plat("wasm") then
        add_packages("rustup")
    else
        add_packages("rust")
    end
    local ffi_dir = path.join(os.projectdir(), "3rdparty", "mogan-loro-ffi")
    local profile = (is_mode("release") or is_mode("releasedbg"))
                        and "release"
                        or "dev"
    local subdir = (profile == "release")
                        and "release"
                        or "debug"
    -- WASM 需交叉编译到 wasm32-unknown-emscripten，产物在独立的 target 子目录；
    -- native 走默认 target/<profile>
    local rust_target = is_plat("wasm") and "wasm32-unknown-emscripten" or nil
    if rust_target then
        subdir = path.join(rust_target, subdir)
    end

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

    before_build(function(target)
        cprint("${yellow}setting up rust toolchain")
        if is_plat("wasm") then
            local rust_version = "1.87.0" 
            os.vrunv("rustup", {"toolchain", "install", rust_version})
            os.vrunv("rustup", {"default", rust_version})
            os.vrunv("rustup", {"target", "add", rust_target})
        end
    end)

    on_build(function (target)
        import("core.base.option")
        cprint("${yellow}building loro in rust")

        local args = {"build", "--manifest-path", path.join(ffi_dir, "Cargo.toml")}
        table.join2(args, {"--profile", profile})
        if rust_target then
            table.join2(args, {"--target", rust_target})
        end

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
