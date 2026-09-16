
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : bibtex.scm
;; DESCRIPTION : bibtex data plugin
;; COPYRIGHT   : (C) 2010, 2014  David MICHEL and Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (latex bibtex-format))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; BibTeX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format bibtex (:name "BibTeX") (:suffix "bib"))

(lazy-define (latex convert-bibtex-bibtextm) parse-bibtex-snippet)
(lazy-define (latex convert-bibtex-bibtextm) parse-bibtex-document)
(lazy-define (latex convert-bibtex-bibtextm) bibtex->texmacs)
(lazy-define (latex convert-bibtex-bibtexout) serialize-bibtex)
(lazy-define (latex convert-bibtex-tmbibtex) texmacs->bibtex)

(converter bibtex-snippet bibtex-stree (:function parse-bibtex-snippet))

(converter bibtex-document bibtex-stree (:function parse-bibtex-document))

(converter bibtex-stree texmacs-stree (:function bibtex->texmacs))

(converter bibtex-stree bibtex-document (:function serialize-bibtex))

(converter bibtex-stree bibtex-snippet (:function serialize-bibtex))

(converter texmacs-stree bibtex-stree (:function texmacs->bibtex))
