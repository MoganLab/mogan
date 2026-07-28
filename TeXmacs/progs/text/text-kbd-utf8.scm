
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : text-kbd-utf8.scm
;; DESCRIPTION : keystrokes in text mode
;; COPYRIGHT   : (C) 2024   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (text text-kbd-utf8)
  (:use (generic generic-kbd)
    (utils edit auto-close)
    (text text-edit)
    (various comment-edit)
    (various comment-widgets)
  ) ;:use
) ;texmacs-module

(kbd-map (:mode in-text?)
 ("<" "<less>")
 (">" "<gtr>")
 ("< var" "<#2039>")
 ("> var" "<#203A>")
 ("- >" "<#2192>")
 ("- > var" "->")
 ("< -" "<#2190>")
 ("< - var" "<less>-")
) ;kbd-map

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Greek symbols
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map ("alpha" "<#3B1>")
 ("beta" "<#3B2>")
 ("gamma" "<#3B3>")
 ("delta" "<#3B4>")
 ("epsilon" "<#3F5>")
 ("varepsilon" "<#3B5>")
 ("zeta" "<#3B6>")
 ("eta" "<#3B7>")
 ("theta" "<#3B8>")
 ("vartheta" "<#3D1>")
 ("iota" "<#3B9>")
 ("kappa" "<#3BA>")
 ("varkappa" "<#3F0>")
 ("lambda" "<#3BB>")
 ("mu" "<#3BC>")
 ("nu" "<#3BD>")
 ("xi" "<#3BE>")
 ("omicron" "<#3BF>")
 ("pi" "<#3C0>")
 ("varpi" "<#3D6>")
 ("rho" "<#3C1>")
 ("varrho" "<#3F1>")
 ("sigma" "<#3C3>")
 ("varsigma" "<#3C2>")
 ("tau" "<#3C4>")
 ("upsilon" "<#3C5>")
 ("phi" "<#3C6>")
 ("varphi" "<#3D5>")
 ("chi" "<#3C7>")
 ("psi" "<#3C8>")
 ("omega" "<#3C9>")
 ("Alpha" "<#391>")
 ("Beta" "<#392>")
 ("Gamma" "<#393>")
 ("Delta" "<#394>")
 ("Epsilon" "<#395>")
 ("Zeta" "<#396>")
 ("Eta" "<#397>")
 ("Theta" "<#398>")
 ("Iota" "<#399>")
 ("Kappa" "<#39A>")
 ("Lambda" "<#39B>")
 ("Mu" "<#39C>")
 ("Nu" "<#39D>")
 ("Xi" "<#39E>")
 ("Omicron" "<#39F>")
 ("Pi" "<#3A0>")
 ("Rho" "<#3A1>")
 ("Sigma" "<#3A3>")
 ("Tau" "<#3A4>")
 ("Upsilon" "<#3A5>")
 ("Phi" "<#3A6>")
 ("Chi" "<#3A7>")
 ("Psi" "<#3A8>")
 ("Omega" "<#3A9>")
 ("varOmega" "<#1D6FA>")
) ;kbd-map

(kbd-map (:profile no-macos)
 ("text ;" (make-folded-comment "comment"))
 ("text :" (make-unfolded-comment "comment"))
) ;kbd-map

(kbd-map (:profile macos)
 ("std ;" (make-folded-comment "comment"))
 ("std /" (make-unfolded-comment "comment"))
) ;kbd-map
