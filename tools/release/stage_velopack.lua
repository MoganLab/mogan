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
-- 且 --mainExe 只能是根部的文件名（如 MoganSTEM.exe）。因此主 exe、Qt DLL、
-- Qt 插件子目录（platforms/ qml/ ...）必须与数据同根平铺。
-- 应用按 $TEXMACS_PATH/progs、$TEXMACS_PATH/doc、$TEXMACS_PATH/fonts 等**扁平**
-- 查找资源（见 init_texmacs.cpp 的 TEXMACS_PATH 探测），所以 TeXmacs 内容直接拷到
-- 暂存根，不能收进 TeXmacs/ 包装目录，否则 $TEXMACS_PATH/progs 永远解析失败。
-- 辅助二进制（pandoc.exe 等）保留在暂存根 bin/ 子目录；vc_redist.x64.exe
-- 只作为运行库提取源使用、不随包携带：Qt DLL 为 /MD，依赖 VC++ 14.3 运行库，
-- 本脚本从官方 vc_redist.x64.exe 提取 vcruntime140/msvcp140 等 DLL 放进暂存根
-- 做 app-local 部署（见步骤 4），安装期无需联网。该文件由 Qt 部署
-- （windeployqt）在 xmake install 时从构建机 VS 的 VC\Redist 目录自动拷入
-- 安装树 bin/（与旧 NSIS 流程一致）。find-binary 与 pandoc 等按
-- $TEXMACS_PATH/bin 查找；.pdb 调试符号不发布。
-- 目录拷贝一律用内容合并语义（merge_copy）：Qt 插件子目录（如 styles/）与同名
-- 数据目录合并同层，避免 os.cp 在 dst 已存在时整目录嵌套成 dst/basename(src)。
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

-- 目录内容合并拷贝：逐条目复制，同名子目录递归合并。
-- 不能直接用 os.cp(src_dir, dst_dir)：dst 已存在时 xmake 会整目录嵌套成
-- dst/basename(src)（bin/styles 与数据 styles 撞名即因此产生 out/styles/styles/）。
-- 文件过滤掉 *.pdb（debug 符号属构建中间产物，不进暂存根）。
local function merge_copy (src, dst)
    os.mkdir (dst)
    for _, f in ipairs (os.files (path.join (src, "*"))) do
        local name = path.filename (f)
        if not name:match ("%.pdb$") then
            os.cp (f, path.join (dst, name))
        end
    end
    for _, sub in ipairs (os.dirs (path.join (src, "*"))) do
        merge_copy (sub, path.join (dst, path.filename (sub)))
    end
end

-- 1) bin/ 拆分：
--    - 顶层目录（platforms/ qml/ 等 Qt 插件子目录）用内容合并语义拷到暂存根：
--      Qt 插件子目录（如 styles/）与同名数据目录合并同层，避免 os.cp 整目录嵌套
--    - 顶层 MoganSTEM.exe 与 *.dll（含 velopack_libc.dll、d3dcompiler、dxcompiler、
--      dxil）拷到暂存根 —— Qt DLL 与主 exe 同根平铺，Qt 插件才能被发现
--    - 顶层 *.pdb 跳过（调试符号不发布；vpk pack 默认 --exclude .*\.pdb 也会剔除）
--    - 顶层其他文件（pandoc.exe 等辅助程序）拷入暂存根 bin/ 子目录，find-binary
--      与 pandoc 等按 $TEXMACS_PATH/bin 查找；vc_redist.x64.exe 明确排除
--      （仅作运行库提取源，见步骤 4）
local bin_dir = path.join (src_dir, "bin")
if os.isdir (bin_dir) then
    os.mkdir (path.join (out_dir, "bin"))
    for _, f in ipairs (os.files (path.join (bin_dir, "*"))) do
        local name = path.filename (f)
        if name:lower () == "vc_redist.x64.exe" then
            -- 不随包携带：只用作 app-local 运行库提取源（见步骤 4），
            -- 随包只是 ~25MB 死文件
        elseif name:match ("%.pdb$") then
            -- 调试符号不发布
        elseif name:match ("%.dll$") or name == "MoganSTEM.exe" then
            os.cp (f, path.join (out_dir, name))
        else
            os.cp (f, path.join (out_dir, "bin", name))
        end
    end
    for _, d in ipairs (os.dirs (path.join (bin_dir, "*"))) do
        merge_copy (d, path.join (out_dir, path.filename (d)))
    end
