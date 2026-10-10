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

;; Copyright (C) Oleg Kiselyov (1998). All Rights Reserved.
;;
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;
;; Based on the syntax-rules implementation contributed by
;; Marc Nieper-Wißkirchen to SRFI 2 (public domain).

(define-library (srfi srfi-2)
  (import (scheme base))
  (export and-let*)
  (begin

    (define-syntax and-let*
      (syntax-rules ()
        ((_ ()) #t)
        ((_ () form form* ...) (begin form form* ...))
        ((_ ((id expr))) expr)
        ((_ ((expr))) expr)
        ((_ (id)) id)
        ((_ ((id expr) . claw*) . body)
         (let ((id expr))
           (and id (and-let* claw* . body))
         ) ;let
        ) ;
        ((_ ((expr) . claw*) . body) (and expr (and-let* claw* . body)))
        ((_ (id . claw*) . body) (and id (and-let* claw* . body)))
        ((_ . _) (syntax-error "ill-formed and-let* form"))
      ) ;syntax-rules
    ) ;define-syntax

  ) ;begin
) ;define-library
