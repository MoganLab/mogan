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
  (export read-raw-string)
  (begin

   (define (read-raw-string S)
     ;; This parser starts reading after `"`.
     ;; In the given examples, the parser starts at the dot:
     ;;
     ;; #"."asdf""
     ;; #"--."#"()""--"
     (define (read-char* location)
       (let ((ch (read-char))) ; (read-char (current-input-string))
         (if (eof-object? ch)
             (error (list "eof in raw string literal" location))
             ch)))
     (define delimiter
       ;; always expect `"`
       (let ((ch (read-char* 'delim)))
         (append (string->list S) (list ch))))
     (call-with-port (open-output-string)
       (lambda (out)
         (define (read-delimiter n rest-of-delimiter)
           (if (null? rest-of-delimiter)
               (get-output-string out)
               (let ((ch (read-char* 'check)))
                 (if (char=? ch (car rest-of-delimiter))
                     (read-delimiter (+ n 1) (cdr rest-of-delimiter))
                     (do ((n n (- n 1))
                          (delimiter delimiter (cdr delimiter)))
                         ((zero? n) (read-raw ch))
                       (write-char (car delimiter) out))))))
         (define (read-raw ch)
           (if (char=? ch (car delimiter))
               (read-delimiter 1 (cdr delimiter))
               (begin (write-char ch out)
                      (read-raw (read-char* 'read)))))
         (read-raw (read-char* 'read)))))

     (set! *#readers*
       (cons (cons #\" read-raw-string)
             *#readers*))

    ) ; end of begin
  ) ; end of library