end

-- 2) 数据平铺到暂存根：不再建 out/TeXmacs 包装目录，每个条目直接拷到 out/<name>。
--    应用按 $TEXMACS_PATH/progs、$TEXMACS_PATH/doc、$TEXMACS_PATH/fonts 等扁平查找，
--    TeXmacs/ 包装目录会让 $TEXMACS_PATH/progs 永远解析失败。
-- tests/ 不发布：TeXmacs/tests/{tm,tex,bib} 是格式回归样例文档，仅开发/测试用，
-- 进发布物徒增体积且无运行期作用，故从 texmacs_entries 清单中排除。
local texmacs_entries = {
    "doc", "fonts", "langs", "misc", "packages", "plugins", "progs",
    "styles", "templates", "texts",
    "COPYING", "INSTALL", "README", "TEX_FONTS",
}
local tx_src = path.join (src_dir, "TeXmacs")
if not os.isdir (tx_src) then
    -- 当前 Windows install 产物：TeXmacs 内容直接摊在 src 根（bin 除外）
    tx_src = src_dir
end
for _, name in ipairs (texmacs_entries) do
    local f = path.join (tx_src, name)
    if os.isdir (f) then
        -- 目录条目用内容合并语义：与 bin/ 拆分落下的同名目录（如 styles/）合并同层
        merge_copy (f, path.join (out_dir, name))
    elseif os.isfile (f) then
        -- 文件条目（COPYING/INSTALL/README/TEX_FONTS）无嵌套问题，直接拷到根
        os.cp (f, path.join (out_dir, name))
    end
end

-- 3) LICENSE 与图标放根部（有则复制）
for _, name in ipairs ({"LICENSE", "TeXmacs.ico"}) do
    local f = path.join (src_dir, name)
    if os.isfile (f) then
        os.cp (f, path.join (out_dir, name))
    end
end

-- 4) VC++ 运行库 app-local 部署：Qt DLL（Qt6Core.dll 等）为 /MD，依赖
--    VC++ 14.3（VS2022）x64 运行库。从官方 vc_redist.x64.exe（源安装树 bin/
--    下的文件，由 Qt 部署 windeployqt 从构建机 VS 的 VC\Redist 目录自动拷入，
--    版本即构建机 VS 自带的运行库版本）提取运行库 DLL 放进暂存根、与 Qt DLL
--    同根平铺，安装期不联网、不需要任何安装器步骤。提取依赖 dark.exe（WiX，
--    scoop main bucket）与 msiexec /a（管理安装，只解文件不落系统）。
--    vc_redist.x64.exe 缺失时硬失败。
local vc_redist = path.join (bin_dir, "vc_redist.x64.exe")
if os.isfile (vc_redist) then
    local work = path.join (os.projectdir (), "build/velopack_crt_work")
    if os.exists (work) then os.rm (work) end
    os.mkdir (work)
    -- payload 二进制实际引用的运行库 DLL（导入扫描结果）
    local crt_dlls = {
        "vcruntime140.dll", "vcruntime140_1.dll",
        "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll",
        "msvcp140_atomic_wait.dll",
    }
    -- dark 解包 Burn 引导器，取出内嵌 MSI
    local code = os.execv ("dark", {vc_redist, "-x", work}, {try = true})
    if code ~= 0 then
        cprint ("${bright red}error: dark 解包 vc_redist.x64.exe 失败，退出码 " .. code .. "${clear}")
        os.exit (1)
    end
    -- 定位 x64 Minimum MSI（运行库 DLL 都在其中；Additional 只有 MFC，不需要）
    local msi = nil
    for _, f in ipairs (os.files (path.join (work, "AttachedContainer", "packages", "*", "*"))) do
        local n = path.filename (f):lower ()
        if n:match ("vc_runtimeminimum") and n:match ("%.msi$") and n:match ("x64") then
            msi = f
            break
        end
    end
    if msi == nil then
        cprint ("${bright red}error: vc_redist.x64.exe 中未找到 x64 Minimum MSI${clear}")
        os.exit (1)
    end
    -- msiexec /a 管理安装：仅把 MSI 内容解到临时目录，不写系统
    local msi_out = path.join (work, "msi")
    code = os.execv ("msiexec.exe", {"/a", msi, "/qn", "TARGETDIR=" .. msi_out}, {try = true})
    if code ~= 0 then
        cprint ("${bright red}error: msiexec /a 提取运行库失败，退出码 " .. code .. "${clear}")
        os.exit (1)
    end
    local sys64 = path.join (msi_out, "System64")
    for _, n in ipairs (crt_dlls) do
        local f = path.join (sys64, n)
        if os.isfile (f) then
            os.cp (f, path.join (out_dir, n))
        else
            cprint ("${bright red}error: 运行库缺少 " .. n .. "${clear}")
            os.exit (1)
        end
    end
    os.rm (work)
    cprint ("${green}app-local 运行库已提取: " .. #crt_dlls .. " 个 DLL${clear}")
else
    cprint ("${bright red}error: 缺少 vc_redist.x64.exe: " .. vc_redist .. "${clear}")
    cprint ("${yellow}该文件由 Qt 部署（windeployqt）在 xmake install 时从构建机 VS 的${clear}")
    cprint ("${yellow}VC\\Redist 目录自动拷入；若缺失，请确认在 Windows + MSVC 下执行了 xmake install stem，${clear}")
    cprint ("${yellow}或手动从 VS 的 VC\\Redist 拷贝一份到源安装树 bin/ 目录${clear}")
    os.exit (1)
end

-- 5) 清单校验：必选文件/目录缺失即失败（硬失败，不静默放行）
local ok = true
if not os.isfile (path.join (out_dir, "MoganSTEM.exe")) then
    cprint ("${bright red}error: 暂存根缺少 MoganSTEM.exe${clear}")
    ok = false
