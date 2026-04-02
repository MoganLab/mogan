;
; Copyright (C) 2024 The Goldfish Scheme Authors
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

(define-library (goldfish repl)
(import (texmacs protocol)
        (liii list)
        (liii string)
        (liii sys)
        (liii base))
(export goldfish-welcome goldfish-repl is-sicp-mode?)
(begin

(define (goldfish-welcome)
  (let1 mode (last (argv))
    (if (string=? mode "default")
        (flush-prompt "> ")
        (flush-prompt (string-append (string-upcase mode) "] "))))

  (flush-verbatim
    (string-append
      "Goldfish Scheme " (version) " Community Edition by LiiiLabs\n"
      "implemented on S7 Scheme (" (substring (*s7* 'version) 3) ")")))

(define (escape-string str)
  (string-join
    (map (lambda (char)
           (if (char=? char #\")
               (string #\\ #\")
               (if (char=? char #\\)
                   (string #\\ #\\)
                   (string char))))
         (string->list str))))

(define (goldfish-quote s)
  (string-append "\"" (escape-string s) "\""))

(define (build-goldfish-result obj)
  (let ((output (object->string obj))
        (leadings (list "(document" "(math" "(equation*" "(align" "(with" "(graphics")))
    (if (find (lambda (x) (string-starts? output x)) leadings)
        output
        (string-append "(goldfish-result " (goldfish-quote output) ")"))))

(define (goldfish-print obj)
  (if (eq? obj #<unspecified>)
    (flush-scheme-u8 "")
    (flush-scheme-u8 (build-goldfish-result obj))))

(define (eval-and-print code)
  (catch #t
    (lambda ()
      (goldfish-print (eval-string code (rootlet))))
    (lambda args
      (begin
        (flush-scheme-u8
          (string-append "(errput (document "
            (goldfish-quote (symbol->string (car args)))
            (if (and (>= (length args) 2)
                     (not (null? (cadr args))))
              (goldfish-quote (object->string (cadr args)))
              "")
            "))"))))))

(define (read-eval-print)
  (let ((code (read-paragraph-by-visible-eof)))
    (if (string=? code "")
      #t
      (eval-and-print code))))

(define (goldfish-repl)
  (begin (read-eval-print)
         (goldfish-repl)))


) ; end of begin
) ; end of define-library
