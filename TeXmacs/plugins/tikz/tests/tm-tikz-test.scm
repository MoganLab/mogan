
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-tikz-test.scm
;; DESCRIPTION : TikZ Binary plugin (pdflatex)
;; COPYRIGHT   : (C) 2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(import (scheme base)
  (liii check)
  (liii string)
  (liii list)
  (liii path)
)

; Simulate the wrap-tikz-code logic from tm-tikz.scm
(define (wrap-tikz-code code)
  (let ((trimmed (string-trim-left code)))
    (if (string-starts? trimmed "\\documentclass")
      code
      (let* ((lines (string-split code #\newline))
             (library-lines
               (filter (lambda (line)
                         (string-starts? (string-trim-left line) "\\usetikzlibrary"))
                       lines))
             (other-lines
               (filter (lambda (line)
                         (not (string-starts? (string-trim-left line) "\\usetikzlibrary")))
                       lines))
             (body (string-join other-lines "\n"))
             (body-trimmed (string-trim-left body)))
        (let ((inner-code
                (if (or (string-null? body-trimmed)
                        (string-starts? body-trimmed "\\begin{tikzpicture}"))
                  body
                  (string-append "\\begin{tikzpicture}\n" body "\n\\end{tikzpicture}")
                ) ;if
              ) ;inner-code
             ) ;
        (string-append
          "\\documentclass[tikz]{standalone}\n\\begin{document}\n"
          (if (null? library-lines) "" (string-append (string-join library-lines "\n") "\n"))
          inner-code
          "\n\\end{document}"
        ) ;string-append
        ) ;let inner-code
      ) ;let*
    ) ;if
  ) ;let
) ;define

; Simulate the parse-magic-line logic from tm-tikz.scm
(define (parse-magic-line magic-line)
  (let ((tokens (filter (lambda (x) (not (string-null? x))) (string-split magic-line #\space)))
        (width "0px")
        (height "0px")
       ) ;
    (let loop ((args (cdr tokens)))
      (cond ((or (null? args) (null? (cdr args))) (list width height))
            ((string=? (car args) "-width")
             (set! width (cadr args))
             (loop (cddr args))
            ) ;
            ((string=? (car args) "-height")
             (set! height (cadr args))
             (loop (cddr args))
            ) ;
            (else (loop (cddr args)))
      ) ;cond
    ) ;let loop
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for wrap-tikz-code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check
  (wrap-tikz-code "\\documentclass{article}\n\\begin{document}\n\\end{document}")
  =>
  "\\documentclass{article}\n\\begin{document}\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\documentclass[tikz]{standalone}")
  =>
  "  \\documentclass[tikz]{standalone}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n  \\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{shapes.geometric}\n\\node[draw, circle] at (0,0) {A};")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{shapes.geometric}\n\\begin{tikzpicture}\n\\node[draw, circle] at (0,0) {A};\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\usetikzlibrary{arrows.meta}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\usetikzlibrary{arrows.meta}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{shapes.geometric}")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{shapes.geometric}\n\n\\end{document}"
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for parse-magic-line
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check (parse-magic-line "%") => (list "0px" "0px"))

(check (parse-magic-line "% -width 0.8par") => (list "0.8par" "0px"))

(check (parse-magic-line "% -height 100px") => (list "0px" "100px"))

(check (parse-magic-line "% -width 0.8par -height 100px") => (list "0.8par" "100px"))

(check (parse-magic-line "% -height 100px -width 0.8par") => (list "0.8par" "100px"))

(check (parse-magic-line "% -foo bar -width 0.5par -baz qux -height 50px") => (list "0.5par" "50px"))

(check (parse-magic-line "% -width 0.8par -height") => (list "0.8par" "0px"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; image-valid? helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (image-valid? path)
  (and (file-exists? path) (> (path-getsize path) 10)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for image-valid?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-empty-path "/tmp/tikz-test-empty.txt")

(with-output-to-file test-empty-path
  (lambda ()
    (display "")))

(check (image-valid? "/tmp/nonexistent-file-12345.txt") => #f)
(check (image-valid? test-empty-path) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; pdf-page-empty? helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (pdf-page-empty? log-path)
  (let ((p (open-input-file log-path)))
    (let loop ()
      (let ((line (read-line p)))
        (if (eof-object? line)
            (begin (close-input-port p) #t)
            (if (string-contains? line "papersize=")
                (let* ((parts (string-split line #\=))
                       (size-part (cadr parts))
                       (dims (string-split size-part #\,))
                       (w-str (string-remove-suffix (car dims) "pt"))
                       (h-str (string-remove-suffix (cadr dims) "pt"))
                       (w (string->number w-str))
                       (h (string->number h-str)))
                  (close-input-port p)
                  (or (not w) (not h)
                      (and (<= w 1.0) (<= h 1.0))))
                (loop)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for pdf-page-empty?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-log-empty-path "/tmp/tikz-test-log-empty.log")
(define test-log-valid-path "/tmp/tikz-test-log-valid.log")
(define test-log-vertical-path "/tmp/tikz-test-log-vertical.log")
(define test-log-horizontal-path "/tmp/tikz-test-log-horizontal.log")

(with-output-to-file test-log-empty-path
  (lambda ()
    (display "<special> papersize=0.4pt,0.4pt\n")))

(with-output-to-file test-log-valid-path
  (lambda ()
    (display "<special> papersize=28.85274pt,28.85274pt\n")))

(with-output-to-file test-log-vertical-path
  (lambda ()
    (display "<special> papersize=0.4pt,28.85274pt\n")))

(with-output-to-file test-log-horizontal-path
  (lambda ()
    (display "<special> papersize=28.85274pt,0.4pt\n")))

(check (pdf-page-empty? test-log-empty-path) => #t)
(check (pdf-page-empty? test-log-valid-path) => #f)
(check (pdf-page-empty? test-log-vertical-path) => #f)
(check (pdf-page-empty? test-log-horizontal-path) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Run all tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check-report "TikZ plugin unit tests")
