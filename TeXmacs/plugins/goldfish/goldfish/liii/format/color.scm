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

(define-library (liii format color)
  (import (scheme base) (liii format base))
  (export
    ;; foreground
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
    ;; background
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

    (define (color->ansi x)
      (case x
       ((bold) "1")
       ((dark) "2")
       ((italic) "3")
       ((underline) "4")

       ((bold-off) "22")
       ((italic-off) "23")
       ((underline-off) "24")

       ((black) "30")
       ((red) "31")
       ((green) "32")
       ((yellow) "33")
       ((blue) "34")
       ((magenta) "35")
       ((cyan) "36")
       ((white) "37")
       ((default-fg) "39")

       ((on-black) "40")
       ((on-red) "41")
       ((on-green) "42")
       ((on-yellow) "43")
       ((on-blue) "44")
       ((on-magenta) "45")
       ((on-cyan) "46")
       ((on-white) "47")
       ((default-bg) "49")
      ) ;case
    ) ;define

    (define (ansi-escape color-val)
      (if (string? color-val)
        color-val
        (string-append "\x1b;[" (color->ansi color-val) "m")
      ) ;if
    ) ;define

    (define color-var (make-state-variable 'color 'default-fg #f))
    (define background-var (make-state-variable 'background 'default-bg #f))
    (define bold-var (make-state-variable 'bold 'bold-off #f))
    (define italic-var (make-state-variable 'italic 'italic-off #f))
    (define underline-var (make-state-variable 'underline 'underline-off #f))

    (define (with-attr var new-attr . args)
      (fn ((orig-attr var))
        (with ((var new-attr))
          (each (ansi-escape new-attr) (each-in-list args) (ansi-escape orig-attr))
        ) ;with
      ) ;fn
    ) ;define

    (define (as-bold . args)
      (with-attr bold-var 'bold (each-in-list args))
    ) ;define
    (define (as-italic . args)
      (with-attr italic-var 'italic (each-in-list args))
    ) ;define
    (define (as-underline . args)
      (with-attr underline-var 'underline (each-in-list args))
    ) ;define

    (define (as-red . args)
      (with-attr color-var 'red (each-in-list args))
    ) ;define
    (define (as-blue . args)
      (with-attr color-var 'blue (each-in-list args))
    ) ;define
    (define (as-green . args)
      (with-attr color-var 'green (each-in-list args))
    ) ;define
    (define (as-cyan . args)
      (with-attr color-var 'cyan (each-in-list args))
    ) ;define
    (define (as-yellow . args)
      (with-attr color-var 'yellow (each-in-list args))
    ) ;define
    (define (as-magenta . args)
      (with-attr color-var 'magenta (each-in-list args))
    ) ;define
    (define (as-white . args)
      (with-attr color-var 'white (each-in-list args))
    ) ;define
    (define (as-black . args)
      (with-attr color-var 'black (each-in-list args))
    ) ;define

    (define (on-red . args)
      (with-attr background-var 'on-red (each-in-list args))
    ) ;define
    (define (on-blue . args)
      (with-attr background-var 'on-blue (each-in-list args))
    ) ;define
    (define (on-green . args)
      (with-attr background-var 'on-green (each-in-list args))
    ) ;define
    (define (on-cyan . args)
      (with-attr background-var 'on-cyan (each-in-list args))
    ) ;define
    (define (on-yellow . args)
      (with-attr background-var 'on-yellow (each-in-list args))
    ) ;define
    (define (on-magenta . args)
      (with-attr background-var 'on-magenta (each-in-list args))
    ) ;define
    (define (on-white . args)
      (with-attr background-var 'on-white (each-in-list args))
    ) ;define
    (define (on-black . args)
      (with-attr background-var 'on-black (each-in-list args))
    ) ;define

    (define (rgb-escape red-level green-level blue-level bg?)
      (unless (and (exact-integer? red-level) (<= 0 red-level 5))
        (error "invalid red-level value" red-level)
      ) ;unless
      (unless (and (exact-integer? green-level) (<= 0 green-level 5))
        (error "invalid green-level value" green-level)
      ) ;unless
      (unless (and (exact-integer? blue-level) (<= 0 blue-level 5))
        (error "invalid blue-level value" blue-level)
      ) ;unless
      (string-append (if bg? "\x1b;[48;5;" "\x1b;[38;5;")
        (number->string (+ (* 36 red-level) (* 6 green-level) blue-level 16))
        "m"
      ) ;string-append
    ) ;define

    (define (rgb24-escape red-level green-level blue-level bg?)
      (unless (and (exact-integer? red-level) (<= 0 red-level 255))
        (error "invalid red-level value" red-level)
      ) ;unless
      (unless (and (exact-integer? green-level) (<= 0 green-level 255))
        (error "invalid green-level value" green-level)
      ) ;unless
      (unless (and (exact-integer? blue-level) (<= 0 blue-level 255))
        (error "invalid blue-level value" blue-level)
      ) ;unless
      (string-append (if bg? "\x1b;[48;2;" "\x1b;[38;2;")
        (number->string red-level)
        ";"
        (number->string green-level)
        ";"
        (number->string blue-level)
        "m"
      ) ;string-append
    ) ;define

    (define (as-color red green blue . fmt)
      (with-attr color-var (rgb-escape red green blue #f) (each-in-list fmt))
    ) ;define

    (define (as-true-color red green blue . fmt)
      (with-attr color-var (rgb24-escape red green blue #f) (each-in-list fmt))
    ) ;define

    (define (on-color red green blue . fmt)
      (with-attr background-var (rgb-escape red green blue #t) (each-in-list fmt))
    ) ;define

    (define (on-true-color red green blue . fmt)
      (with-attr background-var (rgb24-escape red green blue #t) (each-in-list fmt))
    ) ;define

  ) ;begin
) ;define-library
