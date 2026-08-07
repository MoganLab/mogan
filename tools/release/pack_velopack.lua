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
-- 环境变量覆盖：
--   VPK_PATH         vpk 可执行文件；默认自动定位
--   VPK_PACK_DIR     默认 build/velopack_staging
--   VPK_OUTPUT_DIR   默认 build/velopack_release
--   VPK_CHANNEL      默认 stable
--   VPK_VERSION      默认从 xmake/vars.lua 解析 XMACS_VERSION
--   VPK_SIGN_PARAMS  默认空字符串；签名机注入 signtool 参数
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

pack_dir = path.absolute (path.join (os.projectdir (), pack_dir))
out_dir = path.absolute (path.join (os.projectdir (), out_dir))
local icon = path.absolute (path.join (os.projectdir (), "packages/windows/Xmacs.ico"))

-- 先决条件：暂存根与图标必须存在
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
