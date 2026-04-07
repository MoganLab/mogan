
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

;; 判断当前焦点位置是否存在可显示的选区工具栏上下文。
(tm-define (text-toolbar-allowed-context?)
  (not (not (text-toolbar-context (focus-tree)))))

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

;; 提取当前选区对应的语义块根节点，例如定理、命题等环境。
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

;; 提取当前选区对应的章节层级节点。
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

;; 返回当前章节节点可切换的结构变体列表。
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
            (inert ((balloon (icon "tm_section.xpm") "Structured variant")
                    (noop))))
          (assuming (> (length l) 1)
            (=> (balloon (icon "tm_section.xpm") "Structured variant")
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

;; 判断当前选区是否处于表格或单元格相关上下文中。
(tm-define (table-selection-context? t)
  (or (selection-active-table?)
      (and (selection-active-any?)
           (table-markup-context? (selection-tree)))))

;; 判断当前选区是否处于语义块上下文中。
(tm-define (semantic-block-selection-context? t)
  (not (not (semantic-block-selection-tree))))

;; 判断当前选区是否处于章节标题上下文中。
(tm-define (chatper-selection-context? t)
  (not (not (chatper-selection-tree))))

;; 合并两个模式列表，并去掉重复项。
(define (mode-list-union l1 l2)
  (if (null? l1) l2
      (with mode (car l1)
        (mode-list-union
         (cdr l1)
         (let loop ((l l2))
           (cond ((null? l) (cons mode l2))
                 ((== (car l) mode) l2)
                 (else (loop (cdr l)))))))))

;; 递归计算当前选区树实际携带的所有 mode。
(define (selection-tree-modes t mode)
  (cond ((tree-atomic? t)
         (list mode))
        ((and (tm-func? t 'with) (>= (tree-arity t) 3))
         (with n (- (tree-arity t) 1)
           (let loop ((i 0) (mode* mode))
             (if (>= i n)
                 (selection-tree-modes (tree-ref t n) mode*)
                 (with var (tree->string (tree-ref t i))
                   (with val (tree->string (tree-ref t (+ i 1)))
                     (loop (+ i 2)
                           (if (== var "mode") val mode*))))))))
        (else
         (let loop ((i 0) (modes '()))
           (if (>= i (tree-arity t))
               (if (null? modes) (list mode) modes)
               (loop (+ i 1)
                     (mode-list-union
                      modes
                      (selection-tree-modes
                       (tree-ref t i)
                       (or (tm->string (tree-child-env t i "mode" mode))
                           mode)))))))))

;; 计算当前选区内容的唯一 mode；若混合了多种 mode 则返回 #f。
(tm-define (selection-content-mode)
  (and (selection-active-any?)
       ;; `selection-path` 是选区两端的公共祖先，可能已经位于外层环境。
       ;; 这里使用选区起点恢复实际生效的 mode。
       (with mode (tree->string (get-env-tree-at "mode" (selection-get-start)))
         (with modes (selection-tree-modes (selection-tree) mode)
           (and (== (length modes) 1) (car modes))))))

;; 判断当前选区是否位于目录区域中，或直接包含整个目录节点。
(tm-define (table-of-contents-selection-context? t)
  (and (selection-active-any?)
       (or (tree-search-upwards t
                                (lambda (u) (tm-func? u 'table-of-contents 2)))
           (tm-func? (path->tree (selection-path)) 'table-of-contents 2)
           (with sel (selection-tree)
             (or (tm-func? sel 'table-of-contents 2)
                 (nnull? (tree-search sel
                                      (lambda (u)
                                        (tm-func? u 'table-of-contents 2)))))))))

;; 判断当前选区是否包含图片节点。
(tm-define (image-selection-context? t)
  (and (selection-active-any?)
       (or (not (not (any-image-context?)))
           (nnull? (tree-search (selection-tree)
                                (lambda (u) (tree-is? u 'image)))))))

;; 判断当前选区是否为纯文本上下文。
(tm-define (text-selection-context? t)
  (== (selection-content-mode) "text"))

;; 判断当前选区是否为纯数学上下文。
(tm-define (math-selection-context? t)
  (== (selection-content-mode) "math"))

;; 根据当前选区推导应显示的工具栏类别。
(tm-define (text-toolbar-context t)
  (and (selection-active-any?)
       (cond
        ((table-of-contents-selection-context? t) #f)
        ((image-selection-context? t) #f)
        ((table-selection-context? t) 'table)
        ((chatper-selection-context? t) 'chatper)
        ((semantic-block-selection-context? t) 'semantic)
        ((math-selection-context? t) 'math)
        ((text-selection-context? t) 'text)
        (else #f))))

(menu-bind text-toolbar-icons
  (with context (text-toolbar-context (focus-tree))
    (cond
     ((== context 'table)
      (link text-toolbar-table-icons))
     ((== context 'chatper)
      (link text-toolbar-chatper-icons))
     ((== context 'semantic)
      (link text-toolbar-semantic-icons))
     ((== context 'text)
      (link text-toolbar-text-icons))
     ((== context 'math)
      (link text-toolbar-math-icons)))))
