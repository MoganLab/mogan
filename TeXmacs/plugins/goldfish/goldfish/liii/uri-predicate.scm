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
;;; (liii uri-predicate) - URI 谓词函数
;;; 本模块包含 URI 相关的所有谓词函数

(define-library (liii uri-predicate)
  (import (scheme base)
          (liii uri-record)
  ) ;import

  ;;; ---------- 导出接口 ----------
  (export uri-absolute?)
  (export uri-relative?)
  (export uri-default-port)
  (export uri-default-port?)
  (export uri-network-scheme?)

  (begin
    ;;; ---------- 常量定义 ----------
    ;; 默认端口映射表
    (define DEFAULT-PORTS
      '(("http" . 80)
        ("https" . 443)
        ("ftp" . 21)
        ("ssh" . 22)
        ("smtp" . 25)
        ("dns" . 53)
        ("pop3" . 110)
        ("imap" . 143)
        ("ldap" . 389)
    ) ;define
  ) ;begin

    ;;; ---------- 谓词函数 ----------
    ;; 检查是否为绝对 URI（有 scheme）
    (define (uri-absolute? uri-obj)
      (and (uri? uri-obj)
           (uri-scheme-raw uri-obj)
           (not (string=? (uri-scheme-raw uri-obj) ""))
      ) ;and
    ) ;define

    ;; 检查是否为相对 URI（无 scheme）
    (define (uri-relative? uri-obj)
      (and (uri? uri-obj)
           (or (not (uri-scheme-raw uri-obj))
               (string=? (uri-scheme-raw uri-obj) "")
           ) ;or
      ) ;and
    ) ;define

    ;; 获取 scheme 的默认端口
    (define (uri-default-port scheme)
      (let ((pair (assoc scheme DEFAULT-PORTS)))
        (if pair (cdr pair) #f)
      ) ;let
    ) ;define

    ;; 检查 URI 是否使用默认端口
    (define (uri-default-port? uri-obj)
      (let* ((scheme (uri-scheme-raw uri-obj))
             (explicit-port #f)  ; 简化实现，实际需要解析 netloc
             (default-port (and scheme (uri-default-port scheme))))
        (and default-port
             explicit-port
             (= explicit-port default-port)
        ) ;and
      ) ;let*
    ) ;define

    ;; 检查是否为网络 scheme
    (define (uri-network-scheme? scheme)
      (and (assoc scheme DEFAULT-PORTS) #t))
    ) ;define

) ;define-library
