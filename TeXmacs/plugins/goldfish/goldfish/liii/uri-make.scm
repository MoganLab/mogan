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
;;; (liii uri-make) - URI 构造器函数
;;; 本模块包含 URI 的构造器函数

(define-library (liii uri-make)
  (import (scheme base)
          (scheme char)
          (liii string)
          (liii error)
          (liii base)
          (liii uri-record)
          (liii uri-parse)
  ) ;import

  ;;; ---------- 导出接口 ----------
  (export make-uri)
  (export uri-build)
  (export string->uri)

  (begin
    ;;; ---------- 解析辅助函数 ----------
    ;; 分割 scheme 和剩余部分
    (define (split-scheme str)
      (let ((colon-pos (string-index str #\:)))
        (if colon-pos
          (cons (substring str 0 colon-pos)
                (substring str (+ colon-pos 1) (string-length str))
          ) ;cons
          (cons #f str)
        ) ;if
      ) ;let
    ) ;define

    ;; 分割 authority 和路径部分
    (define (split-authority str)
      ;; 查找路径开始（/）、查询（?）或片段（#）
      (let loop ((i 0) (slash-pos #f) (question-pos #f) (hash-pos #f))
        (if (>= i (string-length str))
          (let
            ((netloc (if slash-pos
                        (substring str 0 slash-pos)
                        str
                     ) ;if
             ) ;netloc
             (path (if slash-pos
                      (substring str slash-pos
                                (or question-pos
                                    hash-pos
                                    (string-length str)
                                ) ;or
                      ) ;substring
                      ""
                   ) ;if
             ) ;path
             (query (if question-pos
                       (substring str (+ question-pos 1)
                                 (or hash-pos
                                     (string-length str)
                                 ) ;or
                       ) ;substring
                       ""
                    ) ;if
             ) ;query
             (fragment (if hash-pos
                          (substring str (+ hash-pos 1) (string-length str))
                          #f
                       ) ;if
             ) ;fragment
            ) ;
            (list netloc path query fragment)
          ) ;let
          (let ((c (string-ref str i)))
            (cond
              ((char=? c #\/)
               (if slash-pos
                 (loop (+ i 1) slash-pos question-pos hash-pos)
                 (loop (+ i 1) i question-pos hash-pos)
               ) ;if
              ) ;
              ((char=? c #\?)
               (loop (+ i 1) (or slash-pos i) i hash-pos)
              ) ;
              ((char=? c #\#)
               (loop (+ i 1) (or slash-pos i) question-pos i)
              ) ;
              (else
                (loop (+ i 1) slash-pos question-pos hash-pos)
              ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; 分割路径和查询字符串
    (define (split-query path+query)
      (let ((q-pos (string-index path+query #\?)))
        (if q-pos
          (cons (substring path+query 0 q-pos)
                (substring path+query (+ q-pos 1) (string-length path+query))
          ) ;cons
          (cons path+query "")
        ) ;if
      ) ;let
    ) ;define

    ;;; ---------- 构造函数 ----------
    ;; 从字符串构造 URI
    (define (make-uri str)
      (if (not (string? str))
        (type-error "make-uri: expected string")
        ;; 检查是否是 Git SSH 格式：git@host:path
        (if (and (string-index str #\@)
                 (not (string-starts? str "http://"))
                 (not (string-starts? str "https://"))
                 (not (string-starts? str "ssh://"))
                 (let ((colon-pos (string-index str #\:)))
                   (and colon-pos
                        (> colon-pos (string-index str #\@)))
                   ) ;and
                 ) ;let
          ;; Git SSH 格式: git@host:path
          (let* ((at-pos (string-index str #\@))
                 (colon-pos (string-index str #\:))
                 (user (substring str 0 at-pos))
                 (host (substring str (+ at-pos 1) colon-pos))
                 (path (substring str colon-pos (string-length str))))
            (make-uri-raw "ssh" (build-netloc user #f host #f) path '() #f)
          ) ;let*
          ;; 标准 URI 格式
          (let*
            ((scheme+rest (split-scheme str))
             (scheme (car scheme+rest))
             (rest (cdr scheme+rest))
             (authority+path+query+frag (if (string-starts? rest "//")
                                           (split-authority (substring rest 2 (string-length rest)))
                                           (list "" rest "" "")
                                        ) ;if
             ) ;authority+path+query+frag
             (netloc (list-ref authority+path+query+frag 0))
             (path (list-ref authority+path+query+frag 1))
             (query-str (list-ref authority+path+query+frag 2))
             (fragment (list-ref authority+path+query+frag 3))
            ) ;
            (make-uri-raw scheme netloc path (query-string->alist query-str) fragment)
          ) ;let*
        ) ;if
      ) ;if
    ) ;define

    ;; 从组件构建 URI
    (define* (uri-build (scheme #f) (user #f) (password #f) (host #f)
                        (port #f) (path #f) (query '()) (fragment #f))
      (make-uri-raw scheme
                    (build-netloc user password host port)
                    (or path "")
                    query
                    fragment
      ) ;make-uri-raw
    ) ;define*

    ;; string->uri 是 make-uri 的别名
    (define (string->uri str)
      (make-uri str)
    ) ;define

  ) ;begin
) ;define-library
