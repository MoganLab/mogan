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

;; Copyright (C) Sebastian Egner (2002). All Rights Reserved.
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
;; Based on the reference implementation of SRFI 26 written by Al Petrofsky
;; and Sebastian Egner (placed in the public domain).

(define-library (srfi srfi-26)
  (import (scheme base))
  (export cut cute)
  (begin

    (define-syntax srfi-26-internal-cut
      (syntax-rules (<> <...>)
        ((srfi-26-internal-cut (slot-name ...) (proc arg ...))
         (lambda (slot-name ...) (proc arg ...))
        ) ;
        ((srfi-26-internal-cut (slot-name ...) (proc arg ...) <...>)
         (lambda (slot-name ... . rest-slot) (apply proc arg ... rest-slot))
        ) ;
        ((srfi-26-internal-cut (slot-name ...) (position ...) <> . se)
         (srfi-26-internal-cut (slot-name ... x) (position ... x) . se)
        ) ;
        ((srfi-26-internal-cut (slot-name ...) (position ...) nse . se)
         (srfi-26-internal-cut (slot-name ...) (position ... nse) . se)
        ) ;
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax srfi-26-internal-cute
      (syntax-rules (<> <...>)
        ((srfi-26-internal-cute (slot-name ...) nse-bindings (proc arg ...))
         (let nse-bindings
           (lambda (slot-name ...) (proc arg ...))
         ) ;let
        ) ;
        ((srfi-26-internal-cute (slot-name ...) nse-bindings (proc arg ...) <...>)
         (let nse-bindings
           (lambda (slot-name ... . x) (apply proc arg ... x))
         ) ;let
        ) ;
        ((srfi-26-internal-cute (slot-name ...) nse-bindings (position ...) <> . se)
         (srfi-26-internal-cute (slot-name ... x) nse-bindings (position ... x) . se)
        ) ;
        ((srfi-26-internal-cute slot-names nse-bindings (position ...) nse . se)
         (srfi-26-internal-cute
           slot-names
           ((x nse) . nse-bindings)
           (position ... x)
           .
           se
         ) ;
        ) ;
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax cut
      (syntax-rules ()
        ((cut . slots-or-exprs) (srfi-26-internal-cut () () . slots-or-exprs))
      ) ;syntax-rules
    ) ;define-syntax

    (define-syntax cute
      (syntax-rules ()
        ((cute . slots-or-exprs) (srfi-26-internal-cute () () () . slots-or-exprs))
      ) ;syntax-rules
    ) ;define-syntax

  ) ;begin
) ;define-library
