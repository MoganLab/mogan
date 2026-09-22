;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6102.scm
;; DESCRIPTION : Test for AI_CHAT telemetry event
;; COPYRIGHT   : (C) 2026 Mogan Developers
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii path))

(check-set-mode! 'report-failed)

(use-modules (telemetry telemetry-utils))
(use-modules (telemetry telemetry-track))

(define (test_6102)
  ;; 验证 AI_CHAT 属于重要事件，可立即触发上传
  (check (important-event? "AI_CHAT") => #t)
  (check (important-event? "UNKNOWN_EVENT") => #f)

  ;; 在非社区版下验证 track-event 触发立即落盘
  (when (not (community-stem?))
    (let ((old-pref (get-preference "telemetry")))
      (set-preference "telemetry" "1")
      (set! *telemetry-event-queue* '())
      (track-event "AI_CHAT" '())
      ;; 重要事件 track-event 后会立即 flush，队列长度被清零
      (check (telemetry-queue-length) => 0)
      ;; 清理生成的文件与 meta
      (let ((meta (telemetry-read-meta)))
        (for-each (lambda (entry)
                    (let ((f (assoc-ref entry "filename")))
                      (when f
                        (let ((p (telemetry-full-path f)))
                          (when (path-exists? p)
                            (path-unlink p)
                          ) ;when
                        ) ;let
                      ) ;when
                    ) ;let
                  ) ;lambda
          meta
        ) ;for-each
      ) ;let
      (telemetry-write-meta '())
      (if old-pref
        (set-preference "telemetry" old-pref)
        (reset-preference "telemetry")
      ) ;if
    ) ;let
  ) ;when

  (check-report)
) ;define
