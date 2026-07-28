
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : prog-kbd.scm
;; DESCRIPTION : Shortcuts for program modes
;; COPYRIGHT   : (C) 2013 Miguel de Benito Delgado
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog prog-kbd)
  (:use (kernel gui kbd-define)
    (utils edit selections)
    (prog scheme-tools)
    (prog prog-mode)
    (code scheme-edit)
    (code cpp-edit)
  ) ;:use
) ;texmacs-module

(kbd-map (:mode in-prog?)
 ("cmd i" (program-indent #f))
 ("cmd I" (program-indent #t))
 ("cmd tab" (program-indent #f))
 ("cmd S-tab" (program-indent #t))
 ("cmd A-tab" (program-indent-all #f))

 ;; override some text mode shortcuts
 ("space var" (insert-tabstop))
 ("space var var" (begin (insert-tabstop) (insert-tabstop)))
 ("$" (insert "$"))
 ("$ var" (make 'math))
 ("\\" "\\")
 ("\\ var" (make 'hybrid))
 ("\"" "\"")
 ("`" (utf8-insert "`"))
 ("'" "<#27>")
 ("` var" "<#2018>")
 ("' var" "<#2019>")
 ("< <" "<less><less>")
 ("> >" "<gtr><gtr>")
 ("- -" "--")
 ("- - -" "---")
) ;kbd-map
