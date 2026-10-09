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

;; Copyright (C) Lars T Hansen (1999). All Rights Reserved.
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
;; Based on the reference implementation of SRFI 16 written by Lars T Hansen
;; (placed in the public domain).

(define-library (scheme case-lambda)
  (import (scheme base))
  (export case-lambda)
  (begin

    (define-syntax case-lambda
      (syntax-rules ()
        ((case-lambda
         ) ;case-lambda
         (lambda args (error 'wrong-number-of-args "CASE-LAMBDA without any clauses."))
        ) ;
        ((case-lambda
           (?a1 ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (lambda args
           (let ((l (length args)))
             (case-lambda
               "CLAUSE"
               args
               l
               (?a1 ?e1 ...)
               ?clause1
               ...
             ) ;case-lambda
           ) ;let
         ) ;lambda
        ) ;
        ((case-lambda
           "CLAUSE"
           ?args
           ?l
           ((?a1 ...) ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (if (= ?l (length '(?a1 ...)))
           (apply (lambda (?a1 ...) ?e1 ...) ?args)
           (case-lambda "CLAUSE" ?args ?l ?clause1 ...
           ) ;case-lambda
         ) ;if
        ) ;
        ((case-lambda
           "CLAUSE"
           ?args
           ?l
           ((?a1 . ?ar) ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (case-lambda
           "IMPROPER"
           ?args
           ?l
           1
           (?a1 . ?ar)
           (?ar ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
        ) ;
        ((case-lambda
           "CLAUSE"
           ?args
           ?l
           (?a1 ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (let ((?a1 ?args))
           ?e1
           ...
         ) ;let
        ) ;
        ((case-lambda
           "CLAUSE"
           ?args
           ?l
         ) ;case-lambda
         (error 'wrong-number-of-args "Wrong number of arguments to CASE-LAMBDA.")
        ) ;
        ((case-lambda
           "IMPROPER"
           ?args
           ?l
           ?k
           ?al
           ((?a1 . ?ar) ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (case-lambda
           "IMPROPER"
           ?args
           ?l
           (+ ?k 1)
           ?al
           (?ar ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
        ) ;
        ((case-lambda
           "IMPROPER"
           ?args
           ?l
           ?k
           ?al
           (?ar ?e1 ...)
           ?clause1
           ...
         ) ;case-lambda
         (if (>= ?l ?k)
           (apply (lambda ?al ?e1 ...) ?args)
           (case-lambda "CLAUSE" ?args ?l ?clause1 ...
           ) ;case-lambda
         ) ;if
        ) ;
      ) ;syntax-rules
    ) ;define-syntax

  ) ;begin
) ;define-library
