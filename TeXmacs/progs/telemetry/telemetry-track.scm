
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-track.scm
;; DESCRIPTION : Telemetry event tracking with memory queue and flush
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry telemetry-track)
  (:use (telemetry telemetry-utils) (utils plugins plugin-eval))
) ;texmacs-module

(import (scheme base)
  (liii base)
  (liii os)
  (liii path)
  (liii string)
  (liii list)
) ;import

(define-public *telemetry-event-queue* '())

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Upload trigger: notify the telemetry plugin that an upload should start
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; telemetry-api-url
;; 上报接口地址。
;; worker 端 telemetry-build-request 构造 {"events":[...]} 请求体发往此 url。
(define (telemetry-api-url)
  "https://telemetry.liiistem.cn/api/v1/telemetry/events"
) ;define

;; telemetry-current-token
;; 读取当前登录用户的 OAuth2 token；未登录或 account 未加载时返回空串。
;; 空串时 worker 走 auth 分支，不会发送无认证请求。
(define (telemetry-current-token)
  (catch #t (lambda () (account-load-token)) (lambda args ""))
) ;define

;; build-upload-payload event
;; 构造投递给 worker 的 payload JSON 字符串。
;; worker 端 parse-telemetry-payload 读取 main-dir/api-url/api-key 等字段。
;; event 仅供日志，worker 不消费。
(define (build-upload-payload event)
  (let ((payload `((,"event" unquote event)
                   (,"main-dir" unquote (telemetry-main-dir))
                   (,"api-url" unquote (telemetry-api-url))
                   (,"api-key" unquote (telemetry-current-token)))
            ) ;payload
       ) ;
    (telemetry->json payload)
  ) ;let
) ;define

(define-public (upload-events event)
  (if (not (telemetry-enabled?))
    #f
    (let* ((ts (telemetry-now))
           (json (build-upload-payload event))
          ) ;
      (silent-feed* "telemetry" "" `(document ,json) (lambda (r) (noop)) '())
      (display (string-append "[telemetry] upload triggered by "
                 event
                 " (ts="
                 (number->string ts)
                 ")\n"
               ) ;string-append
      ) ;display
      #t
    ) ;let*
  ) ;if
) ;define-public

(define-public (important-event? event-type)
  (or (string=? event-type "HEART_BEAT")
    (string=? event-type "STARTUP")
    (string=? event-type "TUTORIAL")
    (string=? event-type "INVITE_CLICK")
    (string=? event-type "VIP_CLICK")
  ) ;or
) ;define-public

(define-public (track-event event-type properties)
  (if (not (telemetry-enabled?))
    #f
    (if (and (string? event-type) (not (string-null? event-type)))
      (begin
        (set! *telemetry-event-queue*
          (cons (telemetry-make-event event-type properties) *telemetry-event-queue*)
        ) ;set!
        (let ((len (length *telemetry-event-queue*)))
          (display (string-append "[telemetry] track: "
                     event-type
                     " (queue: "
                     (number->string len)
                     "/"
                     (number->string (telemetry-get-buffer-size))
                     ")\n"
                   ) ;string-append
          ) ;display
          (if (> len telemetry-max-queue-size)
            (begin
              (set! *telemetry-event-queue*
                (list-head *telemetry-event-queue* telemetry-max-queue-size)
              ) ;set!
              (display (string-append "[telemetry] warn: queue truncated to "
                         (number->string telemetry-max-queue-size)
                         "\n"
                       ) ;string-append
              ) ;display
            ) ;begin
          ) ;if
          (if (important-event? event-type)
            (begin
              (telemetry-flush)
              (upload-events event-type)
            ) ;begin
          ) ;if
          (if (>= len (telemetry-get-buffer-size)) (telemetry-flush))
        ) ;let
        #t
      ) ;begin
      #f
    ) ;if
  ) ;if
) ;define-public

(define-public (telemetry-queue-length) (length *telemetry-event-queue*))

(define-public (telemetry-flush-if-needed)
  (if (not (telemetry-enabled?))
    #t
    (if (not (null? *telemetry-event-queue*)) (telemetry-flush) #t)
  ) ;if
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Flush implementation: independent jsonl files + atomic meta update
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (telemetry-write-pending events)
  (if (null? events)
    #t
    (let* ((filename (telemetry-generate-filename))
           (filepath (telemetry-full-path filename))
           (lines (map telemetry->json events))
          ) ;
      (catch #t
        (lambda ()
          (let ((text (string-append (string-join lines "\n") "\n")))
            (string-save text (system->url filepath))
            (if (telemetry-meta-add-entry filename)
              (begin
                (display (string-append "[telemetry] flush: "
                           (number->string (length events))
                           " events -> "
                           filename
                           "\n"
                         ) ;string-append
                ) ;display
                #t
              ) ;begin
              (begin
                (display (string-append "[telemetry] error: meta update failed for " filename "\n")
                ) ;display
                #f
              ) ;begin
            ) ;if
          ) ;let
        ) ;lambda
        (lambda args
          (display (string-append "[telemetry] error: write failed: " (object->string args) "\n")
          ) ;display
          #f
        ) ;lambda
      ) ;catch
    ) ;let*
  ) ;if
) ;define-public

(define-public (telemetry-flush)
  (if (null? *telemetry-event-queue*)
    #t
    (let ((ok? (telemetry-write-pending (reverse *telemetry-event-queue*))))
      (if ok?
        (begin
          (set! *telemetry-event-queue* '())
          #t
        ) ;begin
        (begin
          (display (string-append "[telemetry] error: flush failed, keeping "
                     (number->string (length *telemetry-event-queue*))
                     " events in memory queue\n"
                   ) ;string-append
          ) ;display
          #f
        ) ;begin
      ) ;if
    ) ;let
  ) ;if
) ;define-public
