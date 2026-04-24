;; Acknowledgements
;;
;; This implementation is based on:
;;
;; 1. The idea deindent from guile-raw-strings by François Joulaud
;;    https://codeberg.org/avalenn/guile-raw-strings
;;    SPDX-License-Identifier: 0BSD
;;
;; 2. The deindentation follows C# raw string literal rules
;;    https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/tokens/raw-string

(define-library (liii raw-string)
  (import (srfi srfi-267)
    (srfi srfi-1)
    (srfi srfi-13)
    (liii error)
  ) ;import
  (export raw-string-read-error?
    raw-string-write-error?
    read-raw-string
    read-raw-string-after-prefix
    can-delimit?
    generate-delimiter
    write-raw-string
    deindent
    &-
  ) ;export
  (begin
    (define (string-split-lines str)
      (let ((len (string-length str)))
        (let loop
          ((start 0) (result '()))
          (let ((nl-pos (string-index str #\newline start len)
                ) ;nl-pos
               ) ;
            (if (not nl-pos)
              (reverse (cons (substring str start len) result)
              ) ;reverse
              (loop (+ nl-pos 1)
                (cons (substring str start nl-pos)
                  result
                ) ;cons
              ) ;loop
            ) ;if
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (f-deindent str)
      (when (or (string-null? str)
              (not (char=? #\newline (string-ref str 0))
              ) ;not
            ) ;or
        (value-error "Raw string must start on a new line after the opening delimiter"
        ) ;value-error
      ) ;when

      (let* ((lines (string-split-lines (substring str 1 (string-length str))
                    ) ;string-split-lines
             ) ;lines
             (closing-line (last lines))
             (ref-indent (if (string-null? closing-line)
                           (value-error "Raw string delimiter must be on its own line"
                           ) ;value-error
                           (string-count closing-line #\space)
                         ) ;if
             ) ;ref-indent
             (content-lines (drop-right lines 1))
            ) ;

        ;; check indentation
        (for-each (lambda (line idx)
                    (unless (string-null? line)
                      (let ((indent (or (string-skip line #\space) 0)
                            ) ;indent
                           ) ;
                        (when (< indent ref-indent)
                          (value-error "Line ~a does not start with the same whitespace as the closing line of the raw string"
                            (+ idx 1)
                          ) ;value-error
                        ) ;when
                      ) ;let
                    ) ;unless
                  ) ;lambda
          content-lines
          (iota (length content-lines))
        ) ;for-each

        (string-join (map (lambda (line)
                            (if (string-null? line)
                              ""
                              (substring line ref-indent)
                            ) ;if
                          ) ;lambda
                       content-lines
                     ) ;map
          "\n"
        ) ;string-join
      ) ;let*
    ) ;define

    (define-macro (stx-deindent v)
      (if (string? v)
        `(quote ,(f-deindent v))
        `(quote ,v)
      ) ;if
    ) ;define-macro

    (define deindent stx-deindent)
    (define &- stx-deindent)
  ) ;begin
) ;define-library
