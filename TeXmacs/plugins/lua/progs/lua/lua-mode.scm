
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : lua-lang.scm
;; DESCRIPTION : Lua Language mode
;; COPYRIGHT   : (C) 2025  Fanjie Meng
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code lua-mode)
  (:use (kernel texmacs tm-modes)))

(texmacs-modes
  (in-lua% (== (get-env "prog-language") "lua"))
  (in-prog-lua% #t in-prog% in-lua%))
