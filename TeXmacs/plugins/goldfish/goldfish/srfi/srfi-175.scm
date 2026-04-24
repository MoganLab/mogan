;; Copyright 2019 Lassi Kortela
;; SPDX-License-Identifier: MIT

(define-library (srfi srfi-175)
  (export ascii-codepoint?
    ascii-bytevector?

    ascii-char?
    ascii-string?

    ascii-control?
    ascii-non-control?
    ascii-whitespace?
    ascii-space-or-tab?
    ascii-other-graphic?
    ascii-upper-case?
    ascii-lower-case?
    ascii-alphabetic?
    ascii-alphanumeric?
    ascii-numeric?

    ascii-digit-value
    ascii-upper-case-value
    ascii-lower-case-value
    ascii-nth-digit
    ascii-nth-upper-case
    ascii-nth-lower-case
    ascii-upcase
    ascii-downcase
    ascii-control->graphic
    ascii-graphic->control
    ascii-mirror-bracket

    ascii-ci=?
    ascii-ci<?
    ascii-ci>?
    ascii-ci<=?
    ascii-ci>=?

    ascii-string-ci=?
    ascii-string-ci<?
    ascii-string-ci>?
    ascii-string-ci<=?
    ascii-string-ci>=?
  ) ;export
  (begin
    (define (ensure-int x)
      (if (char? x) (char->integer x) x)
    ) ;define

    (define (base-offset-limit x base offset limit)
      (let ((cc (ensure-int x)))
        (and (>= cc base)
          (< cc (+ base limit))
          (+ offset (- cc base))
        ) ;and
      ) ;let
    ) ;define

    (define (char->int->char map-int char)
      (let ((int (map-int (char->integer char))))
        (and int (integer->char int))
      ) ;let
    ) ;define

    ;;

    (define (ascii-codepoint? x)
      (and (exact-integer? x) (<= 0 x 127))
    ) ;define

    (define (ascii-char? x)
      (and (char? x)
        (< (char->integer x) 128)
      ) ;and
    ) ;define

    (define (ascii-bytevector? x)
      (and (bytevector? x)
        (let check
          ((i (- (bytevector-length x) 1)))
          (or (< i 0)
            (and (< (bytevector-u8-ref x i) 128)
              (check (- i 1))
            ) ;and
          ) ;or
        ) ;let
      ) ;and
    ) ;define

    (define (ascii-string? x)
      (and (string? x)
        (call-with-port (open-input-string x)
          (lambda (in)
            (let check
              ()
              (let ((char (read-char in)))
                (or (eof-object? char)
                  (and (< (char->integer char) 128)
                    (check)
                  ) ;and
                ) ;or
              ) ;let
            ) ;let
          ) ;lambda
        ) ;call-with-port
      ) ;and
    ) ;define

    (define (ascii-control? x)
      (let ((cc (ensure-int x)))
        (or (<= 0 cc 31) (= cc 127))
      ) ;let
    ) ;define

    (define (ascii-non-control? x)
      (let ((cc (ensure-int x)))
        (<= 32 cc 126)
      ) ;let
    ) ;define

    (define (ascii-whitespace? x)
      (let ((cc (ensure-int x)))
        (cond ((< cc 9) #f)
              ((< cc 14) #t)
              (else (= cc 32))
        ) ;cond
      ) ;let
    ) ;define

    (define (ascii-space-or-tab? x)
      (let ((cc (ensure-int x)))
        (case cc
         ((9 32) #t)
         (else #f)
        ) ;case
      ) ;let
    ) ;define

    (define (ascii-other-graphic? x)
      (let ((cc (ensure-int x)))
        (or (<= 33 cc 47)
          (<= 58 cc 64)
          (<= 91 cc 96)
          (<= 123 cc 126)
        ) ;or
      ) ;let
    ) ;define

    (define (ascii-upper-case? x)
      (let ((cc (ensure-int x)))
        (<= 65 cc 90)
      ) ;let
    ) ;define

    (define (ascii-lower-case? x)
      (let ((cc (ensure-int x)))
        (<= 97 cc 122)
      ) ;let
    ) ;define

    (define (ascii-alphabetic? x)
      (let ((cc (ensure-int x)))
        (or (<= 65 cc 90) (<= 97 cc 122))
      ) ;let
    ) ;define

    (define (ascii-alphanumeric? x)
      (let ((cc (ensure-int x)))
        (or (<= 48 cc 57)
          (<= 65 cc 90)
          (<= 97 cc 122)
        ) ;or
      ) ;let
    ) ;define

    (define (ascii-numeric? x)
      (let ((cc (ensure-int x)))
        (<= 48 cc 57)
      ) ;let
    ) ;define

    ;;

    (define (ascii-digit-value x limit)
      (base-offset-limit x
        48
        0
        (min limit 10)
      ) ;base-offset-limit
    ) ;define

    (define (ascii-upper-case-value x offset limit)
      (base-offset-limit x
        65
        offset
        (min limit 26)
      ) ;base-offset-limit
    ) ;define

    (define (ascii-lower-case-value x offset limit)
      (base-offset-limit x
        97
        offset
        (min limit 26)
      ) ;base-offset-limit
    ) ;define

    (define (ascii-nth-digit n)
      (and (<= 0 n 9)
        (integer->char (+ 48 n))
      ) ;and
    ) ;define

    (define (ascii-nth-upper-case n)
      (integer->char (+ 65 (modulo n 26)))
    ) ;define

    (define (ascii-nth-lower-case n)
      (integer->char (+ 97 (modulo n 26)))
    ) ;define

    (define (ascii-upcase x)
      (if (char? x)
        (integer->char (ascii-upcase (char->integer x))
        ) ;integer->char
        (or (ascii-lower-case-value x 65 26) x)
      ) ;if
    ) ;define

    (define (ascii-downcase x)
      (if (char? x)
        (integer->char (ascii-downcase (char->integer x))
        ) ;integer->char
        (or (ascii-upper-case-value x 97 26) x)
      ) ;if
    ) ;define

    (define (ascii-control->graphic x)
      (if (char? x)
        (char->int->char ascii-control->graphic
          x
        ) ;char->int->char
        (or (and (<= 0 x 31) (+ x 64))
          (and (= x 127) 63)
        ) ;or
      ) ;if
    ) ;define

    (define (ascii-graphic->control x)
      (if (char? x)
        (char->int->char ascii-graphic->control
          x
        ) ;char->int->char
        (or (and (<= 64 x 95) (- x 64))
          (and (= x 63) 127)
        ) ;or
      ) ;if
    ) ;define

    (define (ascii-mirror-bracket x)
      (if (char? x)
        (case x
         ((#\() #\))
         ((#\)) #\()
         ((#\[) #\])
         ((#\]) #\[)
         ((#\{) #\})
         ((#\}) #\{)
         ((#\<) #\>)
         ((#\>) #\<)
         (else #f)
        ) ;case
        (let ((x (ascii-mirror-bracket (integer->char x))
              ) ;x
             ) ;
          (and x (char->integer x))
        ) ;let
      ) ;if
    ) ;define

    (define (ascii-ci-cmp char1 char2)
      (let ((cc1 (ensure-int char1))
            (cc2 (ensure-int char2))
           ) ;
        (when (<= 65 cc1 90)
          (set! cc1 (+ cc1 32))
        ) ;when
        (when (<= 65 cc2 90)
          (set! cc2 (+ cc2 32))
        ) ;when
        (cond ((< cc1 cc2) -1)
              ((> cc1 cc2) 1)
              (else 0)
        ) ;cond
      ) ;let
    ) ;define

    (define (ascii-ci=? char1 char2)
      (= (ascii-ci-cmp char1 char2) 0)
    ) ;define

    (define (ascii-ci<? char1 char2)
      (< (ascii-ci-cmp char1 char2) 0)
    ) ;define

    (define (ascii-ci>? char1 char2)
      (> (ascii-ci-cmp char1 char2) 0)
    ) ;define

    (define (ascii-ci<=? char1 char2)
      (<= (ascii-ci-cmp char1 char2) 0)
    ) ;define

    (define (ascii-ci>=? char1 char2)
      (>= (ascii-ci-cmp char1 char2) 0)
    ) ;define

    (define (ascii-string-ci-cmp string1 string2)
      (call-with-port (open-input-string string1)
        (lambda (in1)
          (call-with-port (open-input-string string2)
            (lambda (in2)
              (let loop
                ()
                (let ((char1 (read-char in1))
                      (char2 (read-char in2))
                     ) ;
                  (cond ((eof-object? char1)
                         (if (eof-object? char2) 0 -1)
                        ) ;
                        ((eof-object? char2) 1)
                        (else (let ((cc1 (char->integer char1))
                                    (cc2 (char->integer char2))
                                   ) ;
                                (when (<= 65 cc1 90)
                                  (set! cc1 (+ cc1 32))
                                ) ;when
                                (when (<= 65 cc2 90)
                                  (set! cc2 (+ cc2 32))
                                ) ;when
                                (cond ((< cc1 cc2) -1)
                                      ((> cc1 cc2) 1)
                                      (else (loop))
                                ) ;cond
                              ) ;let
                        ) ;else
                  ) ;cond
                ) ;let
              ) ;let
            ) ;lambda
          ) ;call-with-port
        ) ;lambda
      ) ;call-with-port
    ) ;define

    (define (ascii-string-ci=? string1 string2)
      (= (ascii-string-ci-cmp string1 string2)
        0
      ) ;=
    ) ;define

    (define (ascii-string-ci<? string1 string2)
      (< (ascii-string-ci-cmp string1 string2)
        0
      ) ;<
    ) ;define

    (define (ascii-string-ci>? string1 string2)
      (> (ascii-string-ci-cmp string1 string2)
        0
      ) ;>
    ) ;define

    (define (ascii-string-ci<=? string1 string2)
      (<= (ascii-string-ci-cmp string1 string2)
        0
      ) ;<=
    ) ;define

    (define (ascii-string-ci>=? string1 string2)
      (>= (ascii-string-ci-cmp string1 string2)
        0
      ) ;>=
    ) ;define
  ) ;begin
) ;define-library
