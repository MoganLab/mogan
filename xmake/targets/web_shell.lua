-------------------------------------------------------------------------------
--
-- MODULE      : web_shell.lua
-- DESCRIPTION : Drive the React/Vite shell under web/ from xmake.
--               On WASM builds `stem` depends on this target, so `xmake build
--               stem` runs `npm install && npm run build` automatically;
--               `xmake clean` removes the npm build cache (dist, node_modules,
--               *.tsbuildinfo).
-- COPYRIGHT   : (C) 2026       JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

target("web_shell")
    set_kind("phony")
    -- The shell only ships with the WASM build; other plats skip it entirely.
    set_enabled(is_plat("wasm"))

    local web_dir = path.join(os.projectdir(), "web")

    on_build(function (target)
        -- Hard fail if the JS toolchain is missing: we don't want a silent
        -- fallback to the legacy stem.html when xmake owns the web build.
        import("lib.detect.find_tool")

        local node = find_tool("node")
        local npm  = find_tool("npm")

        assert(node, "node not found in PATH (required to build web/)")
        assert(npm,  "npm not found in PATH (required to build web/)")

        cprint("${yellow}building web shell (npm)")

        -- Smart incremental install: only run `npm install` when node_modules
        -- is missing or package-lock.json is newer than the installed copy.
        -- npm writes node_modules/.package-lock.json as an install stamp.
        local stamp = path.join(web_dir, "node_modules", ".package-lock.json")
        local need_install = not os.isfile(stamp)
        if not need_install then
            local lock = path.join(web_dir, "package-lock.json")
            if os.mtime(lock) > os.mtime(stamp) then
                need_install = true
            end
        end
        if need_install then
            os.vrunv("npm", {"install"}, {curdir = web_dir})
        end

        os.vrunv("npm", {"run", "build"}, {curdir = web_dir})
    end)

    on_clean(function (target)
        -- Clear the full npm build cache: dist (build output), node_modules
        -- (installed deps) and the TypeScript incremental build info files.
        for _, sub in ipairs({"dist", "node_modules"}) do
            local p = path.join(web_dir, sub)
            if os.exists(p) then
                cprint("${yellow}cleaning %s", p)
                os.rm(p)
            end
        end
        os.rm(path.join(web_dir, "tsconfig.tsbuildinfo"))
        os.rm(path.join(web_dir, "tsconfig.node.tsbuildinfo"))
    end)
target_end()
