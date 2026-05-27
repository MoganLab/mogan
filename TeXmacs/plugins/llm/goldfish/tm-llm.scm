
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-llm.scm
;; DESCRIPTION : Fake LLM plugin (echo functionality with llm styling)
;; COPYRIGHT   : (C) 2025 Darcy Shen
;;
;; MIT License
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
