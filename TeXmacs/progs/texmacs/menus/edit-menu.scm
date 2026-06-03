
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : edit-menu.scm
;; DESCRIPTION : the edit menu
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus edit-menu)
  (:use (utils library cursor) (utils edit selections) (generic paste-widget))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (clipboard-extern-menu cvs fun)
  (with l
    (filter (lambda (x)
              (or (with-developer-tool?)
                (and (not (string=? x "mgs")) (not (string=? x "stm")))
              ) ;or
            ) ;lambda
      (cvs "texmacs-snippet" "-snippet" #t)
    ) ;filter
    (for (fm l) (with name (format-get-name fm) ((eval name) (fun fm "primary"))))
  ) ;with
) ;tm-menu

(tm-define (clipboard-copy-export-menu)
  (clipboard-extern-menu converters-from-special clipboard-copy-export)
) ;tm-define
(tm-define (clipboard-cut-export-menu)
  (clipboard-extern-menu converters-from-special clipboard-cut-export)
) ;tm-define
(tm-define (clipboard-paste-import-menu)
  (clipboard-extern-menu converters-to-special clipboard-paste-import)
) ;tm-define

(tm-menu (redo-menu)
  (for (i (.. 0 (redo-possibilities)))
   ((eval `(concat ,"Branch " ,(number->string (+ i 1)))) (redo i))
  ) ;for
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Edit menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-property (search-start forward?) (:interactive #t))
(tm-property (spell-start) (:interactive #t))

(menu-bind edit-menu
  (when (> (undo-possibilities) 0)
    ("Undo" (undo 0))
  ) ;when
  (when (> (redo-possibilities) 0)
    (if (<= (redo-possibilities) 1) ("Redo" (redo 0)))
    (if (> (redo-possibilities) 1) (-> "Redo" (link redo-menu)))
  ) ;when
  ---
  (when (or (selection-active-any?) (and (in-graphics?) (graphics-selection-active?)))
    ("Copy" (kbd-copy))
    ("Cut" (kbd-cut))
  ) ;when
  ("Paste" (kbd-paste))
  ("Magic paste" (kbd-magic-paste))
  ("Paste special" (interactive-paste-special))
  (if (detailed-menus?) ("Clear" (kbd-cancel)))
  ---
  ("Search" (interactive-search))
  ("Replace" (interactive-replace))
  (if (not (in-math?)) ("Spell" (interactive-spell)))
  (if (in-math?) (=> "Correct" (link math-correct-menu)))
  (if (detailed-menus?)
    ---
    (when (selection-active-any?)
      (-> "Copy to"
        (link clipboard-copy-export-menu)
        (if (qt-gui?) ("Image" (clipboard-copy-image "")))
        ---
        ("Other" (interactive clipboard-copy))
      ) ;->
      (-> "Cut to"
        (link clipboard-cut-export-menu)
        ---
        ("Other" (interactive clipboard-cut))
      ) ;->
    ) ;when
    (-> "Paste from"
      (link clipboard-paste-import-menu)
      ---
      ("Other" (interactive clipboard-cut))
    ) ;->
  ) ;if
  ---
  ("Search recent documents" (interactive docgrep-in-recent))
  ---
  (if (use-menus?) (-> "Preferences" (link preferences-menu)))
  (if (use-popups?) ("Preferences" (open-preferences)))
) ;menu-bind
