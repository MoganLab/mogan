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

;; Acknowledgements
;;
;; This implementation is based on:
;;
;; 1. SRFI-267: Raw String Syntax by Peter McGoron
;;    https://srfi.schemers.org/srfi-267/
;;    Copyright © 2025-2026 Peter McGoron
;;    Permission granted under the MIT-style license
;;
;; 2. Syntax #"" proposed by John Cowan
;;    https://codeberg.org/scheme/r7rs/issues/32#issuecomment-8863095
;;    in Scheme raw string discussions

(define-library (srfi srfi-267)
  (import (only (srfi srfi-13)
            string-contains
            string-suffix?
          ) ;only
  ) ;import
  (export raw-string-read-error?
    raw-string-write-error?
    read-raw-string
    read-raw-string-after-prefix
    can-delimit?
    generate-delimiter
    write-raw-string
  ) ;export
  (begin

    (define (raw-string-read-error? obj)
      (or (eq? obj 'raw-string-read-error)
        (and (pair? obj)
          (eq? (car obj) 'raw-string-read-error)
        ) ;and
      ) ;or
    ) ;define

    (define (raw-string-write-error? obj)
      (or (eq? obj 'raw-string-write-error)
        (and (pair? obj)
          (eq? (car obj) 'raw-string-write-error)
        ) ;and
      ) ;or
    ) ;define

    (define (raise-raw-string-read-error . args)
      (throw 'raw-string-read-error
        (cons 'raw-string-read-error args)
      ) ;throw
    ) ;define

    (define (raise-raw-string-write-error . args)
      (throw 'raw-string-write-error
        (cons 'raw-string-write-error args)
      ) ;throw
    ) ;define

    (define (valid-delimiter? delimiter)
      (and (string? delimiter)
        (not (string-contains delimiter "\""))
      ) ;and
    ) ;define

    (define (can-delimit? str delimiter)
      (when (not (string? str))
        (error 'type-error
          "can-delimit?: first parameter must be string"
        ) ;error
      ) ;when
      (when (not (string? delimiter))
        (error 'type-error
          "can-delimit?: second parameter must be string"
        ) ;error
      ) ;when
      (and (valid-delimiter? delimiter)
        (let ((needle (string-append "\"" delimiter "\"")
              ) ;needle
              (suffix (string-append "\"" delimiter))
             ) ;
          (and (not (string-contains str needle))
            (not (string-suffix? suffix str))
          ) ;and
        ) ;let
      ) ;and
    ) ;define

    (define (generate-delimiter str)
      (when (not (string? str))
        (error 'type-error
          "generate-delimiter: parameter must be string"
        ) ;error
      ) ;when
      (let loop
        ((n 0))
        (let ((candidate (make-string n #\-)))
          (if (can-delimit? str candidate)
            candidate
            (loop (+ n 1))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (read-raw-body delimiter port)
      (let ((out (open-output-string))
            (delimiter-len (string-length delimiter)
            ) ;delimiter-len
           ) ;
        (define (read-next-char where)
          (let ((ch (read-char port)))
            (if (eof-object? ch)
              (raise-raw-string-read-error "unexpected end of input while reading raw string"
                where
              ) ;raise-raw-string-read-error
              ch
            ) ;if
          ) ;let
        ) ;define
        (define (write-prefix matched-count)
          (let loop
            ((i 0))
            (when (< i matched-count)
              (write-char (string-ref delimiter i)
                out
              ) ;write-char
              (loop (+ i 1))
            ) ;when
          ) ;let
        ) ;define
        (define (read-body-from-char ch)
          (if (char=? ch (string-ref delimiter 0))
            (match-delimiter 1)
            (begin
              (write-char ch out)
              (read-body)
            ) ;begin
          ) ;if
        ) ;define
        (define (match-delimiter matched-count)
          (if (= matched-count delimiter-len)
            (get-output-string out)
            (let ((ch (read-next-char 'delimiter)))
              (if (char=? ch
                    (string-ref delimiter matched-count)
                  ) ;char=?
                (match-delimiter (+ matched-count 1))
                (begin
                  (write-prefix matched-count)
                  (read-body-from-char ch)
                ) ;begin
              ) ;if
            ) ;let
          ) ;if
        ) ;define
        (define (read-body)
          (read-body-from-char (read-next-char 'body)
          ) ;read-body-from-char
        ) ;define
        (read-body)
      ) ;let
    ) ;define

    (define (read-raw-string-after-prefix-fragment prefix-fragment
              port
            ) ;read-raw-string-after-prefix-fragment
      ;; The fragment starts immediately after the #, so it must begin with ".
      ;; The sharp-reader hook can stop before the delimiter is complete, so we
      ;; continue reading until the opener's closing quote is found.
      (when (not (string? prefix-fragment))
        (raise-raw-string-read-error "reader prefix fragment must be string"
        ) ;raise-raw-string-read-error
      ) ;when
      (when (or (= (string-length prefix-fragment) 0)
              (not (char=? #\"
                     (string-ref prefix-fragment 0)
                   ) ;char=?
              ) ;not
            ) ;or
        (raise-raw-string-read-error "raw string prefix must begin with a double quote"
        ) ;raise-raw-string-read-error
      ) ;when
      (let ((prefix-out (open-output-string)))
        (display prefix-fragment prefix-out)
        (let loop
          ()
          (let ((ch (read-char port)))
            (cond ((eof-object? ch)
                   (raise-raw-string-read-error "unexpected end of input while reading raw string delimiter"
                   ) ;raise-raw-string-read-error
                  ) ;
                  ((char=? ch #\")
                   (read-raw-body (string-append (get-output-string prefix-out)
                                    "\""
                                  ) ;string-append
                     port
                   ) ;read-raw-body
                  ) ;
                  (else (write-char ch prefix-out) (loop))
            ) ;cond
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (read-raw-string-after-prefix
              .
              maybe-port
            ) ;
      (let ((port (cond ((null? maybe-port)
                         (current-input-port)
                        ) ;
                        ((null? (cdr maybe-port))
                         (car maybe-port)
                        ) ;
                        (else (error 'wrong-number-of-args))
                  ) ;cond
            ) ;port
           ) ;
        (read-raw-string-after-prefix-fragment "\""
          port
        ) ;read-raw-string-after-prefix-fragment
      ) ;let
    ) ;define

    (define (read-raw-string . maybe-port)
      (let ((port (cond ((null? maybe-port)
                         (current-input-port)
                        ) ;
                        ((null? (cdr maybe-port))
                         (car maybe-port)
                        ) ;
                        (else (error 'wrong-number-of-args))
                  ) ;cond
            ) ;port
           ) ;
        (let ((hash (read-char port)))
          (when (or (eof-object? hash)
                  (not (char=? hash #\#))
                ) ;or
            (raise-raw-string-read-error "expected raw string to start with #\""
            ) ;raise-raw-string-read-error
          ) ;when
          (let ((quote-char (read-char port)))
            (when (or (eof-object? quote-char)
                    (not (char=? quote-char #\"))
                  ) ;or
              (raise-raw-string-read-error "expected raw string to start with #\""
              ) ;raise-raw-string-read-error
            ) ;when
            (read-raw-string-after-prefix port)
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (write-raw-string
              str
              delimiter
              .
              maybe-port
            ) ;
      (when (not (string? str))
        (error 'type-error
          "write-raw-string: first parameter must be string"
        ) ;error
      ) ;when
      (when (not (string? delimiter))
        (error 'type-error
          "write-raw-string: second parameter must be string"
        ) ;error
      ) ;when
      (let ((port (cond ((null? maybe-port)
                         (current-output-port)
                        ) ;
                        ((null? (cdr maybe-port))
                         (car maybe-port)
                        ) ;
                        (else (error 'wrong-number-of-args))
                  ) ;cond
            ) ;port
           ) ;
        (when (not (can-delimit? str delimiter))
          (raise-raw-string-write-error "delimiter cannot represent the given string"
            delimiter
          ) ;raise-raw-string-write-error
        ) ;when
        (display "#\"" port)
        (display delimiter port)
        (write-char #\" port)
        (display str port)
        (write-char #\" port)
        (display delimiter port)
        (write-char #\" port)
      ) ;let
    ) ;define
    (define (reader-read-raw-string prefix-fragment)
      (read-raw-string-after-prefix-fragment prefix-fragment
        (current-input-port)
      ) ;read-raw-string-after-prefix-fragment
    ) ;define
    (set! *#readers*
      (cons (cons #\" reader-read-raw-string)
        *#readers*
      ) ;cons
    ) ;set!
  ) ;begin
) ;define-library
