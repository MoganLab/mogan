;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : semver.scm
;; DESCRIPTION : 语义化版本（Semantic Versioning 2.0.0）解析与比较
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils misc semver))

(import (liii ascii) (liii string))

;; prerelease 字符集由 SemVer 规范定义：ASCII 字母、数字及连字符

(define (prerelease-char? ch)
  (or (ascii-alphanumeric? ch) (char=? ch #\-))
) ;define

;; 纯数字串判定：非空且全部由 ASCII 数字组成

(define (string-all-digits? s)
  (and (not (string-null? s)) (string-every ascii-numeric? s))
) ;define

;; 去除首尾空白，兼容剥离前导 'v' 或 'V'（若后紧跟数字）
(tm-define (semver-clean s)
  (if (not (string? s))
    ""
    (let* ((trimmed (string-trim-both s)) (tlen (string-length trimmed)))
      (if (and (> tlen 1)
            (or (char=? (string-ref trimmed 0) #\v) (char=? (string-ref trimmed 0) #\V))
            (ascii-numeric? (string-ref trimmed 1))
          ) ;and
        (substring trimmed 1 tlen)
        trimmed
      ) ;if
    ) ;let*
  ) ;if
) ;tm-define

;; 解析 semver 字符串为 (semver core-nums pre-list) 结构，非法返回 #f
(tm-define (semver-parse s)
  (let ((cleaned (semver-clean s)))
    (if (string=? cleaned "")
      #f
      (let* ((plus-pos (string-index cleaned #\+))
             (without-build (if plus-pos (substring cleaned 0 plus-pos) cleaned))
             (dash-pos (string-index without-build #\-))
             (core-str (if dash-pos (substring without-build 0 dash-pos) without-build))
             (pre-str (if dash-pos
                        (substring without-build (+ dash-pos 1) (string-length without-build))
                        #f
                      ) ;if
             ) ;pre-str
            ) ;
        (let ((core-parts (string-split core-str #\.)))
          (if (null? core-parts)
            #f
            (let loop
              ((parts core-parts) (nums '()))
              (if (null? parts)
                (let ((core-nums (reverse nums)))
                  (if (not pre-str)
                    (list 'semver core-nums '())
                    (let ((pre-parts (string-split pre-str #\.)))
                      (if (null? pre-parts)
                        #f
                        (let pre-loop
                          ((ps pre-parts) (ids '()))
                          (if (null? ps)
                            (list 'semver core-nums (reverse ids))
                            (let* ((p (car ps)) (plen (string-length p)))
                              (if (= plen 0)
                                #f
                                (if (string-all-digits? p)
                                  (if (and (> plen 1) (char=? (string-ref p 0) #\0))
                                    #f
                                    (pre-loop (cdr ps) (cons (cons 'num (string->number p)) ids))
                                  ) ;if
                                  (if (string-every prerelease-char? p)
                                    (pre-loop (cdr ps) (cons (cons 'str p) ids))
                                    #f
                                  ) ;if
                                ) ;if
                              ) ;if
                            ) ;let*
                          ) ;if
                        ) ;let
                      ) ;if
                    ) ;let
                  ) ;if
                ) ;let
                (let* ((p (car parts)) (plen (string-length p)))
                  (if (or (= plen 0)
                        (not (string-all-digits? p))
                        (and (> plen 1) (char=? (string-ref p 0) #\0))
                      ) ;or
                    #f
                    (loop (cdr parts) (cons (string->number p) nums))
                  ) ;if
                ) ;let*
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;if
  ) ;let
) ;tm-define

;; 判断是否为合法的语义化版本
(tm-define (semver-valid? s) (if (semver-parse s) #t #f))

;; 按 SemVer 2.0.0 规范比较 s1 与 s2：
;; s1 < s2 返回 -1；s1 > s2 返回 1；s1 == s2 返回 0；任一非法返回 #f
(tm-define (semver-compare s1 s2)
  (let ((p1 (semver-parse s1)) (p2 (semver-parse s2)))
    (if (or (not p1) (not p2))
      #f
      (let* ((core1 (cadr p1))
             (core2 (cadr p2))
             (pre1 (caddr p1))
             (pre2 (caddr p2))
             (len1 (length core1))
             (len2 (length core2))
             (max-len (max len1 len2 3))
            ) ;
        (let loop
          ((i 0))
          (if (< i max-len)
            (let ((n1 (if (< i len1) (list-ref core1 i) 0))
                  (n2 (if (< i len2) (list-ref core2 i) 0))
                 ) ;
              (cond ((< n1 n2) -1)
                    ((> n1 n2) 1)
                    (else (loop (+ i 1)))
              ) ;cond
            ) ;let
            (let ((has1 (not (null? pre1))) (has2 (not (null? pre2))))
              (cond ((and (not has1) has2) 1)
                    ((and has1 (not has2)) -1)
                    ((and (not has1) (not has2)) 0)
                    (else (let pre-loop
                            ((ps1 pre1) (ps2 pre2))
                            (cond ((and (null? ps1) (null? ps2)) 0)
                                  ((null? ps1) -1)
                                  ((null? ps2) 1)
                                  (else (let* ((id1 (car ps1))
                                               (id2 (car ps2))
                                               (t1 (car id1))
                                               (v1 (cdr id1))
                                               (t2 (car id2))
                                               (v2 (cdr id2))
                                              ) ;
                                          (cond ((and (== t1 'num) (== t2 'num))
                                                 (cond ((< v1 v2) -1)
                                                       ((> v1 v2) 1)
                                                       (else (pre-loop (cdr ps1) (cdr ps2)))
                                                 ) ;cond
                                                ) ;
                                                ((and (== t1 'num) (== t2 'str)) -1)
                                                ((and (== t1 'str) (== t2 'num)) 1)
                                                (else (cond ((string<? v1 v2) -1)
                                                            ((string>? v1 v2) 1)
                                                            (else (pre-loop (cdr ps1) (cdr ps2)))
                                                      ) ;cond
                                                ) ;else
                                          ) ;cond
                                        ) ;let*
                                  ) ;else
                            ) ;cond
                          ) ;let
                    ) ;else
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;if
  ) ;let
) ;tm-define

(tm-define (semver>? s1 s2) (let ((c (semver-compare s1 s2))) (and c (== c 1))))

(tm-define (semver<? s1 s2)
  (let ((c (semver-compare s1 s2)))
    (and c (== c -1))
  ) ;let
) ;tm-define

(tm-define (semver=? s1 s2) (let ((c (semver-compare s1 s2))) (and c (== c 0))))

(tm-define (semver>=? s1 s2)
  (let ((c (semver-compare s1 s2)))
    (and c (or (== c 1) (== c 0)))
  ) ;let
) ;tm-define

(tm-define (semver<=? s1 s2)
  (let ((c (semver-compare s1 s2)))
    (and c (or (== c -1) (== c 0)))
  ) ;let
) ;tm-define
