;;
;; Copyright (C) 2024 The Goldfish Scheme Authors
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

(define-library (liii diff hunk)
  (export format-range-unified format-group format-all-groups make-diff-header)
  (import (scheme base) (liii base) (liii list) (liii string))
  (begin

    (define (format-range-unified start stop)
      (let* ((beginning (+ start 1)) (len (- stop start)))
        (cond ((= len 1) (number->string beginning))
              ((= len 0) (string-append (number->string (- beginning 1)) ",0"))
              (else (string-append (number->string beginning) "," (number->string len)))
        ) ;cond
      ) ;let*
    ) ;define

    (define (append-prefix-lines prefix idx end vec acc)
      (if (>= idx end)
        acc
        (append-prefix-lines prefix
          (+ idx 1)
          end
          vec
          (cons (string-append prefix (vector-ref vec idx)) acc)
        ) ;append-prefix-lines
      ) ;if
    ) ;define

    (define (append-group-lines code a b lines)
      (let ((tag (list-ref code 0))
            (i1 (list-ref code 1))
            (i2 (list-ref code 2))
            (j1 (list-ref code 3))
            (j2 (list-ref code 4))
           ) ;
        (if (eq? tag 'equal)
          (append-prefix-lines " " i1 i2 a lines)
          (let* ((acc1 (if (or (eq? tag 'replace) (eq? tag 'delete))
                         (append-prefix-lines "-" i1 i2 a lines)
                         lines
                       ) ;if
                 ) ;acc1
                 (acc2 (if (or (eq? tag 'replace) (eq? tag 'insert))
                         (append-prefix-lines "+" j1 j2 b acc1)
                         acc1
                       ) ;if
                 ) ;acc2
                ) ;
            acc2
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (format-group group a b)
      (let* ((first-c (car group))
             (last-c (car (last-pair group)))
             (first-i1 (list-ref first-c 1))
             (last-i2 (list-ref last-c 2))
             (first-j1 (list-ref first-c 3))
             (last-j2 (list-ref last-c 4))
             (range-a (format-range-unified first-i1 last-i2))
             (range-b (format-range-unified first-j1 last-j2))
             (hunk-header (string-append "@@ -" range-a " +" range-b " @@"))
             (lines (list hunk-header))
            ) ;
        (let loop
          ((rem group) (acc lines))
          (if (null? rem)
            (reverse acc)
            (loop (cdr rem) (append-group-lines (car rem) a b acc))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (make-diff-header fromfile tofile fromdate todate)
      (let ((h1
              (if (and (string? fromdate) (not (string-null? fromdate)))
                (string-append "--- " fromfile "\t" fromdate)
                (string-append "--- " fromfile)
              ) ;if
            ) ;h1
            (h2
              (if (and (string? todate) (not (string-null? todate)))
                (string-append "+++ " tofile "\t" todate)
                (string-append "+++ " tofile)
              ) ;if
            ) ;h2
           ) ;
        (list h1 h2)
      ) ;let
    ) ;define

    (define (format-all-groups groups vec-a vec-b)
      (let loop
        ((rem groups) (acc '()))
        (if (null? rem)
          (reverse acc)
          ;; 逆序累积 hunk 行，避免循环内 append 的 O(n^2) 复制
          (loop (cdr rem) (append (reverse (format-group (car rem) vec-a vec-b)) acc))
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
