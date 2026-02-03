;
; Copyright (C) 2026 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

;; Acknowledgements
;;
;; This implementation is based on:
;;
;; 1. The idea deindent from guile-raw-strings by François Joulaud
;;    https://codeberg.org/avalenn/guile-raw-strings
;;    SPDX-License-Identifier: 0BSD
;;
;; 2. The deindentation follows C# raw string literal rules
;;    https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/tokens/raw-string

(define-library (liii raw-string)
  (import (srfi srfi-267)
          (srfi srfi-1)
          (srfi srfi-13)
          (liii error))
  (export deindent &-)
  (begin
    (define (string-split-lines str)
      (let ((len (string-length str)))
        (let loop ((start 0) (result '()))
          (let ((nl-pos (string-index str #\newline start len)))
            (if (not nl-pos)
              (reverse (cons (substring str start len) result))
              (loop (+ nl-pos 1)
                    (cons (substring str start nl-pos) result)))))))

    (define (f-deindent str)
      (when (or (string-null? str)
                (not (char=? #\newline (string-ref str 0))))
          (value-error "Raw string must start on a new line after the opening delimiter"))

      (let* ((lines (string-split-lines (substring str 1 (string-length str))))
             (closing-line (last lines))
             (ref-indent (if (string-null? closing-line)
                             (value-error "Raw string delimiter must be on its own line")
                             (string-count closing-line #\space)))
             (content-lines (drop-right lines 1)))

        ;; check indentation
        (for-each (lambda (line idx)
                    (unless (string-null? line)
                      (let ((indent (or (string-skip line #\space) 0)))
                        (when (< indent ref-indent)
                          (value-error "Line ~a does not start with the same whitespace as the closing line of the raw string" (+ idx 1))))))
                  content-lines
                  (iota (length content-lines)))

        (string-join
         (map (lambda (line)
                (if (string-null? line)
                    ""
                    (substring line ref-indent)))
              content-lines)
         "\n")))

    (define-macro (stx-deindent v)
      (if (string? v)
          `(quote ,(f-deindent v))
          `(quote ,v)))

    (define deindent stx-deindent)
    (define &-       stx-deindent)))
