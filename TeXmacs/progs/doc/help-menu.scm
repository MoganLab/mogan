
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : help-menu.scm
;; DESCRIPTION : the help menu
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (doc help-menu)
  (:use (doc help-funcs))); (doc apidoc)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Help menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (plugin-documented? name)
  (or (url-exists-in-help? (string-append name ".en.tmu"))
      (url-exists-in-help? (string-append name ".en.tm"))))

(tm-menu (help-plugins-menu)
  (for (name (list-filter (map symbol->string (plugin-list))
                          plugin-documented?))
    (with menu-name `(verbatim ,(session-name name))
      ((eval menu-name)
       (load-local-plugin-doc name)))))

(tm-menu (help-manual-menu)
  ("Getting started"
   (load-local-doc "manual/manu.gettingstarted"))
  ("Typing simple texts"
   (load-local-doc "manual/manu.typing"))
  ("Mathematical formulas"
   (load-local-doc "manual/manu.formulas"))
  ("Editing tools"
   (load-local-doc "manual/manu.editing")))

(menu-bind help-menu
  ("Welcome" (mogan-welcome))
  (-> "Manual"
    (link help-manual-menu))
  (-> "Plugins"
    (link help-plugins-menu)))
