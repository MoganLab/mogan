;
; Copyright (C) 2024 The Mogan STEM Suite Authors
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

(import (texmacs protocol)
        (liii os)
        (liii path)
        (liii uuid)
        (liii sys)
        (liii string)
        (liii list)
        (liii error)
        (liii argparse))

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

(define (gnuplot-welcome)
  (let ((format (last (argv))))
    (flush-prompt (string-append format "] "))
    (flush-verbatim
      (string-append
        "Gnuplot session by XmacsLabs\n"
        "implemented in Goldfish Scheme (" (version) ")"))))

(define (gnuplot-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (string=? line "<EOF>\n")
          code
          (read-code (string-append code line)))))

  (read-code ""))

(define (gen-temp-path)
  (let ((gnuplot-tmpdir (string-append (os-temp-dir) "/gnuplot")))
    (when (not (file-exists? gnuplot-tmpdir))
      (mkdir gnuplot-tmpdir))
    (string-append gnuplot-tmpdir "/" (uuid4))))

(define (gen-pdf-precode pdf-path)
  (string-append
   "reset\n"
   "set terminal pdfcairo enhanced\n"
   "set output\n"
   "set output '" pdf-path "'\n"
   "set size 1,1\n"
   "set autoscale\n"))

(define (gen-eps-precode eps-path)
  (string-append
   "reset\n"
   "set terminal postscript eps enhanced\n"
   "set output\n"
   "set output '" eps-path "'\n"
   "set size 1,1\n"
   "set autoscale\n"))

(define (gen-png-precode png-path)
  (string-append
    "reset\n"
    "set terminal pngcairo enhanced\n"
    "set output\n"
    "set output '" png-path "'\n"
    "set size 1,1\n"))

(define (gen-svg-precode svg-path)
  (string-append
    "reset\n"
    "set terminal svg enhanced\n"
    "set output\n"
    "set output '" svg-path "'\n"
    "set size 1,1\n"
    "set autoscale\n"))

(define (gen-precode format path)
  (cond ((string=? format "png") (gen-png-precode path))
        ((string=? format "svg") (gen-svg-precode path))
        ((string=? format "eps") (gen-eps-precode path))
        ((string=? format "pdf") (gen-pdf-precode path))
        (else (error 'wrong-args))))

(define (gnuplot-dump-code code-path image-format image-path code)
  (with-output-to-file code-path
    (lambda ()
      (display (gen-precode image-format image-path))
      (display code))))

(define (gnuplot-plot code-path)
  (let ((cmd (fourth (argv))))
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (os-call (string-append (goldfish-quote cmd) " " "-c" " " code-path))))

(define (parse-magic-line magic-line)
  (let ((parser (make-argument-parser)))
    (parser :add '((name . "width") (short . "width") (default . "0.8par")))
    (parser :add '((name . "height") (short . "height") (default . "0px")))
    (parser :add '((name . "output") (short . "output") (default . "")))
    (parser :parse (cdr (string-tokenize magic-line)))
    (list (parser 'width) (parser 'height) (parser 'output))))
  
(define (flush-image path width height)
  (if (and (file-exists? path) (> (path-getsize path) 10))
    (flush-file (string-append path "?" "width=" width "&" "height=" height))
    (flush-verbatim "Failed to plot due to:")))

(define (eval-and-print magic-line code)
  (let* ((parsed (parse-magic-line magic-line))
         (width (first parsed))
         (height (second parsed))
         (output (third parsed))
         (option (last (argv)))
         (format (if (string-null? output) option output))
         (temp-path (gen-temp-path))
         (image-path (string-append temp-path "." format))
         (code-path (string-append temp-path ".gnuplot")))
    (gnuplot-dump-code code-path format image-path code)
    (gnuplot-plot code-path)
    (flush-image image-path width height)))

(define (split-code-and-magic-line code)
  (if (not (string-starts? code "%"))
      (list "" code)
      (let1 i/false (string-index code #\newline)
        (if (not i/false)
            (list code "")
            (list (substring code 0 i/false)
                  (substring code (+ i/false 1)))))))

(define (read-eval-print)
  (let* ((raw-code (gnuplot-read-code))
         (l (split-code-and-magic-line raw-code))
         (magic-line (car l))
         (code (cadr l)))
    (if (string-null? code)
      (flush-verbatim "No code provided!")
      (eval-and-print magic-line code))))

(define (safe-read-eval-print)
  (catch #t
    (lambda ()
      (read-eval-print))
    (lambda args
      (begin
        (flush-scheme
          (string-append "(errput (document "
            (goldfish-quote (symbol->string (car args)))
            (if (and (>= (length args) 2)
                     (not (null? (cadr args))))
              (goldfish-quote (object->string (cadr args)))
              "")
            "))"))))))

(define (gnuplot-repl)
  (begin (safe-read-eval-print)
         (gnuplot-repl)))

(gnuplot-welcome)
(gnuplot-repl)
