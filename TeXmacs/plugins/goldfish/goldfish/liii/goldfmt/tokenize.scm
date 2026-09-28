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
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

(define-library (liii goldfmt tokenize)
  (export tokenize tokens->string escape-string-content)
  (import (liii base) (liii string) (liii unicode))
  (begin
    (define (trim-right-spaces str)
      (if
        (string-every (lambda (c) (or (char=? c #\space) (char=? c #\tab))) str)
        str
        (utf8-string-trim-right str)
      ) ;if
    ) ;define
    (define (escape-string-content content)
      (let loop
        ((chars (string->list content)) (result '()))
        (if (null? chars)
          (list->string (reverse result))
          (let ((c (car chars)))
            (cond
             ((char=? c #\\) (loop (cdr chars) (cons #\\ (cons #\\ result))))
             ((char=? c #\") (loop (cdr chars) (cons #\" (cons #\\ result))))
             ((char=? c #\newline) (loop (cdr chars) (cons #\n (cons #\\ result))))
             ((char=? c #\return) (loop (cdr chars) result))
             (else (loop (cdr chars) (cons c result)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define
    (define (blank-line? str)
      (or (string-null? str)
        (string-every (lambda (c) (or (char=? c #\space) (char=? c #\tab))) str)
      ) ;or
    ) ;define
    (define (tokenize content)
      (let ((len (string-length content))
            (tokens '())
            (chunk-start 0)
            (line-chunks '())
            (in-string #f)
            (string-escaped #f)
            (in-block-comment #f)
            (in-raw-string #f)
            (raw-delimiter "")
            (raw-delimiter-match 0)
           ) ;
        (define (get-current-line i)
          (let ((final-chunk (if (> i chunk-start) (list (substring content chunk-start i)) '())
                ) ;final-chunk
               ) ;
            (if (null? line-chunks)
              (if (null? final-chunk) "" (car final-chunk))
              (apply string-append (reverse (append final-chunk line-chunks)))
            ) ;if
          ) ;let
        ) ;define
        (define (reset-current-line next-pos)
          (set! chunk-start next-pos)
          (set! line-chunks '())
        ) ;define
        (define (process-char i)
          (if (>= i len)
            (let ((current-line (get-current-line i)))
              (if
                (and (not (string-null? current-line))
                  (not
                    (string-every (lambda (c)
                                    (or (char=? c #\space)
                                      (char=? c #\tab)
                                      (char=? c #\newline)
                                      (char=? c #\return)
                                    ) ;or
                                  ) ;lambda
                      current-line
                    ) ;string-every
                  ) ;not
                ) ;and
                (set! tokens (cons (cons 'code current-line) tokens))
              ) ;if
              (reverse tokens)
            ) ;let
            (let ((c (string-ref content i))
                  (next-c (if (< (+ i 1) len) (string-ref content (+ i 1)) #\nul))
                 ) ;
              (cond
                (in-string
                  (cond (string-escaped (set! string-escaped #f) (process-char (+ i 1)))
                        ((char=? c #\\) (set! string-escaped #t) (process-char (+ i 1)))
                        ((char=? c #\") (set! in-string #f) (process-char (+ i 1)))
                        (else (process-char (+ i 1)))
                  ) ;cond
                ) ;in-string
                (in-block-comment
                  (cond ((and (char=? c #\|) (char=? next-c #\#))
                         (set! in-block-comment #f)
                         (set! chunk-start (+ i 2))
                         (process-char (+ i 2))
                        ) ;
                        (else (process-char (+ i 1)))
                  ) ;cond
                ) ;in-block-comment
                (in-raw-string
                  (cond
                   ((and (= (string-length raw-delimiter) 0) (char=? c #\") (char=? next-c #\"))
                    (set! in-raw-string #f)
                    (set! raw-delimiter "")
                    (set! raw-delimiter-match 0)
                    (process-char (+ i 2))
                   ) ;
                   ((and (< raw-delimiter-match (string-length raw-delimiter))
                      (char=? c (string-ref raw-delimiter raw-delimiter-match))
                    ) ;and
                    (set! raw-delimiter-match (+ raw-delimiter-match 1))
                    (if (>= raw-delimiter-match (string-length raw-delimiter))
                      (if
                        (and (< (+ i 1) len) (char=? (string-ref content (+ i 1)) #\"))
                        (begin
                          (set! in-raw-string #f)
                          (set! raw-delimiter "")
                          (set! raw-delimiter-match 0)
                          (process-char (+ i 2))
                        ) ;begin
                        (process-char (+ i 1))
                      ) ;if
                      (process-char (+ i 1))
                    ) ;if
                   ) ;
                   ((char=? c #\newline) (set! raw-delimiter-match 0) (process-char (+ i 1)))
                   (else (set! raw-delimiter-match 0) (process-char (+ i 1)))
                  ) ;cond
                ) ;in-raw-string
                (else
                  (cond
                   ((and (char=? c #\#) (char=? next-c #\|))
                    (when (> i chunk-start)
                      (set! line-chunks (cons (substring content chunk-start i) line-chunks))
                    ) ;when
                    (set! in-block-comment #t)
                    (process-char (+ i 2))
                   ) ;
                   ((and (char=? c #\#) (char=? next-c #\"))
                    (let ((delim-end (string-index content #\" (+ i 2))))
                      (if delim-end
                        (begin
                          (set! in-raw-string #t)
                          (set! raw-delimiter (substring content (+ i 2) delim-end))
                          (set! raw-delimiter-match 0)
                          (process-char (+ delim-end 1))
                        ) ;begin
                        (process-char (+ i 1))
                      ) ;if
                    ) ;let
                   ) ;
                   ((char=? c #\") (set! in-string #t) (process-char (+ i 1)))
                   ((and (char=? c #\;) (char=? next-c #\;))
                    (let ((current-line (get-current-line i)))
                      (if
                        (and (not (string-null? current-line))
                          (not
                            (string-every (lambda (c) (or (char=? c #\space) (char=? c #\tab)))
                              current-line
                            ) ;string-every
                          ) ;not
                        ) ;and
                        (set! tokens (cons (cons 'code current-line) tokens))
                      ) ;if
                      (let* ((comment-start (+ i 2))
                             (newline-pos (string-index content #\newline i))
                             (comment-end (or newline-pos len))
                             (raw-content (substring content comment-start comment-end))
                             (trimmed-content (trim-right-spaces raw-content))
                            ) ;
                        (set! tokens (cons (cons 'comment trimmed-content) tokens))
                        (if newline-pos
                          (begin
                            (reset-current-line (+ newline-pos 1))
                            (process-char (+ newline-pos 1))
                          ) ;begin
                          (reverse tokens)
                        ) ;if
                      ) ;let*
                    ) ;let
                   ) ;
                   ((char=? c #\newline)
                    (let ((current-line (get-current-line i)))
                      (if (and (not (string-null? current-line)) (not (blank-line? current-line)))
                        (begin
                          (set! tokens (cons (cons 'code current-line) tokens))
                          (reset-current-line (+ i 1))
                          (process-char (+ i 1))
                        ) ;begin
                        (if (null? tokens)
                          (begin
                            (reset-current-line (+ i 1))
                            (process-char (+ i 1))
                          ) ;begin
                          (let count-loop
                            ((pos (+ i 1)) (blank-count (if (blank-line? current-line) 1 0)))
                            (if (>= pos len)
                              (reverse tokens)
                              (let ((next-char (string-ref content pos)))
                                (cond ((char=? next-char #\return) (count-loop (+ pos 1) blank-count))
                                      ((char=? next-char #\newline) (count-loop (+ pos 1) (+ blank-count 1)))
                                      (else
                                        (when (> blank-count 0)
                                          (set! tokens (cons (cons 'newline blank-count) tokens))
                                        ) ;when
                                        (reset-current-line pos)
                                        (process-char pos)
                                      ) ;else
                                ) ;cond
                              ) ;let
                            ) ;if
                          ) ;let
                        ) ;if
                      ) ;if
                    ) ;let
                   ) ;
                   (else (process-char (+ i 1)))
                  ) ;cond
                ) ;else
              ) ;cond
            ) ;let
          ) ;if
        ) ;define
        (process-char 0)
      ) ;let
    ) ;define
    (define (tokens->string tokens)
      (let ((out (open-output-string)))
        (let loop
          ((rest tokens))
          (if (null? rest)
            (get-output-string out)
            (let* ((token (car rest)) (type (car token)) (content (cdr token)))
              (cond ((eq? type 'comment)
                     (display "(*comment* \"" out)
                     (display (escape-string-content content) out)
                     (display "\")" out)
                    ) ;
                    ((eq? type 'newline)
                     (display "(*newline* " out)
                     (display (number->string content) out)
                     (display ")" out)
                    ) ;
                    (else (display content out))
              ) ;cond
              (when (not (null? (cdr rest)))
                (display "\n" out)
              ) ;when
              (loop (cdr rest))
            ) ;let*
          ) ;if
        ) ;let
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
