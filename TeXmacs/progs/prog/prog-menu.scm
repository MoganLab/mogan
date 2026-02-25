
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : prog-menu.scm
;; DESCRIPTION : program menus and icons
;; COPYRIGHT   : (C) 2010  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog prog-menu)
  (:use (generic format-edit)
  (generic insert-menu)
  (prog prog-edit)))

(tm-menu (focus-code-icons t)
  (mini #t
    (inert ((eval (format-get-name (get-env "prog-language"))) (noop)))))

(tm-menu (standard-focus-icons t)
  (:require (in-code?))
  (dynamic (focus-code-icons t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main Format menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind full-prog-format-menu
  (group "Font")
  (link prog-font-menu)
  (if (simple-menus?)
      (-> "Color" (link color-menu)))
  (if (detailed-menus?)
      ---
      (group "Text")
      (link textual-properties-menu))
  ---
  (group "Paragraph")
  (link paragraph-menu)
  ---
  (when (in-main-flow?)
    (group "Page")
    (link page-menu)))

(menu-bind compressed-prog-format-menu
  ("Font" (interactive open-font-selector))
  ("Paragraph" (open-paragraph-format))
  (when (in-main-flow?)
    ("Page" (open-page-format)))
  (when (inside? 'table)
    ("Cell" (open-cell-properties))
    ("Table" (open-table-properties)))
  ---
;;  (-> "Whitespace" (link space-menu))
  (-> "Indentation" (link indentation-menu))
  (-> "Break" (link break-menu))
  ---
  (-> "Color"
      (if (== (get-preference "experimental alpha") "on")
	  (-> "Opacity" (link opacity-menu))
	  ---)
      (link color-menu))
  (-> "Adjust" (link adjust-menu))
  (-> "Transform" (link linear-transform-menu))
  (-> "Specific" (link specific-menu))
  (-> "Special" (link format-special-menu))
  (-> "Font effects" (link text-font-effects-menu))
  (assuming (== (get-preference "bitmap effects") "on")
    (-> "Graphical effects" (link text-effects-menu))))

(menu-bind prog-format-menu
  (if (use-menus?)
      (link full-prog-format-menu))
  (if (use-popups?)
      (link compressed-prog-format-menu)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Icons for modifying text properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind prog-format-icons
  ((balloon (icon "tm_italic.xpm") "Write italic text")
   (make-with "prog-font-shape" "italic"))
  ((balloon (icon "tm_bold.xpm") "Write bold text")
   (make-with "prog-font-series" "bold"))
  ((balloon (icon "tm_sansserif.xpm") "Use a sans serif font")
   (make-with "prog-font-family" "ss"))
  (if (not (in-graphics?))
      (=> (balloon (icon "tm_color.xpm") "Select a foreground color")
    (link color-menu))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Icons in prog mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind prog-icons
  (link prog-format-icons)
  (link texmacs-insert-icons)
  (if (and (in-presentation?) (not (visible-icon-bar? 0)))
    /
    (link dynamic-icons)))

(menu-bind code-menu
  ("Algorithm" (make 'specified-algorithm))
  ("Pseudo code" (make 'render-code))
  ---
  ("Indent" (make 'indent))
  (when (not (selection-active?))
    ("Tabbed" (make 'wide-tabbed)))
  ---
  ;; 统一通过 code-popup-make 创建代码标签，确保对应语言包已自动加载。
  (-> "Inline code"
      ("Verbatim" (code-popup-make 'verbatim))
      ("C++" (code-popup-make 'cpp))
      ("Scheme" (code-popup-make 'scm))
      ("Shell" (code-popup-make 'shell))
      ("Goldfish" (code-popup-make 'goldfish-lang))
      ("Scala" (code-popup-make 'scala))
      ("Python" (code-popup-make 'python))
      ("R" (code-popup-make 'r))
      ("SQL" (code-popup-make 'sql))
      ("Bash" (code-popup-make 'bash)))
  (-> "Block of code"
      ("Verbatim" (code-popup-make 'verbatim-code))
      ("C++" (code-popup-make 'cpp-code))
      ("Scheme" (code-popup-make 'scm-code))
      ("Shell" (code-popup-make 'shell-code))
      ("Goldfish" (code-popup-make 'goldfish-code))
      ("Scala" (code-popup-make 'scala-code))
      ("Python" (code-popup-make 'python-code))
      ("R" (code-popup-make 'r-code))
      ("SQL" (code-popup-make 'sql-code))
      ("Bash" (code-popup-make 'bash-code)))
  (-> "Listing"
      ("Verbatim" (code-popup-make 'listing))
      ("C++" (code-popup-make 'cpp-listing))
      ("Scheme" (code-popup-make 'scm-listing))
      ("Shell" (code-popup-make 'shell-listing))))