end
for _, sub in ipairs ({"progs", "plugins", "fonts"}) do
    local d = path.join (out_dir, sub)
    if not os.isdir (d) then
        cprint ("${bright red}error: 暂存根缺少扁平数据目录 " .. sub .. "${clear}")
        ok = false
    end
end

-- 6) 全树扫描：运行期日志/构建中间产物（.log/.tmp/.pdb/~ 结尾、.git 目录）不得发布。
--    merge_copy 已过滤 pdb，此处 %.pdb$ 是硬失败兜底：正常源树不应再出现 pdb，
--    出现即说明拷贝逻辑有漏洞。用逐目录递归而不是 **/* 批量 glob：实测 xmake 的
--    **/* 会漏掉深层大文件（如 fonts/opentype/noto 下的 CJK 字体），逐目录扫描才是
--    可靠全集。
local all_files = {}
local forbidden = {}
local total_bytes = 0
local function scan (d)
    for _, f in ipairs (os.files (path.join (d, "*"))) do
        table.insert (all_files, f)
        total_bytes = total_bytes + (os.filesize (f) or 0)
        local name = path.filename (f)
        if name:match ("%.log$") or name:match ("%.tmp$") or name:match ("%.pdb$") or name:match ("~$") then
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

-- 7) 顶层条目应只有主 exe / bin / LICENSE / TeXmacs.ico + 平铺的 DLL、
--    数据目录与 Qt 插件子目录。此处仅告警不失败：MoganSTEM.pdb 等构建中间产物
--    会被 stage 显式跳过，vpk pack 阶段也会被默认 --exclude .*\.pdb 剔除。
local named = {
    ["MoganSTEM.exe"] = true, ["bin"] = true,
    ["LICENSE"] = true, ["TeXmacs.ico"] = true,
    ["COPYING"] = true, ["INSTALL"] = true, ["README"] = true, ["TEX_FONTS"] = true,
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

-- 8) 汇总输出
local top_entries = #os.files (path.join (out_dir, "*")) + #os.dirs (path.join (out_dir, "*"))
cprint ("${green}staging 完成: " .. out_dir .. "${clear}")
cprint ("  顶层条目数: " .. top_entries)
cprint ("  文件数: " .. #all_files)
cprint (string.format ("  总大小: %.1f MB", total_bytes / 1048576))
