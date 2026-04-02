
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
        (generic generic-menu)
        (table table-menu)
        (text text-menu)
        (math math-menu)))

(tm-define (text-toolbar-allowed-context?)
  (and (not (in-prog?))
       (not (in-code?))
       (not (in-verbatim?))))

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

(tm-define (semantic-block-selection-tree)
  (and (selection-active-any?)
       (with t (path->tree (selection-path))
         (and (== (selection-tree) t)
              (let loop ((t t))
                (cond ((or (tree-in? t (numbered-unnumbered-append (enunciation-tag-list)))
                           (tree-in? t (render-enunciation-tag-list)))
                       t)
                      ((tm-func? t 'document 1)
                       (loop (tree-ref t 0)))
                      (else #f)))))))

(menu-bind text-toolbar-semantic-icons
  (with t (semantic-block-selection-tree)
    (when (and t (numbered-context? t))
      ((check (balloon (icon "tm_numbered.xpm") "Numbered") "v"
              (numbered-numbered? t))
       (numbered-toggle t)))
    ((check (balloon (icon "tm_cell_border.xpm") "Framed theorems") "v"
            (has-style-package? "framed-theorems"))
     (toggle-style-package "framed-theorems"))
    (when (and t (> (length (focus-variants-of t)) 1))
      (=> (balloon (icon "tm_switch.xpm") "Structured variant")
          (dynamic (focus-variant-menu t))))))

(tm-define (chatper-selection-tree . opt-t)
  (with l '(chapter section subsection subsubsection)
    (if (nnull? opt-t)
        (and (tree-in? (car opt-t) (numbered-unnumbered-append l))
             l)
        (and (selection-active-any?)
             (with t (path->tree (selection-path))
               (and (== (selection-tree) t)
                    (let loop ((t t))
                      (cond ((tree-in? t (numbered-unnumbered-append l))
                             t)
                            ((tm-func? t 'document 1)
                             (loop (tree-ref t 0)))
                            (else #f)))))))))

(tm-define (focus-variants-of t)
  (:require (chatper-selection-tree t))
  (chatper-selection-tree t))

(menu-bind text-toolbar-chatper-icons
  (with t (chatper-selection-tree)
    (when (and t (numbered-context? t))
      ((check (balloon (icon "tm_numbered.xpm") "Numbered") "v"
              (numbered-numbered? t))
       (numbered-toggle t)))
    (when t
      (mini #t
        (with l (focus-variants-of t)
          (assuming (<= (length l) 1)
            (inert ((eval `(verbatim ,(focus-tag-name (tree-label t))))
                    (noop))))
          (assuming (> (length l) 1)
            (=> (balloon (eval `(verbatim ,(focus-tag-name (tree-label t))))
                         (eval
                          (string-append "Structured variant ("
                           (string-append (translate (kbd-system-rewrite "A-S-up"))
                            (string-append "/"
                             (string-append (translate (kbd-system-rewrite "A-S-down"))
                              (string-append ")")))))))
                (dynamic (focus-variant-menu t)))))))
    (with var (and t (focus-section-title-style-var t))
      (when var
        ((check (balloon (icon "tm_cell_left.xpm") "Left aligned") "v"
                (== (safe-init-env var) "left"))
         (init-env var "left"))
        ((check (balloon (icon "tm_cell_center.xpm") "Centered") "v"
                (== (safe-init-env var) "center"))
         (init-env var "center"))))
    (with num-var (and t (section-number-style-var t))
      (when num-var
        (=> (balloon (icon "tm_focus_prefs.xpm") "number style")
            ((check "Arabic (1, 2, 3)" "v" (== (safe-init-env num-var) "arabic"))
             (init-env num-var "arabic"))
            ((check "Hanzi (一, 二, 三)" "v" (== (safe-init-env num-var) "hanzi"))
             (init-env num-var "hanzi"))
            ((check "Roman (I, II, III)" "v" (== (safe-init-env num-var) "Roman"))
             (init-env num-var "Roman"))
            ((check "roman (i, ii, iii)" "v" (== (safe-init-env num-var) "roman"))
             (init-env num-var "roman"))
            ((check "Alpha (A, B, C)" "v" (== (safe-init-env num-var) "Alpha"))
             (init-env num-var "Alpha"))
            ((check "alpha (a, b, c)" "v" (== (safe-init-env num-var) "alpha"))
             (init-env num-var "alpha"))
            ((check (verbatim "Circle (①, ②, ③)") "v"
                    (== (safe-init-env num-var) "circle"))
             (init-env num-var "circle")))))))

(tm-define (table-selection-context? t)
  (or (selection-active-table?)
      (and (selection-active-any?)
           (table-markup-context? (selection-tree)))))

(tm-define (semantic-block-selection-context? t)
  (not (not (semantic-block-selection-tree))))

(tm-define (chatper-selection-context? t)
  (not (not (chatper-selection-tree))))

(menu-bind text-toolbar-icons
  (cond
   ((table-selection-context? (focus-tree))
    (link text-toolbar-table-icons))
   ((chatper-selection-context? (focus-tree))
    (link text-toolbar-chatper-icons))
   ((semantic-block-selection-context? (focus-tree))
    (link text-toolbar-semantic-icons))
   (else
    (if (in-text?) (link text-toolbar-text-icons))
    (if (in-math?) (link text-toolbar-math-icons)))))
