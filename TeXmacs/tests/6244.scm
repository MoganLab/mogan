;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6244.scm
;; DESCRIPTION : Regression tests for \varphi / \varpi / \phi LaTeX import and export
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")
(import-from (latex latex-format))
(import-from (latex convert-latex-tmtex))

(define (contains? t x)
  (cond ((== t x) #t)
        ((pair? t) (or (contains? (car t) x) (contains? (cdr t) x)))
        ((string? t) (string-contains? t x))
        (else #f)
  ) ;cond
) ;define

(define (test-import-varphi-varpi-phi)
  (check (tree->stree (latex->texmacs (parse-latex "\\(\\varphi\\)")))
    =>
    '(math "<varphi>")
  ) ;check
  (check (tree->stree (latex->texmacs (parse-latex "\\(\\varpi\\)")))
    =>
    '(math "<varpi>")
  ) ;check
  (check (tree->stree (latex->texmacs (parse-latex "\\(\\phi\\)")))
    =>
    '(math "<phi>")
  ) ;check
  (check (tree->stree (latex->texmacs (parse-latex "\\(\\varphi \\varpi \\phi\\)")))
    =>
    '(math "<varphi>*<varpi>*<phi>")
  ) ;check
  (check (tree->stree (latex->texmacs (parse-latex "\\(\\Delta_{\\varphi} \\Delta_{\\varpi} \\Delta_{\\phi}\\)")
                      ) ;latex->texmacs
         ) ;tree->stree
    =>
    '(math (concat "<Delta>"
             (rsub "<varphi>")
             "*<Delta>"
             (rsub "<varpi>")
             "*<Delta>"
             (rsub "<phi>")))
  ) ;check
) ;define

(define (test-import-ope-example)
  (let* ((src (string-append "**一阶修正（\\(N^{-2}\\)）：树图与无新算符情形**\n"
                "进入 \\(N^{-2}\\) 阶，我们需要修正 OPE 数据。如果在这个阶数没有新算符出现，OPE 依然是 \\(\\varphi \\varphi = 1 + [\\varphi \\varphi]\\)。\n"
                "\\[\n"
                " \\mathcal{G}^{(1)} (U, V) = \\sum_{n, \\ell} U^{\\Delta_{\\varphi} + n} \\left(a_{n, \\ell}^{(1)} + \\frac{1}{2} a_{n, \\ell}^{(0)} \\gamma_{n, \\ell}^{(1)} \\left(\\log U + \\frac{\\partial}{\\partial n}\\right)\\right) g_{2 \\Delta_{\\varphi} + 2 n, \\ell} (U, V)\n"
                "\\]"
              ) ;string-append
         ) ;src
         (st (tree->stree (latex->texmacs (parse-latex src))))
        ) ;
    (check (contains? st "<varphi>") => #t)
    (check (contains? st "<phi>") => #f)
  ) ;let*
) ;define

(define (test-document-importers)
  (let* ((doc "\\documentclass{article}\\begin{document}\\(\\varphi \\varpi \\phi\\)\\end{document}"
         ) ;doc
         (expected
           '(document (TeXmacs "2.1.4")
              (style (tuple "article" "std-latex"))
              (body (document (math "<varphi>*<varpi>*<phi>"))))
         ) ;expected
        ) ;
    (check (tree->stree (cpp-latex-document->texmacs doc #f)) => expected)
    (check (tree->stree (tracked-latex->texmacs doc #f)) => expected)
    (check (tree->stree (conservative-latex->texmacs doc #f)) => expected)
  ) ;let*
) ;define

(define (test-export-varphi-varpi-phi)
  (check (tmtex '(math "<varphi>")) => '(!math (!symbol (varphi))))
  (check (tmtex '(math "<varpi>")) => '(!math (!symbol (varpi))))
  (check (tmtex '(math "<phi>")) => '(!math (!symbol (phi))))
) ;define

(tm-define (test_6244)
  (test-import-varphi-varpi-phi)
  (test-import-ope-example)
  (test-document-importers)
  (test-export-varphi-varpi-phi)
  (check-report)
) ;tm-define
