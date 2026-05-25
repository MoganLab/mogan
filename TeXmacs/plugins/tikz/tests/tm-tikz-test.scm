;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-tikz-test.scm
;; DESCRIPTION : TikZ Binary plugin (pdflatex)
;; COPYRIGHT   : (C) 2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
  (liii check)
  (liii string)
  (liii list)
  (liii path)
)

; Simulate the wrap-tikz-code logic from tm-tikz.scm
(define (wrap-tikz-code code)
  (let ((trimmed (string-trim-left code)))
    (if (or (string-starts? trimmed "\\documentclass")
            (string-contains? code "\\begin{document}"))
      code
      (let* ((lines (string-split code #\newline))
             (library-lines
               (filter (lambda (line)
                         (string-starts? (string-trim-left line) "\\usetikzlibrary"))
                       lines))
             (package-lines
               (filter (lambda (line)
                         (string-starts? (string-trim-left line) "\\usepackage"))
                       lines))
             (other-lines
               (filter (lambda (line)
                         (let ((trimmed-line (string-trim-left line)))
                           (and (not (string-starts? trimmed-line "\\usetikzlibrary"))
                                (not (string-starts? trimmed-line "\\usepackage"))
                                (not (string-null? trimmed-line)))))
                       lines))
             (body (string-join other-lines "\n"))
             (body-trimmed (string-trim-left body)))
        (let* ((has-chemfig? (or (string-starts? body-trimmed "\\chem")
                                 (string-starts? body-trimmed "\\scheme")))
               (need-chemfig-package?
                 (or has-chemfig?
                     (string-contains? body "\\chem")
                     (string-contains? body "\\scheme")))
               (need-optikz-package?
                 (or (string-contains? body "\\spectrometer")
                     (string-contains? body "\\camera")
                     (string-contains? body "\\diode")
                     (string-contains? body "\\splitter")
                     (string-contains? body "\\convexlens")
                     (string-contains? body "\\concavelens")
                     (string-contains? body "\\planconvexlens")
                     (string-contains? body "\\mirror")
                     (string-contains? body "\\curvedmirror")
                     (string-contains? body "\\pockelscell")
                     (string-contains? body "\\laser")
                     (string-contains? body "\\grating")
                     (string-contains? body "\\drawrainbow")
                     (string-contains? body "\\TFP")))
               (extra-packages
                 (let ((pkgs '()))
                   (when (and need-chemfig-package?
                              (null? (filter (lambda (line) (string-contains? line "chemfig"))
                                             package-lines)))
                     (set! pkgs (cons "\\usepackage{chemfig}" pkgs)))
                   (when (and need-optikz-package?
                              (null? (filter (lambda (line) (string-contains? line "optikz"))
                                             package-lines)))
                     (set! pkgs (cons "\\usepackage{optikz}" pkgs)))
                   (when (and need-optikz-package?
                              (null? (filter (lambda (line) (string-contains? line "calc"))
                                             package-lines)))
                     (set! pkgs (cons "\\usepackage{calc}" pkgs)))
                   (reverse pkgs)))
               (inner-code
                 (if (or (string-null? body-trimmed)
                         (string-contains? body "\\begin{tikzpicture}")
                         has-chemfig?)
                   body
                   (string-append "\\begin{tikzpicture}\n" body "\n\\end{tikzpicture}")
                 ) ;if
               ) ;inner-code
              ) ;
        (string-append
          "\\documentclass[tikz,rgb]{standalone}\n"
          (if (null? package-lines) "" (string-append (string-join package-lines "\n") "\n"))
          (if (null? extra-packages) "" (string-append (string-join extra-packages "\n") "\n"))
          "\\begin{document}\n"
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for wrap-tikz-code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check
  (wrap-tikz-code "\\documentclass{article}\n\\begin{document}\n\\end{document}")
  =>
  "\\documentclass{article}\n\\begin{document}\n\\end{document}"
)

(check
  (wrap-tikz-code "% some comment\n\\documentclass[tikz]{standalone}\n\\begin{document}\n\\draw (0,0) -- (1,1);\n\\end{document}")
  =>
  "% some comment\n\\documentclass[tikz]{standalone}\n\\begin{document}\n\\draw (0,0) -- (1,1);\n\\end{document}"
)

(check
  (wrap-tikz-code "\\begin{document}\n\\draw (0,0) -- (1,1);\n\\end{document}")
  =>
  "\\begin{document}\n\\draw (0,0) -- (1,1);\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\documentclass[tikz]{standalone}")
  =>
  "  \\documentclass[tikz]{standalone}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\def\\skala{0.8}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\def\\skala{0.8}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n  \\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{shapes.geometric}\n\\node[draw, circle] at (0,0) {A};")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\usetikzlibrary{shapes.geometric}\n\\begin{tikzpicture}\n\\node[draw, circle] at (0,0) {A};\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\usetikzlibrary{arrows.meta}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\usetikzlibrary{arrows.meta}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{shapes.geometric}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\begin{document}\n\\usetikzlibrary{shapes.geometric}\n\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usepackage{chemfig}\n\\chemfig{*6(=-=-=-)}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{chemfig}\n\\begin{document}\n\\chemfig{*6(=-=-=-)}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\chemfig{*6(=-=-=-)}")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{chemfig}\n\\begin{document}\n\\chemfig{*6(=-=-=-)}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usepackage{chemfig}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{chemfig}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\node (molecule) at (0,0) {\\chemfig{*6(--=--)}};\n\\draw[->, red] (molecule) -- (2,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{chemfig}\n\\begin{document}\n\\begin{tikzpicture}\n\\node (molecule) at (0,0) {\\chemfig{*6(--=--)}};\n\\draw[->, red] (molecule) -- (2,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usepackage{pgfplots}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{pgfplots}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\def\\skala{0.8}\n\\spectrometer[angle=0] at (5.5,4);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{optikz}\n\\usepackage{calc}\n\\begin{document}\n\\begin{tikzpicture}\n\\def\\skala{0.8}\n\\spectrometer[angle=0] at (5.5,4);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\usepackage{optikz}\n\\usepackage{calc}\n\\def\\skala{0.8}\n\\spectrometer[angle=0] at (5.5,4);")
  =>
  "\\documentclass[tikz,rgb]{standalone}\n\\usepackage{optikz}\n\\usepackage{calc}\n\\begin{document}\n\\begin{tikzpicture}\n\\def\\skala{0.8}\n\\spectrometer[angle=0] at (5.5,4);\n\\end{tikzpicture}\n\\end{document}"
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for parse-magic-line
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check (parse-magic-line "%") => (list "0px" "0px"))

(check (parse-magic-line "% -width 0.8par") => (list "0.8par" "0px"))

(check (parse-magic-line "% -height 100px") => (list "0px" "100px"))

(check (parse-magic-line "% -width 0.8par -height 100px") => (list "0.8par" "100px"))

(check (parse-magic-line "% -height 100px -width 0.8par") => (list "0.8par" "100px"))

(check (parse-magic-line "% -foo bar -width 0.5par -baz qux -height 50px") => (list "0.5par" "50px"))

(check (parse-magic-line "% -width 0.8par -height") => (list "0.8par" "0px"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; image-valid? helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (image-valid? path)
  (and (file-exists? path) (> (path-getsize path) 10)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for image-valid?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-empty-path "/tmp/tikz-test-empty.txt")

(with-output-to-file test-empty-path
  (lambda ()
    (display "")))

(check (image-valid? "/tmp/nonexistent-file-12345.txt") => #f)
(check (image-valid? test-empty-path) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; pdf-page-empty? helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (get-pdf-page-size log-path)
  (let ((p (open-input-file log-path)))
    (let loop ()
      (let ((line (read-line p)))
        (if (eof-object? line)
            (begin (close-input-port p) #f)
            (if (string-contains? line "papersize=")
                (let* ((parts (string-split line #\=))
                       (size-part (cadr parts))
                       (dims (string-split size-part #\,))
                       (w-str (string-remove-suffix (car dims) "pt"))
                       (h-str (string-remove-suffix (cadr dims) "pt"))
                       (w (string->number w-str))
                       (h (string->number h-str)))
                  (close-input-port p)
                  (if (and w h)
                      (list w h)
                      #f))
                (loop)))))))

(define (pdf-page-empty? log-path)
  (let ((size (get-pdf-page-size log-path)))
    (if (not size)
        #t
        (let ((w (car size))
              (h (cadr size)))
          (and (<= w 0.1) (<= h 0.1))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for pdf-page-empty?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-log-empty-path "/tmp/tikz-test-log-empty.log")
(define test-log-valid-path "/tmp/tikz-test-log-valid.log")
(define test-log-vertical-path "/tmp/tikz-test-log-vertical.log")
(define test-log-horizontal-path "/tmp/tikz-test-log-horizontal.log")
(define test-log-point-path "/tmp/tikz-test-log-point.log")

(with-output-to-file test-log-empty-path
  (lambda ()
    (display "<special> papersize=0.0pt,0.0pt\n")))

(with-output-to-file test-log-valid-path
  (lambda ()
    (display "<special> papersize=28.85274pt,28.85274pt\n")))

(with-output-to-file test-log-vertical-path
  (lambda ()
    (display "<special> papersize=0.4pt,28.85274pt\n")))

(with-output-to-file test-log-horizontal-path
  (lambda ()
    (display "<special> papersize=28.85274pt,0.4pt\n")))

(with-output-to-file test-log-point-path
  (lambda ()
    (display "<special> papersize=0.4pt,0.4pt\n")))

(check (get-pdf-page-size test-log-empty-path) => (list 0.0 0.0))
(check (get-pdf-page-size test-log-valid-path) => (list 28.85274 28.85274))
(check (get-pdf-page-size test-log-vertical-path) => (list 0.4 28.85274))
(check (get-pdf-page-size test-log-horizontal-path) => (list 28.85274 0.4))
(check (get-pdf-page-size test-log-point-path) => (list 0.4 0.4))

(check (pdf-page-empty? test-log-empty-path) => #t)
(check (pdf-page-empty? test-log-valid-path) => #f)
(check (pdf-page-empty? test-log-vertical-path) => #f)
(check (pdf-page-empty? test-log-horizontal-path) => #f)
(check (pdf-page-empty? test-log-point-path) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Run all tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check-report "TikZ plugin unit tests")
