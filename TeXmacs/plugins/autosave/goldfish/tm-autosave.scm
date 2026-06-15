(import (texmacs protocol))
(import (liii path))

(define (welcome)
  (flush-verbatim "autosave")
) ;define

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (path-append-text "/tmp/debug.log" code)
    (if (string=? code "") #t (flush-verbatim code))
  ) ;let
) ;define

(define (repl)
  (read-eval-print)
  (repl)
) ;define

(welcome)
(repl)
