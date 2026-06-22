# 
#  protocol.jl
#  Mogan/TeXmacs plugin protocol implementation for Julia
#  (c) 2021  Massimiliano Gubinelli <mgubi@mac.com>
#      2026  Tianyou Liu <tianyou@liii.pro>
# 
#  This software falls under the GNU general public license version 3 or later.
#  It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
#  in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
#

const DATA_BEGIN = Char(2)
const DATA_END = Char(5)
const DATA_ESCAPE = Char(27)
const DATA_COMMAND = Char(16)
const VERBATIM = "verbatim:"
const SCHEME = "scheme:"
const COMMAND = "command:"
const PROMPT = "prompt#"

mogan_escape(data) = replace(replace(replace(data,
        DATA_ESCAPE => DATA_ESCAPE * DATA_ESCAPE),
        DATA_BEGIN => DATA_ESCAPE * DATA_BEGIN), 
        DATA_END => DATA_ESCAPE * DATA_END)
  
# Mogan expects all output to be bracketed in a DATA_BEGIN and DATA_END
# so that it can determines when the plugin ended the interaction        
tm_begin() = write(orig_stdout[], DATA_BEGIN, VERBATIM)
tm_end() = begin
    write(orig_stdout[], DATA_END)
    flush(orig_stdout[]) 
end

tm_out(data) = begin
    write(orig_stdout[], mogan_escape(data))
    flush(orig_stdout[]) 
end

tm_out(header, data) = begin
    write(orig_stdout[], 
        DATA_BEGIN, header, mogan_escape(data), DATA_END)
    flush(orig_stdout[]) 
end

tm_err(header, data) = begin
    write(orig_stderr[], 
        DATA_BEGIN, header, mogan_escape(data), DATA_END)
    flush(orig_stderr[]) 
end
