
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-llm.scm
;; DESCRIPTION : Fake LLM plugin (echo functionality with llm styling)
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

(import (texmacs protocol))

(define (welcome)
  (flush-prompt "llm> ")
  (flush-verbatim "Fake LLM: demo plugin (echo mode)")
) ;define

(define echo-count 0)

(define (eval-and-print code)
  (set! echo-count (+ echo-count 1))
  (let* ((count-str (number->string echo-count)) (N (length code)))
    (flush-verbatim (string-append count-str " " (number->string N)))
  ) ;let*
) ;define

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (if (string=? code "") #t (eval-and-print code))
  ) ;let
) ;define

(define (repl)
  (read-eval-print)
  (repl)
) ;define

(welcome)
(repl)
