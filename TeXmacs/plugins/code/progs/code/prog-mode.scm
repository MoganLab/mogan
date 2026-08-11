
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : prog-mode.scm
;; DESCRIPTION : Various prog modes
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code prog-mode) (:use (kernel texmacs tm-modes)))

(texmacs-modes (in-cpp% (== (get-env "prog-language") "cpp"))
  (in-prog-cpp% #t in-prog% in-cpp%)
  (in-dot% (== (get-env "prog-language") "dot"))
  (in-prog-dot% #t in-prog% in-dot%)
  (in-java% (== (get-env "prog-language") "java"))
  (in-prog-java% #t in-prog% in-java%)
  (in-javascript% (== (get-env "prog-language") "javascript"))
  (in-prog-javascript% #t in-prog% in-javascript%)
  (in-json% (== (get-env "prog-language") "json"))
  (in-prog-json% #t in-prog% in-json%)
  (in-fortran% (== (get-env "prog-language") "fortran"))
  (in-prog-fortran% #t in-prog% in-fortran%)
  (in-scheme% (== (get-env "prog-language") "scheme"))
  (in-prog-scheme% #t in-prog% in-scheme%)
  (in-python% (== (get-env "prog-language") "python"))
  (in-prog-python% #t in-prog% in-python%)
  (in-julia% (== (get-env "prog-language") "julia"))
  (in-prog-julia% #t in-prog% in-julia%)
) ;texmacs-modes
