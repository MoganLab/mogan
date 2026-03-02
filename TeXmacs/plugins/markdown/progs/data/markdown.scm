;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : markdown.scm
;; DESCRIPTION : markdown data format (export snippet)
;; COPYRIGHT   : (C) 2026  Liii Network
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data markdown)
  (:use (binary pandoc)
        (kernel gui menu-widget)))

(import (liii uuid)
        (liii os))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Markdown format definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format markdown
  (:name "Markdown")
  (:suffix "md" "markdown"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TeXmacs -> Markdown (snippet)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (latex->markdown-via-pandoc text-latex pandoc-path)
  (let* ((temp-dir (string-append (os-temp-dir) "/markdown"))
         (temp-name (uuid4))
         (latex-temp (string-append temp-dir "/" temp-name ".tex"))
         (md-temp (string-append temp-dir "/" temp-name ".md")))
    (when (not (file-exists? temp-dir))
      (mkdir temp-dir))
    (string-save text-latex latex-temp)
    (let ((cmd (string-append "\"" pandoc-path "\""
                              " "
                              latex-temp
                              " -o "
                              md-temp
                              " -f latex -t markdown --no-highlight")))
      (system cmd)
      (let ((result (string-load md-temp)))
        (system-remove latex-temp)
        (system-remove md-temp)
        result))))

(tm-define (texmacs-tree->markdown-snippet t)
  (:synopsis "Export TeXmacs snippet to markdown using Pandoc")
  (if (has-binary-pandoc?)
      (latex->markdown-via-pandoc
        (texmacs->generic t "latex-snippet")
        (url->string (find-binary-pandoc)))
      (error "No pandoc binary detected")))

(converter texmacs-tree markdown-snippet
  (:function texmacs-tree->markdown-snippet))
