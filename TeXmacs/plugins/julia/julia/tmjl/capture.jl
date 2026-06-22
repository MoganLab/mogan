# 
#  capture.jl
#  Standard stream capturing and redirection for Mogan Julia plugin
#  (c) 2021  Massimiliano Gubinelli <mgubi@mac.com>
#      2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

import Base.Libc: flush_cstdio

#=============================================================================#
### Flush all redirected streams to Mogan

function flush_all()
    flush_cstdio() # flush writes to stdout/stderr by external C code
    flush(stdout)
    flush(stderr)
end

#=============================================================================#
### Stream redirection (from IJulia)

# create a wrapper type around redirected stdio streams,
# both for overloading things like `flush` and so that we
# can set properties like `color`.
struct TMJuliaStdio{IO_t <: IO} <: Base.AbstractPipe
    io::IOContext{IO_t}
    read_stream::Base.PipeEndpoint
end

TMJuliaStdio(io::IO, read_stream::Base.PipeEndpoint, stream::AbstractString="unknown") =
    TMJuliaStdio{typeof(io)}(IOContext(io, :color=>false,
                            :mogan_stream=>stream,
                            :displaysize=>displaysize()), read_stream)
Base.pipe_reader(io::TMJuliaStdio) = io.io.io
Base.pipe_writer(io::TMJuliaStdio) = io.io.io
Base.lock(io::TMJuliaStdio) = lock(io.io.io)
Base.unlock(io::TMJuliaStdio) = unlock(io.io.io)
Base.in(key_value::Pair, io::TMJuliaStdio) = in(key_value, io.io)
Base.haskey(io::TMJuliaStdio, key) = haskey(io.io, key)
Base.getindex(io::TMJuliaStdio, key) = getindex(io.io, key)
Base.get(io::TMJuliaStdio, key, default) = get(io.io, key, default)
Base.displaysize(io::TMJuliaStdio) = displaysize(io.io)
Base.unwrapcontext(io::TMJuliaStdio) = Base.unwrapcontext(io.io)
Base.setup_stdio(io::TMJuliaStdio, readable::Bool) = Base.setup_stdio(io.io.io, readable)

Base.flush(io::TMJuliaStdio) = begin
    #write(orig_stdout[],"FLUSHING $(get(io.io, :mogan_stream, "error"))\n")
    Base.flush(io.io.io)
    # add one more char so that we do not block on readavailable later
    write(io.io.io,"!")
    local buf = chop(String(readavailable(io.read_stream)));
    buf == "" && return
    if get(io.io, :mogan_stream, "error") == "stdout"
        tm_out(buf * "\n")
    elseif get(io.io, :mogan_stream, "error") == "stderr"
        tm_err(VERBATIM, buf)
    end
end

if VERSION < v"1.7.0-DEV.254"
    for s in ("stdout", "stderr", "stdin")
        f = Symbol("redirect_", s)
        sq = QuoteNode(Symbol(s))
        @eval function Base.$f(io::TMJuliaStdio)
            io[:mogan_stream] != $s && throw(ArgumentError(string("expecting ", $s, " stream")))
            Core.eval(Base, Expr(:(=), $sq, io))
            return io
        end
    end
end
