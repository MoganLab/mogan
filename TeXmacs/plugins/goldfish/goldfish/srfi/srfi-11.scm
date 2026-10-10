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
;; Based on the reference implementation of SRFI 11 written by Lars T Hansen
;; (placed in the public domain).

(define-library (srfi srfi-11)
  (export let-values let*-values)
  (begin

    (define-syntax let-values
      (syntax-rules ()
        ((let-values () ?body0 ?body1 ...) (begin ?body0 ?body1 ...))

        ((let-values ((?formals ?expr))
           ?body0
           ?body1
           ...
         ) ;let-values
         (call-with-values (lambda () ?expr) (lambda ?formals ?body0 ?body1 ...))
        ) ;

        ((let-values (?binding0 ?binding1 ?binding2 ...)
           ?body0
           ?body1
           ...
         ) ;let-values
         (let-values "eval"
           (?binding0 ?binding1 ?binding2 ...)
           ()
           ()
           (begin
             ?body0
             ?body1
             ...
           ) ;begin
         ) ;let-values
        ) ;

        ((let-values "eval"
           ()
           (?formal ...)
           (?t ...)
           ?body
         ) ;let-values
         (let-values "apply"
           (?formal ...)
           (?t ...)
           ?body
         ) ;let-values
        ) ;

        ((let-values "eval"
           ((?f ?e) . ?more)
           (?formal ...)
           (?t ...)
           ?body
         ) ;let-values
         (call-with-values (lambda () ?e)
           (lambda t (let-values "eval" ?more (?formal ... ?f) (?t ... t) ?body))
         ) ;call-with-values
        ) ;

        ((let-values "apply" () () ?body) ?body)

        ((let-values "apply"
           (?f . ?f-more)
           (?t . ?t-more)
           ?body
         ) ;let-values
         (apply (lambda ?f (let-values "apply" ?f-more ?t-more ?body)) ?t)
        ) ;
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax let*-values
      (syntax-rules ()
        ((let*-values () ?body0 ?body1 ...) (begin ?body0 ?body1 ...))

        ((let*-values (?binding0 ?binding1 ...)
           ?body0
           ?body1
           ...
         ) ;let*-values
         (let-values (?binding0)
           (let*-values (?binding1 ...)
             ?body0
             ?body1
             ...
           ) ;let*-values
         ) ;let-values
        ) ;
      ) ;syntax-rules
    ) ;define-syntax

  ) ;begin
) ;define-library
