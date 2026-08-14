-------------------------------------------------------------------------------
--
-- MODULE      : pack_velopack.lua
-- DESCRIPTION : 调用 vpk pack 生成 Velopack 发布（Setup.exe / releases.*.json / packages/）
-- COPYRIGHT   : (C) 2026 Xmacs Labs
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

-------------------------------------------------------------------------------
--
-- 设计说明：
-- 常规 Windows 发布已由 vpk pack 接管（NSIS 仅迁移期桥接保留，见 xpack.lua）。
-- vpk pack 把 --packDir 的扁平目录打成安装包与全量/增量包，并生成
-- releases.<channel>.json 供客户端查询更新。
--
-- 关于签名：CI/本地默认不签名（VPK_SIGN_PARAMS 为空）。正式签名在 SafeNet
-- 签名机上通过环境变量 VPK_SIGN_PARAMS 注入 signtool 参数完成。SafeNet
-- 交互式令牌（interactive token）不允许并发签名，签名机上须固定
-- --signParallel 1；该参数与 --signParams 一起由签名机脚本传入，这里只
-- 注释说明，不写死进默认命令。
--
-- 关于 delta：--delta 保持默认 BestSpeed，当 outputDir 已存在上一个版本时会
-- 自动生成增量包；首次发布没有上一个版本，自然只有全量包。
-- 关于同版本重打：vpk 拒绝 channel 内已有 ≥ 当前 packVersion 的包（防误覆盖已发布
-- 版本）。本地重复验证若需重打同一 packVersion，须先删除 outputDir 中的上一发布再 pack。
--
-- 关于 MSI：--msi 暂不启用（默认 False）。按机器安装的 MSI 属独立渠道决策，
-- 当前仍走每用户 Setup.exe 通道。
--
-- 关于 VC++ 运行库：Qt DLL（Qt6Core.dll 等）为 /MD，依赖 VC++ 14.3（VS2022）
-- x64 运行库。stage_velopack.lua 已从官方 vc_redist.x64.exe 提取运行库 DLL
-- （vcruntime140/vcruntime140_1/msvcp140 等）放进暂存根，与 Qt DLL 同根做
-- app-local 部署：安装期不联网、无额外安装器步骤。因此这里不使用 vpk 的
-- --framework vcredist143-x64（那会让 Setup.exe 在目标机缺库时从微软 CDN
-- 下载并安装运行库）。
--
-- 环境变量覆盖：
--   VPK_PATH         vpk 可执行文件；默认自动定位
--   VPK_PACK_DIR     默认 build/velopack_staging
--   VPK_OUTPUT_DIR   默认 build/velopack_release
--   VPK_CHANNEL      默认 stable
--   VPK_VERSION      默认从 xmake/vars.lua 解析 XMACS_VERSION
--   VPK_SIGN_PARAMS  默认空字符串；签名机注入 signtool 参数
--   VPK_PRUNE_ONLY   只跑后处理（清理历史 full 包 + manifest），不重新 pack
-------------------------------------------------------------------------------

-- 定位 vpk：显式环境变量优先，其次常见安装路径，最后回退到 PATH 上的 vpk
local vpk = os.getenv ("VPK_PATH")
if vpk == nil or vpk == "" then
    local candidates = {
        path.join (os.getenv ("USERPROFILE") or "", ".dotnet/tools/vpk.exe"),
        path.join ("C:/Program Files/dotnet/tools/vpk.exe"),
    }
    for _, c in ipairs (candidates) do
        if os.isfile (c) then
            vpk = c
            break
        end
    end
    if vpk == nil or vpk == "" then
        vpk = "vpk"
    end
end

-- 解析版本号：从 xmake/vars.lua 的 XMACS_VERSION 取值，保证与产物版本一致
local version = os.getenv ("VPK_VERSION")
if version == nil or version == "" then
    local f = io.open (path.join (os.projectdir (), "xmake/vars.lua"), "r")
    if f == nil then
        cprint ("${bright red}error: 无法读取 xmake/vars.lua${clear}")
        os.exit (1)
    end
    local content = f:read ("*a")
    f:close ()
    version = content:match ('XMACS_VERSION%s*=%s*"(.-)"')
    if version == nil then
        cprint ("${bright red}error: xmake/vars.lua 中未找到 XMACS_VERSION${clear}")
        os.exit (1)
    end
