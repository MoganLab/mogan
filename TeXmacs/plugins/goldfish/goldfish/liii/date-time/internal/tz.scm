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

;; Copyright (C) 2026 Peter McGoron
;;
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;

(define-library (liii date-time internal tz)
  (import (scheme base) (liii date-time internal tz-types))
  (export tz-offset tz-offset? tz-offset-sign-negative? tz-offset-hour
    tz-offset-minute tz-offset-second tz-month-week-day tz-month-week-day?
    tz-month-week-day:month tz-month-week-day:week tz-month-week-day:day
    tz-julian-without-leap-day tz-julian-without-leap-day?
    tz-julian-without-leap-day:date tz-julian-with-leap-day
    tz-julian-with-leap-day? tz-julian-with-leap-day:date tz-transition
    tz-transition? tz-transition-date tz-transition-time tz-string tz-string?
    tz-string-stdname tz-string-stdoffset tz-string-dstname tz-string-dstoffset
    tz-string-change-to-dst tz-string-change-to-std parse-tz
  ) ;export
  (begin

    (define (ascii-alpha? ch)
      (or (char<=? #\A ch #\Z) (char<=? #\a ch #\z))
    ) ;define

    (define (ascii-digit? ch)
      (char<=? #\0 ch #\9)
    ) ;define

    (define (digit-val ch)
      (- (char->integer ch) (char->integer #\0))
    ) ;define

    (define (make-parser-state str)
      (vector str (string-length str) 0)
    ) ;define

    (define (state-peek s)
      (let ((str (vector-ref s 0)) (len (vector-ref s 1)) (pos (vector-ref s 2)))
        (if (< pos len) (string-ref str pos) #f)
      ) ;let
    ) ;define

    (define (state-next s)
      (let ((str (vector-ref s 0)) (len (vector-ref s 1)) (pos (vector-ref s 2)))
        (if (< pos len) (begin (vector-set! s 2 (+ pos 1)) (string-ref str pos)) #f)
      ) ;let
    ) ;define

    (define (parse-quoted-name s)
      (state-next s)
      (let loop
        ((acc '()))
        (let ((c (state-next s)))
          (cond ((not c) (error 'type-error "EOF in quoted timezone name"))
                ((char=? c #\>) (list->string (reverse acc)))
                (else (loop (cons c acc)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (parse-unquoted-name s)
      (let loop
        ((acc '()))
        (let ((c (state-peek s)))
          (if (and c (ascii-alpha? c))
            (begin
              (state-next s)
              (loop (cons c acc))
            ) ;begin
            (list->string (reverse acc))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (parse-name s)
      (let ((ch (state-peek s)))
        (cond ((not ch) #f)
              ((char=? ch #\<) (parse-quoted-name s))
              ((ascii-alpha? ch) (parse-unquoted-name s))
              (else #f)
        ) ;cond
      ) ;let
    ) ;define

    (define (parse-number s)
      (let loop
        ((n 0) (count 0))
        (let ((ch (state-peek s)))
          (if (and ch (ascii-digit? ch))
            (begin
              (state-next s)
              (loop (+ (* n 10) (digit-val ch)) (+ count 1))
            ) ;begin
            (if (zero? count) #f n)
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (parse-offset-parts s is-neg hour)
      (let ((has-min? (and (state-peek s) (char=? (state-peek s) #\:))))
        (if (not has-min?)
          (tz-offset is-neg hour 0 0)
          (begin
            (state-next s)
            (let ((m (parse-number s)))
              (unless m
                (error 'type-error "parse-offset: expected minute")
              ) ;unless
              (let ((has-sec? (and (state-peek s) (char=? (state-peek s) #\:))))
                (if (not has-sec?)
                  (tz-offset is-neg hour m 0)
                  (begin
                    (state-next s)
                    (let ((sec (parse-number s)))
                      (unless sec
                        (error 'type-error "parse-offset: expected second")
                      ) ;unless
                      (tz-offset is-neg hour m sec)
                    ) ;let
                  ) ;begin
                ) ;if
              ) ;let
            ) ;let
          ) ;begin
        ) ;if
      ) ;let
    ) ;define

    (define (parse-offset s)
      (let* ((ch (state-peek s))
             (is-neg
               (cond ((and ch (char=? ch #\-)) (state-next s) #t)
                     ((and ch (char=? ch #\+)) (state-next s) #f)
                     (else #f)
               ) ;cond
             ) ;is-neg
             (h (parse-number s))
            ) ;
        (unless h
          (error 'type-error "parse-offset: expected hour")
        ) ;unless
        (parse-offset-parts s is-neg h)
      ) ;let*
    ) ;define

    (define (offset-starts? s)
      (let ((ch (state-peek s)))
        (and ch (or (char=? ch #\+) (char=? ch #\-) (ascii-digit? ch)))
      ) ;let
    ) ;define

    (define (parse-month-week-day-date s)
      (state-next s)
      (let* ((m (parse-number s))
             (_1
               (unless (and (state-peek s) (char=? (state-next s) #\.))
                 (error 'type-error "expected .")
               ) ;unless
             ) ;_1
             (w (parse-number s))
             (_2
               (unless (and (state-peek s) (char=? (state-next s) #\.))
                 (error 'type-error "expected .")
               ) ;unless
             ) ;_2
             (d (parse-number s))
            ) ;
        (tz-month-week-day m w d)
      ) ;let*
    ) ;define

    (define (parse-date s)
      (let ((ch (state-peek s)))
        (cond ((not ch) (error 'type-error "parse-date: unexpected EOF"))
              ((char=? ch #\J) (state-next s) (tz-julian-without-leap-day (parse-number s)))
              ((char=? ch #\M) (parse-month-week-day-date s))
              (else (tz-julian-with-leap-day (parse-number s)))
        ) ;cond
      ) ;let
    ) ;define

    (define (parse-time-after-date s)
      (if (and (state-peek s) (char=? (state-peek s) #\/))
        (begin
          (state-next s)
          (parse-offset s)
        ) ;begin
        (tz-offset #f 2 0 0)
      ) ;if
    ) ;define

    (define (parse-transition s)
      (let* ((d (parse-date s)) (t (parse-time-after-date s)))
        (tz-transition d t)
      ) ;let*
    ) ;define

    (define (calc-default-dst-offset stdoffset)
      (let ((is-neg (tz-offset-sign-negative? stdoffset))
            (h (tz-offset-hour stdoffset))
            (m (tz-offset-minute stdoffset))
            (sec (tz-offset-second stdoffset))
           ) ;
        (cond (is-neg (tz-offset #t (+ h 1) m sec))
              ((zero? h) (tz-offset #t 1 m sec))
              (else (tz-offset #f (- h 1) m sec))
        ) ;cond
      ) ;let
    ) ;define

    (define (parse-tz-rules s stdname stdoffset dstname dstoffset)
      (if
        (not (and (state-peek s) (char=? (state-peek s) #\,)))
        (tz-string stdname stdoffset dstname dstoffset #f #f)
        (begin
          (state-next s)
          (let* ((to-dst (parse-transition s))
                 (_
                   (unless (and (state-peek s) (char=? (state-next s) #\,))
                     (error 'type-error "expected , between transitions")
                   ) ;unless
                 ) ;_
                 (to-std (parse-transition s))
                ) ;
            (tz-string stdname stdoffset dstname dstoffset to-dst to-std)
          ) ;let*
        ) ;begin
      ) ;if
    ) ;define

    (define (parse-tz-from-state s)
      (let* ((stdname (parse-name s)) (stdoffset (parse-offset s)) (dstname (parse-name s)))
        (if (not dstname)
          (tz-string stdname stdoffset #f #f #f #f)
          (let ((dstoffset (if (offset-starts? s) (parse-offset s) (calc-default-dst-offset stdoffset))
                ) ;dstoffset
               ) ;
            (parse-tz-rules s stdname stdoffset dstname dstoffset)
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (parse-tz input)
      (let ((str (cond ((string? input) input)
                       ((bytevector? input) (utf8->string input))
                       (else (error 'type-error "parse-tz: invalid input" input))
                 ) ;cond
            ) ;str
           ) ;
        (parse-tz-from-state (make-parser-state str))
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
