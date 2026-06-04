;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0190.scm
;; DESCRIPTION : Test LaTeX export of non-ASCII characters (e.g. ö)
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(define base-opts
  '(("texmacs->latex:source-tracking" . "off")
    ("texmacs->latex:conservative" . "on")
    ("texmacs->latex:transparent-source-tracking" . "on")
    ("texmacs->latex:attach-tracking-info" . "on")
    ("texmacs->latex:replace-style" . "on")
    ("texmacs->latex:expand-macros" . "on")
    ("texmacs->latex:expand-user-macros" . "off")
    ("texmacs->latex:indirect-bib" . "off")
    ("texmacs->latex:use-macros" . "off")))

(define utf8-opts
  (acons "texmacs->latex:encoding" "UTF-8" base-opts))

(define ascii-opts
  (acons "texmacs->latex:encoding" "ascii" base-opts))

(define cork-opts
  (acons "texmacs->latex:encoding" "cork" base-opts))

(define (snippet->latex snippet opts)
  (serialize-latex (texmacs->latex snippet opts)))

(define (only-ascii? s)
  (let loop ((i 0))
    (if (>= i (string-length s)) #t
      (if (> (char->integer (string-ref s i)) 127) #f
        (loop (+ i 1))))))

(tm-define (test_0190)
  ;; Test snippet -> LaTeX with UTF-8 encoding: ö stays as UTF-8
  (with result (snippet->latex "Erwin Schrödinger" utf8-opts)
    (check (string-contains? result "Schrödinger") => #t))

  ;; Test snippet -> LaTeX with ascii encoding: output must be pure ASCII
  ;; and the umlaut should become a correct LaTeX escape like \"o,
  ;; NOT broken escapes like {\~A}{\H u} from double-encoding
  (with result (snippet->latex "Erwin Schrödinger" ascii-opts)
    (check (string-contains? result "Schr") => #t)
    (check (string-contains? result "dinger") => #t)
    (check (only-ascii? result) => #t)
    ;; must NOT produce the broken double-encoded form
    (check (string-contains? result "\\~A") => #f))

  ;; Test snippet -> LaTeX with cork encoding
  (with result (snippet->latex "Erwin Schrödinger" cork-opts)
    (check (string-contains? result "Schr") => #t)
    (check (string-contains? result "dinger") => #t))

  (check-report))
