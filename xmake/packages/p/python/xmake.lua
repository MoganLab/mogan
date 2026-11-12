-- 本地覆盖 xmake-repo 的 python 配方：
-- Mogan 及其构建工具链（emscripten、meson 等）仅需将 python/python3 作为命令行工具运行，
-- 从不链接 libpython 或引用 Python.h。
-- 上游 xmake-repo 默认将 python 作为 C/C++ 库管理，当环境未提供 Python 头文件时会尝试从源码
-- 构建 Python 3.14，在 macOS runner 上因 OpenSSL 符号不兼容报错并超时挂起（见 #4779）。
-- 此处声明为 binary 类型，直接探测系统或 runner 环境中的可执行文件。
package("python")
    set_kind("binary")
    set_homepage("https://www.python.org/")
    set_description("The python programming language.")
    set_license("PSF")

    on_fetch(function (package, opt)
        import("lib.detect.find_program")
        import("lib.detect.find_programver")

        local opt_search = {paths = {"/opt/homebrew/bin", "/usr/local/bin", "$(env PATH)"}}
        local candidates = {
            find_program("python3", opt_search),
            find_program("python", opt_search)
        }
        for _, program in ipairs(candidates) do
            if program then
                local ok = try { function () return os.iorunv(program, {"-m", "pip", "--version"}) end }
                if ok then
                    local version = find_programver(program, {command = "-V"})
                    package:addenv("PATH", path.directory(program))
                    return {program = program, version = version}
                end
            end
        end
        local fallback = find_program("python3", opt_search) or find_program("python", opt_search)
        if fallback then
            local version = find_programver(fallback, {command = "-V"})
            package:addenv("PATH", path.directory(fallback))
            return {program = fallback, version = version}
        end
    end)
