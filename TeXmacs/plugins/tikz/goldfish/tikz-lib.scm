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
  (export escape-string goldfish-quote wrap-tikz-code gen-temp-path)
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

  ) ;begin
) ;define-library
