# 
#  julia.jl
#  A Mogan plugin for the Julia language
#  (c) 2021  Massimiliano Gubinelli <mgubi@mac.com>
#      2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

module MoganJulia

# Imports
using REPL
using Markdown
using UUIDs
import Base: AbstractDisplay, display, redisplay, catch_stack, show
import REPL: helpmode
import REPL.REPLCompletions: completions, completion_text
import Base.Libc: flush_cstdio

const current_module = Ref{Module}(Main)
const orig_stdout = Ref{IO}(stdout)
const orig_stderr = Ref{IO}(stderr)

# Resolve include path relative to the directory containing this file
const julia_plugin_dir = @__DIR__

# Include submodules/files
include(joinpath(julia_plugin_dir, "tmjl", "protocol.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "capture.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "display.jl"))
include(joinpath(julia_plugin_dir, "tmjl", "completion.jl"))

#=============================================================================#
### Some utilities

function banner()
    io = IOBuffer()
    # the Julia banner is in REPL in Julia >1.11, and in Base before that
    # so we need to do a little chasing
    if isdefined(REPL,:banner)
        REPL.banner(io)
    elseif isdefined(Base,:banner)
        Base.banner(io)
    else
        write(io, "Cannot find the startup banner, sorry!\n");
    end    
    write(io, "Julia plugin for Mogan STEM.\n");
    tm_out(String(take!(io)))
end

#=============================================================================#
### Main loop

# we do not want to exit on SIGINT
# we can then catch InterruptException
Base.exit_on_sigint(false)

local read_stdout, read_stderr
# redirect output/error
read_stdout, = redirect_stdout()
redirect_stdout(TMJuliaStdio(stdout,read_stdout,"stdout"))
read_stderr, = redirect_stderr()
redirect_stderr(TMJuliaStdio(stderr,read_stderr,"stderr"))
#redirect_stdin(TMJuliaStdio(stdin,"stdin"))

# redirect display
pushdisplay(InlineDisplay())

# print banner
tm_begin()
banner()
tm_out(PROMPT,">>> ")
tm_end()

# go
n = 0 # execution counter
ans = nothing # record last successful answer in ans

while true
    line = readline(stdin)
    if length(line) == 0 && eof(stdin)
        break
    end
    length(line) == 0 && continue
    if line[1] == DATA_COMMAND
        # is tab completion the only possible command?
        do_tab_complete(line)
        continue
    end
    lines = []
    while line != "<EOF>"
        push!(lines, line)
        line = readline(stdin)
    end
    local code = join(lines,"\n")
    local result = nothing
    local err = nothing
    global ans = nothing
    tm_begin()
    try
        global n += 1
        # "; ..." cells are interpreted as shell commands for run
        code = replace(code, r"^\s*;.*$" =>
            m -> string(replace(m, r"^\s*;" => "Base.repl_cmd(`"),
                        "`, stdout)"))
        # a cell beginning with "? ..." is interpreted as a help request
        hcode = replace(code, r"^\s*\?" => "")
        # Let's try to run the input
        if hcode != code # help request
            buf = IOBuffer()
            help = Core.eval(Main, helpmode(buf, hcode))
            #flush_output()
            tm_out("HELP: $(String(take!(buf)))\n")
            display(help)
        else
            # finally run the code! 
            result = include_string(current_module[], code, "In[$n]")
            REPL.ends_with_semicolon(code) ? result = nothing : ans = result
        end
    catch e
        err = e
        result = catch_stack()
    end    

    # output
    try 
        if err != nothing 
            Base.invokelatest(Base.display_error, stderr, result)
        elseif result != nothing 
            Base.invokelatest(display, result)
            #show(stdout, result) # display the result as string
        end
    catch e 
        write(stdout, "Error showing values $(e)");
        Base.invokelatest(Base.display_error, stderr, catch_stack())
    end
    flush_all() # send all to mogan
#    flush_output() # send all to mogan
    tm_end()
end # while true

end # module MoganJulia
