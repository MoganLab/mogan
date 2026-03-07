-- Claw AI 构建配置
-- 添加到 mogan/xmake.lua 中

-- ============================================================================
-- Claw AI Widget
-- ============================================================================

target("claw_ai_widget")
    set_kind("object")
    set_enabled(true)
    
    add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
    add_files("src/Plugins/Qt/claw_ai_glue.cpp")
    
    add_includedirs("src/Plugins/Qt")
    add_includedirs("src/Scheme")
    add_includedirs("src/Scheme/Scheme")
    
    add_packages("qt6widgets")
    
    add_deps("libmogan")
    
    -- 编译选项
    set_languages("c++17")
    
    -- Qt 元对象编译
    add_rules("qt.moc")
    add_files("src/Plugins/Qt/QTMClawAIWidget.hpp")

-- ============================================================================
-- Claw AI 单元测试
-- ============================================================================

target("claw_ai_widget_test")
    set_kind("binary")
    set_enabled(true)
    
    add_files("tests/claw-ai/claw_ai_widget_test.cpp")
    add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
    
    add_includedirs("src/Plugins/Qt")
    add_includedirs("src/Scheme")
    add_includedirs("src/Scheme/Scheme")
    
    add_packages("qt6widgets", "gtest")
    
    add_deps("libmogan")
    
    -- 编译选项
    set_languages("c++17")
    
    -- 链接 Qt 测试支持
    add_frameworks("Qt6Test", {target = "claw_ai_widget_test"})

-- ============================================================================
-- 修改现有目标：将 Claw AI 添加到 libmogan
-- ============================================================================

-- 在 libmogan 目标中添加：
-- add_files("src/Plugins/Qt/QTMClawAIWidget.cpp")
-- add_files("src/Plugins/Qt/claw_ai_glue.cpp")

-- ============================================================================
-- Scheme 文件安装
-- ============================================================================

-- 在 install 目标中添加：
-- add_installfiles("TeXmacs/progs/claw-ai/*.scm", {prefixdir = "share/mogan/progs/claw-ai"})
