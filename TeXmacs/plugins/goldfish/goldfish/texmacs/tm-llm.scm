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

(define-library (texmacs tm-llm)
(import (texmacs protocol)
        (liii path)
        (liii uuid)
        (liii string))
(export llm-welcome llm-repl)
(begin

(define *large-data-threshold* 1048576) ; 1M

(define (llm-welcome)
  (flush-prompt "LLM] ")
  (flush-verbatim "Goldfish Scheme LLM Plugin"))

(define (llm-write-temp-file data)
  (let* ((tmp-dir (path-temp-dir))
         (tmp-name (uuid4))
         (tmp-path (path-join (path->string tmp-dir) tmp-name)))
    (path-write-text tmp-path data)
    tmp-path))

; data: the input from Mogan plugin
; Uses flush-scheme-u8 to place data into the code environment.
; If data exceeds 1M, writes to a temp file and returns the file path.
(define (eval-and-print data)
  (if (> (string-length data) *large-data-threshold*)
    (let ((tmp-path (llm-write-temp-file data)))
      (flush-scheme-u8
        (string-append "(document \"(temp-file " tmp-path ")\")")))
    (flush-scheme-u8 data)))

(define (read-eval-print)
  (let ((data (read-paragraph-by-visible-eof)))
    (if (string=? data "")
      #t
      (eval-and-print data))))

(define (llm-repl)
  (begin (read-eval-print)
         (llm-repl)))

) ; end of begin
) ; end of define-library
