
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

(import (liii base) (liii path))
(set! *load-path*
  (cons (path->string (path-parent (port-filename))) *load-path*)
) ;set!

(import (texmacs protocol))

(define telemetry-log-path "/tmp/telemetry.log")

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
  (catch #t
    (lambda () (path-append-text telemetry-log-path (string-append message "\n")))
    (lambda args
      (flush-verbatim (string-append "telemetry error: " (object->string args)))
    ) ;lambda
  ) ;catch
) ;define

(define (handle-telemetry payload)
  (catch #t
    (lambda ()
      (let ((s (document->string payload)))
        (if (string=? s "")
          (flush-verbatim "telemetry skipped: empty payload")
          (begin
            (telemetry-log s)
            (flush-verbatim "telemetry logged")
          ) ;begin
        ) ;if
      ) ;let
    ) ;lambda
    (lambda args
      (flush-verbatim (string-append "telemetry exception: " (object->string args)))
    ) ;lambda
  ) ;catch
) ;define

(define (read-eval-print)
  (catch #t
    (lambda ()
      (let ((code (read-paragraph-by-visible-eof)))
        (if (or (eof-object? code) (string=? code ""))
          #t
          (begin
            (handle-telemetry code)
            (read-eval-print)
          ) ;begin
        ) ;if
      ) ;let
    ) ;lambda
    (lambda args #t)
  ) ;catch
) ;define

(welcome)
(read-eval-print)
