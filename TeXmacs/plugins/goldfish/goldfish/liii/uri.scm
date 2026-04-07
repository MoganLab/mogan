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
;;; (liii uri) - URI 处理库
;;; 基于 RFC 3986 实现的 URI 解析与构造库
;;; 本模块是统一接口，实际实现分散在 uri-record、uri-parse、uri-predicate、
;;; uri-compare、uri-make、uri-transform、uri-convert 等子模块中

(define-library (liii uri)
  (import (scheme base)
          (liii uri-parse)
          (liii uri-record)
          (liii uri-predicate)
          (liii uri-compare)
          (liii uri-make)
          (liii uri-transform)
          (liii uri-convert)
  ) ;import

  ;;; ---------- 导出接口 ----------
  ;; 从 uri-record 重新导出
  (export make-uri-raw uri?)
  (export uri-error)

  ;; scheme/host/port 访问器
  (export uri-scheme uri-raw-scheme)
  (export uri-host uri-raw-host)
  (export uri-port uri-explicit-port)

  ;; authority 访问器
  (export uri-user uri-raw-user)
  (export uri-password uri-raw-password)
  (export uri-authority uri-raw-authority)

  ;; path 访问器
  (export uri-path uri-raw-path uri-path->list)

  ;; query 访问器
  (export uri-query uri-query-string uri-query-ref uri-query-ref*)

  ;; fragment 访问器
  (export uri-fragment uri-raw-fragment)

  ;; 路径相关访问器
  (export uri-parent uri-name uri-suffix uri-suffixes)

  ;; 从 uri-predicate 重新导出
  (export uri-absolute? uri-relative?)
  (export uri-default-port uri-default-port? uri-network-scheme?)

  ;; 从 uri-make 重新导出
  (export make-uri uri-build string->uri)

  ;; 从 uri-transform 重新导出 - with- 系列函数
  (export uri-with-scheme)
  (export uri-with-host)
  (export uri-with-port)
  (export uri-with-path)
  (export uri-with-fragment)

  ;; 从 uri-transform 重新导出 - query 更新函数
  (export uri-update-query)
  (export uri-extend-query)
  (export uri-without-query)
  (export uri-without-query-param)

  ;; 从 uri-transform 重新导出 - 路径操作函数
  (export uri-join-path)
  (export uri-with-name)
  (export uri-with-suffix)
  (export uri-join)

  ;; 从 uri-convert 重新导出 - 转换函数
  (export uri->string)
  (export uri->human-string)

  ;; 从 uri-compare 重新导出
  (export uri=? uri<? uri>? uri-hash)

  ;; 编码/解码函数（从 uri-record 重新导出）
  (export uri-encode)
  (export uri-decode)
  (export uri-encode-path)
  (export uri-decode-path)

  ;; 查询字符串处理（从 uri-record 重新导出）
  (export query-string->alist)
  (export alist->query-string)
) ;define-library
