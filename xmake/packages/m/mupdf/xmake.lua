-- 
-- Copyright (C) 2025 The Mogan Stem Authors
-- 
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
-- 
-- http://www.apache.org/licenses/LICENSE-2.0
-- 
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
-- WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
-- License for the specific language governing permissions and limitations
-- under the License.
-- 

package("mupdf")
    set_homepage("https://mupdf.com")
    set_description("MuPDF is an open source software framework for viewing, converting, and manipulating PDF, XPS, and E-book documents.")
    -- MuPDF 1.25.1 Mirror
    add_urls("https://gitee.com/XmacsLabs/mogan/attach_files/2323404/download#mupdf-$(version)-source.tar.gz")
    add_urls("https://mupdf.com/downloads/archive/mupdf-$(version)-source.tar.gz")
    set_license("AGPL-3.0")

    add_versions("1.24.10", "939285b5f97caf770fd46cbe7e6cc3a695ab19bb5bfaf5712904549cef390b7b")
    add_versions("1.25.1", "81aa1361252418cc45347b4ac075532096957a7ab772e20e046f3bb418d7263c")

    if is_plat("linux", "macosx") then
        add_deps("pkg-config", "make", "libjpeg", "freetype", "libcurl", "zlib")
    elseif is_plat("wasm") then
        add_deps("freetype", {configs={png=true}}) -- 保持与 liii-pdfhummus 一致，避免多次安装
        add_deps("pkg-config", "make", "libjpeg", "harfbuzz", "zlib")
    end

    on_load(function (package)
        if not is_plat("windows") then
            if is_plat("linux") then
                package:add("links", "mupdf", "mupdf-third", "harfbuzz")
            elseif is_plat("wasm") then
                package:add("links", "mupdf", "mupdf-third", "harfbuzz", "libjpeg", "freetype", "zlib")
            else
                package:add("links", "mupdf", "mupdf-third")
            end
        end
    end)

    on_install("linux", "macosx", "wasm", function (package)
        if is_plat("macosx") then
            io.writefile("user.make", "CFLAGS = -arch " .. package:targetarch())
            -- Use pkg-config to detect system library
            io.replace("Makerules", "else ifeq ($(LINUX_OR_OPENBSD),yes)", "", {plain = true})
        end
        -- Use system library from xmake to compat with other program
        local configs = {
            "install-libs",
            "USE_SYSTEM_LIBJPEG=yes",
            "USE_SYSTEM_FREETYPE=yes",
            "USE_SYSTEM_ZLIB=yes",
            "USE_SYSTEM_CURL=yes",
            "tofu=yes",
            "tofu_cjk=yes",
            "prefix=" .. package:installdir()
        }
        if not is_plat("wasm") then
            import("package.tools.make").build(package, configs)
        else
            -- xmake seems not passing the right configs to mupdf's make
            local cflags = {}
            local ldflags = {}
            local harfbuzz = package:dep("harfbuzz"):fetch()
            local libjpeg  = package:dep("libjpeg"):fetch()
            local freetype = package:dep("freetype"):fetch()
            local zlib = package:dep("zlib"):fetch()
            local cc = package:tool("cc")
            local cxx = package:tool("cxx")
            local ld = package:tool("ld")
            local ar = package:tool("ar")
            local ranlib = package:tool("ranlib")
            table.insert(configs, "CC=" .. cc)
            table.insert(configs, "CXX=" .. cxx)
            table.insert(configs, "LD=" .. ld)
            table.insert(configs, "AR=" .. ar)
            table.insert(configs, "RANLIB=" .. ranlib)
            table.insert(configs, "USE_SYSTEM_HARFBUZZ=yes")
            table.insert(ldflags, "-DZ_PREFIX=OFF")
            for _, pkg in ipairs({harfbuzz, libjpeg, freetype, zlib}) do
                for _, dir in ipairs(pkg.sysincludedirs or {}) do
                    table.insert(cflags, "-I" .. dir)
                end
                for _, dir in ipairs(pkg.includedirs or {}) do
                    table.insert(cflags, "-I" .. dir)
                end
                for _, dir in ipairs(pkg.linkdirs or {}) do
                    table.insert(ldflags, "-L" .. dir)
                end
            end
            os.execv("make verbose=yes", configs, {
                envs = {
                    CFLAGS  = table.concat(cflags, " "),
                    LDFLAGS = table.concat(ldflags, " ")
                }
            })
            os.execv("make", table.join({"install-libs"}, configs))
        end
    end)

    on_install("windows", function (package)
        local configs = {"platform/win32/mupdf.sln", "-t:libmupdf", "-maxcpucount"}
        local build_type = (package:debug() and "Debug" or "Release")
        local arch = (package:is_arch("x64") and "x64" or "Win32")
        table.insert(configs, "/p:Configuration=" .. build_type)
        table.insert(configs, "/p:Platform=" .. arch)
        io.replace("platform/win32/libmupdf.vcxproj", "%(PreprocessorDefinitions)</PreprocessorDefinitions>", "TOFU;TOFU_CJK;SHARE_JPEG;%(PreprocessorDefinitions)</PreprocessorDefinitions>",{plain = true})
        if package:has_runtime("MT", "MTd") then
            -- Allow MT, MTd
            for i, target in ipairs({"libmupdf.vcxproj", "libextract.vcxproj", "libharfbuzz.vcxproj", "libleptonica.vcxproj", "libpkcs7.vcxproj", "libtesseract.vcxproj", "libthirdparty.vcxproj"}) do 
                io.replace("platform/win32/" .. target, "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>", {plain = true})
                io.replace("platform/win32/" .. target, "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>", "<RuntimeLibrary>MultiThreaded</RuntimeLibrary>", {plain = true})
            end
        end
        import("package.tools.msbuild").build(package, configs)
        os.cp("include/**.h", package:installdir("include"), {rootdir = "include"})
        if arch == "Win32" then
            arch = ""
        end
        local output = format("platform/win32/%s/%s/", arch, build_type)
        os.cp(output .. "libmupdf.lib", package:installdir("lib"))
        os.cp(output .. "libthirdparty.lib", package:installdir("lib"))
    end)