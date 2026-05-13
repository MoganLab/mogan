;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

;; SRFI-215: Central Log Exchange
;; 严格按照 SRFI-215 标准实现

(define-library (srfi srfi-215)
  (export send-log
    current-log-fields
    current-log-callback
    EMERGENCY
    ALERT
    CRITICAL
    ERROR
    WARNING
    NOTICE
    INFO
    DEBUG
  ) ;export
  (import (scheme base) (scheme write))
  (begin

    ;; 日志级别常量（来自 RFC 5424 Syslog Protocol）
    (define EMERGENCY 0)
    (define ALERT 1)
    (define CRITICAL 2)
    (define ERROR 3)
    (define WARNING 4)
    (define NOTICE 5)
    (define INFO 6)
    (define DEBUG 7)

    ;; 将 plist 转为 alist，同时进行值类型转换
    (define (field-list->alist plist)
      (let f
        ((fields plist))
        (cond ((null? fields) '())
              ((or (not (pair? fields)) (not (pair? (cdr fields))))
               (error "short field list" plist)
              ) ;
              (else (let ((k (car fields)) (v (cadr fields)))
                      (if (not v)
                        (f (cddr fields))
                        (let ((k^ (cond ((symbol? k) k) (else (error "invalid key" k plist))))
                              (v^ (cond ((string? v) v)
                                        ((and (integer? v) (exact? v)) v)
                                        ((bytevector? v) v)
                                        (else (let ((p (open-output-string))) (write v p) (get-output-string p)))
                                  ) ;cond
                              ) ;v^
                             ) ;
                          (cons (cons k^ v^) (f (cddr fields)))
                        ) ;let
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    ;; current-log-fields: 全局默认字段，值为 plist
    ;; (current-log-fields) 读取，(current-log-fields plist) 设置
    (define current-log-fields
      (let ((value '()))
        (lambda args (if (null? args) value (set! value (car args))))
      ) ;let
    ) ;define

    ;; current-log-callback: 日志回调函数
    ;; (current-log-callback) 读取，(current-log-callback proc) 设置
    (define current-log-callback
      (let ((value (lambda (log-entry) (values))))
        (lambda args (if (null? args) value (set! value (car args))))
      ) ;let
    ) ;define

    ;; send-log: 发送日志消息
    (define (send-log severity message . plist)
      (unless (and (exact? severity) (integer? severity) (<= 0 severity 7))
        (error 'wrong-type-arg
          "send-log: expected a severity from 0 to 7"
          severity
          message
          plist
        ) ;error
      ) ;unless
      (unless (string? message)
        (error 'wrong-type-arg
          "send-log: expected message to be a string"
          severity
          message
          plist
        ) ;error
      ) ;unless
      (let* ((fields (append plist (current-log-fields)))
             (alist (field-list->alist fields))
             (callback (current-log-callback))
            ) ;
        (when (procedure? callback)
          (callback `((SEVERITY unquote severity)
                      (MESSAGE unquote message)
                      ,@alist))
        ) ;when
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
