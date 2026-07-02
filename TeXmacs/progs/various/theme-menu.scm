
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : theme-menu.scm
;; DESCRIPTION : menus for standard themes
;; COPYRIGHT   : (C) 2020  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (various theme-menu)
  (:use (various theme-edit) (generic document-menu))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Web themes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (basic-theme-button-name theme)
  (let ((translation-key (string-append (if (== theme "plain") "theme" theme) "::theme")
        ) ;translation-key
       ) ;
    (translate translation-key)
  ) ;let
) ;tm-define

(tm-define (basic-theme-menu-name theme)
  (let ((translation-key (string-append (if (== theme "plain") "Plain" theme) "::theme")
        ) ;translation-key
       ) ;
    (translate translation-key)
  ) ;let
) ;tm-define

(tm-define (basic-theme-name theme) (basic-theme-button-name theme))

(tm-define (other-basic-themes) (list-difference (basic-themes) (list "dark")))

(menu-bind other-basic-themes-menu
  (for (theme (other-basic-themes))
   ((check (eval (basic-theme-menu-name theme)) "v" (has-style-package? theme))
    (toggle-style-package theme)
   ) ;
  ) ;for
) ;menu-bind

(menu-bind basic-theme-menu
 ((eval (basic-theme-menu-name "plain")) (select-default-basic-theme))
 ((check (eval (basic-theme-menu-name "dark")) "v" (has-style-package? "dark"))
  (toggle-style-package "dark")
 ) ;
 ---
 ((check "Alternative colors" "v" (has-style-package? "alt-colors"))
  (toggle-style-package "alt-colors")
 ) ;
 ((check "Framed theorems" "v" (has-style-package? "framed-theorems"))
  (toggle-style-package "framed-theorems")
 ) ;
 ---
 (-> "Other" (link other-basic-themes-menu))
) ;menu-bind
