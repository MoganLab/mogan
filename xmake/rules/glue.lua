-- rule: mogan.glue
-- 把 src/Scheme/**/glue_*.lua 声明生成 .cpp。
--
-- 两种模式（由声明文件里 standalone = true 切换）：
--   1. 文本包含（默认）：产物只放到 includedirs 暴露的目录，供源文件
--      `#include "glue_xxx.cpp"` 文本包含。不进 objectfiles。
--   2. 独立编译（standalone = true）：产物自带完整 #include 前缀，作为
--      独立编译单元注入 target:objectfiles()，源文件不再 #include 它。
--
-- 不用 set_extensions（避免污染所有 .lua），改由 xmake.lua 里
--   add_files("src/Scheme/**/glue_*.lua", {rule="mogan.glue"})
-- 显式绑定。

rule("mogan.glue")
    on_load(function (target)
        target:add("includedirs", path.join(target:autogendir(), "glue"))
    end)

    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        local build_glue_path = path.join(os.projectdir(), "src", "Scheme", "Glue", "build_glue.lua")
        local glue_name = path.basename(sourcefile)                          -- glue_basic.lua -> glue_basic
        local out_cpp   = path.join(target:autogendir(), "glue", glue_name .. ".cpp")

        batchcmds:add_depfiles(sourcefile, build_glue_path)
        batchcmds:mkdir(path.directory(out_cpp))
        batchcmds:show_progress(opt.progress, "${color.build.object}generating.glue %s", sourcefile)

        local build_glue = import("build_glue", {rootdir = path.directory(build_glue_path)})
        local glue_table = import(glue_name, {rootdir = path.directory(sourcefile)})()
        io.writefile(out_cpp, build_glue(glue_table, glue_name))

        if glue_table.standalone then
            -- 独立编译单元：注入 target 的 object 流
            local objectfile = target:objectfile(out_cpp)
            table.insert(target:objectfiles(), objectfile)
            batchcmds:compile(out_cpp, objectfile)
            batchcmds:set_depmtime(os.mtime(objectfile))
            batchcmds:set_depcache(target:dependfile(objectfile))
        else
            -- 文本包含：产物只作为 #include 目标，不独立编译
            batchcmds:set_depmtime(os.mtime(out_cpp))
            batchcmds:set_depcache(target:dependfile(out_cpp))
        end
    end)
