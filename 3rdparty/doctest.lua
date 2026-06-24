-------------------------------------------------------------------------------
--
-- MODULE      : doctest.lua
-- DESCRIPTION : Xmake package definition for doctest
--
-- Derived from xmake-io/xmake-repo (packages/d/doctest/xmake.lua).
-- Copyright 2017-2018 Xmake Open Source Community
-- Licensed under the Apache License, Version 2.0.
-- See <http://www.apache.org/licenses/LICENSE-2.0> for details.
--

package("liii-doctest")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/doctest/doctest")
    set_description("The fastest feature-rich C++11/14/17/20/23 single-header testing framework")
    set_license("MIT")

    set_sourcedir(path.join(os.scriptdir(), "doctest"))

    add_includedirs("include", "include/doctest")

    on_install(function (package)
        os.cp("doctest", package:installdir("include"))
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }

            TEST_CASE("testing the factorial function") {
                CHECK(factorial(1) == 1);
                CHECK(factorial(2) == 2);
                CHECK(factorial(3) == 6);
                CHECK(factorial(10) == 3628800);
            }
        ]]}, {configs = {languages = "c++11"}, includes = "doctest/doctest.h", defines = "DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN"}))
    end)
package_end()