end

local pack_dir = os.getenv ("VPK_PACK_DIR")
if pack_dir == nil or pack_dir == "" then pack_dir = "build/velopack_staging" end
local out_dir = os.getenv ("VPK_OUTPUT_DIR")
if out_dir == nil or out_dir == "" then out_dir = "build/velopack_release" end
local channel = os.getenv ("VPK_CHANNEL")
if channel == nil or channel == "" then channel = "stable" end
local sign_params = os.getenv ("VPK_SIGN_PARAMS") or ""
local prune_only  = os.getenv ("VPK_PRUNE_ONLY") == "1"

pack_dir = path.absolute (path.join (os.projectdir (), pack_dir))
out_dir = path.absolute (path.join (os.projectdir (), out_dir))
local icon = path.absolute (path.join (os.projectdir (), "packages/windows/Xmacs.ico"))

-- 先决条件：暂存根与图标必须存在（prune-only 模式跳过，只依赖 outputDir）
if not prune_only then
if not os.isdir (pack_dir) then
    cprint ("${bright red}error: 暂存目录不存在: " .. pack_dir .. "${clear}")
    cprint ("${yellow}请先运行 xmake l tools/release/stage_velopack.lua${clear}")
    os.exit (1)
end
if not os.isfile (path.join (pack_dir, "MoganSTEM.exe")) then
    cprint ("${bright red}error: 暂存根缺少 MoganSTEM.exe: " .. pack_dir .. "${clear}")
    os.exit (1)
end
if not os.isfile (icon) then
    cprint ("${bright red}error: 图标不存在: " .. icon .. "${clear}")
    os.exit (1)
end

-- 输出目录由 vpk 创建，但先确保父目录存在，避免歧义
os.mkdir (out_dir)

-- 命令参数固定为 Velopack 锁定集；--signParams 仅在非空时追加
local args = {
    "pack",
    "--packId", "Mogan",
    "--packVersion", version,
    "--packDir", pack_dir,
    "--mainExe", "MoganSTEM.exe",
    "--channel", channel,
    "--outputDir", out_dir,
    "--runtime", "win-x64",
    "--packTitle", "Mogan STEM",
    "--packAuthors", "Xmacs Labs",
    "--icon", icon,
}
if sign_params ~= "" then
    table.insert (args, "--signParams")
    table.insert (args, sign_params)
end

cprint ("${cyan}vpk pack 调用:${clear}")
for _, a in ipairs (args) do
    print ("  " .. a)
end
print ("")

-- try=true：非零退出不抛异常，由本脚本统一报错退出
local code = os.execv (vpk, args, {try = true})
if code ~= 0 then
    cprint ("${bright red}error: vpk pack 失败，退出码 " .. code .. "${clear}")
    os.exit (1)
end

cprint ("${green}vpk pack 完成: " .. out_dir .. "${clear}")

-- 发布物改名：沿用旧 NSIS 命名 MoganSTEM-v<版本>-64bit-<渠道>-Setup/Portable。
-- vpk 默认名是 <packId>-<channel>-Setup.exe / -Portable.zip；改名后同步更新
-- assets.<channel>.json 里的 Installer/Portable 引用，避免下载链接失效。
local function rename_asset (old_name, new_name)
    local old_path= path.join (out_dir, old_name)
    local new_path= path.join (out_dir, new_name)
    if os.isfile (old_path) then
        if os.isfile (new_path) then
            os.rm (new_path) -- 同名残留先清掉，避免 os.mv 目标已存在失败
        end
        os.mv (old_path, new_path)
        cprint ("${green}已改名: " .. new_name .. "${clear}")
        return true
    end
    return false
end

