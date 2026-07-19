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
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (scheme char)
  (export char-upcase
    char-downcase
    char-foldcase
    char-upper-case?
    char-lower-case?
    digit-value
    char-numeric?
    char-alphabetic?
    char-whitespace?
    char-ci=?
    char-ci<?
    char-ci>?
    char-ci<=?
    char-ci>=?
    string-ci=?
    string-ci<?
    string-ci>?
    string-ci<=?
    string-ci>=?
    string-upcase
    string-downcase
    string-foldcase
  ) ;export
  (import (scheme base) (liii unicode))
  (begin
    (define (digit-value ch)
      (case ch
       ((#\0 #\x3007 #\x96F6 #\xC601) 0)
       ;; 0 〇 零 영
       ((#\1 #\x4E00 #\x58F1 #\x58F9 #\xC77C #\x20B20) 1)
       ;; 1 一 壱 壹 일 𠬠
       ((#\2 #\x4E8C #\x5F10 #\x8D30 #\xC774 #\x20129) 2)
       ;; 2 二 弐 贰 이 𠄩
       ((#\3 #\x4E09 #\x53C2 #\x53C1 #\xC0BC #\x20027) 3)
       ;; 3 三 参 叁 삼 𠀧
       ((#\4 #\x56DB #\x8086 #\xC0AC #\x2629A) 4)
       ;; 4 四 肆 사 𦊚
       ((#\5 #\x4E94 #\x4F0D #\xC624 #\x2013C) 5)
       ;; 5 五 伍 오 𠄼
       ((#\6 #\x516D #\x9678 #\x9646 #\xC721 #\x264B9) 6)
       ;; 6 六 陸 陆 육 𦒹
       ((#\7 #\x4E03 #\x67D2 #\xCE60 #\x26271) 7)
       ;; 7 七 柒 칠 𦉱
       ((#\8 #\x516B #\x634C #\xD314 #\x20969) 8)
       ;; 8 八 捌 팔 𠥩
       ((#\9 #\x4E5D #\x7396 #\xAD6C #\x200E9) 9)
       ;; 9 九 玖 구 𠃩
       (else #f)
      ) ;case
    ) ;define

    (define char-upcase g_char-upcase)

    (define char-downcase g_char-downcase)

    (define (char-foldcase char)
      (unless (char? char)
        (error 'type-error "char-foldcase: parameter must be character")
      ) ;unless
      (let ((cp (char->integer char)))
        (let ((result (hash-table-ref *char-foldcase-ht* cp)))
          (if result (integer->char result) (char-downcase char))
        ) ;let
      ) ;let
    ) ;define

    (unless (defined? '*char-foldcase-ht*)
      (define *char-foldcase-ht* (s7-make-hash-table))
      ;; char-foldcase hash-table initialization
      (s7-hash-table-set! *char-foldcase-ht* 181 956)
      (s7-hash-table-set! *char-foldcase-ht* 304 304)
      (s7-hash-table-set! *char-foldcase-ht* 383 115)
      (s7-hash-table-set! *char-foldcase-ht* 837 953)
      (s7-hash-table-set! *char-foldcase-ht* 962 963)
      (s7-hash-table-set! *char-foldcase-ht* 976 946)
      (s7-hash-table-set! *char-foldcase-ht* 977 952)
      (s7-hash-table-set! *char-foldcase-ht* 981 966)
      (s7-hash-table-set! *char-foldcase-ht* 982 960)
      (s7-hash-table-set! *char-foldcase-ht* 1008 954)
      (s7-hash-table-set! *char-foldcase-ht* 1009 961)
      (s7-hash-table-set! *char-foldcase-ht* 1013 949)
      (do ((i 5024 (+ i 1)))
        ((> i 5109))
        (s7-hash-table-set! *char-foldcase-ht* i i)
      ) ;do
      (do ((i 5112 (+ i 1)))
        ((> i 5117))
        (s7-hash-table-set! *char-foldcase-ht* i (+ i -8))
      ) ;do
      (s7-hash-table-set! *char-foldcase-ht* 7296 1074)
      (s7-hash-table-set! *char-foldcase-ht* 7297 1076)
      (s7-hash-table-set! *char-foldcase-ht* 7298 1086)
      (s7-hash-table-set! *char-foldcase-ht* 7299 1089)
      (s7-hash-table-set! *char-foldcase-ht* 7300 1090)
      (s7-hash-table-set! *char-foldcase-ht* 7301 1090)
      (s7-hash-table-set! *char-foldcase-ht* 7302 1098)
      (s7-hash-table-set! *char-foldcase-ht* 7303 1123)
      (s7-hash-table-set! *char-foldcase-ht* 7304 42571)
      (s7-hash-table-set! *char-foldcase-ht* 7835 7777)
      (s7-hash-table-set! *char-foldcase-ht* 8126 953)
      (s7-hash-table-set! *char-foldcase-ht* 8147 912)
      (s7-hash-table-set! *char-foldcase-ht* 8163 944)
      (do ((i 43888 (+ i 1)))
        ((> i 43967))
        (s7-hash-table-set! *char-foldcase-ht* i (+ i -38864))
      ) ;do
      (s7-hash-table-set! *char-foldcase-ht* 64261 64262)
    ) ;unless

    (define (char-ht-set-range! ht start end)
      (do ((i start (+ i 1)))
        ((> i end))
        (s7-hash-table-set! ht i #t)
      ) ;do
    ) ;define

    (unless (defined? '*char-numeric-ht*)
      (define *char-numeric-ht* (s7-make-hash-table))
      (char-ht-set-range! *char-numeric-ht* 48 57)
      (char-ht-set-range! *char-numeric-ht* 178 179)
      (s7-hash-table-set! *char-numeric-ht* 185 #t)
      (char-ht-set-range! *char-numeric-ht* 188 190)
      (char-ht-set-range! *char-numeric-ht* 1632 1641)
      (char-ht-set-range! *char-numeric-ht* 1776 1785)
      (char-ht-set-range! *char-numeric-ht* 1984 1993)
      (char-ht-set-range! *char-numeric-ht* 2406 2415)
      (char-ht-set-range! *char-numeric-ht* 2534 2543)
      (char-ht-set-range! *char-numeric-ht* 2548 2553)
      (char-ht-set-range! *char-numeric-ht* 2662 2671)
      (char-ht-set-range! *char-numeric-ht* 2790 2799)
      (char-ht-set-range! *char-numeric-ht* 2918 2927)
      (char-ht-set-range! *char-numeric-ht* 2930 2935)
      (char-ht-set-range! *char-numeric-ht* 3046 3058)
      (char-ht-set-range! *char-numeric-ht* 3174 3183)
      (char-ht-set-range! *char-numeric-ht* 3192 3198)
      (char-ht-set-range! *char-numeric-ht* 3302 3311)
      (char-ht-set-range! *char-numeric-ht* 3416 3422)
      (char-ht-set-range! *char-numeric-ht* 3430 3448)
      (char-ht-set-range! *char-numeric-ht* 3558 3567)
      (char-ht-set-range! *char-numeric-ht* 3664 3673)
      (char-ht-set-range! *char-numeric-ht* 3792 3801)
      (char-ht-set-range! *char-numeric-ht* 3872 3891)
      (char-ht-set-range! *char-numeric-ht* 4160 4169)
      (char-ht-set-range! *char-numeric-ht* 4240 4249)
      (char-ht-set-range! *char-numeric-ht* 4969 4988)
      (char-ht-set-range! *char-numeric-ht* 5870 5872)
      (char-ht-set-range! *char-numeric-ht* 6112 6121)
      (char-ht-set-range! *char-numeric-ht* 6128 6137)
      (char-ht-set-range! *char-numeric-ht* 6160 6169)
      (char-ht-set-range! *char-numeric-ht* 6470 6479)
      (char-ht-set-range! *char-numeric-ht* 6608 6618)
      (char-ht-set-range! *char-numeric-ht* 6784 6793)
      (char-ht-set-range! *char-numeric-ht* 6800 6809)
      (char-ht-set-range! *char-numeric-ht* 6992 7001)
      (char-ht-set-range! *char-numeric-ht* 7088 7097)
      (char-ht-set-range! *char-numeric-ht* 7232 7241)
      (char-ht-set-range! *char-numeric-ht* 7248 7257)
      (s7-hash-table-set! *char-numeric-ht* 8304 #t)
      (char-ht-set-range! *char-numeric-ht* 8308 8313)
      (char-ht-set-range! *char-numeric-ht* 8320 8329)
      (char-ht-set-range! *char-numeric-ht* 8528 8578)
      (char-ht-set-range! *char-numeric-ht* 8581 8585)
      (char-ht-set-range! *char-numeric-ht* 9312 9371)
      (char-ht-set-range! *char-numeric-ht* 9450 9471)
      (char-ht-set-range! *char-numeric-ht* 10102 10131)
      (s7-hash-table-set! *char-numeric-ht* 11517 #t)
      (s7-hash-table-set! *char-numeric-ht* 12295 #t)
      (char-ht-set-range! *char-numeric-ht* 12321 12329)
      (char-ht-set-range! *char-numeric-ht* 12344 12346)
      (char-ht-set-range! *char-numeric-ht* 12690 12693)
      (char-ht-set-range! *char-numeric-ht* 12832 12841)
      (char-ht-set-range! *char-numeric-ht* 12872 12879)
      (char-ht-set-range! *char-numeric-ht* 12881 12895)
      (char-ht-set-range! *char-numeric-ht* 12928 12937)
      (char-ht-set-range! *char-numeric-ht* 12977 12991)
      (char-ht-set-range! *char-numeric-ht* 42528 42537)
      (char-ht-set-range! *char-numeric-ht* 42726 42735)
      (char-ht-set-range! *char-numeric-ht* 43056 43061)
      (char-ht-set-range! *char-numeric-ht* 43216 43225)
      (char-ht-set-range! *char-numeric-ht* 43264 43273)
      (char-ht-set-range! *char-numeric-ht* 43472 43481)
      (char-ht-set-range! *char-numeric-ht* 43504 43513)
      (char-ht-set-range! *char-numeric-ht* 43600 43609)
      (char-ht-set-range! *char-numeric-ht* 44016 44025)
      (s7-hash-table-set! *char-numeric-ht* 63851 #t)
      (s7-hash-table-set! *char-numeric-ht* 63859 #t)
      (s7-hash-table-set! *char-numeric-ht* 63864 #t)
      (s7-hash-table-set! *char-numeric-ht* 63922 #t)
      (s7-hash-table-set! *char-numeric-ht* 63953 #t)
      (s7-hash-table-set! *char-numeric-ht* 63955 #t)
      (s7-hash-table-set! *char-numeric-ht* 63997 #t)
      (char-ht-set-range! *char-numeric-ht* 65296 65305)
      (char-ht-set-range! *char-numeric-ht* 65799 65843)
      (char-ht-set-range! *char-numeric-ht* 65856 65912)
      (char-ht-set-range! *char-numeric-ht* 65930 65931)
      (char-ht-set-range! *char-numeric-ht* 66273 66299)
      (char-ht-set-range! *char-numeric-ht* 66336 66339)
      (s7-hash-table-set! *char-numeric-ht* 66369 #t)
      (s7-hash-table-set! *char-numeric-ht* 66378 #t)
      (char-ht-set-range! *char-numeric-ht* 66513 66517)
      (char-ht-set-range! *char-numeric-ht* 66720 66729)
      (char-ht-set-range! *char-numeric-ht* 67672 67679)
      (char-ht-set-range! *char-numeric-ht* 67705 67711)
      (char-ht-set-range! *char-numeric-ht* 67751 67759)
      (char-ht-set-range! *char-numeric-ht* 67835 67839)
      (char-ht-set-range! *char-numeric-ht* 67862 67867)
      (char-ht-set-range! *char-numeric-ht* 68028 68029)
      (char-ht-set-range! *char-numeric-ht* 68032 68047)
      (char-ht-set-range! *char-numeric-ht* 68050 68095)
      (char-ht-set-range! *char-numeric-ht* 68160 68168)
      (char-ht-set-range! *char-numeric-ht* 68221 68222)
      (char-ht-set-range! *char-numeric-ht* 68253 68255)
      (char-ht-set-range! *char-numeric-ht* 68331 68335)
      (char-ht-set-range! *char-numeric-ht* 68440 68447)
      (char-ht-set-range! *char-numeric-ht* 68472 68479)
      (char-ht-set-range! *char-numeric-ht* 68521 68527)
      (char-ht-set-range! *char-numeric-ht* 68858 68863)
      (char-ht-set-range! *char-numeric-ht* 68912 68921)
      (char-ht-set-range! *char-numeric-ht* 68928 68937)
      (char-ht-set-range! *char-numeric-ht* 69216 69246)
      (char-ht-set-range! *char-numeric-ht* 69405 69414)
      (char-ht-set-range! *char-numeric-ht* 69457 69460)
      (char-ht-set-range! *char-numeric-ht* 69573 69579)
      (char-ht-set-range! *char-numeric-ht* 69714 69743)
      (char-ht-set-range! *char-numeric-ht* 69872 69881)
      (char-ht-set-range! *char-numeric-ht* 69942 69951)
      (char-ht-set-range! *char-numeric-ht* 70096 70105)
      (char-ht-set-range! *char-numeric-ht* 70113 70132)
      (char-ht-set-range! *char-numeric-ht* 70384 70393)
      (char-ht-set-range! *char-numeric-ht* 70736 70745)
      (char-ht-set-range! *char-numeric-ht* 70864 70873)
      (char-ht-set-range! *char-numeric-ht* 71248 71257)
      (char-ht-set-range! *char-numeric-ht* 71360 71369)
      (char-ht-set-range! *char-numeric-ht* 71376 71395)
      (char-ht-set-range! *char-numeric-ht* 71472 71483)
      (char-ht-set-range! *char-numeric-ht* 71904 71922)
      (char-ht-set-range! *char-numeric-ht* 72016 72025)
      (char-ht-set-range! *char-numeric-ht* 72688 72697)
      (char-ht-set-range! *char-numeric-ht* 72784 72812)
      (char-ht-set-range! *char-numeric-ht* 73040 73049)
      (char-ht-set-range! *char-numeric-ht* 73120 73129)
      (char-ht-set-range! *char-numeric-ht* 73552 73561)
      (char-ht-set-range! *char-numeric-ht* 73664 73684)
      (char-ht-set-range! *char-numeric-ht* 74752 74862)
      (char-ht-set-range! *char-numeric-ht* 90416 90425)
      (char-ht-set-range! *char-numeric-ht* 92768 92777)
      (char-ht-set-range! *char-numeric-ht* 92864 92873)
      (char-ht-set-range! *char-numeric-ht* 93008 93017)
      (char-ht-set-range! *char-numeric-ht* 93019 93025)
      (char-ht-set-range! *char-numeric-ht* 93552 93561)
      (char-ht-set-range! *char-numeric-ht* 93824 93846)
      (char-ht-set-range! *char-numeric-ht* 118000 118009)
      (char-ht-set-range! *char-numeric-ht* 119488 119507)
      (char-ht-set-range! *char-numeric-ht* 119520 119539)
      (char-ht-set-range! *char-numeric-ht* 119648 119672)
      (char-ht-set-range! *char-numeric-ht* 120782 120831)
      (char-ht-set-range! *char-numeric-ht* 123200 123209)
      (char-ht-set-range! *char-numeric-ht* 123632 123641)
      (char-ht-set-range! *char-numeric-ht* 124144 124153)
      (char-ht-set-range! *char-numeric-ht* 124401 124410)
      (char-ht-set-range! *char-numeric-ht* 125127 125135)
      (char-ht-set-range! *char-numeric-ht* 125264 125273)
      (char-ht-set-range! *char-numeric-ht* 126065 126123)
      (char-ht-set-range! *char-numeric-ht* 126125 126127)
      (char-ht-set-range! *char-numeric-ht* 126129 126132)
      (char-ht-set-range! *char-numeric-ht* 126209 126253)
      (char-ht-set-range! *char-numeric-ht* 126255 126269)
      (char-ht-set-range! *char-numeric-ht* 127232 127244)
      (char-ht-set-range! *char-numeric-ht* 130032 130041)
      (s7-hash-table-set! *char-numeric-ht* 194704 #t)
    ) ;unless

    (define (char-numeric? char)
      (unless (char? char)
        (error 'type-error "char-numeric?: parameter must be character")
      ) ;unless
      (hash-table-ref *char-numeric-ht* (char->integer char))
    ) ;define

    (define char-alphabetic? g_char-alphabetic?)
    (define (char-whitespace? char)
      (unless (char? char)
        (error 'type-error "char-whitespace?: parameter must be character")
      ) ;unless
      (hash-table-ref *char-whitespace-ht* (char->integer char))
    ) ;define

    (unless (defined? '*char-whitespace-ht*)
      (define *char-whitespace-ht* (s7-make-hash-table))
      (char-ht-set-range! *char-whitespace-ht* 9 13)
      (s7-hash-table-set! *char-whitespace-ht* 32 #t)
      (s7-hash-table-set! *char-whitespace-ht* 133 #t)
      (s7-hash-table-set! *char-whitespace-ht* 160 #t)
      (s7-hash-table-set! *char-whitespace-ht* 5760 #t)
      (char-ht-set-range! *char-whitespace-ht* 8192 8202)
      (char-ht-set-range! *char-whitespace-ht* 8232 8233)
      (s7-hash-table-set! *char-whitespace-ht* 8239 #t)
      (s7-hash-table-set! *char-whitespace-ht* 8287 #t)
      (s7-hash-table-set! *char-whitespace-ht* 12288 #t)
    ) ;unless

    (define char-upper-case? g_char-upper-case?)

    (define char-lower-case? g_char-lower-case?)

    (define (char-ci=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error "char-ci=?: first parameter must be character")
      ) ;unless
      (unless (char? char2)
        (error 'type-error "char-ci=?: second parameter must be character")
      ) ;unless
      (let ((f1 (char-foldcase char1)) (f2 (char-foldcase char2)))
        (let loop
          ((current (char=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-char (car remaining)))
              (unless (char? next-char)
                (error 'type-error "char-ci=?: parameter must be character")
              ) ;unless
              (let ((next-folded (char-foldcase next-char)))
                (and current (loop (char=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (char-ci<? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error "char-ci<?: first parameter must be character")
      ) ;unless
      (unless (char? char2)
        (error 'type-error "char-ci<?: second parameter must be character")
      ) ;unless
      (let ((f1 (char-foldcase char1)) (f2 (char-foldcase char2)))
        (let loop
          ((current (char<? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-char (car remaining)))
              (unless (char? next-char)
                (error 'type-error "char-ci<?: parameter must be character")
              ) ;unless
              (let ((next-folded (char-foldcase next-char)))
                (and current (loop (char<? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (char-ci>? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error "char-ci>?: first parameter must be character")
      ) ;unless
      (unless (char? char2)
        (error 'type-error "char-ci>?: second parameter must be character")
      ) ;unless
      (let ((f1 (char-foldcase char1)) (f2 (char-foldcase char2)))
        (let loop
          ((current (char>? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-char (car remaining)))
              (unless (char? next-char)
                (error 'type-error "char-ci>?: parameter must be character")
              ) ;unless
              (let ((next-folded (char-foldcase next-char)))
                (and current (loop (char>? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (char-ci>=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error "char-ci>=?: first parameter must be character")
      ) ;unless
      (unless (char? char2)
        (error 'type-error "char-ci>=?: second parameter must be character")
      ) ;unless
      (let ((f1 (char-foldcase char1)) (f2 (char-foldcase char2)))
        (let loop
          ((current (char>=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-char (car remaining)))
              (unless (char? next-char)
                (error 'type-error "char-ci>=?: parameter must be character")
              ) ;unless
              (let ((next-folded (char-foldcase next-char)))
                (and current (loop (char>=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (char-ci<=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error "char-ci<=?: first parameter must be character")
      ) ;unless
      (unless (char? char2)
        (error 'type-error "char-ci<=?: second parameter must be character")
      ) ;unless
      (let ((f1 (char-foldcase char1)) (f2 (char-foldcase char2)))
        (let loop
          ((current (char<=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-char (car remaining)))
              (unless (char? next-char)
                (error 'type-error "char-ci<=?: parameter must be character")
              ) ;unless
              (let ((next-folded (char-foldcase next-char)))
                (and current (loop (char<=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-ci=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error "string-ci=?: first parameter must be string")
      ) ;unless
      (unless (string? str2)
        (error 'type-error "string-ci=?: second parameter must be string")
      ) ;unless
      (let ((f1 (string-foldcase str1)) (f2 (string-foldcase str2)))
        (let loop
          ((current (string=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-str (car remaining)))
              (unless (string? next-str)
                (error 'type-error "string-ci=?: parameter must be string")
              ) ;unless
              (let ((next-folded (string-foldcase next-str)))
                (and current (loop (string=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-ci<? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error "string-ci<?: first parameter must be string")
      ) ;unless
      (unless (string? str2)
        (error 'type-error "string-ci<?: second parameter must be string")
      ) ;unless
      (let ((f1 (string-foldcase str1)) (f2 (string-foldcase str2)))
        (let loop
          ((current (string<? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-str (car remaining)))
              (unless (string? next-str)
                (error 'type-error "string-ci<?: parameter must be string")
              ) ;unless
              (let ((next-folded (string-foldcase next-str)))
                (and current (loop (string<? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-ci>? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error "string-ci>?: first parameter must be string")
      ) ;unless
      (unless (string? str2)
        (error 'type-error "string-ci>?: second parameter must be string")
      ) ;unless
      (let ((f1 (string-foldcase str1)) (f2 (string-foldcase str2)))
        (let loop
          ((current (string>? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-str (car remaining)))
              (unless (string? next-str)
                (error 'type-error "string-ci>?: parameter must be string")
              ) ;unless
              (let ((next-folded (string-foldcase next-str)))
                (and current (loop (string>? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-ci<=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error "string-ci<=?: first parameter must be string")
      ) ;unless
      (unless (string? str2)
        (error 'type-error "string-ci<=?: second parameter must be string")
      ) ;unless
      (let ((f1 (string-foldcase str1)) (f2 (string-foldcase str2)))
        (let loop
          ((current (string<=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-str (car remaining)))
              (unless (string? next-str)
                (error 'type-error "string-ci<=?: parameter must be string")
              ) ;unless
              (let ((next-folded (string-foldcase next-str)))
                (and current (loop (string<=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-ci>=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error "string-ci>=?: first parameter must be string")
      ) ;unless
      (unless (string? str2)
        (error 'type-error "string-ci>=?: second parameter must be string")
      ) ;unless
      (let ((f1 (string-foldcase str1)) (f2 (string-foldcase str2)))
        (let loop
          ((current (string>=? f1 f2)) (prev f2) (remaining rest))
          (if (null? remaining)
            current
            (let ((next-str (car remaining)))
              (unless (string? next-str)
                (error 'type-error "string-ci>=?: parameter must be string")
              ) ;unless
              (let ((next-folded (string-foldcase next-str)))
                (and current (loop (string>=? prev next-folded) next-folded (cdr remaining)))
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (utf8-string-map proc str)
      (let* ((bv (string->utf8 str)) (len (bytevector-length bv)))
        (let loop
          ((pos 0) (result '()))
          (if (>= pos len)
            (apply utf8-string (reverse result))
            (let* ((next (bytevector-advance-utf8 bv pos len))
                   (ch (integer->char (utf8->codepoint-at bv pos)))
                   (new-ch (proc ch))
                  ) ;
              (loop next (cons new-ch result))
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-upcase str)
      (unless (string? str)
        (error 'type-error "string-upcase: parameter must be string")
      ) ;unless
      (utf8-string-map char-upcase str)
    ) ;define

    (define (string-downcase str)
      (unless (string? str)
        (error 'type-error "string-downcase: parameter must be string")
      ) ;unless
      (utf8-string-map char-downcase str)
    ) ;define

    (define (string-foldcase str)
      (unless (string? str)
        (error 'type-error "string-foldcase: parameter must be string")
      ) ;unless
      (utf8-string-map char-foldcase str)
    ) ;define

  ) ;begin
) ;define-library
