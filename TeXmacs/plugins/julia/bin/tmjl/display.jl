# 
#  display.jl
#  Inline rich displays and symbolic math formatting for Mogan Julia plugin
#  (c) 2021  Massimiliano Gubinelli <mgubi@mac.com>
#      2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

import UUIDs
import Markdown
import Base: AbstractDisplay, display, redisplay, catch_stack, show

#=============================================================================#
### display redirection

# need special handling for showing a string as a textmime
# type, since in that case the string is assumed to be
# raw data unless it is text/plain
israwtext(::MIME, x::AbstractString) = true
israwtext(::MIME"text/plain", x::AbstractString) = false
israwtext(::MIME, x) = false

# convert x to a string of type mime, making sure to use an
# IOContext that tells the underlying show function to limit output
function limitstringmime(mime::MIME, x)
    buf = IOBuffer()
    if israwtext(mime, x)
        return String(x)
    else
        show(IOContext(buf, :limit=>true, :color=>false), mime, x)
    end
    return String(take!(buf))
end

struct InlineDisplay <: AbstractDisplay end

showtofile(file::AbstractString, m::MIME, x) = begin
    open("$(ENV["TEXMACS_HOME_PATH"])/system/tmp/$(file)", "w") do io
        show(io, m, x)
    end
    tm_out("file:", file)
end

sendimage(ext::AbstractString, m::MIME, x) = begin
    buf = IOBuffer()
    show(buf, m, x)
    tm_out("mogan:","<image|<tuple|<#$(bytes2hex(take!(buf)))>|julia-output-$(UUIDs.uuid1()).$(ext)>|0.618par|||>")
end

display(d::InlineDisplay, m::MIME"image/png", x) = 
    sendimage("png", m, x)

display(d::InlineDisplay, m::MIME"image/jpeg", x) = 
    sendimage("jpg", m, x)

display(d::InlineDisplay, m::MIME"application/pdf", x) = 
    sendimage("pdf", m, x)

display(d::InlineDisplay, m::MIME"text/html", x) = 
    tm_out("html:", limitstringmime(m, x))

display(d::InlineDisplay, m::MIME"text/latex", x) = begin
    s = strip(limitstringmime(m, x))
    # 去除外层 $$, $, \[ \], or \( \) 修饰符直接解析
    s = replace(s, r"^(\$\$?|\\\[|\\\()|(\$\$?|\\\]|\\\))$" => "")
    s = strip(s)
    # 去掉环境编号
    s = replace(s, "begin{equation}" => "begin{equation*}")
    s = replace(s, "end{equation}" => "end{equation*}")
    s = replace(s, "begin{align}" => "begin{align*}")
    s = replace(s, "end{align}" => "end{align*}")
    if occursin(r"^\\begin", s)
        tm_out("latex:", s)
    else
        tm_out("latex:", "\$\\rmfamily{" * s * "}\$")
    end
end

display(d::InlineDisplay, m::MIME"text/markdown", x) = 
    display(d, MIME("text/html"), Markdown.html(x))

display(d::InlineDisplay, m::MIME"text/plain", s::AbstractString) = 
    tm_out(s)

# fallback
display(d::InlineDisplay, m::MIME, x) =
    tm_out(limitstringmime(m, x))

# generic display overloading
display(d::InlineDisplay, x::Markdown.MD) = display(d, MIME("text/markdown"), x) 

# we try to display data according to these mime types
# in order
const tm_mimetypes = [
    MIME("image/svg"),
    MIME("application/pdf"),
    MIME("image/png"),
    MIME("image/jpg"),
    MIME("text/latex"),
    MIME("text/html"), 
    MIME("text/markdown")]

is_symbolic_type(t::Type) = begin
    t isa Union && return false
    s = string(t)
    (occursin("SymPy", s) || occursin("Symbolics", s) || occursin("SymbolicUtils", s) || s == "Num") && return true
    try
        m = parentmodule(t)
        m_name = string(Symbol(m))
        return occursin("SymPy", m_name) || occursin("Symbolics", m_name) || occursin("SymbolicUtils", m_name)
    catch
        return false
    end
end

is_symbolic_object(x) = is_symbolic_type(typeof(x))
is_symbolic_object(x::AbstractArray) = any(is_symbolic_object, x)
is_symbolic_object(x::Tuple) = any(is_symbolic_object, x)
is_symbolic_object(x::Set) = any(is_symbolic_object, x)
is_symbolic_object(x::Dict) = any(is_symbolic_object, keys(x)) || any(is_symbolic_object, values(x))

is_plots_type(t::Type) = begin
    t isa Union && return false
    s = string(t)
    (occursin("Plots.Plot", s) || s == "Plot") && return true
    try
        m = parentmodule(t)
        m_name = string(Symbol(m))
        return occursin("Plots", m_name)
    catch
        return false
    end
end

is_plots_object(x) = is_plots_type(typeof(x))

function display(d::InlineDisplay, x)
    if is_plots_object(x)
        try
            tmp_path = tempname() * ".pdf"
            t = typeof(x)
            m = parentmodule(t)
            if isdefined(m, :savefig)
                m.savefig(x, tmp_path)
                tm_out("file:", tmp_path)
                return
            elseif isdefined(Main, :Plots) && isdefined(Main.Plots, :savefig)
                Main.Plots.savefig(x, tmp_path)
                tm_out("file:", tmp_path)
                return
            end
        catch e
            tm_out("Error rendering Plots: $(e)\n")
        end
    end

    if is_symbolic_object(x)
        if showable(MIME("text/latex"), x)
            display(d, MIME("text/latex"), x)
            return
        elseif isdefined(Main, :Latexify)
            try
                lx = Main.Latexify.latexify(x)
                display(d, MIME("text/latex"), lx)
                return
            catch
                # If latexify fails, fall back to default behavior
            end
        end
    end

    for m in tm_mimetypes
        if showable(m, x)
            display(d, m, x)
            return
        end
    end
    # default behaviour is showing text
    display(d, MIME("text/plain"), x)
#    tm_out("TODO: display an object of type [$(typeof(x))]")   
end

function pdf_out(x) 
    if showable(MIME("application/pdf"), x)
        display(MIME("application/pdf"), x)
    else
        tm_out("[Cannot display PDF for $(typeof(x))]")
    end
end
