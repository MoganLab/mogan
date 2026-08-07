-------------------------------------------------------------------------------
--
-- MODULE      : stage_velopack.lua
-- DESCRIPTION : 将 stem 安装树装配为 Velopack 扁平暂存根
-- COPYRIGHT   : (C) 2026 Xmacs Labs
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

-------------------------------------------------------------------------------
--
-- 设计说明：
-- Velopack 的 Windows 打包约定要求 --packDir 的根部就是安装后的 current/ 目录，
-- 且 --mainExe 只能是根部的文件名（如 MoganSTEM.exe）。而 xmake install stem
-- 产出的是 bin/ 与 TeXmacs 内容（doc/langs/progs/...）分立的树，vpk pack 无法
-- 直接消费，因此这里把 bin/ 摊平到暂存根、TeXmacs 内容收进暂存根的 TeXmacs/。
-- 为何不沿用 xpack 的 NSIS 目录布局：NSIS 允许任意 --packDir 结构，Velopack
-- 要求主程序位于根部，摊平这一步不可省。
--
-- 兼容两种源布局：
--   1) src/TeXmacs/ 子目录存在（含 doc/langs/progs/...）
--   2) TeXmacs 内容直接摊在 src 根（当前 xmake install stem 在 Windows 的
--      实际产物，见 xmake/targets/stem.lua 的 add_installfiles 前缀捕获）
-- 判断依据：src/TeXmacs 是否为目录；否则按已知条目清单从 src 根收集。
--
-- 本脚本只做装配与校验，不打包；打包见 pack_velopack.lua。
--
-- 环境变量覆盖：
--   VPK_STAGING_SRC  默认 build/packages/stem/data（xmake install stem 产物）
--   VPK_STAGING_OUT  默认 build/velopack_staging
-------------------------------------------------------------------------------

local src = os.getenv ("VPK_STAGING_SRC")
if src == nil or src == "" then src = "build/packages/stem/data" end
local out = os.getenv ("VPK_STAGING_OUT")
if out == nil or out == "" then out = "build/velopack_staging" end

-- xmake l 的工作目录即项目根，但显式求绝对路径更稳妥
local src_dir = path.absolute (path.join (os.projectdir (), src))
local out_dir = path.absolute (path.join (os.projectdir (), out))

-- 源安装树必须存在（xmake install stem 的产物），否则无从装配
if not os.isdir (src_dir) then
    cprint ("${bright red}error: 源安装树不存在: " .. src_dir .. "${clear}")
    cprint ("${yellow}请先执行 xmake install stem 生成安装树${clear}")
    os.exit (1)
end

-- 清空重建暂存根：保证可重复执行，且不含上次残留
if os.exists (out_dir) then
    os.rm (out_dir)
end
os.mkdir (out_dir)

-- 1) 摊平 bin/ 到暂存根：bin/MoganSTEM.exe -> out/MoganSTEM.exe，
--    bin/platforms/qwindows.dll -> out/platforms/qwindows.dll。
--    Qt DLL 与 velopack_libc.dll 必须与主 exe 同根平铺，Qt 插件才能被发现。
local bin_dir = path.join (src_dir, "bin")
if os.isdir (bin_dir) then
    os.cp (path.join (bin_dir, "*"), out_dir)
end

-- 2) 收拢 TeXmacs 内容到 out/TeXmacs/。条目清单覆盖 stem.lua 安装的全部
--    TeXmacs 内容；两种源布局都能从这份清单取到对应条目。
local texmacs_entries = {
    "doc", "fonts", "langs", "misc", "packages", "plugins", "progs",
    "styles", "templates", "tests", "texts",
    "COPYING", "INSTALL", "README", "TEX_FONTS",
}
local tx_src = path.join (src_dir, "TeXmacs")
if not os.isdir (tx_src) then
    -- 当前 Windows install 产物：TeXmacs 内容直接摊在 src 根（bin 除外）
    tx_src = src_dir
end
os.mkdir (path.join (out_dir, "TeXmacs"))
for _, name in ipairs (texmacs_entries) do
    local f = path.join (tx_src, name)
    if os.exists (f) then
        os.cp (f, path.join (out_dir, "TeXmacs", name))
    end
end

-- 3) LICENSE 与图标放根部（有则复制）
for _, name in ipairs ({"LICENSE", "TeXmacs.ico"}) do
    local f = path.join (src_dir, name)
    if os.isfile (f) then
        os.cp (f, path.join (out_dir, name))
    end
end

-- 4) 清单校验：必选文件/目录缺失即失败（硬失败，不静默放行）
local ok = true
if not os.isfile (path.join (out_dir, "MoganSTEM.exe")) then
    cprint ("${bright red}error: 暂存根缺少 MoganSTEM.exe${clear}")
    ok = false
end
for _, sub in ipairs ({"progs", "plugins", "fonts"}) do
    local d = path.join (out_dir, "TeXmacs", sub)
    if not os.isdir (d) then
        cprint ("${bright red}error: 暂存根缺少 TeXmacs/" .. sub .. "${clear}")
        ok = false
    end
end

-- 5) 全树扫描：运行期日志/构建中间产物（.log/.tmp/~ 结尾、.git 目录）不得发布。
--    用逐目录递归而不是 **/* 批量 glob：实测 xmake 的 **/* 会漏掉深层大文件
--    （如 fonts/opentype/noto 下的 CJK 字体），逐目录扫描才是可靠全集。
local all_files = {}
local forbidden = {}
local total_bytes = 0
local function scan (d)
    for _, f in ipairs (os.files (path.join (d, "*"))) do
        table.insert (all_files, f)
        total_bytes = total_bytes + (os.filesize (f) or 0)
        local name = path.filename (f)
        if name:match ("%.log$") or name:match ("%.tmp$") or name:match ("~$") then
            table.insert (forbidden, f)
        end
    end
    for _, sub in ipairs (os.dirs (path.join (d, "*"))) do
        -- .git 是版本库元数据目录，绝对不允许进入安装树
        if path.filename (sub) == ".git" then
            table.insert (forbidden, sub)
        end
        scan (sub)
    end
end
scan (out_dir)
if #forbidden > 0 then
    for _, f in ipairs (forbidden) do
        cprint ("${bright red}error: 含禁止发布内容: " .. f .. "${clear}")
    end
    os.exit (1)
end

-- 6) 顶层条目应只有主 exe / TeXmacs / LICENSE / TeXmacs.ico + bin 摊平的
--    DLL 与插件子目录。此处仅告警不失败：MoganSTEM.pdb 等构建中间产物会在
--    vpk pack 阶段被默认 --exclude .*\.pdb 剔除。
local named = {
    ["MoganSTEM.exe"] = true, ["TeXmacs"] = true,
    ["LICENSE"] = true, ["TeXmacs.ico"] = true,
}
for _, f in ipairs (os.files (path.join (out_dir, "*"))) do
    local name = path.filename (f)
    if not named[name] and not name:match ("%.dll$") and not name:match ("%.exe$") then
        cprint ("${yellow}warn: 顶层意外文件（需人工确认是否随包发布）: " .. name .. "${clear}")
    end
end

if not ok then
    os.exit (1)
end

-- 7) 汇总输出
local top_entries = #os.files (path.join (out_dir, "*")) + #os.dirs (path.join (out_dir, "*"))
cprint ("${green}staging 完成: " .. out_dir .. "${clear}")
cprint ("  顶层条目数: " .. top_entries)
cprint ("  文件数: " .. #all_files)
cprint (string.format ("  总大小: %.1f MB", total_bytes / 1048576))
