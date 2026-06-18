
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-telemetry.scm
;; DESCRIPTION : Telemetry initialization and periodic flush
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry init-telemetry)
  (:use (telemetry telemetry-track) (telemetry telemetry-utils))
) ;texmacs-module

(import (scheme base))

(define telemetry-scheduled? #f)

(define (telemetry-scheduler-step)
  (when (telemetry-enabled?)
    (telemetry-flush-if-needed)
  ) ;when
  (telemetry-delayed)
) ;define

(define (telemetry-delayed)
  (delayed (:pause (telemetry-get-flush-interval)) (telemetry-scheduler-step))
) ;define

(define (telemetry-clean-orphans)
  ;; 启动时清理：删除不在 meta 列表中的孤儿 jsonl
  (let* ((meta (telemetry-read-meta))
         (valid-files (map (lambda (e) (assoc-ref e "filename")) meta))
         (dir-url (system->url (telemetry-main-dir)))
         (pattern (url-append dir-url (url-wildcard "*.jsonl")))
         (files (url->list (url-expand (url-complete pattern "fr"))))
        ) ;
    (for-each (lambda (f)
                (let ((fname (url->string (url-tail f))))
                  (when (and (string-starts? fname "detail-telemetry-")
                          (not (member fname valid-files))
                        ) ;and
                    (catch #t (lambda () (path-unlink (url->system f))) (lambda args #f))
                  ) ;when
                ) ;let
              ) ;lambda
      files
    ) ;for-each
  ) ;let*
) ;define

(define-public (init-telemetry)
  (if telemetry-scheduled?
    (display "[telemetry] init: already initialized\n")
    (if (telemetry-enabled?)
      (begin
        (telemetry-clean-orphans)
        (set! telemetry-scheduled? #t)
        (display (string-append "[telemetry] init: enabled, buffer="
                   (number->string (telemetry-get-buffer-size))
                   ", interval="
                   (number->string (telemetry-get-flush-interval))
                   "ms\n"
                 ) ;string-append
        ) ;display
        (on-exit (catch #t
                   (lambda () (track-event "CLOSE" '()) (telemetry-flush-if-needed))
                   (lambda args
                     (display (string-append "[telemetry] error: exit flush failed: "
                                (object->string args)
                                "\n"
                              ) ;string-append
                     ) ;display
                   ) ;lambda
                 ) ;catch
        ) ;on-exit
        (telemetry-delayed)
      ) ;begin
      (display "[telemetry] init: disabled\n")
    ) ;if
  ) ;if
) ;define-public
