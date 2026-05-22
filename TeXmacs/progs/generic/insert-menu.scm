
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : insert-menu.scm
;; DESCRIPTION : menus for inserting new structure
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic insert-menu)
  (:use (utils edit selections)
    (generic generic-edit)
    (generic format-edit)
    (generic format-geometry-edit)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Insert links
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind insert-link-menu
  (when (not (selection-active-non-small?))
    ("Label" (make-label))
    ("Reference" (make 'reference))
    ("Page reference" (make 'pageref))
  ) ;when
  ---
  (when (not (selection-active?))
    (if (detailed-menus?)
     ("Include" (choose-file make-include "Include file" "action_include"))
    ) ;if
  ) ;when
  (when (not (selection-active-non-small?))
    ("Link to URL" (make 'slink))
    ("Hyperlink" (make 'hlink))
    (if (detailed-menus?) ("Action" (make 'action)))
  ) ;when
  (if (simple-menus?) ("Footnote" (make 'footnote)))
  (if (and (style-has? "std-dtd") (in-text?))
    ---
    (when (not (selection-active-non-small?))
      (-> "Citation"
        (if (not (style-has? "cite-author-year-dtd"))
         ("Visible" (make 'cite))
         ("Invisible" (make 'nocite))
         ("Detailed" (make 'cite-detail))
        ) ;if
        (if (style-has? "cite-author-year-dtd")
          (group "Abbreviated authors")
          ("Raw" (make 'cite-raw))
          ("Textual" (make 'cite-textual))
          ("Parenthesized" (make 'cite-parenthesized))
          ---
          (group "Full author list")
          ("Raw" (make 'cite-raw*))
          ("Textual" (make 'cite-textual*))
          ("Parenthesized" (make 'cite-parenthesized*))
          ---
          (group "Decomposed")
          ("Parenthesis" (make 'render-cite))
          ("Abbreviated authors" (make 'cite-author-link))
          ("Full author list" (make 'cite-author*-link))
          ("Year" (make 'cite-year-link))
          ("Invisible" (make 'nocite))
        ) ;if
      ) ;->
      (-> "Index entry"
       ("Main" (make 'index))
       ("Sub" (make 'subindex))
       ("Subsub" (make 'subsubindex))
       ("Complex" (make 'index-complex))
       ---
       ("Interjection" (make 'index-line))
      ) ;->
      (-> "Glossary entry"
       ("Regular" (make 'glossary))
       ("Explained" (make 'glossary-explain))
       ("Duplicate" (make 'glossary-dup))
       ---
       ("Interjection" (make 'glossary-line))
      ) ;->
    ) ;when
    (-> "Alternate"
     ("Bibliography" (make-alternate "Name of bibliography" "bib" 'with-bib))
     ("Table of contents"
       (make-alternate "Name of table of contents" "toc" 'with-toc)
     ) ;
     ("Index" (make-alternate "Name of index" "idx" 'with-index))
     ("Glossary" (make-alternate "Name of glossary" "gly" 'with-glossary))
     ("List of figures"
       (make-alternate "Name of list of figures" "figure" 'with-figure-list)
     ) ;
     ("List of tables"
       (make-alternate "Name of list of tables" "table" 'with-table-list)
     ) ;
    ) ;->
    ---
    ("Reference to note" (make-note-ref))
    (-> "Text for note"
     ("Inline" (make-note-inline))
     ("Wide" (make-note-wide))
     (when (in-main-flow?)
       ("Footnote" (make-note-footnote))
     ) ;when
    ) ;->
  ) ;if
  (if (and (style-has? "calc-dtd") (not (style-has? "icourse-dtd")) (calc-ready?))
    ---
    (link calc-insert-menu)
  ) ;if
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Insert images
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind insert-image-menu
  (if (and (style-has? "env-float-dtd") (in-text?))
    (when (not (selection-active-non-small?))
      ("Small figure" (wrap-selection-small (make 'small-figure)))
      ("Big figure"
        (wrap-selection-small (insert-go-to '(big-figure "" (document "")) '(0
                                                                             0)))
      ) ;
      ---
    ) ;when
  ) ;if
  ("Link image" (choose-file make-link-image "Load image" "image"))
  ("Insert image" (choose-file make-inline-image "Load image" "image"))
  (if (or (lazy-plugin-force)
        (and (style-has? "scripts-dtd") (scripts-defined? "gnuplot"))
      ) ;or
    (-> "Plot" (link scripts-plot-menu))
  ) ;if
  ---
  ("Draw image" (make-graphics))
  (when (selection-active-small?)
    ("Draw over selection" (make-graphics-over-selection))
  ) ;when
  ("Ink here" (make-graphics-over))
  (if (in-math?) ("Commutative diagram" (make-cd)))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main Insert menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind texmacs-insert-menu
  (-> "Macro" (link insert-macro-menu))
  (if (not (in-text?)) ("Text" (make 'text)))
  (if (not (in-math?)) (-> "Mathematics" (link insert-math-menu)))
  (-> "Table" (link insert-table-menu))
  (-> "Image" (link insert-image-menu))
  (-> "Link" (link insert-link-menu))
  (if (detailed-menus?)
    (if (style-has? "std-fold-dtd") (-> "Fold" (link insert-fold-menu)))
    (if (with-developer-tool?) (-> "Animation" (link insert-animation-menu)))
  ) ;if
  (if (and (style-has? "session-dtd") (detailed-menus?) (not (in-math?)))
    (-> "Session" (link insert-session-menu))
  ) ;if
) ;menu-bind

(menu-bind insert-menu
  (if (in-text?) (link text-menu))
  (if (in-math?) (link math-menu))
  (if (not (or (in-text?) (in-math?))) (link texmacs-insert-menu))
) ;menu-bind

(tm-define (zhihu-share-current-buffer)
  (use-modules (data zhihu))
  (if (logged-in?)
    (and-with html
      (current-buffer->zhihu-html)
      (if (zhihu-share-set-clipboard-html html)
        (open-url "https://zhuanlan.zhihu.com/write")
        (set-message "HTML clipboard unavailable"
          "Zhihu share requires Qt HTML clipboard support"
        ) ;set-message
      ) ;if
    ) ;and-with
    (login)
  ) ;if
) ;tm-define

(menu-bind insert-zhihu-share-menu
  (if (not (community-stem?))
   ((balloon (icon "tm_share.svg") "Share to Zhihu") (zhihu-share-current-buffer))
  ) ;if
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main Insert icons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind texmacs-insert-icons
  /
  (if (and (not (== (get-preference "gui theme") "liii"))
        (not (== (get-preference "gui theme") "liii-night"))
        (not (== (get-preference "gui theme") "default"))
      ) ;and
    (=> (balloon (icon "tm_macro.xpm") "Insert a personal macro")
      (link insert-macro-menu)
    ) ;=>
  ) ;if
  (if (not (in-text?))
   ((balloon (icon "tm_text.xpm") "Insert plain text") (make 'text))
  ) ;if
  (if (not (in-math?))
    (=> (balloon (icon "tm_math.xpm") "Insert mathematics") (link insert-math-menu))
  ) ;if
  (=> (balloon (icon "tm_table.xpm") "Insert a table") (link insert-table-menu))
  (=> (balloon (icon "tm_image.xpm") "Insert a picture") (link insert-image-menu))
  (=> (balloon (icon "tm_link.xpm") "Insert a link") (link insert-link-menu))
  (if (detailed-menus?)
    (if (style-has? "std-fold-dtd")
      (=> (balloon (icon "tm_switch.xpm") "Switching and folding")
        (link insert-fold-menu)
      ) ;=>
    ) ;if
  ) ;if
  (if (and (style-has? "session-dtd") (detailed-menus?) (in-text?))
    (=> (balloon (icon "tm_shell.xpm") "Start an interactive session")
      (link insert-session-menu)
    ) ;=>
  ) ;if
  (if (not (community-stem?)) (link insert-zhihu-share-menu))
) ;menu-bind
