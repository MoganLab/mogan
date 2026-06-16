(import (texmacs protocol))
(import (liii path))
(import (liii json))
(import (liii os))
(import (srfi srfi-19))

;; 插件进程与主进程的握手：启动时必须 flush 插件名 "autosave"，
;; 否则主进程不会进入 DATA_COMMAND 状态、无法投递后续负载。

(define (welcome)
  (flush-verbatim "autosave")
) ;define

(define (autosave-log message)
  (path-append-text "/tmp/debug.log" (string-append message "\n"))
) ;define

(define (ensure-dir path)
  (when (not (path-exists? path))
    (mkdir (path->string path))
  ) ;when
  (and (path-exists? path) (path-dir? path))
) ;define

(define (autosave-home)
  (path-from-env "TEXMACS_HOME_PATH")
) ;define

(define (autosave-dir doc-id)
  (path-join (autosave-home) "system" "backup" doc-id)
) ;define

(define (autosave-target doc-id)
  (let* ((date (current-date))
         (stamp (date->string date "~Y~m~d_~H~M~S"))
        ) ;
    (path-join (autosave-dir doc-id) (string-append stamp ".tmu"))
  ) ;let*
) ;define

(define (ensure-parent-dir doc-id)
  (let ((home (autosave-home))
        (system-dir (path-join (autosave-home) "system"))
        (backup-dir (path-join (autosave-home) "system" "backup"))
        (doc-dir (autosave-dir doc-id))
       ) ;
    (and home
      (ensure-dir home)
      (ensure-dir system-dir)
      (ensure-dir backup-dir)
      (ensure-dir doc-dir)
    ) ;and
  ) ;let
) ;define

(define (document->string doc)
  (cond ((and (pair? doc) (eq? (car doc) 'document) (= (length doc) 2))
         (cadr doc)
        ) ;
        ((string? doc) doc)
        (else "")
  ) ;cond
) ;define

(define (handle-copy payload)
  (catch #t
    (lambda ()
      (let* ((json (string->json payload))
             (source (json-ref json "path"))
             (doc-id (json-ref json "id"))
             (target (if (string? doc-id) (autosave-target doc-id) ""))
            ) ;
        (if (or (not (string? source))
              (not (string? doc-id))
              (string=? source "")
              (string=? doc-id "")
            ) ;or
          (autosave-log "autosave copy skipped: missing source/id")
          (if (not (file-exists? source))
            (autosave-log (string-append "autosave copy skipped: source missing " source))
            (when (ensure-parent-dir doc-id)
              (if (path-copy source target)
                (autosave-log (string-append "autosave copied " source " -> " target))
                (autosave-log (string-append "autosave copy failed " source " -> " target))
              ) ;if
            ) ;when
          ) ;if
        ) ;if
      ) ;let*
    ) ;lambda
    (lambda args
      (autosave-log (string-append "autosave copy exception " (object->string args)))
    ) ;lambda
  ) ;catch
) ;define

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (let ((payload (document->string code)))
      (if (string=? payload "")
        #t
        (begin
          (handle-copy payload)
          (flush-verbatim payload)
        ) ;begin
      ) ;if
    ) ;let
  ) ;let
) ;define

(welcome)
(read-eval-print)
