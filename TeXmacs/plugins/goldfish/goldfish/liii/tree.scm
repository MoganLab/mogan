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
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

(define-library (liii tree)
  (export tree-cyclic? tree-leaves tree-memq? tree-member? tree-set-memq
    tree-count tree-depth
  ) ;export
  (import (scheme base) (liii error) (liii syntax))
  (begin
    (define (tree-depth tree)
      (if (tree-cyclic? tree)
        (value-error "tree-depth: tree is cyclic: ~S" tree)
        (let loop
          ((x tree))
          (cond ((not (pair? x)) 0)
                ((quote-form? x) (loop (cadr x)))
                (else
                  (+ 1
                    (let elt-loop
                      ((rest x) (max-d 0))
                      (if (not (pair? rest))
                        max-d
                        (elt-loop (cdr rest) (max max-d (loop (car rest))))
                      ) ;if
                    ) ;let
                  ) ;+
                ) ;else
          ) ;cond
        ) ;let
      ) ;if
    ) ;define
  ) ;begin
) ;define-library