local setup_old   = "Mogan-" .. channel .. "-Setup.exe"
local setup_new   = "MoganSTEM-v" .. version .. "-64bit-" .. channel .. "-Setup.exe"
local portable_old= "Mogan-" .. channel .. "-Portable.zip"
local portable_new= "MoganSTEM-v" .. version .. "-64bit-" .. channel .. "-Portable.zip"

if not rename_asset (setup_old, setup_new) then
    cprint ("${bright red}error: 未找到 Setup.exe: " .. path.join (out_dir, setup_old) .. "${clear}")
    os.exit (1)
end
if not rename_asset (portable_old, portable_new) then
    cprint ("${bright red}error: 未找到 Portable.zip: " .. path.join (out_dir, portable_old) .. "${clear}")
    os.exit (1)
end

-- 同步 assets.<channel>.json 中的文件名引用
local assets_file= path.join (out_dir, "assets." .. channel .. ".json")
if os.isfile (assets_file) then
    local content= io.readfile (assets_file) or ""
    local function swap_name (old_name, new_name)
        -- Lua 模式中 - . 是 magic 字符，先转义再替换
        local escaped= old_name:gsub ("([%^%$%(%)%%%.%[%]%*%+%-%?])", "%%%1")
        content= content:gsub (escaped, function () return new_name end)
    end
    swap_name (setup_old, setup_new)
    swap_name (portable_old, portable_new)
    io.writefile (assets_file, content)
end
end -- if not prune_only

-------------------------------------------------------------------------------
-- 后处理：清理发布目录中的历史 full 包，release 只留当前版本产物。
-- vpk pack 把 outputDir 当作 channel 累积目录：上一版本 full 会留在 outputDir
-- 并被写进 releases.<channel>.json。那份旧 full 只是算 delta 的基线，客户端
-- 走 delta 用的是本地 packages 目录里的旧 full（见联调日志），feed 上的旧 full
-- 不会被拉取，纯属冗余（回滚另走 OSS 保留策略）。这里删掉旧 full 文件并在
-- manifest 中去掉对应条目，只保留当前版本 full 与历史 delta。outputDir 仍
-- 保留当前版本 full，作为下一次 pack 的 delta 基线，不受影响。
-------------------------------------------------------------------------------
local function prune_old_full (out_dir, channel, version)
    local json = import ("core.base.json")
    local current_full = "Mogan-" .. version .. "-" .. channel .. "-full.nupkg"

    -- 1) 删除旧版本 full 包文件（当前版本 full 保留，作下次 delta 基线）
    for _, f in ipairs (os.files (path.join (out_dir, "Mogan-*-" .. channel .. "-full.nupkg"))) do
        if path.filename (f) ~= current_full then
            os.rm (f)
            cprint ("${yellow}已清理历史 full: " .. path.filename (f) .. "${clear}")
        end
    end

    -- 2) releases.<channel>.json 去掉旧 full 条目，保留当前 full 与全部 delta。
    -- 注意 xmake lua 沙箱无 pcall / try-catch（见本文件 git 历史），防御靠
    -- 内容结构检查：缺 "Assets" 就跳过，vpk 生成的 manifest 正常情况必然合法。
    local rel_file= path.join (out_dir, "releases." .. channel .. ".json")
    if os.isfile (rel_file) then
        local content= io.readfile (rel_file) or ""
        if content:find ('"Assets"') then
            local data= json.decode (content)
            if type (data) == "table" and type (data.Assets) == "table" then
                local kept= {}
                for _, a in ipairs (data.Assets) do
                    if a.Type ~= "Full" or a.Version == version then
                        table.insert (kept, a)
                    end
                end
                data.Assets= kept
                io.writefile (rel_file, json.encode (data))
                cprint ("${green}releases." .. channel .. ".json 已去除历史 full 条目${clear}")
            end
        else
            cprint ("${yellow}warning: " .. path.filename (rel_file) .. " 缺少 Assets 结构，跳过 manifest 清理${clear}")
        end
    end
end

-- 打包完成后做后处理；VPK_PRUNE_ONLY=1 时独立重跑清理
prune_old_full (out_dir, channel, version)
