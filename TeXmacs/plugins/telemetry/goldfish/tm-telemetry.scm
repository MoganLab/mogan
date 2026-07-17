
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-telemetry.scm
;; DESCRIPTION : Fake telemetry plugin entry for goldfish
;; COPYRIGHT   : (C) 2026 Mogan Developers
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base) (liii base) (liii path) (liii json) (srfi srfi-19))

(import (texmacs protocol))

(define (telemetry-home)
  (path-from-env "TEXMACS_HOME_PATH")
) ;define

(define telemetry-log-path
  (path->string (path-join (telemetry-home) "telemetry.log"))
) ;define

;; 插件进程与主进程的握手：启动时必须 flush 插件名 "telemetry"，
;; 否则主进程不会进入 DATA_COMMAND 状态、无法投递后续负载。

(define (welcome)
  (flush-verbatim "telemetry")
) ;define

;; 主进程发送的 payload 可能是 (document "...") 或直接字符串，
;; 统一提取为字符串；无法识别时返回空字符串。

(define (document->string doc)
  (cond ((and (pair? doc) (eq? (car doc) 'document) (= (length doc) 2)) (cadr doc))
        ((string? doc) doc)
        (else "")
  ) ;cond
) ;define

(define (telemetry-log message)
  (path-append-text telemetry-log-path (string-append message "\n"))
) ;define

;; 把 payload 中的 Unix 时间戳转换成本地可读时间，方便日志排查。
;; 如果解析失败或没有时间戳，则按原样记录。

(define (format-payload s)
  (catch #t
    (lambda ()
      (let* ((json (string->json s)) (ts (json-ref json "time")))
        (if (number? ts)
          (let* ((sec (inexact->exact (truncate ts)))
                 (date (time-utc->date (make-time TIME-UTC 0 sec)))
                 (date-str (date->string date "~Y-~m-~d ~H:~M:~S"))
                ) ;
            (json->string (json-push (json-drop json "time") "time" date-str))
          ) ;let*
          (json->string json)
        ) ;if
      ) ;let*
    ) ;lambda
    (lambda args s)
  ) ;catch
) ;define

(define (handle-telemetry payload)
  (let ((s (document->string payload)))
    (if (string=? s "")
      (flush-verbatim "telemetry skipped: empty payload")
      (begin
        (telemetry-log (format-payload s))
        (flush-verbatim "telemetry logged")
      ) ;begin
    ) ;if
  ) ;let
) ;define

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (if (or (eof-object? code) (string=? code ""))
      #t
      (begin
        (handle-telemetry code)
        (read-eval-print)
      ) ;begin
    ) ;if
  ) ;let
) ;define

(welcome)
(read-eval-print)
