# 
#  runtests.jl
#  Native white-box unit tests for Mogan Julia plugin
#  (c) 2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

using Test
using REPL

# Disable GUI popups and sockets for GR backend (Plots.jl) during tests
ENV["GKSwstype"] = "100"

# Mock modules/types representing SymPy, Symbolics, SymbolicUtils for test detection
module MockSymPy struct Sym end end
module MockSymbolics struct Num end end
module MockSymbolicUtils struct Basic end end
module MockPlots struct Plot end end

# Mock global variables and refs expected by included files
const current_module = Ref{Module}(Main)
const orig_stdout = Ref{IO}(stdout)
const orig_stderr = Ref{IO}(stderr)

# Resolve path relative to this test file
const julia_plugin_dir = joinpath(@__DIR__, "..", "bin")

# Include all the modularized Julia plugin source files under test
include(joinpath(julia_plugin_dir, "tmjl", "protocol.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "capture.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "display.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "completion.jl"))

# A helper module for testing negative symbolic-object detection
module TestSymbolicLike
    struct MyNum end
end

@testset "Julia Backend Unit Tests" begin

    @testset "1. Protocol Escaping (mogan_escape)" begin
        # Empty string
        @test mogan_escape("") == ""

        # Normal strings with no escape characters
        @test mogan_escape("hello world") == "hello world"
        @test mogan_escape("NCEA Level 3 Physics") == "NCEA Level 3 Physics"

        # Escaping of individual protocol characters
        @test mogan_escape(string(DATA_ESCAPE)) == string(DATA_ESCAPE, DATA_ESCAPE)
        @test mogan_escape(string(DATA_BEGIN))  == string(DATA_ESCAPE, DATA_BEGIN)
        @test mogan_escape(string(DATA_END))    == string(DATA_ESCAPE, DATA_END)

        # Mixed and consecutive escape characters
        @test mogan_escape("a" * DATA_BEGIN * "b" * DATA_END * "c") == 
              "a" * DATA_ESCAPE * DATA_BEGIN * "b" * DATA_ESCAPE * DATA_END * "c"
              
        @test mogan_escape(string(DATA_BEGIN, DATA_ESCAPE, DATA_END)) == 
              string(DATA_ESCAPE, DATA_BEGIN, DATA_ESCAPE, DATA_ESCAPE, DATA_ESCAPE, DATA_END)
    end

    @testset "2. Protocol Output Commands" begin
        # Buffer capture setup
        buf_out = IOBuffer()
        buf_err = IOBuffer()
        orig_stdout[] = buf_out
        orig_stderr[] = buf_err

        # Test tm_begin
        tm_begin()
        @test String(take!(buf_out)) == string(DATA_BEGIN, VERBATIM)

        # Test tm_end
        tm_end()
        @test String(take!(buf_out)) == string(DATA_END)

        # Test tm_out (direct string)
        tm_out("test-data")
        @test String(take!(buf_out)) == "test-data"

        # Test tm_out with header
        tm_out("latex:", "x^2")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "x^2", DATA_END)

        # Test tm_err with header
        tm_err("verbatim:", "load-error")
        @test String(take!(buf_err)) == string(DATA_BEGIN, "verbatim:", "load-error", DATA_END)

        # Restore original IO streams
        orig_stdout[] = stdout
        orig_stderr[] = stderr
    end

    @testset "3. MIME Display formatting & LaTeX stripping" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out

        # Test text/html display dispatch
        display(InlineDisplay(), MIME("text/html"), "<h1>Julia</h1>")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "html:", "<h1>Julia</h1>", DATA_END)

        # Test text/plain fallback
        display(InlineDisplay(), MIME("text/plain"), "plain text")
        @test String(take!(buf_out)) == "plain text"

        # Test text/latex inline stripping of delimiters ($$, $, \[, \], etc.)
        display(InlineDisplay(), MIME("text/latex"), "\$\$x^2\$\$")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\$\\rmfamily{x^2}\$", DATA_END)

        display(InlineDisplay(), MIME("text/latex"), "\\[ a + b \\]")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\$\\rmfamily{a + b}\$", DATA_END)

        display(InlineDisplay(), MIME("text/latex"), "\\( \\sqrt{2} \\)")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\$\\rmfamily{\\sqrt{2}}\$", DATA_END)

        # Test LaTeX environments (equation, align) - should rewrite but NOT wrap in \rmfamily
        display(InlineDisplay(), MIME("text/latex"), "\\begin{equation}y = x^2\\end{equation}")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\\begin{equation*}y = x^2\\end{equation*}", DATA_END)

        display(InlineDisplay(), MIME("text/latex"), "\\begin{align}a &= b\\end{align}")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\\begin{align*}a &= b\\end{align*}", DATA_END)

        # Restore original IO streams
        orig_stdout[] = stdout
    end

    @testset "4. Symbolic Objects Recognition (is_symbolic_object)" begin
        # 1. Standard types (should return false)
        @test !is_symbolic_object(1)
        @test !is_symbolic_object("x")
        @test !is_symbolic_object(1.0 + 2.0im)
        @test !is_symbolic_object([1, 2, 3])
        @test !is_symbolic_object(Union{Int, Float64}) # Union type edge case

        # 2. Direct symbolic types
        @test is_symbolic_object(MockSymPy.Sym())
        @test is_symbolic_object(MockSymbolics.Num())
        @test is_symbolic_object(MockSymbolics.Num())

        # 3. Deeply nested container checks (Arrays, Tuples, Sets, Dicts)
        @test is_symbolic_object([1, MockSymbolics.Num()])                 # Array
        @test is_symbolic_object((MockSymPy.Sym(), 1.0))                   # Tuple
        @test is_symbolic_object(Set([MockSymbolics.Num(), 1]))            # Set
        @test is_symbolic_object(Dict(MockSymPy.Sym() => "value"))         # Dict keys
        @test is_symbolic_object(Dict("key" => MockSymbolics.Num()))       # Dict values
    end

    @testset "5. Plots Objects Recognition (is_plots_object)" begin
        @test !is_plots_object(1)
        @test !is_plots_object("plot")
        # MockPlots.Plot is not from a module named Plots, so it must not be recognized
        @test !is_plots_object(MockPlots.Plot())
        # Positive case (real Plots.jl object) is covered by testset 17 when Plots is installed
    end

    @testset "6. Auto-completion Parsing (do_tab_complete)" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out

        # Test valid command format: DATA_COMMAND (complete "re" 2)
        cmd = string(DATA_COMMAND, "(complete \"re\" 2)")
        do_tab_complete(cmd)
        output = String(take!(buf_out))
        @test occursin("scheme:", output)
        @test occursin("tuple", output)
        @test occursin("addir", output) || occursin("ad", output)

        # Test invalid completion format (must catch exception internally and not crash)
        invalid_cmd = string(DATA_COMMAND, "(complete \"re\" \"invalid_cursor\")")
        @test_nowarn do_tab_complete(invalid_cmd)

        # Restore original IO streams
        orig_stdout[] = stdout
    end

    @testset "7. Stream Redirection (TMJuliaStdio)" begin
        old_stdout = stdout
        old_stderr = stderr
        buf_out = IOBuffer()
        buf_err = IOBuffer()
        orig_stdout[] = buf_out
        orig_stderr[] = buf_err

        # stdout branch
        rd_out, wr_out = redirect_stdout()
        try
            stdio_out = TMJuliaStdio(wr_out, rd_out, "stdout")
            println(stdio_out, "stdout-line")
            flush(stdio_out)
            out_data = String(take!(buf_out))
            @test occursin("stdout-line", out_data)
        finally
            redirect_stdout(old_stdout)
        end

        # stderr branch
        rd_err, wr_err = redirect_stderr()
        try
            stdio_err = TMJuliaStdio(wr_err, rd_err, "stderr")
            println(stdio_err, "stderr-line")
            flush(stdio_err)
            err_data = String(take!(buf_err))
            @test occursin("stderr-line", err_data)
            @test occursin(VERBATIM, err_data)
        finally
            redirect_stderr(old_stderr)
        end

        orig_stdout[] = old_stdout
        orig_stderr[] = old_stderr
    end

    @testset "8. TMJuliaStdio Properties" begin
        old_stdout = stdout
        rd, wr = redirect_stdout()
        try
            stdio = TMJuliaStdio(wr, rd, "stdout")
            @test get(stdio, :mogan_stream, "") == "stdout"
            @test get(stdio, :color, true) == false
        finally
            redirect_stdout(old_stdout)
        end
    end

    @testset "9. Code Evaluation Core Logic" begin
        # These are the exact primitives used inside the main REPL loop
        @test include_string(Main, "1 + 2", "In[1]") == 3
        @test REPL.ends_with_semicolon("1 + 2;")
        @test !REPL.ends_with_semicolon("1 + 2")

        # Shell-mode transformation
        shell_code = ";pwd"
        transformed = replace(shell_code, r"^\s*;.*$" =>
            m -> string(replace(m, r"^\s*;" => "Base.repl_cmd(`"),
                        "`, stdout)"))
        @test occursin("Base.repl_cmd(`pwd`", transformed)

        # Help-mode transformation
        @test replace("?sin", r"^\s*\?" => "") == "sin"
    end

    @testset "10. Help Mode Integration" begin
        buf_help = IOBuffer()
        help_obj = Core.eval(Main, REPL.helpmode(buf_help, "sqrt"))
        @test help_obj isa Markdown.MD
    end

    @testset "11. Additional MIME Displays" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out

        # Markdown renders through text/html
        md = Markdown.parse("# Title\nparagraph")
        display(InlineDisplay(), md)
        html_out = String(take!(buf_out))
        @test occursin("html:", html_out)

        # A type providing its own text/latex show method
        struct LatexType end
        Base.show(io::IO, ::MIME"text/latex", ::LatexType) = print(io, "\\frac{1}{2}")
        display(InlineDisplay(), LatexType())
        latex_out = String(take!(buf_out))
        @test occursin("latex:", latex_out)
        @test occursin("\\frac{1}{2}", latex_out)

        # Fallback text/plain for arbitrary structs
        struct PlainType x::Int end
        display(InlineDisplay(), PlainType(7))
        plain_out = String(take!(buf_out))
        @test occursin("7", plain_out)

        orig_stdout[] = stdout
    end

    @testset "12. LaTeX Display Edge Cases" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out

        # Already-starred environments must not be double-starred
        display(InlineDisplay(), MIME("text/latex"), "\\begin{equation*}x = 1\\end{equation*}")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\\begin{equation*}x = 1\\end{equation*}", DATA_END)

        # Bracket-style delimiters
        display(InlineDisplay(), MIME("text/latex"), "  \\[ \\alpha + \\beta \\]  ")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\$\\rmfamily{\\alpha + \\beta}\$", DATA_END)

        # Empty content
        display(InlineDisplay(), MIME("text/latex"), "")
        @test String(take!(buf_out)) == string(DATA_BEGIN, "latex:", "\$\\rmfamily{}\$", DATA_END)

        orig_stdout[] = stdout
    end

    @testset "13. PDF Output Dispatch" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out
        pdf_out(42)
        @test String(take!(buf_out)) == "[Cannot display PDF for Int64]"
        orig_stdout[] = stdout
    end

    @testset "14. Completion Edge Cases" begin
        buf_out = IOBuffer()
        orig_stdout[] = buf_out

        # Empty prefix at cursor 0
        cmd = string(DATA_COMMAND, "(complete \"\" 0)")
        do_tab_complete(cmd)
        out1 = String(take!(buf_out))
        @test occursin("scheme:", out1)

        # Module-qualified prefix
        cmd = string(DATA_COMMAND, "(complete \"Base.ab\" 7)")
        do_tab_complete(cmd)
        out2 = String(take!(buf_out))
        @test occursin("scheme:", out2)

        # Malformed commands must not throw
        @test_nowarn do_tab_complete(string(DATA_COMMAND, "(complete \"x\")"))
        @test_nowarn do_tab_complete(string(DATA_COMMAND, "not-a-form"))

        orig_stdout[] = stdout
    end

    @testset "15. Symbolic Object Detection in Containers" begin
        @test is_symbolic_object([[MockSymbolics.Num()]])
        @test is_symbolic_object((a=MockSymPy.Sym(), b=1))
        @test is_symbolic_object(Dict(:x => [MockSymbolics.Num(), 1]))
        @test !is_symbolic_object(Dict(:x => 1, :y => 2.0))
        @test !is_symbolic_object(Set([1, 2, 3]))

        # Custom type that looks symbolic only by module name
        @test !is_symbolic_object(TestSymbolicLike.MyNum())
    end

    @testset "16. Scientific Computing Workflows" begin
        using LinearAlgebra
        A = [1.0 2.0; 3.0 4.0]
        @test det(A) ≈ -2.0
        vals = eigvals(A)
        @test length(vals) == 2
        @test prod(vals) ≈ det(A)

        # Display a numeric array through the plugin pipeline
        buf_out = IOBuffer()
        orig_stdout[] = buf_out
        display(InlineDisplay(), A)
        arr_out = String(take!(buf_out))
        @test occursin("html:", arr_out) || occursin("1.0", arr_out)
        orig_stdout[] = stdout

        # Complex numbers and special values are not symbolic
        @test !is_symbolic_object(1 + 2im)
        @test !is_symbolic_object(π)
    end

    @testset "17. Optional Symbolic Packages Integration" begin
        function pkg_available(pkg::String)
            try
                @eval using $(Symbol(pkg))
                return true
            catch
                return false
            end
        end

        if pkg_available("Symbolics")
            expr = @eval begin
                using Symbolics
                @variables x y
                x^2 + y^2
            end
            @test is_symbolic_object(expr)
            buf_out = IOBuffer()
            orig_stdout[] = buf_out
            display(InlineDisplay(), expr)
            sym_out = String(take!(buf_out))
            # Symbolics alone may fall back to text/plain unless text/latex is
            # showable or Latexify is loaded.
            if showable(MIME("text/latex"), expr) || isdefined(Main, :Latexify)
                @test occursin("latex:", sym_out)
            else
                @test occursin("x^2", sym_out)
            end
            orig_stdout[] = stdout
        else
            @test_skip "Symbolics.jl not installed"
        end

        if pkg_available("SymPy")
            z = @eval begin
                using SymPy
                SymPy.@syms z
                z
            end
            @test is_symbolic_object(z)
        else
            @test_skip "SymPy.jl not installed"
        end

        if pkg_available("Latexify")
            lt = @eval begin
                using Latexify
                latexify(:(x / y))
            end
            buf_out = IOBuffer()
            orig_stdout[] = buf_out
            display(InlineDisplay(), MIME("text/latex"), lt)
            @test occursin("latex:", String(take!(buf_out)))
            orig_stdout[] = stdout
        else
            @test_skip "Latexify.jl not installed"
        end

        if pkg_available("Plots")
            plt = @eval begin
                using Plots
                plot(1:10, rand(10))
            end
            @test is_plots_object(plt)
            buf_out = IOBuffer()
            orig_stdout[] = buf_out
            display(InlineDisplay(), plt)
            plt_out = String(take!(buf_out))
            @test occursin("file:", plt_out)
            orig_stdout[] = stdout
        else
            @test_skip "Plots.jl not installed"
        end
    end

    @testset "18. Protocol Escaping Edge Cases" begin
        @test mogan_escape("α + β = γ") == "α + β = γ"
        @test mogan_escape(string(DATA_ESCAPE, DATA_BEGIN, DATA_END)) ==
              string(DATA_ESCAPE, DATA_ESCAPE, DATA_ESCAPE, DATA_BEGIN, DATA_ESCAPE, DATA_END)
        @test mogan_escape("<EOF>") == "<EOF>"
    end

    @testset "19. flush_all Utility" begin
        @test_nowarn flush_all()
    end

end
