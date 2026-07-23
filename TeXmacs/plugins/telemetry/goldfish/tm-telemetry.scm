
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

(import (scheme base)
  (liii base)
  (liii path)
  (liii string)
  (liii json)
  (srfi srfi-19)
) ;import

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

;; 把事件 JSON 里的 Unix 时间戳转成本地可读时间，方便日志排查；
;; 解析失败或没有时间戳则按原样记录。

(define (format-event-line s)
  (catch #t
    (lambda ()
      (let* ((json (string->json s)) (ts (json-ref json "timestamp")))
        (if (number? ts)
          (let* ((sec (inexact->exact (truncate ts)))
                 (date (time-utc->date (make-time TIME-UTC 0 sec)))
                 (date-str (date->string date "~Y-~m-~d ~H:~M:~S"))
                ) ;
            (json->string (json-push (json-drop json "timestamp") "timestamp" date-str))
          ) ;let*
          (json->string json)
        ) ;if
      ) ;let*
    ) ;lambda
    (lambda args s)
  ) ;catch
) ;define

;; process-events main-dir
;; 读取 main-dir 下 meta 登记的所有 jsonl，逐条把事件写入 telemetry.log，
;; 再删除已处理的 jsonl 并清空 meta（模拟真实 worker 上传成功后的清理），
;; 避免下次触发时重复记录。返回处理的事件条数。

(define (process-events main-dir)
  (let ((meta-path (path->string (path-join main-dir "main-telemetry.json"))))
    (if (not (path-exists? meta-path))
      0
      (let ((entries (vector->list (string->json (path-read-text meta-path)))) (count 0))
        (for-each (lambda (entry)
                    (let ((file-path (path->string (path-join main-dir (json-ref entry "filename")))))
                      (when (path-exists? file-path)
                        (for-each (lambda (line)
                                    (when (not (string=? line ""))
                                      (set! count (+ count 1))
                                      (telemetry-log (format-event-line line))
                                    ) ;when
                                  ) ;lambda
                          (string-split (path-read-text file-path) #\newline)
                        ) ;for-each
                        (path-unlink file-path)
                      ) ;when
                    ) ;let
                  ) ;lambda
          entries
        ) ;for-each
        (path-write-text meta-path "[]")
        count
      ) ;let
    ) ;if
  ) ;let
) ;define

;; 上报触发 payload 只含 event/main-dir/api-url/api-key，事件本体在
;; main-dir 的 jsonl 里；有 main-dir 时按 worker 语义消费事件，
;; 否则退化为原样记录 payload。

(define (handle-telemetry payload)
  (let* ((s (document->string payload))
         (json (catch #t (lambda () (string->json s)) (lambda args #f)))
        ) ;
    (cond ((string=? s "") (flush-verbatim "telemetry skipped: empty payload"))
          ((and (pair? json) (string? (json-ref json "main-dir")))
           (let ((n (catch #t
                      (lambda () (process-events (json-ref json "main-dir")))
                      (lambda args -1)
                    ) ;catch
                 ) ;n
                 (trigger (json-ref json "event"))
                ) ;
             (telemetry-log (string-append "upload trigger: "
                              (if (string? trigger) trigger "?")
                              ", "
                              (number->string n)
                              " events"
                            ) ;string-append
             ) ;telemetry-log
             (flush-verbatim (string-append "telemetry logged " (number->string n) " events")
             ) ;flush-verbatim
           ) ;let
          ) ;
          (else (telemetry-log s) (flush-verbatim "telemetry logged"))
    ) ;cond
  ) ;let*
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
