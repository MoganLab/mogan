;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tikz-lib.scm
;; DESCRIPTION : Pure functions for TikZ session backend
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (tikz lib)
  (export escape-string
    goldfish-quote
    wrap-tikz-code
    gen-temp-path
    tikz-read-code-from-port
    tikz-build-cmd
  ) ;export
  (import (scheme base)
    (liii os)
    (liii path)
    (liii uuid)
    (liii string)
    (liii list)
  ) ;import
  (begin

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

    (define (wrap-tikz-code code)
      (string-append "\\documentclass[tikz,border=10pt]{standalone}\n"
        "\\usepackage{tikz}\n"
        "\\begin{document}\n"
        code
        "\\end{document}\n"
      ) ;string-append
    ) ;define

    (define (gen-temp-path)
      (let ((tikz-tmpdir (string-append (os-temp-dir) "/tikz")))
        (when (not (file-exists? tikz-tmpdir))
          (mkdir tikz-tmpdir)
        ) ;when
        (string-append tikz-tmpdir "/" (uuid4))
      ) ;let
    ) ;define

    (define (tikz-read-code-from-port port)
      (define (read-code code)
        (let ((line (read-line port)))
          (cond ((eof-object? line) code)
                ((or (string=? line "<EOF>\n") (string=? line "<EOF>")) code)
                (else (let ((normalized (if (and (> (string-length line) 0)
                                              (char=? (string-ref line (- (string-length line) 1)) #\newline)
                                            ) ;and
                                          line
                                          (string-append line (string #\newline))
                                        ) ;if
                            ) ;normalized
                           ) ;
                        (read-code (string-append code normalized))
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let
      ) ;define
      (read-code "")
    ) ;define

    (define (tikz-build-cmd cmd tex-path pdf-path)
      (let ((redirect (if (os-windows?) " > NUL 2>&1 " " > /dev/null 2>&1 ")))
        (string-append (goldfish-quote cmd)
          " -interaction=nonstopmode -output-directory "
          (goldfish-quote (path->string (path-parent (path pdf-path))))
          " "
          (goldfish-quote tex-path)
          redirect
        ) ;string-append
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
