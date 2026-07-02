# 
#  completion.jl
#  Autocompletion protocol implementation for Mogan Julia plugin
#  (c) 2021  Massimiliano Gubinelli <mgubi@mac.com>
#      2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

import REPL.REPLCompletions: completions, completion_text

function do_tab_complete(cmd::AbstractString)
    # syntax [DATA_COMMAND](complete [STRING] [CURSOR])
    try
        pos = 12
        arg1,pos = Meta.parse(cmd,pos; greedy=false) # [STRING]
        arg2,pos = Meta.parse(cmd,pos; greedy=false) # [CURSOR]
        if isa(arg1,AbstractString) && isa(arg2,Integer)
            ret,range,shouldcomplete = completions(arg1,arg2)
            compls = join(unique!(map(x -> "\"$(completion_text(x)[range.stop+2-range.start:end])\"",ret))," ")
            tm_out("scheme:", "(tuple \"$(arg1[range])\" $(compls))")
        end
    catch e 
        # ignore errors 
    end
end
