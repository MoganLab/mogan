;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-autosave.scm
;; DESCRIPTION : Fake autosave plugin implemented in Scheme/Goldfish
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
        (liii json)
        (liii path))

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

(define (handle-request raw-json)
  (let* ((payload (string->json raw-json))
         (lock (lock-path payload)))
    (path-write-text lock raw-json)
    (path-unlink lock #t)
    (flush-scheme "ok")))

(define (repl)
  (let ((raw-json (read-paragraph-by-visible-eof)))
    (when (not (string=? raw-json ""))
      (handle-request raw-json))
    (repl)))

(repl)
