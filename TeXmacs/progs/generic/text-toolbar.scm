
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : text-toolbar.scm
;; DESCRIPTION : text selection toolbar icons
;; COPYRIGHT   : (C) 2026   Jie Chen
;;                          Yifan Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic text-toolbar)
  (:use (generic format-edit)
        (generic format-menu)
        (generic generic-edit)
        (table table-menu)
        (text text-menu)
        (math math-menu)))

(menu-bind text-toolbar-text-icons
  ((balloon (icon "tm_bold.xpm") "Write bold text")
   (toggle-bold))
  ((balloon (icon "tm_italic.xpm") "Write italic text")
   (toggle-italic))
  ((balloon (icon "tm_underline.xpm") "Write underline")
   (toggle-underlined))
  ((balloon (icon "tm_marked.xpm") "Marked text")
   (mark-text))
  (=> (balloon (icon "tm_color.xpm") "Select a foreground color")
      (link color-menu))        
  ((balloon (icon "tm_cell_left.xpm") "left aligned")
   (make 'padded-left-aligned))
  ((balloon (icon "tm_cell_center.xpm") "center")
   (make 'padded-center))
  ((balloon (icon "tm_cell_right.xpm") "right aligned")
   (make 'padded-right-aligned)))

(menu-bind text-toolbar-math-icons
  (=> (balloon (icon "tm_fraction.xpm") "Insert a fraction")
      ("Standard fraction" (make-fraction))
      ("Small inline fraction" (make 'tfrac))
      ("Large displayed fraction" (make 'dfrac))
      ("Slashed fraction" (make 'frac*))
      ("Continued fraction" (make 'cfrac)))
  (=> (balloon (icon "tm_root.xpm") "Insert a root")
      ("Square root" (make-sqrt))
      ("Multiple root" (make-var-sqrt)))
  (=> (balloon (icon "tm_subsup.xpm") "Insert a script")
      ("Subscript" (make-script #f #t))
      ("Superscript" (make-script #t #t))
      ("Left subscript" (make-script #f #f))
      ("Left superscript" (make-script #t #f))
      ("Subscript below" (make-below))
      ("Superscript above" (make-above)))
  (=> (balloon (icon "tm_wide.xpm") "Insert an accent")
      (tile 6
            ((balloon (icon "tm_tilda.xpm") "keyboard equivalent:")
             (make-wide "~"))
            ((balloon (icon "tm_bar.xpm") "keyboard equivalent:")
             (make-wide "<bar>"))
            ((balloon (icon "tm_vect.xpm") "keyboard equivalent:")
             (make-wide "<vect>"))
            ((balloon (icon "tm_hat.xpm") "keyboard equivalent:")
             (make-wide "^"))
            ((balloon (icon "tm_check.xpm") "keyboard equivalent:")
             (make-wide "<check>"))
            ((balloon (icon "tm_invbreve.xpm") "keyboard equivalent:")
             (make-wide "<invbreve>"))
            ((balloon (icon "tm_breve.xpm") "keyboard equivalent:")
             (make-wide "<breve>"))
            ((balloon (icon "tm_dot.xpm") "keyboard equivalent:")
             (make-wide "<dot>"))
            ((balloon (icon "tm_ddot.xpm") "keyboard equivalent:")
             (make-wide "<ddot>"))
            ((balloon (icon "tm_acute.xpm") "keyboard equivalent:")
             (make-wide "<acute>"))
            ((balloon (icon "tm_grave.xpm") "keyboard equivalent:")
             (make-wide "<grave>"))
            ((balloon (icon "tm_overbrace.xpm") "keyboard equivalent:")
             (make-wide "<wide-overbrace>"))
            ((balloon (icon "tm_underbrace.xpm") "keyboard equivalent:")
             (make-wide-under "<wide-underbrace>"))
            ((balloon (icon "tm_underbar.xpm") "keyboard equivalent:")
             (make-wide-under "<wide-bar>"))))
  ((balloon (icon "tm_marked.xpm") "Marked text")
   (mark-text))
  (=> (balloon (icon "tm_color.xpm") "Select a foreground color")
      (link color-menu)))

(menu-bind text-toolbar-table-icons
  (=> (balloon (icon "tm_cell_border.xpm") "Change border of cell")
      (mini #f
        (group "Border")
        (link cell-alt-border-menu)
        ---
        (group "Pen width")
        (link cell-compact-pen-width-menu)
        ---
        (group "Padding")
        (link cell-padding-menu)))
  (=> (balloon (icon "tm_cell_center.xpm") "Modify cell alignment")
      (mini #f
        (group "Horizontal alignment")
        (link cell-halign-menu)
        ---
        (group "Vertical alignment")
        (link cell-valign-menu)))
  (=> (balloon (icon "tm_cell_background.xpm") "Set background color of cell")
      (mini #f
        ("None" (cell-set-background ""))
        ("Foreground" (cell-set-background "foreground"))
        ---
        (pick-background "" (cell-set-background answer))
        ---
        ("Other" (interactive cell-set-background)))))

(tm-define (table-selection-context? t)
  (or (selection-active-table?)
      (and (selection-active-any?)
           (table-markup-context? (selection-tree)))))

(menu-bind text-toolbar-icons
  (cond
   ((table-selection-context? (focus-tree))
    (link text-toolbar-table-icons))
   (else
    (if (in-text?) (link text-toolbar-text-icons))
    (if (in-math?) (link text-toolbar-math-icons)))))
