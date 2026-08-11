
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : prog-drd.scm
;; DESCRIPTION : data relation definitions for prog mode
;; COPYRIGHT   : (C) 2005  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code prog-drd))

;; Code fragments

(define-group code-tag (inline-code-tag) (block-code-tag))

(define-group inline-code-tag
  verbatim
  scm
  cpp
  mmx
  r
  fortran
  python
  scilab
  shell
) ;define-group

(define-group block-code-tag
  verbatim-code
  scm-code
  cpp-code
  mmx-code
  r-code
  fortran-code
  python-code
  scilab-code
  shell-code
) ;define-group

;; Listings
(define-group listing-tag listing shell-listing scm-listing cpp-listing)
