
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : abstract.scm
;; DESCRIPTION : abstract style for BibTeX files
;; COPYRIGHT   : (C) 2017  Philippe Joyez
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (latex bibtex-abstract)
  (:use (latex bibtex-bib-utils) (latex bibtex-plain))
) ;texmacs-module

(bib-define-style "abstract" "plain")

(tm-define (bib-format-bibitem n x)
  (:mode bib-abstract?)
  `(bibitem* ,(list-ref x 2))
) ;tm-define
