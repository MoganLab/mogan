;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol)
  (liii os)
  (liii path)
  (liii sys)
  (liii error)
) ;import

(load (string-append (path->string (path-parent (path (list-ref (argv) 2)))) "/tikz-lib.scm"))

(define (tikz-welcome)
  (flush-prompt "tikz] ")
  (flush-verbatim (string-append "TikZ session by XmacsLabs\n"
                    "implemented in Goldfish Scheme ("
                    (version)
                    ")"
                  ) ;string-append
  ) ;flush-verbatim
) ;define

(define (tikz-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (string=? line "<EOF>\n") code (read-code (string-append code line)))
    ) ;let
  ) ;define
  (read-code "")
) ;define

(define (tikz-compile tex-path pdf-path)
  (let ((cmd (fourth (argv)))
        (redirect (if (os-windows?) " > NUL 2>&1 " " > /dev/null 2>&1 "))
       ) ;
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (let ((ret (system (string-append (goldfish-quote cmd)
                         " -interaction=nonstopmode -output-directory "
                         (goldfish-quote (path->string (path-parent pdf-path)))
                         " "
                         (goldfish-quote tex-path)
                         redirect
                       ) ;string-append
               ) ;system
          ) ;ret
         ) ;
      (if (= ret 0)
        #t
        (begin
          (flush-verbatim (string-append "LaTeX compilation failed with exit code " (number->string ret))
          ) ;flush-verbatim
          #f
        ) ;begin
      ) ;if
    ) ;let
  ) ;let
) ;define

(define (flush-image path width height)
  (if (and (path-exists? path) (> (path-getsize path) 10))
    (flush-file (string-append (path->string path) "?" "width=" width "&" "height=" height)
    ) ;flush-file
    (flush-verbatim "Failed to generate image")
  ) ;if
) ;define

(define (eval-and-print code)
  (let* ((width "0.8par")
         (height "0px")
         (temp-path (gen-temp-path))
         (tex-path (string-append temp-path ".tex"))
         (pdf-path (string-append temp-path ".pdf"))
        ) ;
    (path-write-text tex-path (wrap-tikz-code code))
    (when (tikz-compile tex-path pdf-path)
      (flush-image pdf-path width height)
    ) ;when
  ) ;let*
) ;define

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
                        ) ;if
                      ) ;string-append
      ) ;flush-verbatim
    ) ;lambda
  ) ;catch
) ;define

(define (tikz-repl)
  (safe-read-eval-print)
  (tikz-repl)
) ;define

(tikz-welcome)
(tikz-repl)
