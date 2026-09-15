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

(define-library (liii semver)
  (import (scheme base) (liii ascii) (liii string))
  (export semver-clean semver-parse semver-valid? semver-compare semver>?
    semver<? semver=? semver>=? semver<=?
  ) ;export
  (begin

    ;; prerelease 字符集由 SemVer 规范定义：ASCII 字母、数字及连字符
    (define (prerelease-char? ch)
      (or (ascii-alphanumeric? ch) (char=? ch #\-))
    ) ;define

    ;; 纯数字串判定：非空且全部由 ASCII 数字组成
    (define (string-all-digits? s)
      (and (not (string-null? s)) (string-every ascii-numeric? s))
    ) ;define

    ;; SemVer 规范第 2 条：多位数不允许前导零
    (define (leading-zero? s)
      (and (> (string-length s) 1) (char=? (string-ref s 0) #\0))
    ) ;define

    ;; 去除首尾空白，兼容剥离前导 'v' 或 'V'（若后紧跟数字）
    (define (semver-clean s)
      (if (not (string? s))
        ""
        (let* ((trimmed (string-trim-both s)) (tlen (string-length trimmed)))
          (if (and (> tlen 1)
                (ascii-ci=? (string-ref trimmed 0) #\v)
                (ascii-numeric? (string-ref trimmed 1))
              ) ;and
            (substring trimmed 1 tlen)
            trimmed
          ) ;if
        ) ;let*
      ) ;if
    ) ;define

    ;; 按 '.' 切分并逐字段解析；任一字段非法则整体返回 #f
    (define (parse-dotted-fields str field->value)
      (let loop
        ((parts (string-split str #\.)) (acc '()))
        (if (null? parts)
          (reverse acc)
          (let ((v (field->value (car parts))))
            (and v (loop (cdr parts) (cons v acc)))
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; 核心段字段：必须为无前导零的纯数字（空字段由 string-all-digits? 拒绝）
    (define (core-field->number p)
      (and (string-all-digits? p) (not (leading-zero? p)) (string->number p))
    ) ;define

    ;; 预发布字段：纯数字按数值处理，其余须落在 prerelease 字符集内
    (define (prerelease-field->id p)
      (and (not (string-null? p))
        (cond
         ((string-all-digits? p)
          (and (not (leading-zero? p)) (cons 'num (string->number p)))
         ) ;
         ((string-every prerelease-char? p) (cons 'str p))
         (else #f)
        ) ;cond
      ) ;and
    ) ;define

    ;; 构建元数据字段：非空，仅限 prerelease 字符集（与预发布不同，允许前导零）
    (define (build-field->id p)
      (and (not (string-null? p)) (string-every prerelease-char? p) p)
    ) ;define

    ;; 解析 semver 字符串为 (semver core-nums pre-list) 结构，非法返回 #f
    (define (semver-parse s)
      (let ((cleaned (semver-clean s)))
        (if (string=? cleaned "")
          #f
          (let* ((plus-pos (string-index cleaned #\+))
                 (without-build (if plus-pos (substring cleaned 0 plus-pos) cleaned))
                 (build-str (if plus-pos (substring cleaned (+ plus-pos 1) (string-length cleaned)) #f)
                 ) ;build-str
                 (dash-pos (string-index without-build #\-))
                 (core-str (if dash-pos (substring without-build 0 dash-pos) without-build))
                 (pre-str (if dash-pos
                            (substring without-build (+ dash-pos 1) (string-length without-build))
                            #f
                          ) ;if
                 ) ;pre-str
                 (core-nums (parse-dotted-fields core-str core-field->number))
                 (build-ids (if build-str (parse-dotted-fields build-str build-field->id) '()))
                ) ;
            (and core-nums
              ;; 核心段至多三段（1~3 段合法，缺省段在比较时补零）
              (<= (length core-nums) 3)
              ;; 构建元数据若存在则须合法；其值不参与优先级比较，仅校验
              build-ids
              (let ((pre-ids (if pre-str (parse-dotted-fields pre-str prerelease-field->id) '())))
                (and pre-ids (list 'semver core-nums pre-ids))
              ) ;let
            ) ;and
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    ;; 判断是否为合法的语义化版本
    (define (semver-valid? s)
      (if (semver-parse s) #t #f)
    ) ;define

    ;; 核心段逐位比较，缺失的段视为 0（故 1.0 等于 1.0.0）
    (define (compare-core c1 c2)
      (if (and (null? c1) (null? c2))
        0
        (let ((n1 (if (null? c1) 0 (car c1))) (n2 (if (null? c2) 0 (car c2))))
          (cond ((< n1 n2) -1)
                ((> n1 n2) 1)
                (else (compare-core (if (null? c1) '() (cdr c1)) (if (null? c2) '() (cdr c2))))
          ) ;cond
        ) ;let
      ) ;if
    ) ;define

    ;; 预发布标识符逐位比较：纯数字按数值比较、纯数字低于非纯数字、
    ;; 非纯数字按 ASCII 字典序比较、标识符数量多者优先
    (define (compare-prerelease ps1 ps2)
      (cond ((and (null? ps1) (null? ps2)) 0)
            ((null? ps1) -1)
            ((null? ps2) 1)
            (else
              (let* ((id1 (car ps1))
                     (id2 (car ps2))
                     (t1 (car id1))
                     (v1 (cdr id1))
                     (t2 (car id2))
                     (v2 (cdr id2))
                    ) ;
                (cond
                 ((and (eq? t1 'num) (eq? t2 'num))
                  (cond ((< v1 v2) -1)
                        ((> v1 v2) 1)
                        (else (compare-prerelease (cdr ps1) (cdr ps2)))
                  ) ;cond
                 ) ;
                 ((eq? t1 'num) -1)
                 ((eq? t2 'num) 1)
                 (else
                   (cond ((string<? v1 v2) -1)
                         ((string>? v1 v2) 1)
                         (else (compare-prerelease (cdr ps1) (cdr ps2)))
                   ) ;cond
                 ) ;else
                ) ;cond
              ) ;let*
            ) ;else
      ) ;cond
    ) ;define

    ;; 按 SemVer 2.0.0 规范比较 s1 与 s2：
    ;; s1 < s2 返回 -1；s1 > s2 返回 1；s1 == s2 返回 0；任一非法返回 #f
    (define (semver-compare s1 s2)
      (let ((p1 (semver-parse s1)) (p2 (semver-parse s2)))
        (if (or (not p1) (not p2))
          #f
          (let ((core-cmp (compare-core (cadr p1) (cadr p2))))
            (if (not (= core-cmp 0))
              core-cmp
              ;; 预发布版本优先级低于同核心段的正式版
              (let ((pre1 (caddr p1)) (pre2 (caddr p2)))
                (cond ((null? pre1) (if (null? pre2) 0 1))
                      ((null? pre2) -1)
                      (else (compare-prerelease pre1 pre2))
                ) ;cond
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; 由三路比较结果构造比较谓词；任一版本非法时返回 #f
    (define (make-version-predicate rel)
      (lambda (s1 s2) (let ((c (semver-compare s1 s2))) (and c (rel c 0))))
    ) ;define

    (define semver>? (make-version-predicate >))
    (define semver<? (make-version-predicate <))
    (define semver=? (make-version-predicate =))
    (define semver>=? (make-version-predicate >=))
    (define semver<=? (make-version-predicate <=))

  ) ;begin
) ;define-library
