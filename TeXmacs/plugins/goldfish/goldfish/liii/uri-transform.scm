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
;;; (liii uri-transform) - URI 修改函数
;;; 本模块包含 URI 的修改函数（with-系列、query更新、路径操作）

(define-library (liii uri-transform)
  (import (scheme base)
          (scheme char)
          (liii string)
          (liii list)
          (liii error)
          (liii uri-record)
          (liii uri-parse)
  ) ;import

  ;;; ---------- 导出接口 ----------
  ;; with- 系列函数
  (export uri-with-scheme)
  (export uri-with-host)
  (export uri-with-port)
  (export uri-with-path)
  (export uri-with-fragment)

  ;; query 更新函数
  (export uri-update-query)
  (export uri-extend-query)
  (export uri-without-query)
  (export uri-without-query-param)

  ;; 路径操作函数
  (export uri-join-path)
  (export uri-with-name)
  (export uri-with-suffix)
  (export uri-join)

  (begin
    ;;; ---------- 修改函数 ----------
    ;; with- 系列函数
    (define (uri-with-scheme uri-obj new-scheme)
      (make-uri-raw new-scheme
                    (uri-netloc-raw uri-obj)
                    (uri-path-raw uri-obj)
                    (uri-query-raw uri-obj)
                    (uri-fragment-raw uri-obj)
      ) ;make-uri-raw
    ) ;define

    (define (uri-with-host uri-obj new-host)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (make-uri-raw (uri-scheme-raw uri-obj)
                      (build-netloc (list-ref netloc-parts 0)
                                    (list-ref netloc-parts 1)
                                    new-host
                                    (list-ref netloc-parts 3)
                      ) ;build-netloc
                      (uri-path-raw uri-obj)
                      (uri-query-raw uri-obj)
                      (uri-fragment-raw uri-obj)
        ) ;make-uri-raw
      ) ;let
    ) ;define

    (define (uri-with-port uri-obj new-port)
      (let ((netloc-parts (parse-netloc (uri-netloc-raw uri-obj))))
        (make-uri-raw (uri-scheme-raw uri-obj)
                      (build-netloc (list-ref netloc-parts 0)
                                    (list-ref netloc-parts 1)
                                    (list-ref netloc-parts 2)
                                    new-port
                      ) ;build-netloc
                      (uri-path-raw uri-obj)
                      (uri-query-raw uri-obj)
                      (uri-fragment-raw uri-obj)
        ) ;make-uri-raw
      ) ;let
    ) ;define

    (define (uri-with-path uri-obj new-path)
      (make-uri-raw (uri-scheme-raw uri-obj)
                    (uri-netloc-raw uri-obj)
                    new-path
                    (uri-query-raw uri-obj)
                    (uri-fragment-raw uri-obj)
      ) ;make-uri-raw
    ) ;define

    (define (uri-with-fragment uri-obj new-fragment)
      (make-uri-raw (uri-scheme-raw uri-obj)
                    (uri-netloc-raw uri-obj)
                    (uri-path-raw uri-obj)
                    (uri-query-raw uri-obj)
                    new-fragment
      ) ;make-uri-raw
    ) ;define

    ;; query 更新函数
    (define (uri-update-query uri-obj updater)
      (let ((new-query (updater (uri-query-raw uri-obj))))
        (make-uri-raw (uri-scheme-raw uri-obj)
                      (uri-netloc-raw uri-obj)
                      (uri-path-raw uri-obj)
                      new-query
                      (uri-fragment-raw uri-obj)
        ) ;make-uri-raw
      ) ;let
    ) ;define

    (define (uri-extend-query uri-obj alist)
      (uri-update-query uri-obj
                        (lambda (q) (append q alist))
      ) ;uri-update-query
    ) ;define

    (define (uri-without-query uri-obj)
      (uri-update-query uri-obj (lambda (q) '()))
    ) ;define

    (define (uri-without-query-param uri-obj key)
      (uri-update-query uri-obj
                        (lambda (q)
                          (filter (lambda (p) (not (string=? (car p) key))) q)
                        ) ;lambda
      ) ;uri-update-query
    ) ;define

    ;; 路径操作函数
    (define (uri-join-path uri-obj . segments)
      (let ((current-path (uri-path-raw uri-obj))
            (new-segments (string-join segments "/")))
        (uri-with-path uri-obj
                       (string-append (if (string-ends? current-path "/")
                                         current-path
                                         (string-append current-path "/"))
                                     new-segments
                       ) ;string-append
        ) ;uri-with-path
      ) ;let
    ) ;define

    (define (uri-with-name uri-obj new-name)
      (let ((parent (uri-parent uri-obj)))
        (uri-with-path uri-obj
                       (if (and parent (not (string=? parent "")))
                         (string-append parent "/" new-name)
                         new-name
                       ) ;if
        ) ;uri-with-path
      ) ;let
    ) ;define

    (define (uri-with-suffix uri-obj new-suffix)
      (let ((name (uri-name uri-obj)))
        (if name
          (let ((dot-pos (string-index-right name #\.)))
            (uri-with-name uri-obj
                           (if dot-pos
                             (string-append (substring name 0 dot-pos) "." new-suffix)
                             (string-append name "." new-suffix)
                           ) ;if
            ) ;uri-with-name
          ) ;let
          uri-obj
        ) ;if
      ) ;let
    ) ;define

    ;; URI 合并（RFC 3986）
    (define (uri-join base-uri ref-uri)
      ;; 简化实现：假设 ref-uri 是相对路径
      (let ((base-path (uri-path-raw base-uri))
            (ref-path (uri-path-raw ref-uri)))
        (if (or (not ref-path) (string=? ref-path ""))
          base-uri
          (if (char=? (string-ref ref-path 0) #\/)
            ;; 绝对路径
            (uri-with-path base-uri ref-path)
            ;; 相对路径
            (uri-with-path base-uri
                           (normalize-path
                             (string-append (uri-parent base-uri) "/" ref-path)
                           ) ;normalize-path
            ) ;uri-with-path
          ) ;if
        ) ;if
      ) ;let
    ) ;define

    ;; 路径归一化
    (define (normalize-path path)
      (let
        ((segments (string-split path "/"))
         (absolute? (and (> (string-length path) 0)
                         (char=? (string-ref path 0) #\/))
         ) ;absolute?
        ) ;
        (let loop ((segs segments) (result '()))
          (if (null? segs)
            (if (null? result)
              "/"
              (let ((normalized (string-join (reverse result) "/")))
                (if absolute?
                  (string-append "/" normalized)
                  normalized
                ) ;if
              ) ;let
            ) ;if
            (cond
              ((string=? (car segs) "") (loop (cdr segs) result))
              ((string=? (car segs) ".") (loop (cdr segs) result))
              ((string=? (car segs) "..")
               (loop (cdr segs) (if (null? result) result (cdr result)))
              ) ;
              (else (loop (cdr segs) (cons (car segs) result)))
            ) ;cond
          ) ;if
        ) ;let
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
