;
; Copyright (C) 2026 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
;;; (liii uri-convert) - URI 转换函数
;;; 本模块包含 URI 的转换函数（uri->string, uri->human-string）

(define-library (liii uri-convert)
  (import (scheme base)
          (liii error)
          (liii uri-record)
          (liii uri-parse)
  ) ;import

  ;;; ---------- 导出接口 ----------
  (export uri->string)
  (export uri->human-string)

  (begin
    ;;; ---------- 转换函数 ----------
    (define (uri->string uri-obj)
      (if (not (uri? uri-obj))
        (error "uri->string: expected uri")
        (let* ((scheme (uri-scheme-raw uri-obj))
               (netloc (uri-netloc-raw uri-obj))
               (path (uri-path-raw uri-obj))
               (query (uri-query-raw uri-obj))
               (fragment (uri-fragment-raw uri-obj)))
          (string-append
            (if scheme (string-append scheme ":") "")
            (if (and scheme (not (string=? netloc ""))) "//" "")
            (if (not (string=? netloc "")) netloc "")
            (or path "")
            (if (null? query) "" (string-append "?" (alist->query-string query)))
            (if fragment (string-append "#" fragment) ""))
        ) ;let
      ) ;if
    ) ;define

    (define (uri->human-string uri-obj)
      ;; 生成人类可读的 URI 字符串（去除敏感信息如密码）
      (if (not (uri? uri-obj))
        (error "uri->human-string: expected uri")
        (let* ((scheme (uri-scheme-raw uri-obj))
               (netloc-parts (parse-netloc (uri-netloc-raw uri-obj)))
               (host (list-ref netloc-parts 2))
               (port (list-ref netloc-parts 3))
               (path (uri-path-raw uri-obj)))
          (string-append
            (if scheme (string-append scheme "://") "")
            (or host "")
            (if port (string-append ":" (number->string port)) "")
            (or path "/"))
        ) ;let
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
