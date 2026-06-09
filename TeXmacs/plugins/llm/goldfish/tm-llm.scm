
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-llm.scm
;; DESCRIPTION : LLM plugin (eval-and-print with code environment support)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
        (liii path)
        (liii uuid))

(define (welcome)
  (flush-prompt "llm> ")
  (flush-verbatim "LLM Plugin")
) ;define

(define *large-data-threshold* 1048576) ; 1M

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
    (if (string=? data "") #t (eval-and-print data))
  ) ;let
) ;define

(define (repl)
  (read-eval-print)
  (repl)
) ;define

(welcome)
(repl)
