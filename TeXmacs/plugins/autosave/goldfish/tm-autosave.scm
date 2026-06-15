(import (texmacs protocol))

(define (welcome)
  (flush-verbatim "autosave"))
  
(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (if (string=? code "")
        #t
        (flush-verbatim code))))

(define (repl)
  (read-eval-print)
  (repl))

(welcome)
(repl)
