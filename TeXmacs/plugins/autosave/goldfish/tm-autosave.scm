(import (texmacs protocol))
(import (liii path))

;; 插件进程与主进程的握手：启动时必须 flush 插件名 "autosave"，
;; 否则主进程不会进入 DATA_COMMAND 状态、无法投递后续负载。

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
