;
; Copyright (C) 2024 The Goldfish Scheme Authors
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

(define-library (liii string)
  (export
    ; S7 built-in
    string? string-ref string-length
    ; from (scheme base)
    string-copy string-for-each string-map
    ; from (srfi srfi-13)
    string-null? string-join
    string-every string-any
    string-take string-take-right string-drop string-drop-right
    string-pad string-pad-right
    string-trim string-trim-right string-trim-both
    string-index string-index-right string-skip string-skip-right
    string-contains string-count
    string-upcase string-downcase
    string-fold string-fold-right string-for-each-index
    string-reverse
    string-tokenize
    ; Liii extras
    string-starts? string-ends?
    string-remove-prefix string-remove-suffix
    )
  (import (srfi srfi-13)
          (liii base)
          (liii error))
  (begin

    (define (string-starts? str prefix)
      (if (and (string? str) (string? prefix))
          (string-prefix? prefix str)
          (type-error "string-starts? parameter is not a string")))

    (define (string-ends? str suffix)
      (if (and (string? str) (string? suffix))
          (string-suffix? suffix str)
          (type-error "string-ends? parameter is not a string")))

    (define string-remove-prefix
      (typed-lambda ((str string?) (prefix string?))
        (if (string-prefix? prefix str)
            (substring str (string-length prefix))
            str)))

    (define string-remove-suffix
      (typed-lambda ((str string?) (suffix string?))
        (if (string-suffix? suffix str)
            (substring str 0 (- (string-length str) (string-length suffix)))
            (string-copy str))))

    ) ; end of begin
  ) ; end of define-library

