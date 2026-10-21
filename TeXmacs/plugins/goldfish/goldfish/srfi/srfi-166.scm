;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

;;
;; Based on Alex Shinn's SRFI-166 implementation:
;; Copyright (c) 2006-2020 Alex Shinn.  All rights reserved.
;; BSD-style license: http://synthcode.com/license.txt
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions
;; are met:
;; 1. Redistributions of source code must retain the above copyright
;;    notice, this list of conditions and the following disclaimer.
;; 2. Redistributions in binary form must reproduce the above copyright
;;    notice, this list of conditions and the following disclaimer in the
;;    documentation and/or other materials provided with the distribution.
;; 3. The name of the author may not be used to endorse or promote products
;;    derived from this software without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
;; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
;; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
;; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
;; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
;; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
;; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;

(define-library (srfi srfi-166)
  (import (liii format base)
    (liii format color)
    (liii format unicode)
    (liii format columnar)
    (liii format pretty)
  ) ;import
  (export
    ;; basic
    show
    displayed
    written
    written-shared
    written-simply
    escaped
    maybe-escaped
    numeric
    numeric/comma
    numeric/si
    numeric/fitted
    nl
    fl
    space-to
    tab-to
    nothing
    each
    each-in-list
    joined
    joined/prefix
    joined/suffix
    joined/last
    joined/dot
    joined/range
    padded
    padded/right
    padded/both
    trimmed
    trimmed/right
    trimmed/both
    trimmed/lazy
    fitted
    fitted/right
    fitted/both
    output-default
    ;; computations
    fn
    with
    with!
    forked
    call-with-output
    ;; state variables
    make-state-variable
    port
    row
    col
    width
    output
    writer
    pad-char
    ellipsis
    string-width
    substring/width
    substring/preserve
    radix
    precision
    decimal-sep
    decimal-align
    sign-rule
    comma-sep
    comma-rule
    word-separator?
    ambiguous-is-wide?
    pretty-environment
    ;; pretty
    pretty
    pretty-shared
    pretty-simply
    pretty-with-color
    ;; columnar
    columnar
    tabular
    wrapped
    wrapped/list
    wrapped/char
    justified
    from-file
    line-numbers
    ;; unicode
    terminal-aware
    string-terminal-width
    string-terminal-width/wide
    substring-terminal-width
    substring-terminal-width/wide
    substring-terminal-preserve
    upcased
    downcased
    ;; color
    as-red
    as-blue
    as-green
    as-cyan
    as-yellow
    as-magenta
    as-white
    as-black
    as-bold
    as-italic
    as-underline
    as-color
    as-true-color
    on-red
    on-blue
    on-green
    on-cyan
    on-yellow
    on-magenta
    on-white
    on-black
    on-color
    on-true-color
  ) ;export
  (begin
  ) ;begin
) ;define-library
