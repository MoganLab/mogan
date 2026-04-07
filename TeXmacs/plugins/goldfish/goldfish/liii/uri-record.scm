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
;;; (liii uri-record) - URI 记录类型定义
;;; 本模块包含 URI 记录类型的定义和所有访问器函数

(define-library (liii uri-record)
  (import (scheme base)
          (scheme char)
          (liii string)
          (liii list)
          (liii error)
          (liii unicode)
          (liii uri-parse)
  ) ;import

  ;;; ---------- 导出接口 ----------
  ;; 错误条件类型 - 使用 liii/error 风格的 symbol-based 错误
  (export uri-error)

  ;; 记录类型构造函数和谓词
  (export make-uri-raw)
  (export uri?)

  ;; 原始字段访问器（由 define-record-type 生成）
  (export uri-scheme-raw uri-scheme-set!)
  (export uri-netloc-raw uri-netloc-set!)
  (export uri-path-raw uri-path-set!)
  (export uri-query-raw uri-query-set!)
  (export uri-fragment-raw uri-fragment-set!)

  ;; scheme/host/port 访问器
  (export uri-scheme)
  (export uri-raw-scheme)
  (export uri-host)
  (export uri-raw-host)
  (export uri-port)
  (export uri-explicit-port)

  ;; authority 访问器
  (export uri-user)
  (export uri-raw-user)
  (export uri-password)
  (export uri-raw-password)
  (export uri-authority)
  (export uri-raw-authority)

  ;; path 访问器
  (export uri-path)
  (export uri-raw-path)
  (export uri-path->list)

  ;; query 访问器
  (export uri-query)
  (export uri-query-string)
  (export uri-query-ref)
  (export uri-query-ref*)

  ;; fragment 访问器
  (export uri-fragment)
  (export uri-raw-fragment)

  ;; 路径相关访问器
  (export uri-parent)
  (export uri-name)
  (export uri-suffix)
  (export uri-suffixes)

  ;; 编码/解码函数
  (export uri-encode)
  (export uri-decode)
  (export uri-encode-path)
  (export uri-decode-path)

  ;; 查询字符串处理
  (export query-string->alist)
  (export alist->query-string)

  ;; 辅助函数（供其他模块使用）
  (export parse-netloc)
  (export build-netloc)

  (begin
    ;;; ---------- 错误条件类型 ----------
    ;; URI 错误类型，继承自 liii/error 的风格
    (define (uri-error . args)
      (apply error 'uri-error args)
    ) ;define

    ;;; ---------- URI 记录类型 ----------
    ;; URI 记录类型，包含五个核心组件
    ;; scheme: 协议方案（如 http, https, ftp）
    ;; netloc: 网络位置（包含 user, password, host, port）
    ;; path: 路径部分
    ;; query: 查询字符串（alist 格式）
    ;; fragment: 片段标识符
    (define-record-type uri
      (make-uri-raw scheme netloc path query fragment)
      uri?
      (scheme uri-scheme-raw uri-scheme-set!)
      (netloc uri-netloc-raw uri-netloc-set!)
      (path uri-path-raw uri-path-set!)
      (query uri-query-raw uri-query-set!)
      (fragment uri-fragment-raw uri-fragment-set!)
    ) ;define-record-type

    ;;; ---------- 编码/解码工具函数 ----------
    ;; URI 允许的字符集
    (define UNRESERVED_CHARS
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
    ) ;define

    ;; 检查字符是否在不编码字符集中
    (define (unreserved-char? c)
      (string-contains UNRESERVED_CHARS (string c))
    ) ;define

    ;; 检查字节是否在不编码字符集中（用于UTF-8编码）
    (define (unreserved-byte? b)
      (and (>= b 0) (<= b 127)
           (string-contains UNRESERVED_CHARS (string (integer->char b)))
      ) ;and
    ) ;define

    ;; 十六进制数字转字符
    (define (hex-digit n)
      (if (< n 10)
        (integer->char (+ n (char->integer #\0)))
        (integer->char (+ (- n 10) (char->integer #\A)))
      ) ;if
    ) ;define

    ;; 字符转十六进制数字
    (define (char->hex c)
      (let ((code (char->integer c)))
        (cond
          ((and (>= code (char->integer #\0)) (<= code (char->integer #\9)))
           (- code (char->integer #\0))
          ) ;
          ((and (>= code (char->integer #\A)) (<= code (char->integer #\F)))
           (+ 10 (- code (char->integer #\A)))
          ) ;
          ((and (>= code (char->integer #\a)) (<= code (char->integer #\f)))
           (+ 10 (- code (char->integer #\a)))
          ) ;
          (else #f)
        ) ;cond
      ) ;let
    ) ;define

    ;; 辅助函数：遍历字节向量并构建结果列表
    (define (bytevector-fold-encode bv encode-fn)
      (let ((len (bytevector-length bv)))
        (let loop ((i 0) (result '()))
          (if (>= i len)
            (list->string (reverse result))
            (loop (+ i 1) (encode-fn (bytevector-u8-ref bv i) result))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    ;; 对字符串进行百分比编码（UTF-8 编码）
    (define (uri-encode str)
      (if (not (string? str))
        (type-error "uri-encode: expected string")
        (let ((bv (string->utf8 str)))
          (bytevector-fold-encode
            bv
            (lambda (b result)
              (if (unreserved-byte? b)
                (cons (integer->char b) result)
                (let ((hi (hex-digit (quotient b 16)))
                      (lo (hex-digit (remainder b 16))))
                  (cons lo (cons hi (cons #\% result)))
                ) ;let
              ) ;if
            ) ;lambda
          ) ;bytevector-fold-encode
        ) ;let
      ) ;if
    ) ;define

    ;; 对百分比编码的字符串进行解码
    (define (uri-decode str)
      (if (not (string? str))
        (type-error "uri-decode: expected string")
        (let loop ((chars (string->list str)) (result '()))
          (if (null? chars)
            (list->string (reverse result))
            (let ((c (car chars)))
              (cond
                ((char=? c #\%)
                 (if (and (not (null? (cdr chars)))
                          (not (null? (cddr chars)))
                          (char->hex (cadr chars))
                          (char->hex (caddr chars)))
                   (let ((high (char->hex (cadr chars)))
                         (low (char->hex (caddr chars))))
                     (loop (cdddr chars)
                           (cons (integer->char (+ (* high 16) low)) result)
                     ) ;loop
                   ) ;let
                   (error "uri-decode: invalid percent encoding")
                 ) ;if
                ) ;
                ((char=? c #\+)
                 (loop (cdr chars) (cons #\space result))
                ) ;
                (else
                  (loop (cdr chars) (cons c result))
                ) ;else
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    ;; 路径编码（保留斜杠，UTF-8 编码）
    (define (uri-encode-path path)
      (if (not (string? path))
        (error "uri-encode-path: expected string")
        (let ((bv (string->utf8 path)))
          (bytevector-fold-encode
            bv
            (lambda (b result)
              (cond
                ((= b 47)  ;; #\/ = 47
                 (cons #\/ result)
                ) ;
                ((unreserved-byte? b)
                 (cons (integer->char b) result)
                ) ;
                (else
                  (let ((hi (hex-digit (quotient b 16)))
                        (lo (hex-digit (remainder b 16))))
                    (cons lo (cons hi (cons #\% result)))
                  ) ;let
                ) ;else
              ) ;cond
            ) ;lambda
          ) ;bytevector-fold-encode
        ) ;let
      ) ;if
    ) ;define

    ;; 路径解码（保留斜杠）
    (define (uri-decode-path path)
      ;; 与 uri-decode 相同，但保留斜杠不被特殊处理
      (uri-decode path)
    ) ;define

    ;; 查询字符串解析为 alist
    ;; 例如: "a=1&b=2" -> '(("a" . "1") ("b" . "2"))
    (define (query-string->alist qs)
      (if (or (not (string? qs)) (string=? qs ""))
        '()
        (let ((pairs (string-split qs "&")))
          (map (lambda (pair)
                 (let ((eq-pos (string-index pair #\=)))
                   (if eq-pos
                     (cons (substring pair 0 eq-pos)
                           (uri-decode (substring pair (+ eq-pos 1) (string-length pair)))
                     ) ;cons
                     (cons pair "")
                   ) ;if
                 ) ;let
          ) ;map
            pairs
        ) ;let
      ) ;if
    ) ;define
  ) ;begin

    ;; alist 转为查询字符串
    (define (alist->query-string alist)
      (if (null? alist)
        ""
        (string-join
          (map (lambda (pair)
                 (if (cdr pair)
                   (string-append (car pair) "=" (uri-encode (cdr pair)))
                   (car pair)
                 ) ;if
          ) ;map
               alist
        ) ;string-join
          "&"
      ) ;if
    ) ;define
) ;define-library

    ;;; ---------- 访问器函数 ----------
    ;; scheme 访问器
    (define (uri-scheme uri-obj)
      (uri-scheme-raw uri-obj)
    ) ;define

    (define (uri-raw-scheme uri-obj)
      (uri-scheme-raw uri-obj)
    ) ;define

    ;; host 访问器
    (define (uri-host uri-obj)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (list-ref netloc-parts 2)
      ) ;let
    ) ;define

    (define (uri-raw-host uri-obj)
      (uri-host uri-obj)
    ) ;define

    ;; port 访问器
    (define (uri-port uri-obj)
      (let* ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj)))
             (explicit-port (list-ref netloc-parts 3))
             (scheme (uri-scheme-raw uri-obj)))
        (or explicit-port
            (and scheme (uri-default-port scheme))
            #f
        ) ;or
      ) ;let*
    ) ;define

    (define (uri-explicit-port uri-obj)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (list-ref netloc-parts 3)
      ) ;let
    ) ;define

    ;; 默认端口映射表（供 uri-port 使用）
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
    ) ;define

    ;; 获取 scheme 的默认端口
    (define (uri-default-port scheme)
      (let ((pair (assoc scheme DEFAULT-PORTS)))
        (if pair (cdr pair) #f)
      ) ;let
    ) ;define

    ;; user 访问器
    (define (uri-user uri-obj)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (list-ref netloc-parts 0)
      ) ;let
    ) ;define

    (define (uri-raw-user uri-obj)
      (uri-user uri-obj)
    ) ;define

    ;; password 访问器
    (define (uri-password uri-obj)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (list-ref netloc-parts 1)
      ) ;let
    ) ;define

    (define (uri-raw-password uri-obj)
      (uri-password uri-obj)
    ) ;define

    ;; authority 访问器
    (define (uri-authority uri-obj)
      (uri-netloc-raw uri-obj)
    ) ;define

    (define (uri-raw-authority uri-obj)
      (uri-netloc-raw uri-obj)
    ) ;define

    ;; path 访问器
    (define (uri-path uri-obj)
      (uri-path-raw uri-obj)
    ) ;define

    (define (uri-raw-path uri-obj)
      (uri-path-raw uri-obj)
    ) ;define

    (define (uri-path->list uri-obj)
      (let ((path (uri-path-raw uri-obj)))
        (if (or (not path) (string=? path "") (string=? path "/"))
          '()
          (let ((segments (string-split path "/")))
            (if (string=? (car segments) "")
              (cdr segments)
              segments
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; query 访问器
    (define (uri-query uri-obj)
      (uri-query-raw uri-obj)
    ) ;define

    (define (uri-query-string uri-obj)
      (alist->query-string (uri-query-raw uri-obj))
    ) ;define

    (define (uri-query-ref uri-obj key)
      (let ((query (uri-query-raw uri-obj)))
        (let ((pair (assoc key query)))
          (if pair (cdr pair) #f)
        ) ;let
      ) ;let
    ) ;define

    (define (uri-query-ref* uri-obj key . rest)
      (let ((default (if (null? rest) #f (car rest)))
            (query (uri-query-raw uri-obj)))
        (let ((pair (assoc key query)))
          (if pair (cdr pair) default)
        ) ;let
      ) ;let
    ) ;define

    ;; fragment 访问器
    (define (uri-fragment uri-obj)
      (uri-fragment-raw uri-obj)
    ) ;define

    (define (uri-raw-fragment uri-obj)
      (uri-fragment-raw uri-obj)
    ) ;define

    ;; 路径相关访问器
    (define (uri-parent uri-obj)
      (let ((path (uri-path-raw uri-obj)))
        (if (or (not path) (string=? path "") (string=? path "/"))
          #f
          (let ((last-slash (string-index-right path #\/)))
            (if last-slash
              (substring path 0 last-slash)
              ""
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (uri-name uri-obj)
      (let ((path (uri-path-raw uri-obj)))
        (if (or (not path) (string=? path ""))
          #f
          (let ((last-slash (string-index-right path #\/)))
            (if last-slash
              (substring path (+ last-slash 1) (string-length path))
              path
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (uri-suffix uri-obj)
      (let ((name (uri-name uri-obj)))
        (if name
          (let ((last-dot (string-index-right name #\.)))
            (if last-dot
              (substring name (+ last-dot 1) (string-length name))
              #f
            ) ;if
          ) ;let
          #f
        ) ;if
      ) ;let
    ) ;define

    (define (uri-suffixes uri-obj)
      (let ((name (uri-name uri-obj)))
        (if name
          (let ((parts (string-split name ".")))
            (if (> (length parts) 1)
              (cdr parts)
              '()
            ) ;if
          ) ;let
          '()
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
