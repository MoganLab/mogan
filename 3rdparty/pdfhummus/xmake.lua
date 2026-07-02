-- originally from https://github.com/xmake-io/xmake-repo/pull/1709

option("libtiff", {description = "Enable libtiff", default = false})
option("libpng", {description = "Enable libpng", default = false})
option("libjpeg", {description = "Enable libjpeg", default = false})
option("openssl", {description = "Enable openssl", default = false})
add_rules("mode.debug", "mode.release")
if has_config("libtiff") then
    add_requires("libtiff")
end
if has_config("libpng") then
    add_requires("libpng")
end
if has_config("libjpeg") then
    add_requires("libjpeg")
end
if has_config("openssl") then
    add_requires("openssl")
end
add_requires("freetype", "zlib", "liii-libaesgm")
target("pdfhummus")
    set_kind("$(kind)")
    add_files("PDFWriter/*.cpp")
    add_headerfiles("(PDFWriter/*.h)")
    add_packages("freetype")
    add_packages("libtiff", "libpng", "libjpeg", "openssl")
    add_packages("liii-libaesgm", "zlib")
    if has_package("libtiff") then
        add_defines("_INCLUDE_TIFF_HEADER")
        add_cxflags("-Wno-deprecated-declarations")
    else
        add_defines("PDFHUMMUS_NO_TIFF=1")
    end
    if not has_package("libpng") then
        add_defines("PDFHUMMUS_NO_PNG=1")
    end
    if not has_package("libjpeg") then
        add_defines("PDFHUMMUS_NO_DCT=1")
    end
    if not has_package("openssl") then
        add_defines("PDFHUMMUS_NO_OPENSSL=1")
        -- v4.9.0 引入了基于 OpenSSL 的 AES 流（InputAESDecodeStreamSSL /
        -- OutputAESEncodeStreamSSL），其头文件顶部无条件 #include <openssl/evp.h>。
        -- 与上游 PDFWriter/CMakeLists.txt 保持一致：未启用 OpenSSL 时直接跳过这两个
        -- 文件，改用纯软件实现的 Input/OutputAESEncodeStream。
        remove_files("PDFWriter/InputAESDecodeStreamSSL.cpp")
        remove_files("PDFWriter/OutputAESEncodeStreamSSL.cpp")
    end
    -- port symbols for linker
    if is_plat("windows") and is_kind("shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

