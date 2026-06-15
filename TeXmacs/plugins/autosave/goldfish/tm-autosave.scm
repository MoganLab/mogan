;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-autosave.scm
;; DESCRIPTION : Fake autosave plugin implemented in Scheme/Goldfish
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
        (liii json)
        (liii path))

(define autosave-debug-log
  (path-from-string "/tmp/autosave.log"))

(define (append-debug-log msg)
  (path-append-text autosave-debug-log msg))

(define (welcome)
  (append-debug-log "[startup] welcome\n")
  (flush-prompt "autosave> ")
  (flush-verbatim "Autosave Plugin")
) ;define

(define (json-path-ref payload)
  (let ((v1 (json-ref payload "path"))
        (v2 (json-ref payload "path:")))
    (cond ((string? v1) v1)
          ((string? v2) v2)
          (else ""))))

(define (request-directory payload)
  (let* ((raw-path (json-path-ref payload))
         (target (if (string=? raw-path "")
                     (path-cwd)
                     (path-from-string raw-path))))
    (if (path-dir? target)
        target
        (path-parent target))))

(define (lock-path payload)
  (path-join (request-directory payload) "lock"))

(define (log-path payload)
  autosave-debug-log)

(define (handle-request raw-json)
  (let* ((payload (string->json raw-json))
         (lock (lock-path payload))
         (log-file (log-path payload)))
    (append-debug-log "[handle-request] begin\n")
    (append-debug-log (string-append "[handle-request] raw-json=" raw-json "\n"))
    (append-debug-log (string-append "[handle-request] lock=" (path->string lock) "\n"))
    (path-append-text log-file (string-append raw-json "\n"))
    (path-write-text lock raw-json)
    (append-debug-log "[handle-request] lock-written\n")
    (path-unlink lock #t)
    (append-debug-log "[handle-request] lock-removed\n")
    (append-debug-log "[handle-request] reply=ok\n")
    (flush-scheme "ok")))

(define (safe-read-request)
  (catch #t
    (lambda () (read-paragraph-by-visible-eof))
    (lambda args
      (append-debug-log (string-append "[repl] read-error=" (object->string args) "\n"))
      #f)))

(define (repl)
  (append-debug-log "[repl] waiting\n")
  (let ((raw-json (safe-read-request)))
    (if (not raw-json)
      (append-debug-log "[repl] stop-on-eof\n")
      (begin
        (append-debug-log (string-append "[repl] received=" raw-json "\n"))
        (when (not (string=? raw-json ""))
          (handle-request raw-json))
        (repl)
      ) ;begin
    ) ;if
  ) ;let
)

(append-debug-log "[startup] tm-autosave loaded\n")
(welcome)
(repl)
