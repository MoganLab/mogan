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

(define-library (liii diff)
  (export unified-diff unified-diff-string diff-ratio)
  (import (scheme base)
    (scheme char)
    (liii base)
    (liii error)
    (liii string)
    (liii diff matcher)
    (liii diff hunk)
  ) ;import
  (begin

    (define (strip-line-ending s)
      (string-remove-suffix (string-remove-suffix s "\n") "\r")
    ) ;define

    (define (line-end-pos s i start)
      (if
        (and (> i start) (char=? (string-ref s (- i 1)) #\return))
        (- i 1)
        i
      ) ;if
    ) ;define

    (define (handle-newline s i len start acc)
      (let* ((end (line-end-pos s i start)) (line (substring s start end)) (next (+ i 1)))
        (values next (cons line acc))
      ) ;let*
    ) ;define

    (define (split-lines s)
      (let ((len (string-length s)))
        (if (= len 0)
          '()
          (let loop
            ((i 0) (start 0) (acc '()))
            (cond
             ((= i len) (reverse (cons (substring s start len) acc)))
             ((char=? (string-ref s i) #\newline)
              (call-with-values (lambda () (handle-newline s i len start acc))
                (lambda (next new-acc)
                  (if (= next len) (reverse new-acc) (loop next next new-acc))
                ) ;lambda
              ) ;call-with-values
             ) ;
             ((char=? (string-ref s i) #\return)
              (if
                (and (< (+ i 1) len) (char=? (string-ref s (+ i 1)) #\newline))
                (loop (+ i 1) start acc)
                (let ((line (substring s start i)) (next (+ i 1)))
                  (if (= next len) (reverse (cons line acc)) (loop next next (cons line acc)))
                ) ;let
              ) ;if
             ) ;
             (else (loop (+ i 1) start acc))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (string-list? lst)
      (and (list? lst)
        (let loop
          ((l lst))
          (or (null? l) (and (string? (car l)) (loop (cdr l))))
        ) ;let
      ) ;and
    ) ;define

    (define (seq->vector s who)
      (cond ((string? s) (list->vector (string->list s)))
            ((vector? s) s)
            ((list? s) (list->vector s))
            (else (type-error (string-append who ": argument must be sequence (list, vector, or string)")
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (validate-diff-args from-lines to-lines fromfile tofile fromdate todate n who)
      (unless (string-list? from-lines)
        (type-error (string-append who ": lines must be a list of strings"))
      ) ;unless
      (unless (string-list? to-lines)
        (type-error (string-append who ": lines must be a list of strings"))
      ) ;unless
      (unless (string? fromfile)
        (type-error (string-append who ": fromfile must be string"))
      ) ;unless
      (unless (string? tofile)
        (type-error (string-append who ": tofile must be string"))
      ) ;unless
      (unless (string? fromdate)
        (type-error (string-append who ": fromfiledate must be string"))
      ) ;unless
      (unless (string? todate)
        (type-error (string-append who ": tofiledate must be string"))
      ) ;unless
      (unless (and (integer? n) (exact? n))
        (type-error (string-append who ": n must be an exact integer"))
      ) ;unless
      (when (< n 0)
        (value-error (string-append who ": n must be non-negative"))
      ) ;when
    ) ;define

    (define* (unified-diff from-lines
               to-lines
               (fromfile "a")
               (tofile "b")
               (fromfiledate "")
               (tofiledate "")
               (n 3)
             ) ;unified-diff
      (validate-diff-args from-lines to-lines fromfile tofile fromfiledate
        tofiledate n "unified-diff"
      ) ;validate-diff-args
      (let* ((clean-a (map strip-line-ending from-lines))
             (clean-b (map strip-line-ending to-lines))
             (vec-a (list->vector clean-a))
             (vec-b (list->vector clean-b))
             (blocks (get-matching-blocks vec-a vec-b))
             (codes (get-opcodes blocks))
             (groups (get-grouped-opcodes codes n))
            ) ;
        (if (null? groups)
          '()
          (let ((headers (make-diff-header fromfile tofile fromfiledate tofiledate))
                (body (format-all-groups groups vec-a vec-b))
               ) ;
            (append headers body)
          ) ;let
        ) ;if
      ) ;let*
    ) ;define*

    (define* (unified-diff-string from-text to-text (fromfile "a") (tofile "b") (n 3))
      ;; fromfile/tofile/n 的校验交给 unified-diff 内的 validate-diff-args，避免重复
      (unless (string? from-text)
        (type-error "unified-diff-string: from-text must be string")
      ) ;unless
      (unless (string? to-text)
        (type-error "unified-diff-string: to-text must be string")
      ) ;unless

      (let ((diff-lines (unified-diff (split-lines from-text)
                          (split-lines to-text)
                          fromfile
                          tofile
                          ""
                          ""
                          n
                        ) ;unified-diff
            ) ;diff-lines
           ) ;
        (if (null? diff-lines) "" (string-append (string-join diff-lines "\n") "\n"))
      ) ;let
    ) ;define*

    (define (diff-ratio seq1 seq2)
      (let* ((vec1 (seq->vector seq1 "diff-ratio"))
             (vec2 (seq->vector seq2 "diff-ratio"))
             (l1 (vector-length vec1))
             (l2 (vector-length vec2))
            ) ;
        (if (= (+ l1 l2) 0)
          1.0
          (let* ((blocks (get-matching-blocks vec1 vec2))
                 (matches
                   (let loop
                     ((b blocks) (sum 0))
                     (if (null? b) sum (loop (cdr b) (+ sum (list-ref (car b) 2))))
                   ) ;let
                 ) ;matches
                ) ;
            (/ (* 2.0 matches) (+ l1 l2))
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
