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

(define (snippet->latex snippet opts)
  (serialize-latex (texmacs->latex snippet opts)))

(tm-define (test_0190)
  ;; texmacs->latex input must be Cork-encoded (internal representation)
  (with cork-text (utf8->cork "Erwin Schrödinger")
    ;; Test: explicit UTF-8 encoding
    (with result (snippet->latex cork-text '(("texmacs->latex:encoding" . "UTF-8")))
      (display* ";;; UTF-8: " result "\n")
      (check (string-contains? result "Schrödinger") => #t))

    ;; Test: no encoding key → defaults to utf-8
    (with result (snippet->latex cork-text '())
      (display* ";;; no-encoding: " result "\n")
      (check (string-contains? result "Schrödinger") => #t))

    ;; Test: explicit lowercase utf-8
    (with result (snippet->latex cork-text '(("texmacs->latex:encoding" . "utf-8")))
      (display* ";;; lowercase utf-8: " result "\n")
      (check (string-contains? result "Schrödinger") => #t))

    ;; Test: cork encoding
    (with result (snippet->latex cork-text '(("texmacs->latex:encoding" . "cork")))
      (display* ";;; cork: " result "\n")
      (check (string-contains? result "Schr") => #t)
      (check (string-contains? result "dinger") => #t)))

  ;; Test: texmacs->generic "latex-snippet" (clipboard path)
  (with cork-text (utf8->cork "Erwin Schrödinger")
    (with doc `(document ,cork-text)
      (with result (texmacs->generic (stree->tree doc) "latex-snippet")
        (display* ";;; latex-snippet: " result "\n")
        (check (string? result) => #t)
        (check (string-contains? result "Schrödinger") => #t))))

  (check-report))
