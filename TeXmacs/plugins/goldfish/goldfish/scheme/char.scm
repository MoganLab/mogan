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
  (begin
    (define (digit-value ch)
      (if (char-numeric? ch)
        (- (char->integer ch)
          (char->integer #\0)
        ) ;-
        #f
      ) ;if
    ) ;define

    (define s7-char-upcase char-upcase)

    (define (char-upcase char)
      (unless (char? char)
        (error 'type-error
          "char-upcase: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-upcase char)
    ) ;define

    (define s7-char-downcase char-downcase)

    (define (char-downcase char)
      (unless (char? char)
        (error 'type-error
          "char-downcase: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-downcase char)
    ) ;define

    (define (char-foldcase char)
      (unless (char? char)
        (error 'type-error
          "char-foldcase: parameter must be character"
        ) ;error
      ) ;unless
      (char-downcase char)
    ) ;define

    (define s7-char-numeric? char-numeric?)

    (define (char-numeric? char)
      (unless (char? char)
        (error 'type-error
          "char-numeric?: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-numeric? char)
    ) ;define

    (define s7-char-alphabetic?
      char-alphabetic?
    ) ;define

    (define (char-alphabetic? char)
      (unless (char? char)
        (error 'type-error
          "char-alphabetic?: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-alphabetic? char)
    ) ;define

    (define s7-char-whitespace?
      char-whitespace?
    ) ;define

    (define (char-whitespace? char)
      (unless (char? char)
        (error 'type-error
          "char-whitespace?: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-whitespace? char)
    ) ;define

    (define s7-char-upper-case?
      char-upper-case?
    ) ;define

    (define (char-upper-case? char)
      (unless (char? char)
        (error 'type-error
          "char-upper-case?: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-upper-case? char)
    ) ;define

    (define s7-char-lower-case?
      char-lower-case?
    ) ;define

    (define (char-lower-case? char)
      (unless (char? char)
        (error 'type-error
          "char-lower-case?: parameter must be character"
        ) ;error
      ) ;unless
      (s7-char-lower-case? char)
    ) ;define

    (define s7-char-ci=? char-ci=?)

    (define (char-ci=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error
          "char-ci=?: first parameter must be character"
        ) ;error
      ) ;unless
      (unless (char? char2)
        (error 'type-error
          "char-ci=?: second parameter must be character"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-char-ci=? char1 char2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-char (car remaining)))
            (unless (char? next-char)
              (error 'type-error
                "char-ci=?: parameter must be character"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-char-ci=? char2 next-char)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-char-ci<? char-ci<?)

    (define (char-ci<? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error
          "char-ci<?: first parameter must be character"
        ) ;error
      ) ;unless
      (unless (char? char2)
        (error 'type-error
          "char-ci<?: second parameter must be character"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-char-ci<? char1 char2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-char (car remaining)))
            (unless (char? next-char)
              (error 'type-error
                "char-ci<?: parameter must be character"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-char-ci<? char2 next-char)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-char-ci>? char-ci>?)

    (define (char-ci>? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error
          "char-ci>?: first parameter must be character"
        ) ;error
      ) ;unless
      (unless (char? char2)
        (error 'type-error
          "char-ci>?: second parameter must be character"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-char-ci>? char1 char2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-char (car remaining)))
            (unless (char? next-char)
              (error 'type-error
                "char-ci>?: parameter must be character"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-char-ci>? char2 next-char)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-char-ci>=? char-ci>=?)

    (define (char-ci>=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error
          "char-ci>=?: first parameter must be character"
        ) ;error
      ) ;unless
      (unless (char? char2)
        (error 'type-error
          "char-ci>=?: second parameter must be character"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-char-ci>=? char1 char2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-char (car remaining)))
            (unless (char? next-char)
              (error 'type-error
                "char-ci>=?: parameter must be character"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-char-ci>=? char2 next-char)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-char-ci<=? char-ci<=?)

    (define (char-ci<=? char1 char2 . rest)
      (unless (char? char1)
        (error 'type-error
          "char-ci<=?: first parameter must be character"
        ) ;error
      ) ;unless
      (unless (char? char2)
        (error 'type-error
          "char-ci<=?: second parameter must be character"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-char-ci<=? char1 char2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-char (car remaining)))
            (unless (char? next-char)
              (error 'type-error
                "char-ci<=?: parameter must be character"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-char-ci<=? char2 next-char)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-ci=? string-ci=?)

    (define (string-ci=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error
          "string-ci=?: first parameter must be string"
        ) ;error
      ) ;unless
      (unless (string? str2)
        (error 'type-error
          "string-ci=?: second parameter must be string"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-string-ci=? str1 str2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-str (car remaining)))
            (unless (string? next-str)
              (error 'type-error
                "string-ci=?: parameter must be string"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-string-ci=? str2 next-str)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-ci<? string-ci<?)

    (define (string-ci<? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error
          "string-ci<?: first parameter must be string"
        ) ;error
      ) ;unless
      (unless (string? str2)
        (error 'type-error
          "string-ci<?: second parameter must be string"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-string-ci<? str1 str2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-str (car remaining)))
            (unless (string? next-str)
              (error 'type-error
                "string-ci<?: parameter must be string"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-string-ci<? str2 next-str)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-ci>? string-ci>?)

    (define (string-ci>? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error
          "string-ci>?: first parameter must be string"
        ) ;error
      ) ;unless
      (unless (string? str2)
        (error 'type-error
          "string-ci>?: second parameter must be string"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-string-ci>? str1 str2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-str (car remaining)))
            (unless (string? next-str)
              (error 'type-error
                "string-ci>?: parameter must be string"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-string-ci>? str2 next-str)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-ci<=? string-ci<=?)

    (define (string-ci<=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error
          "string-ci<=?: first parameter must be string"
        ) ;error
      ) ;unless
      (unless (string? str2)
        (error 'type-error
          "string-ci<=?: second parameter must be string"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-string-ci<=? str1 str2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-str (car remaining)))
            (unless (string? next-str)
              (error 'type-error
                "string-ci<=?: parameter must be string"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-string-ci<=? str2 next-str)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-ci>=? string-ci>=?)

    (define (string-ci>=? str1 str2 . rest)
      (unless (string? str1)
        (error 'type-error
          "string-ci>=?: first parameter must be string"
        ) ;error
      ) ;unless
      (unless (string? str2)
        (error 'type-error
          "string-ci>=?: second parameter must be string"
        ) ;error
      ) ;unless
      (let loop
        ((current (s7-string-ci>=? str1 str2))
         (remaining rest)
        ) ;
        (if (null? remaining)
          current
          (let ((next-str (car remaining)))
            (unless (string? next-str)
              (error 'type-error
                "string-ci>=?: parameter must be string"
              ) ;error
            ) ;unless
            (and current
              (loop (s7-string-ci>=? str2 next-str)
                (cdr remaining)
              ) ;loop
            ) ;and
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define s7-string-upcase string-upcase)

    (define (string-upcase str)
      (unless (string? str)
        (error 'type-error
          "string-upcase: parameter must be string"
        ) ;error
      ) ;unless
      (s7-string-upcase str)
    ) ;define

    (define s7-string-downcase
      string-downcase
    ) ;define

    (define (string-downcase str)
      (unless (string? str)
        (error 'type-error
          "string-downcase: parameter must be string"
        ) ;error
      ) ;unless
      (s7-string-downcase str)
    ) ;define

    (define (string-foldcase str)
      (unless (string? str)
        (error 'type-error
          "string-foldcase: parameter must be string"
        ) ;error
      ) ;unless
      (string-downcase str)
    ) ;define

  ) ;begin
) ;define-library
