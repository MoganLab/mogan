
(import (texmacs protocol)
  (liii os)
  (liii path)
  (liii uuid)
  (liii sys)
  (liii string)
  (liii list)
  (liii error)
  (liii argparse)
) ;import

(define (escape-string str)
  (string-join (map (lambda (char)
                      (if (char=? char #\")
                        (string #\\ #\")
                        (if (char=? char #\\) (string #\\ #\\) (string char))
                      ) ;if
                    ) ;lambda
                 (string->list str)
               ) ;map
  ) ;string-join
) ;define

(define (goldfish-quote s)
  (string-append "\"" (escape-string s) "\"")
) ;define

(define (gnuplot-welcome)
  (let ((format (last (argv))))
    (flush-prompt (string-append format "] "))
    (flush-verbatim (string-append "Gnuplot session by XmacsLabs\n"
                      "implemented in Goldfish Scheme ("
                      (version)
                      ")"
                    ) ;string-append
    ) ;flush-verbatim
  ) ;let
) ;define

(define (gnuplot-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (string=? line "<EOF>\n") code (read-code (string-append code line)))
    ) ;let
  ) ;define

  (read-code "")
) ;define

(define (gen-temp-path)
  (let ((gnuplot-tmpdir (string-append (os-temp-dir) "/gnuplot")))
    (when (not (file-exists? gnuplot-tmpdir))
      (mkdir gnuplot-tmpdir)
    ) ;when
    (string-append gnuplot-tmpdir "/" (uuid4))
  ) ;let
) ;define

(define (gen-pdf-precode pdf-path)
  (string-append "reset\n"
    "set terminal pdfcairo enhanced\n"
    "set output\n"
    "set output '"
    pdf-path
    "'\n"
    "set size 1,1\n"
    "set autoscale\n"
  ) ;string-append
) ;define

(define (gen-eps-precode eps-path)
  (string-append "reset\n"
    "set terminal postscript eps enhanced\n"
    "set output\n"
    "set output '"
    eps-path
    "'\n"
    "set size 1,1\n"
    "set autoscale\n"
  ) ;string-append
) ;define

(define (gen-png-precode png-path)
  (string-append "reset\n"
    "set terminal pngcairo enhanced\n"
    "set output\n"
    "set output '"
    png-path
    "'\n"
    "set size 1,1\n"
  ) ;string-append
) ;define

(define (gen-svg-precode svg-path)
  (string-append "reset\n"
    "set terminal svg enhanced\n"
    "set output\n"
    "set output '"
    svg-path
    "'\n"
    "set size 1,1\n"
    "set autoscale\n"
  ) ;string-append
) ;define

(define (gen-precode format path)
  (cond ((string=? format "png") (gen-png-precode path))
        ((string=? format "svg") (gen-svg-precode path))
        ((string=? format "eps") (gen-eps-precode path))
        ((string=? format "pdf") (gen-pdf-precode path))
        (else (error 'wrong-args))
  ) ;cond
) ;define

(define (gnuplot-dump-code code-path image-format image-path code)
  (with-output-to-file code-path
    (lambda () (display (gen-precode image-format image-path)) (display code))
  ) ;with-output-to-file
) ;define

(define (gnuplot-plot code-path)
  (let ((cmd (fourth (argv))))
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (os-call (string-append (goldfish-quote cmd) " " "-c" " " code-path))
  ) ;let
) ;define

(define (parse-magic-line magic-line)
  (let ((parser (make-argument-parser)))
    (parser :add '((name . "width") (short . "width") (default . "0.8par")))
    (parser :add '((name . "height") (short . "height") (default . "0px")))
    (parser :add '((name . "output") (short . "output") (default . "")))
    (parser :parse (cdr (string-tokenize magic-line)))
    (list (parser 'width) (parser 'height) (parser 'output))
  ) ;let
) ;define

(define (flush-image path width height)
  (if (and (file-exists? path) (> (path-getsize path) 10))
    (flush-file (string-append path "?" "width=" width "&" "height=" height))
    (flush-verbatim "Failed to plot due to:")
  ) ;if
) ;define

(define (eval-and-print magic-line code)
  (let* ((parsed (parse-magic-line magic-line))
         (width (first parsed))
         (height (second parsed))
         (output (third parsed))
         (option (last (argv)))
         (format (if (string-null? output) option output))
         (temp-path (gen-temp-path))
         (image-path (string-append temp-path "." format))
         (code-path (string-append temp-path ".gnuplot"))
        ) ;
    (gnuplot-dump-code code-path format image-path code)
    (gnuplot-plot code-path)
    (flush-image image-path width height)
  ) ;let*
) ;define

(define (split-code-and-magic-line code)
  (if (not (string-starts? code "%"))
    (list "" code)
    (let ((i/false (string-index code #\newline)))
      (if (not i/false)
        (list code "")
        (list (substring code 0 i/false) (substring code (+ i/false 1)))
      ) ;if
    ) ;let
  ) ;if
) ;define

(define (read-eval-print)
  (let* ((raw-code (gnuplot-read-code))
         (l (split-code-and-magic-line raw-code))
         (magic-line (car l))
         (code (cadr l))
        ) ;
    (if (string-null? code)
      (flush-verbatim "No code provided!")
      (eval-and-print magic-line code)
    ) ;if
  ) ;let*
) ;define

(define (safe-read-eval-print)
  (catch #t
    (lambda () (read-eval-print))
    (lambda args
      (begin
        (flush-scheme (string-append "(errput (document "
                        (goldfish-quote (symbol->string (car args)))
                        (if (and (>= (length args) 2) (not (null? (cadr args))))
                          (goldfish-quote (object->string (cadr args)))
                          ""
                        ) ;if
                        "))"
                      ) ;string-append
        ) ;flush-scheme
      ) ;begin
    ) ;lambda
  ) ;catch
) ;define

(define (gnuplot-repl)
  (begin
    (safe-read-eval-print)
    (gnuplot-repl)
  ) ;begin
) ;define

(gnuplot-welcome)
(gnuplot-repl)
