
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
  (flush-verbatim
    "Fake LLM: demo plugin (echo mode)"))

(define echo-count 0)

(define (eval-and-print code)
  (set! echo-count (+ echo-count 1))
  (let* ((count-str (number->string echo-count))
         (stree (with-input-from-string code (lambda () (read))))
         (tag (if (pair? stree) (car stree) '())))
    (if (member tag '(document math equation* align))
      (flush-scheme `(document (concat ,count-str " " ,stree)))
      (flush-verbatim (string-append count-str " " code)))))

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (if (string=? code "")
        #t
        (eval-and-print code))))

(define (repl)
  (read-eval-print)
  (repl))

(welcome)
(repl)
