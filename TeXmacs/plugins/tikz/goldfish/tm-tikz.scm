
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-tikz.scm
;; DESCRIPTION : Goldfish session backend for TikZ
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;                   2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
  (liii os)
  (liii path)
  (liii uuid)
  (liii sys)
  (liii string)
  (liii list)
  (liii error)
)

(define (escape-string str)
  (string-join (map (lambda (char)
                      (if (char=? char #\")
                        (string #\\ #\")
                        (if (char=? char #\\) (string #\\ #\\) (string char))
                      )
                    )
                 (string->list str)
               )
  )
)

(define (goldfish-quote s)
  (string-append "\"" (escape-string s) "\"")
)

(define (tikz-welcome)
  (flush-prompt "tikz] ")
  (flush-verbatim (string-append "TikZ session by XmacsLabs\n"
                    "implemented in Goldfish Scheme ("
                    (version)
                    ")"
                  ))
)

(define (tikz-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (string=? line "<EOF>\n") code (read-code (string-append code line)))
    )
  )
  (read-code "")
)

(define (gen-temp-path)
  (let ((tikz-tmpdir (string-append (os-temp-dir) "/tikz")))
    (when (not (file-exists? tikz-tmpdir))
      (mkdir tikz-tmpdir)
    )
    (string-append tikz-tmpdir "/" (uuid4))
  )
)

(define (wrap-tikz-code code)
  (string-append
    "\\documentclass[tikz,border=10pt]{standalone}\n"
    "\\usepackage{tikz}\n"
    "\\begin{document}\n"
    code
    "\\end{document}\n"
  )
)

(define (tikz-compile tex-path pdf-path)
  (let ((cmd (fourth (argv))))
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (os-call (string-append (goldfish-quote cmd)
                " -interaction=nonstopmode -output-directory "
                (goldfish-quote (path->string (path-parent pdf-path)))
                " "
                (goldfish-quote tex-path)))
  )
)

(define (flush-image path width height)
  (if (and (path-exists? path) (> (path-getsize path) 10))
    (flush-file (string-append (path->string path) "?" "width=" width "&" "height=" height))
    (flush-verbatim "Failed to generate image")
  )
)

(define (eval-and-print code)
  (let* ((width "0.8par")
         (height "0px")
         (temp-path (gen-temp-path))
         (tex-path (string-append temp-path ".tex"))
         (pdf-path (string-append temp-path ".pdf"))
        )
    (path-write-text tex-path (wrap-tikz-code code))
    (tikz-compile tex-path pdf-path)
    (flush-image pdf-path width height)
  )
)

(define (safe-read-eval-print)
  (catch #t
    (lambda () (eval-and-print (tikz-read-code)))
    (lambda args
      (flush-verbatim (string-append "Error: "
                        (symbol->string (car args))
                        " "
                        (if (and (>= (length args) 2) (not (null? (cadr args))))
                          (object->string (cadr args))
                          ""
                        )))
    )
  )
)

(define (tikz-repl)
  (safe-read-eval-print)
  (tikz-repl)
)

(tikz-welcome)
(tikz-repl)
