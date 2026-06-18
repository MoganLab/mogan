
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : text-menu.scm
;; DESCRIPTION : menus for inserting structure in text mode
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (text text-menu)
  (:use (text text-edit)
    (text text-structure)
    (generic document-menu)
    (prog prog-menu)
    (generic format-edit)
    (generic document-style)
    (various comment-edit)
    (various comment-widgets)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Format menu in text mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind full-text-format-menu
  (group "Font")
  (link text-font-menu)
  (if (simple-menus?) (-> "Color" (link color-menu)))
  (if (detailed-menus?) --- (group "Text") (link text-properties-menu))
  ---
  (group "Paragraph")
  (link paragraph-menu)
  ---
  (when (in-main-flow?)
    (group "Page")
    (link page-menu)
  ) ;when
) ;menu-bind

(menu-bind compressed-text-format-menu
 ("Font" (interactive open-font-selector))
 ("Paragraph" (open-paragraph-format))
 (when (in-main-flow?)
   ("Page" (open-page-format))
 ) ;when
 (when (inside? 'table)
   ("Cell" (open-cell-properties))
   ("Table" (open-table-properties))
 ) ;when
 ---
 ;; (-> "Whitespace" (link space-menu))
 (-> "Indentation" (link indentation-menu))
 (-> "Break" (link break-menu))
 (when (and (selection-active-small?) (tm-atomic? (selection-tree)))
   ("Hyphenate as" (interactive hyphenate-selection-as))
 ) ;when
 ---
 (-> "Color"
   (if (== (get-preference "experimental alpha") "on")
     (-> "Opacity" (link opacity-menu))
     ---
   ) ;if
   (link color-menu)
 ) ;->
 (-> "Adjust" (link adjust-menu))
 (-> "Transform" (link linear-transform-menu))
 (-> "Specific" (link specific-menu))
 (-> "Special" (link format-special-menu))
 (-> "Font effects" (link text-font-effects-menu))
 (assuming (== (get-preference "bitmap effects") "on")
   (-> "Graphical effects" (link text-effects-menu))
 ) ;assuming
) ;menu-bind

(menu-bind text-format-menu
  (if (use-menus?) (link full-text-format-menu))
  (if (use-popups?) (link compressed-text-format-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document headers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind title-menu
  (when (not (inside? 'doc-data))
    ("Insert title" (make-doc-data))
  ) ;when
  (when (and (not (inside? 'doc-data)) (not (inside? 'abstract-data)))
    ("Abstract" (make-abstract-data))
  ) ;when
) ;menu-bind

(menu-bind letter-header-menu
  (when (not (inside? 'letter-header))
    ("Header" (make 'letter-header))
  ) ;when
  (when (inside? 'letter-header)
    ("Address" (make-header 'address))
    ("Date" (make-header 'letter-date))
    ("Today" (begin (make-header 'letter-date) (make 'date 0)))
    ("Destination" (make-header 'destination))
  ) ;when
  ---
  (when (not (inside? 'letter-header))
    ("Opening" (make 'opening))
    ("Closing" (make 'closing))
    ("Signature" (make 'signature))
  ) ;when
  ---
  ("C.C." (make 'cc))
  ("Encl." (make 'encl))
) ;menu-bind

(menu-bind exam-header-menu
 ("Class" (make-header 'class))
 ("Date" (begin (go-end-of-header-element) (make 'title-date)))
 ("Title" (make-header 'title))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind section-menu
 ("Section" (make-section 'section))
 ("Subsection" (make-section 'subsection))
 ("Subsubsection" (make-section 'subsubsection))
 ---
 ("Paragraph::section" (make-section 'paragraph))
 ("Subparagraph" (make-section 'subparagraph))
) ;menu-bind


(menu-bind chapter-menu
  (when (not (inside? 'doc-data))
    ("Insert title" (make-doc-data))
  ) ;when
  (when (and (not (inside? 'doc-data)) (not (inside? 'abstract-data)))
    ("Abstract" (make-abstract-data))
  ) ;when
  ("Chapter" (make-section 'chapter))
  ---
  ("Section" (make-section 'section))
  ("Subsection" (make-section 'subsection))
  ("Subsubsection" (make-section 'subsubsection))
  ---
  ("Paragraph::section" (make-section 'paragraph))
  ("Subparagraph" (make-section 'subparagraph))
  ---
  ("Appendix" (make-section 'appendix))
  ("Prologue::menu" (begin (make-unnamed-section 'prologue) (insert-return)))
  ("Epilogue" (begin (make-unnamed-section 'epilogue) (insert-return)))
  ("List of abbreviations" (make-unnamed-section 'list-of-abbreviations))
) ;menu-bind


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Enunciations, quotations and programs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind enunciation-menu
  (if (style-has? "env-theorem-dtd")
   ("Theorem" (make 'theorem))
   ("Proposition" (make 'proposition))
   ("Lemma" (make 'lemma))
   ("Corollary" (make 'corollary))
   ("Proof" (make 'proof))
   ---
   ("Axiom" (make 'axiom))
   ("Assumption" (make 'assumption))
   ("Definition" (make 'definition))
   ("Notation" (make 'notation))
   ("Convention" (make 'convention))
   ---
   ("Remark" (make 'remark))
   ("Note" (make 'note))
   ("Example" (make 'example))
   ("Warning" (make 'warning))
   ("Acknowledgments" (make 'acknowledgments*))
   ---
  ) ;if
  ("Question" (make 'question))
  ("Answer" (make 'answer*))
  ---
  ("Exercise" (make 'exercise))
  ("Problem" (make 'problem))
  ("Solution" (make 'solution*))
) ;menu-bind

(menu-bind prominent-menu
 ("Quote" (make 'quote-env))
 ("Quotation" (make 'quotation))
 ("Verse" (make 'verse))
 ---
 ("Indent" (make 'indent))
 ("Jump in" (make 'jump-in))
 ---
 ("Centered" (make 'padded-center))
 ("Left aligned" (make 'padded-left-aligned))
 ("Right aligned" (make 'padded-right-aligned))
 (with s
   (get-env "par-par-sep")
   (assuming (and (not (string-ends? s "fns")) (not (string-starts? s "0fn")))
     ---
     ("Compact vertical space" (make 'compact))
     ("Compressed vertical space" (make 'compressed))
     ("Amplified vertical space" (make 'amplified))
   ) ;assuming
 ) ;with
 ---
 ("Padded block" (make 'padded))
 ("Overlined block" (make 'overlined))
 ("Underlined block" (make 'underlined))
 ("Lines around block" (make 'bothlined))
 ("Framed block" (make 'framed))
 ("Ornamented block" (make 'ornamented))
 ---
 (-> "Material"
  ("Manila paper" (make* 'manila-paper "ornaments"))
  ("Rough paper" (make* 'rough-paper "ornaments"))
  ("Ridged paper" (make* 'ridged-paper "ornaments"))
  ("Pine" (make* 'pine "ornaments"))
  ("Granite" (make* 'granite "ornaments"))
  ("Metal" (make* 'metal "ornaments"))
 ) ;->
 (-> "Art frame"
  ("Carved wood" (make* 'carved-wood-frame "std-frame"))
  ("Decorated wood" (make* 'decorated-wood-frame "std-frame"))
  ("Black floral I" (make* 'black-floral1-frame "std-frame"))
  ("Black floral II" (make* 'black-floral2-frame "std-frame"))
 ) ;->
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Notes and floating objects
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind note-menu
  (when (not (or (inside? 'float) (inside? 'footnote)))
    (when (in-main-flow?)
      ("Footnote" (make 'footnote))
      (when (not (selection-active-non-small?))
        ("Marginal note" (make-marginal-note))
      ) ;when
    ) ;when
    ---
    (when (!= (url->string (current-window)) "{}")
      ("Folded comment" (make-folded-comment "comment"))
    ) ;when
    ("Unfolded comment" (make-unfolded-comment "comment"))
    ---
    (when (in-main-flow?)
      ("Floating object" (make-insertion "float"))
      (when (not (selection-active-non-small?))
        ("Floating phantom" (insert '(phantom-float "float" "hf")))
      ) ;when
      (when (not (selection-active-non-small?))
        ("Floating figure"
          (wrap-selection-small (make-insertion "float")
            (insert-go-to '(big-figure "" (document "")) '(0 0))
          ) ;wrap-selection-small
        ) ;
        ("Floating table"
          (wrap-selection-small (make-insertion "float")
            (insert-go-to '(big-table "" (document "")) '(0 0))
            (make 'tabular)
          ) ;wrap-selection-small
        ) ;
      ) ;when
      ("Floating algorithm"
        (wrap-selection-any (make-insertion "float") (make 'algorithm))
      ) ;
    ) ;when
  ) ;when
) ;menu-bind

(menu-bind position-marginal-note-menu
  (group "Horizontal position")
  ("Automatic" (set-marginal-note-hpos "normal"))
  ("Left" (set-marginal-note-hpos "left"))
  ("Right" (set-marginal-note-hpos "right"))
  ("Left on even pages" (set-marginal-note-hpos "even-left"))
  ("Right on even pages" (set-marginal-note-hpos "even-right"))
  ---
  (group "Vertical alignment")
  ("Top" (set-marginal-note-valign "t"))
  ("Center" (set-marginal-note-valign "c"))
  ("Bottom" (set-marginal-note-valign "b"))
) ;menu-bind

(tm-define (marginal-note-context? t) (tree-is? t 'marginal-note))

(tm-menu (focus-float-menu t)
  (:require (marginal-note-context? t))
  ---
  (link position-marginal-note-menu)
  ---
) ;tm-menu

(menu-bind float-menu
 ("Top" (toggle-insertion-positioning "t"))
 ("Here" (toggle-insertion-positioning "h"))
 ("Bottom" (toggle-insertion-positioning "b"))
 ("Other pages" (toggle-insertion-positioning-not "f"))
 (if (tree-innermost float-context? #t)
   ---
   ("Make non floating" (turn-non-floating (tree-innermost float-context? #t)))
 ) ;if
) ;menu-bind

(tm-menu (focus-float-menu t)
  (:require (rich-float-context? t))
  (if (in-multicol-style?) ("Wide float" (float-toggle-wide (focus-tree))))
  (-> "Allowed positions" (link float-menu))
  (if (cursor-at-anchor?) ("Go to float" (go-to-float)))
  (if (not (cursor-at-anchor?)) ("Go to anchor" (go-to-anchor)))
) ;tm-menu

(tm-menu (focus-float-menu t)
  (:require (phantom-float-context? t))
  (-> "Allowed positions" (link float-menu))
) ;tm-menu

(tm-menu (focus-float-menu t)
  (:require (floatable-context? t))
  (if (in-multicol-style?) ("Span over all columns" (floatable-toggle-wide t)))
  ("Make floating" (turn-floating (tree-innermost floatable-context?)))
) ;tm-menu

(tm-menu (focus-float-menu t)
  (:require (tree-is? t 'footnote))
  (if (in-multicol-style?) ("Wide footnote" (float-toggle-wide (focus-tree))))
  (if (cursor-at-anchor?) ("Go to footnote" (go-to-float)))
  (if (not (cursor-at-anchor?)) ("Go to anchor" (go-to-anchor)))
) ;tm-menu

(menu-bind position-balloon-menu
  (group "Horizontal alignment")
  ("Outer left" (set-balloon-halign "Left"))
  ("Inner left" (set-balloon-halign "left"))
  ("Center" (set-balloon-halign "center"))
  ("Inner right" (set-balloon-halign "right"))
  ("Outer right" (set-balloon-halign "Right"))
  ---
  (group "Vertical alignment")
  ("Outer bottom" (set-balloon-valign "Bottom"))
  ("Inner bottom" (set-balloon-valign "bottom"))
  ("Center" (set-balloon-valign "center"))
  ("Inner top" (set-balloon-valign "top"))
  ("Outer top" (set-balloon-valign "Top"))
) ;menu-bind

(tm-menu (focus-float-menu t)
  (:require (balloon-context? t))
  ---
  (link position-balloon-menu)
  ---
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tags
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind content-tag-menu
 ("Strong" (make 'strong))
 ("Emphasize" (make 'em))
 ("Definition" (make 'dfn))
 ("Sample" (make 'samp))
 ---
 ("Name" (make 'name))
 ("Person" (make 'person))
 ("Cite" (make 'cite*))
 (when (not (selection-active-non-small?))
   ("Abbreviation" (make 'abbr))
 ) ;when
 ("Acronym" (make 'acronym))
 ---
 ("Verbatim" (make 'verbatim))
 ("Keyboard" (make 'kbd))
 ("Code" (make 'code*))
 ("Variable" (make 'var))
 ---
 ("Deleted" (make 'deleted))
 ("Fill out" (make 'fill-out))
 ("Marked" (mark-text))
) ;menu-bind

(menu-bind presentation-tag-menu
  (if (style-has? "std-markup-dtd")
   ("Underline" (make 'underline))
   ("Overline" (make 'overline))
   ("Strike through" (make 'strike-through))
   ---
  ) ;if
  ("Subscript" (make-script #f #t))
  ("Superscript" (make-script #t #t))
  (if (and (style-has? "std-markup-dtd")
        (== (get-preference "experimental alpha") "on")
      ) ;and
    ---
    ("Pastel opacity" (make 'pastel))
    ("Greyed opacity" (make 'greyed))
    ("Light opacity" (make 'light))
  ) ;if
) ;menu-bind

(menu-bind size-tag-menu
 ("Really tiny" (make 'really-tiny))
 ("Tiny" (make 'tiny))
 ("Very small" (make 'very-small))
 ("Small" (make 'small))
 ("Normal" (make 'normal-size))
 ("Large" (make 'large))
 ("Very large" (make 'very-large))
 ("Huge" (make 'huge))
 ("Really huge" (make 'really-huge))
) ;menu-bind

(menu-bind text-language-menu
  (for (lan supported-languages)
    (when (supported-language? lan)
      ((check (eval (language-to-language-name lan)) "v" (test-env? "language" lan))
       (make (string->symbol lan))
      ) ;
    ) ;when
  ) ;for
) ;menu-bind

(tm-menu (local-supported-scripts-menu)
  (let* ((dummy (lazy-plugin-force)) (l (scripts-list)))
    (for (name l) ((eval (scripts-name name)) (make-with "prog-scripts" name)))
  ) ;let*
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Enumerations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind itemize-menu
 ((shortcut "Default" "- space") (make-tmlist 'itemize))
 ---
 ("Bullets" (make-tmlist 'itemize-dot))
 ("Dashes" (make-tmlist 'itemize-minus))
 ("Arrows" (make-tmlist 'itemize-arrow))
) ;menu-bind

(menu-bind enumerate-menu
 ((shortcut "Default" "1 . space") (make-tmlist 'enumerate))
 ---
 ("1, 2, 3, ..." (make-tmlist 'enumerate-numeric))
 ("1), 2), 3), ..." (make-tmlist 'enumerate-numeric-bracket))
 ("(1), (2), (3), ..." (make-tmlist 'enumerate-numeric-paren))
 ("i, ii, iii, ..." (make-tmlist 'enumerate-roman))
 ("i), ii), iii), ..." (make-tmlist 'enumerate-roman-bracket))
 ("(i), (ii), (iii), ..." (make-tmlist 'enumerate-roman-paren))
 ("I, II, III, ..." (make-tmlist 'enumerate-Roman))
 ("a, b, c, ..." (make-tmlist 'enumerate-alpha))
 ("a), b), c), ..." (make-tmlist 'enumerate-alpha-bracket))
 ("(a), (b), (c), ..." (make-tmlist 'enumerate-alpha-full-paren))
 ("A, B, C, ..." (make-tmlist 'enumerate-Alpha))
 ("①, ②, ③, ..." (make-tmlist 'enumerate-circle))
 ("一, 二, 三, ..." (make-tmlist 'enumerate-hanzi))
) ;menu-bind

(menu-bind description-menu
 ("Default" (make-tmlist 'description))
 ---
 ("Compact" (make-tmlist 'description-compact))
 ("Aligned" (make-tmlist 'description-aligned))
 ("Dashes" (make-tmlist 'description-dash))
 ("Long" (make-tmlist 'description-long))
 ("Paragraphs" (make-tmlist 'description-paragraphs))
) ;menu-bind

(menu-bind list-menu
 ((shortcut "Itemize" "- space") (make-tmlist 'itemize))
 ---
 ("Bullets" (make-tmlist 'itemize-dot))
 ("Dashes" (make-tmlist 'itemize-minus))
 ("Arrows" (make-tmlist 'itemize-arrow))
 ---
 ((shortcut "Enumerate" "1 . space") (make-tmlist 'enumerate))
 ---
 ("1, 2, 3, ..." (make-tmlist 'enumerate-numeric))
 ("1), 2), 3), ..." (make-tmlist 'enumerate-numeric-bracket))
 ("(1), (2), (3), ..." (make-tmlist 'enumerate-numeric-paren))
 ("i, ii, iii, ..." (make-tmlist 'enumerate-roman))
 ("i), ii), iii), ..." (make-tmlist 'enumerate-roman-bracket))
 ("(i), (ii), (iii), ..." (make-tmlist 'enumerate-roman-paren))
 ("I, II, III, ..." (make-tmlist 'enumerate-Roman))
 ("a, b, c, ..." (make-tmlist 'enumerate-alpha))
 ("a), b), c), ..." (make-tmlist 'enumerate-alpha-bracket))
 ("(a), (b), (c), ..." (make-tmlist 'enumerate-alpha-full-paren))
 ("A, B, C, ..." (make-tmlist 'enumerate-Alpha))
 ("①, ②, ③, ..." (make-tmlist 'enumerate-circle))
 ("一, 二, 三, ..." (make-tmlist 'enumerate-hanzi))
 ---
 ("Description" (make-tmlist 'description))
 ---
 ("Compact" (make-tmlist 'description-compact))
 ("Aligned" (make-tmlist 'description-aligned))
 ("Dashes" (make-tmlist 'description-dash))
 ("Long" (make-tmlist 'description-long))
 ("Paragraphs" (make-tmlist 'description-paragraphs))
 ---
 ("Numbered" (make 'numbered))
) ;menu-bind

(menu-bind lists-menu
  (-> "Itemize" (link itemize-menu))
  (-> "Enumerate" (link enumerate-menu))
  (-> "Description" (link description-menu))
  ("Numbered" (make 'numbered))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatically generated content
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind automatic-menu
 ("Table of contents"
   (begin
     (make-aux "table-of-contents" "toc-prefix" "toc")
     (update-document "all")
   ) ;begin
 ) ;
 ("Bibliography" (open-bibliography-inserter))
 ("Index"
   (begin
     (make-aux "the-index" "index-prefix" "idx")
     (update-document "all")
   ) ;begin
 ) ;
 ("Glossary"
   (begin
     (make-aux "the-glossary" "glossary-prefix" "gly")
     (update-document "all")
   ) ;begin
 ) ;
 ;; ("List of figures" (make-aux* "the-glossary*" "figure-list-prefix" "figure" "List of figures"))
 ;; ("List of tables" (make-aux* "the-glossary*" "table-list-prefix" "table" "List of tables"))
 ("List of figures"
   (begin
     (make-aux "list-of-figures" "figure-list-prefix" "figure")
     (update-document "all")
   ) ;begin
 ) ;
 ("List of tables"
   (begin
     (make-aux "list-of-tables" "table-list-prefix" "table")
     (update-document "all")
   ) ;begin
 ) ;
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Text menus for inserting block content
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(menu-bind text-block-menu
  (if (style-has? "header-letter-dtd") (-> "Header" (link letter-header-menu)))
  (if (style-has? "header-exam-dtd") (-> "Header" (link exam-header-menu)))
  (-> "Chapter::menu" (link chapter-menu))
  (if (or (style-has? "env-theorem-dtd") (style-has? "header-exam-dtd"))
    (-> "Enunciation" (link enunciation-menu))
  ) ;if
  (if (style-has? "std-markup-dtd") (-> "Prominent" (link prominent-menu)))
  (if (style-has? "std-markup-dtd") (-> "Program" (link code-menu)))
  (if (and (style-has? "env-float-dtd") (detailed-menus?))
    (-> "Note" (link note-menu))
  ) ;if
  (if (style-has? "section-base-dtd") (-> "Automatic" (link automatic-menu)))
  (if (style-has? "std-list-dtd") --- (link lists-menu))
) ;menu-bind

(menu-bind text-extra-menu)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Text menus for inserting inline content
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind text-inline-menu
  (if (style-has? "std-markup-dtd")
    (-> "Content tag" (link content-tag-menu))
    (-> "Size tag" (link size-tag-menu))
  ) ;if
  (-> "Presentation tag" (link presentation-tag-menu))
  (if (style-has? "std-markup-dtd") (-> "Language" (link text-language-menu)))
  (-> "Scripts" (link local-supported-scripts-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style dependent menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind text-menu
  (link text-block-menu)
  (link text-extra-menu)
  ---
  (link text-inline-menu)
  ---
  (link texmacs-insert-menu)
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Icons for inserting block markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (chapter-style?)
  (and (or (style-has? "book-style")
         (style-has? "generic-style")
         (style-has? "article-style")
       ) ;or
    (not (style-has? "beamer-style"))
    (not (style-has? "poster-style"))
    (not (style-has? "letter-style"))
    (not (style-has? "seminar-style"))
    (not (style-has? "browser-style"))
  ) ;and
) ;define


(menu-bind text-block-icons
  (if (and (style-has? "section-base-dtd")
        (not (style-has? "header-exam-dtd"))
        (not (in-poster?))
      ) ;and
    (=> (balloon (icon "tm_section.xpm") "Start a new section") (link chapter-menu))
  ) ;if
  (if (in-poster?)
    (=> (balloon (icon "tm_block.xpm") "Insert a section block")
      (link poster-block-menu)
    ) ;=>
  ) ;if
  (if (or (style-has? "env-theorem-dtd") (style-has? "header-exam-dtd"))
    (=> (balloon (icon "tm_theorem.xpm") "Insert an enunciation")
      (link enunciation-menu)
    ) ;=>
  ) ;if
  (if (and (style-has? "std-markup-dtd") (not (in-poster?)))
    (=> (balloon (icon "tm_prominent.xpm") "Insert a prominent piece of text")
      (link prominent-menu)
    ) ;=>
  ) ;if
  (if (and (style-has? "std-markup-dtd") (in-poster?))
    (=> (balloon (icon "tm_prominent.xpm") "Insert a prominent piece of text")
      (link prominent-menu)
    ) ;=>
  ) ;if
  (if (style-has? "std-markup-dtd")
    (=> (balloon (icon "tm_program.xpm") "Insert a computer program")
      (link code-menu)
    ) ;=>
  ) ;if
  (if (style-has? "std-list-dtd")
    (=> (balloon (icon "tm_list.xpm") "Insert a list (- Space, 1. Space)")
      (link list-menu)
    ) ;=>
  ) ;if
  (if (and (style-has? "env-float-dtd") (detailed-menus?))
    ;; ((balloon (icon "tm_footnote.xpm") "Insert a footnote") ())
    ;; ((balloon (icon "tm_margin.xpm") "Insert a marginal note") ())
    ;; ((balloon (icon "tm_floating.xpm") "Insert a floating object") ())
    ;; ((balloon (icon "tm_multicol.xpm") "Start multicolumn context") ())
    (=> (balloon (icon "tm_pageins.xpm") "Insert a note or a floating object")
      (link note-menu)
    ) ;=>
  ) ;if
  (if (style-has? "section-base-dtd")
    (=> (balloon (icon "tm_index.xpm") "Insert automatically generated content")
      (link automatic-menu)
    ) ;=>
  ) ;if
) ;menu-bind

(menu-bind text-extra-icons)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Icons for modifying text properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind text-format-icons
  (if (not (style-has? "std-markup-dtd"))
    (if (and (not (== (get-preference "gui theme") "liii"))
          (not (== (get-preference "gui theme") "liii-night"))
          (not (== (get-preference "gui theme") "default"))
        ) ;and
      (=> (balloon (icon "tm_parstyle.xpm") "Set paragraph mode")
       ((balloon (icon "tm_align_left.xpm") "Align text to the left")
        (make-line-with "par-mode" "left")
       ) ;
       ((balloon (icon "tm_align_center.xpm") "Center text")
        (make-line-with "par-mode" "center")
       ) ;
       ((balloon (icon "tm_align_right.xpm") "Align text to the right")
        (make-line-with "par-mode" "right")
       ) ;
       ((balloon (icon "tm_align_fill.xpm") "Justify text")
        (make-line-with "par-mode" "justify")
       ) ;
      ) ;=>
      (=> (balloon (icon "tm_parindent.xpm") "Set paragraph margins")
       ("Left margin" (make-interactive-line-with "par-left"))
       ("Right margin" (make-interactive-line-with "par-right"))
       ("First indentation" (make-interactive-line-with "par-first"))
      ) ;=>
      /
    ) ;if
  ) ;if
  (if (and (style-has? "std-markup-dtd") (not (in-source?)))
    ;; ((balloon
    ;; (text (roman rm bold right 12 600) "S")
    ;; "Write bold text")
    ;; (make-with "font-series" "bold"))
    ((check (balloon (icon "tm_bold.xpm") "Write bold text") "v" (inside-bold?))
     (toggle-bold)
    ) ;
    ((check (balloon (icon "tm_italic.xpm") "Write italic text")
       "v"
       (inside-italic?)
     ) ;check
     (toggle-italic)
    ) ;
    ((check (balloon (icon "tm_underline.xpm") "Write underline")
       "v"
       (inside-underline?)
     ) ;check
     (make 'underline)
    ) ;
    ((check (balloon (icon "tm_strikethrough.xpm") "Write strike through")
       "v"
       (inside-strike-through?)
     ) ;check
     (make 'strike-through)
    ) ;
    ((balloon (icon "tm_marked.svg") "Marked text") (mark-text))
  ) ;if
  (if (or (not (style-has? "std-markup-dtd")) (in-source?))
   ((balloon (icon "tm_italic.xpm") "Write italic text") (toggle-italic))
   ((balloon (icon "tm_bold.xpm") "Write bold text") (toggle-bold))
  ) ;if
  (=> (balloon (icon "tm_color.xpm") "Select a foreground color")
    (link color-menu)
  ) ;=>
) ;menu-bind

(menu-bind text-inline-icons (link text-format-icons))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Icons for text mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind text-icons
  ;; ("Goedenmiddag" (display* "Hi there\n"))
  ;; (mini #t (input (display* answer "\n") "string" '("Hello" "Bonjour") "0.5w"))
  (link text-block-icons)
  (link text-extra-icons)
  (if (style-has? "std-markup-dtd") /)
  (link text-inline-icons)
  (link texmacs-insert-icons)
  (if (and (in-presentation?) (not (visible-icon-bar? 0))) / (link dynamic-icons))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for entering title information
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-document-extra-menu t)
  (:require (document-propose-title?))
  ("Title" (make-doc-data))
) ;tm-menu

(tm-menu (focus-document-extra-icons t)
  (:require (document-propose-title?))
  (minibar ((balloon "Title" "Insert title") (make-doc-data)))
) ;tm-menu

(tm-menu (focus-document-extra-menu t)
  (:require (document-propose-abstract?))
  ("Abstract" (make-abstract-data))
) ;tm-menu

(tm-menu (focus-document-extra-icons t)
  (:require (document-propose-abstract?))
  (minibar ((balloon "Abstract" "Insert abstract") (make-abstract-data)))
) ;tm-menu

(tm-define (focus-can-move? t) (:require (doc-title-context? t)) #f)

(tm-menu (focus-title-menu)
 ("Subtitle" (make-doc-data-element 'doc-subtitle))
 ("Author" (make-doc-data-element 'doc-author))
 ("Date" (make-doc-data-element 'doc-date))
 ("Today" (begin (make-doc-data-element 'doc-date) (make 'date 0)))
 ("Miscellaneous" (make-doc-data-element 'doc-misc))
 ("Note" (make-doc-data-element 'doc-note))
) ;tm-menu

(tm-menu (focus-title-hidden-menu)
 ("Running title" (make-doc-data-element 'doc-running-title))
 ("Running author" (make-doc-data-element 'doc-running-author))
) ;tm-menu

(tm-menu (focus-title-option-menu)
 ("No clustering" (set-doc-title-clustering #f))
 ("Cluster by affiliation" (set-doc-title-clustering "cluster-by-affiliation"))
 ("Maximal clustering" (set-doc-title-clustering "cluster-all"))
) ;tm-menu

(tm-menu (focus-title-icons)
  (assuming (doc-data-has-hidden?)
   ((check (balloon (icon "tm_show_hidden.xpm") "Show hidden")
      "v"
      (doc-data-deactivated?)
    ) ;check
    (doc-data-activate-toggle)
   ) ;
  ) ;assuming
  (mini #t (inert ("Title" (noop))))
  (=> (balloon (icon "tm_add.xpm") "Add title information")
    (link focus-title-menu)
    (-> "Hidden" (link focus-title-hidden-menu))
  ) ;=>
  (=> (balloon (icon "tm_focus_prefs.xpm") "Title presentation options")
    (link focus-title-option-menu)
  ) ;=>
) ;tm-menu

(tm-menu (focus-ancestor-menu t)
  (:require (doc-title-context? t))
  (group "Title")
  (link focus-title-menu)
  ---
  (group "Hidden")
  (link focus-title-hidden-menu)
  ---
) ;tm-menu

(tm-menu (focus-ancestor-icons t)
  (:require (doc-title-context? t))
  (minibar (dynamic (focus-title-icons)))
  //
) ;tm-menu

(tm-define (focus-has-preferences? t)
  (:require (tree-in? t '(doc-note author-note)))
  #f
) ;tm-define

(tm-menu (focus-tag-edit-menu l)
  (:require (and (in? l '(author-email author-homepage author-misc))
              (or (test-doc-title-clustering? "cluster-all")
                (test-doc-title-clustering? "cluster-by-affiliation")
              ) ;or
            ) ;and
  ) ;:require
  (with l* (symbol-append l '-note) (dynamic (focus-tag-edit-menu l*)))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for entering authors
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-can-move? t) (:require (doc-author-context? t)) #f)

(tm-menu (focus-author-menu)
 ("Affiliation" (make-author-data-element 'author-affiliation))
 ("Email" (make-author-data-element 'author-email))
 ("Homepage" (make-author-data-element 'author-homepage))
 ("Miscellaneous" (make-author-data-element 'author-misc))
 ("Note" (make-author-data-element 'author-note))
) ;tm-menu

(tm-menu (focus-author-icons)
  (mini #t (inert ("Author" (noop))))
  (=> (balloon (icon "tm_add.xpm") "Add author information")
    (link focus-author-menu)
  ) ;=>
) ;tm-menu

(tm-menu (focus-ancestor-menu t)
  (:require (doc-author-context? t))
  (group "Title")
  (link focus-title-menu)
  ---
  (group "Hidden")
  (link focus-title-hidden-menu)
  ---
  (group "Author")
  (link focus-author-menu)
  ---
) ;tm-menu

(tm-menu (focus-ancestor-icons t)
  (:require (doc-author-context? t))
  (minibar (dynamic (focus-title-icons)))
  //
  (minibar (dynamic (focus-author-icons)))
  //
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for abstract data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-can-move? t) (:require (abstract-data-context? t)) #f)

(tm-menu (focus-abstract-menu)
 ("Arxiv category" (make-abstract-data-element 'abstract-arxiv))
 ("A.C.M. computing class" (make-abstract-data-element 'abstract-acm))
 ("A.M.S. subject class" (make-abstract-data-element 'abstract-msc))
 ("Physics and astronomy class" (make-abstract-data-element 'abstract-pacs))
 ("Keywords" (make-abstract-data-element 'abstract-keywords))
) ;tm-menu

(tm-define (focus-tag-name l) (:require (== l 'abstract)) "Abstract text")

(tm-menu (focus-abstract-icons)
  (mini #t (inert ("Abstract" (noop))))
  (=> (balloon (icon "tm_add.xpm") "Add abstract information")
    (link focus-abstract-menu)
  ) ;=>
) ;tm-menu

(tm-menu (focus-ancestor-menu t)
  (:require (abstract-data-context? t))
  (group "Abstract")
  (link focus-abstract-menu)
  ---
) ;tm-menu

(tm-menu (focus-ancestor-icons t)
  (:require (abstract-data-context? t))
  (minibar (dynamic (focus-abstract-icons)))
  //
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parameter-show-in-menu? l)
  (:require (and (string-ends? l "-numbered")
              (in? (string->symbol (string-drop-right l 9)) (section-tag-list))
            ) ;and
  ) ;:require
  #f
) ;tm-define

(tm-define (parameter-show-in-menu? l) (:require (== l "appendix-prefix")) #f)


(define (is-current-tree t)
  (== (tree->path t) (tree->path (focus-tree)))
) ;define

(define (is-book-top-level t)
  (in? (tree-label t) '(chapter part))
) ;define

(define (is-section-top-level t)
  (in? (tree-label t) '(section))
) ;define

(define (get-verbatim-section-title s indent?)
  (if (is-current-tree s)
    `(verbatim ,(string-append (tm/section-get-title-string s indent?)
                  "        <="))
    `(verbatim ,(tm/section-get-title-string s indent?))
  ) ;if
) ;define

(define (filter-sections l f-is-current-tree f-is-top-level)
  (define (section-list->nested l result)
    (cond ((null? l) result)
          ((null? result) (section-list->nested (cdr l) (list (list (car l)))))
          ((f-is-top-level (car l))
           (section-list->nested (cdr l) (cons (list (car l)) result))
          ) ;
          ((and (not (f-is-top-level (car l))) (f-is-top-level (car (car result))))
           (section-list->nested (cdr l)
             (cons (append (car result) (list (car l))) (cdr result))
           ) ;section-list->nested
          ) ;
          (else (section-list->nested (cdr l) (cons (list (car l)) result)))
    ) ;cond
  ) ;define

  (define (nested->filtered l result)
    (cond ((null? l) result)
          ((list-any f-is-current-tree (car l))
           (nested->filtered (cdr l) (append (car l) result))
          ) ;
          (else (nested->filtered (cdr l) (append (list-filter (car l) f-is-top-level) result))
          ) ;else
    ) ;cond
  ) ;define

  (with l2 (section-list->nested l '()) (nested->filtered l2 '()))
) ;define

(define (all-sections)
  (let* ((raw-sections (tree-search-sections (buffer-tree)))
         (main-sections (list-filter raw-sections
                          (lambda (x) (not (equal? (tree-label x) 'subparagraph)))
                        ) ;list-filter
         ) ;main-sections
         (book-main-sections (list-filter raw-sections is-book-top-level))
        ) ;
    (cond ((<= (length main-sections) 42) main-sections)
          ((== (length book-main-sections) 0)
           (filter-sections main-sections is-current-tree is-section-top-level)
          ) ;
          (else (filter-sections main-sections is-current-tree is-book-top-level))
    ) ;cond
  ) ;let*
) ;define

(tm-menu (focus-section-menu)
  (for (s (all-sections))
   ((eval (get-verbatim-section-title s #t))
    (when (and (tree->path s) (section-context? s))
      (tree-go-to s 0 :end)
    ) ;when
   ) ;
  ) ;for
) ;tm-menu

(tm-menu (focus-document-extra-menu t)
  (:require (previous-section))
  (-> "Sections" (link focus-section-menu))
) ;tm-menu

(tm-menu (focus-document-extra-icons t)
  (:require (previous-section))
  (mini #t
    (=> (eval (get-verbatim-section-title (previous-section) #f))
      (link focus-section-menu)
    ) ;=>
  ) ;mini
) ;tm-menu

(tm-menu (focus-extra-menu t)
  (:require (section-context? t))
  ---
  (-> "Go to section" (link focus-section-menu))
) ;tm-menu

(tm-menu (focus-extra-icons t)
  (:require (section-context? t))
  (mini #t
    //
    (=> (eval (get-verbatim-section-title t #f)) (link focus-section-menu))
  ) ;mini
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menu preferences for section titles
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-section-title-style-var t)
  (with l
    (tree-label t)
    (cond ((in? l '(chapter chapter*)) "chapter-title-style")
          ((in? l '(section section*)) "section-title-style")
          ((in? l '(subsection subsection*)) "subsection-title-style")
          ((in? l '(subsubsection subsubsection*)) "subsubsection-title-style")
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (focus-has-preferences? t) (:require (section-context? t)) #t)

(tm-define (section-number-style-var t)
  (with l
    (tree-label t)
    (cond ((== l 'chapter) "chapter-number-style")
          ((== l 'section) "section-number-style")
          ((== l 'subsection) "subsection-number-style")
          ((== l 'subsubsection) "subsubsection-number-style")
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (section-sep-var t)
  (with l
    (tree-label t)
    (cond ((== l 'chapter) "chapter-sep")
          ((== l 'section) "section-sep")
          ((== l 'subsection) "subsection-sep")
          ((== l 'subsubsection) "subsubsection-sep")
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (section-prefix-sep-var t)
  (with l
    (tree-label t)
    (cond ((== l 'section) "section-prefix-sep")
          ((== l 'subsection) "subsection-prefix-sep")
          ((== l 'subsubsection) "subsubsection-prefix-sep")
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (section-display-numbers-var t)
  (with l
    (tree-label t)
    (cond ((== l 'chapter) "chapter-display-numbers")
          ((== l 'section) "section-display-numbers")
          ((== l 'subsection) "subsection-display-numbers")
          ((== l 'subsubsection) "subsubsection-display-numbers")
          ((== l 'paragraph) "paragraph-display-numbers")
          ((== l 'subparagraph) "subparagraph-display-numbers")
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (section-display-label t)
  (with l
    (tree-label t)
    (cond ((== l 'chapter) "Global hide chapter numbers")
          ((== l 'section) "Global hide section numbers")
          ((== l 'subsection) "Global hide subsection numbers")
          ((== l 'subsubsection) "Global hide subsubsection numbers")
          ((== l 'paragraph) "Global hide paragraph numbers")
          ((== l 'subparagraph) "Global hide subparagraph numbers")
          (else "Global hide section numbers")
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (section-numbering-label t)
  (with l
    (tree-label t)
    (cond ((== l 'chapter) "Chapter numbering")
          ((== l 'section) "Section numbering")
          ((== l 'subsection) "Subsection numbering")
          ((== l 'subsubsection) "Subsubsection numbering")
          ((== l 'paragraph) "Paragraph numbering")
          ((== l 'subparagraph) "Subparagraph numbering")
          (else "Section numbering")
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (safe-init-env var)
  (if (or (string? var) (symbol? var)) (get-init-env var) #f)
) ;tm-define

(menu-bind section-number-style-menu
  (with num-var
    (section-number-style-var (focus-tree))
    (when num-var
      ((check "Arabic (1, 2, 3)" "v" (== (safe-init-env num-var) "arabic"))
       (init-env num-var "arabic")
      ) ;
      ((check "Hanzi (一, 二, 三)" "v" (== (safe-init-env num-var) "hanzi"))
       (init-env num-var "hanzi")
      ) ;
      ((check "Roman (I, II, III)" "v" (== (safe-init-env num-var) "Roman"))
       (init-env num-var "Roman")
      ) ;
      ((check "roman (i, ii, iii)" "v" (== (safe-init-env num-var) "roman"))
       (init-env num-var "roman")
      ) ;
      ((check "Alpha (A, B, C)" "v" (== (safe-init-env num-var) "Alpha"))
       (init-env num-var "Alpha")
      ) ;
      ((check "alpha (a, b, c)" "v" (== (safe-init-env num-var) "alpha"))
       (init-env num-var "alpha")
      ) ;
      ((check (verbatim "Circle (①, ②, ③)")
         "v"
         (== (safe-init-env num-var) "circle")
       ) ;check
       (init-env num-var "circle")
      ) ;
    ) ;when
  ) ;with
) ;menu-bind

(menu-bind section-sep-menu
  (with sep-var
    (section-sep-var (focus-tree))
    (when sep-var
      ((check "." "v" (== (safe-init-env sep-var) ".")) (init-env sep-var "."))
      ((check "、" "v" (== (safe-init-env sep-var) "<#3001>"))
       (init-env sep-var "<#3001>")
      ) ;
      ((check "-" "v" (== (safe-init-env sep-var) "-")) (init-env sep-var "-"))
      ((check "space" "v" (== (safe-init-env sep-var) " ")) (init-env sep-var " "))
    ) ;when
  ) ;with
) ;menu-bind

(menu-bind section-prefix-sep-menu
  (with prefix-sep-var
    (section-prefix-sep-var (focus-tree))
    (when prefix-sep-var
      ((check "." "v" (== (safe-init-env prefix-sep-var) "."))
       (init-env prefix-sep-var ".")
      ) ;
      ((check "、" "v" (== (safe-init-env prefix-sep-var) "<#3001>"))
       (init-env prefix-sep-var "<#3001>")
      ) ;
      ((check "-" "v" (== (safe-init-env prefix-sep-var) "-"))
       (init-env prefix-sep-var "-")
      ) ;
      ((check "space" "v" (== (safe-init-env prefix-sep-var) " "))
       (init-env prefix-sep-var " ")
      ) ;
    ) ;when
  ) ;with
) ;menu-bind

(tm-menu (focus-preferences-menu t)
  (:require (section-context? t))
  (with var
    (focus-section-title-style-var t)
    (if var
      (group "Title style")
      ((check "Centered" "v" (== (safe-init-env var) "center"))
       (init-env var "center")
      ) ;
      ((check "Left aligned" "v" (== (safe-init-env var) "left"))
       (init-env var "left")
      ) ;
      ---
    ) ;if
  ) ;with
  (with num-var
    (section-number-style-var t)
    (if num-var (-> "Number style" (link section-number-style-menu)) ---)
  ) ;with
  (with display-num-var
    (section-display-numbers-var t)
    (if display-num-var
      (group (eval (section-numbering-label t)))
      ((check (eval (section-display-label t))
         "v"
         (== (get-init-env display-num-var) "false")
       ) ;check
       (init-env display-num-var
         (if (== (get-init-env display-num-var) "true") "false" "true")
       ) ;init-env
      ) ;
      ---
    ) ;if
  ) ;with
  (with prefix-num-var
    (section-number-style-var t)
    (if prefix-num-var
      (group "Section prefix")
      ((check "Prepend chapter number prefix for section numbers"
         "v"
         (== (get-init-env "sectional-short-style") "false")
       ) ;check
       (init-env "sectional-short-style"
         (if (== (get-init-env "sectional-short-style") "true") "false" "true")
       ) ;init-env
      ) ;
      ---
    ) ;if
  ) ;with
  (with sep-var
    (section-sep-var t)
    (if sep-var (-> "Number-title separator" (link section-sep-menu)) ---)
  ) ;with
  (with prefix-sep-var
    (section-prefix-sep-var t)
    (if prefix-sep-var
      (-> "Sub-level separator" (link section-prefix-sep-menu))
      ---
    ) ;if
  ) ;with
  (dynamic (focus-tag-edit-menu (tree-label t)))
) ;tm-menu

(tm-menu (focus-preferences-menu t)
  (:require (figure-context? t))
  (with l
    (tree-label t)
    (group "Caption separator")
    ((check "." "v" (== (get-init-env "figure-sep") ". "))
     (init-env "figure-sep" ". ")
    ) ;
    ((check (verbatim "、") "v" (== (get-init-env "figure-sep") "<#3001>"))
     (init-env "figure-sep" "<#3001>")
    ) ;
    ((check "space" "v" (== (get-init-env "figure-sep") " "))
     (init-env "figure-sep" " ")
    ) ;
    ---
  ) ;with
  (former t)
) ;tm-menu

(tm-menu (focus-preferences-menu t)
  (:require (table-context? t))
  (with l
    (tree-label t)
    (group "Caption separator")
    ((check "." "v" (== (get-init-env "table-sep") ". "))
     (init-env "table-sep" ". ")
    ) ;
    ((check (verbatim "、") "v" (== (get-init-env "table-sep") "<#3001>"))
     (init-env "table-sep" "<#3001>")
    ) ;
    ((check "space" "v" (== (get-init-env "table-sep") " "))
     (init-env "table-sep" " ")
    ) ;
    ---
  ) ;with
  (former t)
) ;tm-menu


(tm-define (child-proposals t i)
  (:require (and (tree-in? t '(bibliography bibliography*)) (<= i 1)))
  (if (== i 0)
    (list (list 'verbatim "bib") :other)
    (rcons (map (lambda (s) (list 'verbatim s)) (bib-standard-styles)) :other)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menu for lists
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (standard-options l)
  (:require (in? l (list-tag-list)))
  (list "compact-list" "triangle-list" "prefix-enumerations")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menu for theorems and proofs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (standard-options l)
  (:require (or (in? l (enunciation-tag-list))
              (in? l (render-enunciation-tag-list))
              (in? l '(proof render-proof))
            ) ;or
  ) ;:require
  (append (list "number-europe"
            ;; "number-us"
            "number-long-article"
            "framed-theorems"
            "hanging-theorems"
          ) ;list
    (if (style-has? "base-deco-dtd") (list "shadowed-frames") (list))
  ) ;append
) ;tm-define

(tm-menu (focus-extra-menu t)
  (:require (dueto-supporting-context? t))
  ---
  (when (not (dueto-added? t))
    ("Due to" (dueto-add t))
  ) ;when
) ;tm-menu

(tm-menu (focus-extra-icons t)
  (:require (dueto-supporting-context? t))
  //
  (when (not (dueto-added? t))
    (mini #t ("Due to" (dueto-add t)))
  ) ;when
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for algorithms
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-tag-name l)
  (:require (in? l (algorithm-tag-list)))
  (with r
    (algorithm-root l)
    (with s (upcase-first (tree-name (tree r))) (string-replace s "-" " "))
  ) ;with
) ;tm-define

(tm-menu (focus-toggle-menu t)
  (:require (algorithm-context? t))
  (when (not (algorithm-named? (focus-tree)))
    ((check "Numbered" "v" (algorithm-numbered? (focus-tree)))
     (algorithm-toggle-number (focus-tree))
    ) ;
  ) ;when
  ((check "Named" "v" (algorithm-named? (focus-tree))) (algorithm-toggle-name t))
  ((check "Specified" "v" (algorithm-specified? (focus-tree)))
   (algorithm-toggle-specification t)
  ) ;
) ;tm-menu

(tm-menu (focus-toggle-icons t)
  (:require (algorithm-context? t))
  (when (not (algorithm-named? (focus-tree)))
    ((check (balloon (icon "tm_numbered.xpm") "Toggle numbering")
       "v"
       (algorithm-numbered? (focus-tree))
     ) ;check
     (algorithm-toggle-number (focus-tree))
    ) ;
  ) ;when
  ((check (balloon (icon "tm_small_textual.xpm") "Toggle name")
     "v"
     (algorithm-named? (focus-tree))
   ) ;check
   (algorithm-toggle-name t)
  ) ;
) ;tm-menu

(tm-define (standard-options l)
  (:require (in? l (algorithm-tag-list)))
  (list "modern-program" "centered-program" "framed-program")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for floating objects
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-float-icons t)
  (:require (marginal-note-context? t))
  (=> (balloon (icon "tm_position_float.xpm") "Position of marginal note")
    (link position-marginal-note-menu)
  ) ;=>
) ;tm-menu

(tm-menu (focus-float-icons t)
  (:require (rich-float-context? t))
  (if (in-multicol-style?)
   ((check (balloon (icon "tm_wide_float.xpm") "Make float wide")
      "v"
      (float-wide? (focus-tree))
    ) ;check
    (float-toggle-wide (focus-tree))
   ) ;
  ) ;if
  (=> (balloon (icon "tm_position_float.xpm") "Allowed positions of floating object")
    (link float-menu)
  ) ;=>
  ((balloon (icon "tm_anchor.xpm") "Go to anchor or float")
   (cursor-toggle-anchor)
  ) ;
) ;tm-menu

(tm-menu (focus-float-icons t)
  (:require (phantom-float-context? t))
  (=> (balloon (icon "tm_position_float.xpm") "Allowed positions of floating object")
    (link float-menu)
  ) ;=>
) ;tm-menu

(tm-menu (focus-float-icons t)
  (:require (floatable-context? t))
  (if (in-multicol-style?)
   ((check (balloon (icon "tm_wide_float.xpm") "Make wide")
      "v"
      (floatable-wide? (focus-tree))
    ) ;check
    (floatable-toggle-wide (focus-tree))
   ) ;
  ) ;if
  ((balloon (icon "tm_position_float.xpm") "Let the environment float")
   (turn-floating (tree-innermost floatable-context?))
  ) ;
) ;tm-menu

(tm-menu (focus-float-icons t)
  (:require (footnote-context? t))
  (if (in-multicol-style?)
   ((check (balloon (icon "tm_wide_float.xpm") "Make footnote wide")
      "v"
      (float-wide? (focus-tree))
    ) ;check
    (float-toggle-wide (focus-tree))
   ) ;
  ) ;if
  ((balloon (icon "tm_anchor.xpm") "Go to anchor or footnote")
   (cursor-toggle-anchor)
  ) ;
) ;tm-menu

(tm-menu (focus-float-icons t)
  (:require (balloon-context? t))
  (=> (balloon (icon "tm_position_float.xpm") "Alignment of balloon")
    (link position-balloon-menu)
  ) ;=>
) ;tm-menu

(tm-define (standard-options l)
  (:require (in? l (numbered-unnumbered-append '(small-figure big-figure))))
  (list "figure-captions-above" "number-long-article")
) ;tm-define

(tm-define (standard-options l)
  (:require (in? l (numbered-unnumbered-append '(small-table big-table))))
  (list "table-captions-above" "number-long-article")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Detached notes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-toggle-menu t)
  (:require (detached-note-context? t))
  ((check "Named" "v" (custom-note-context? (focus-tree))) (note-toggle-custom t))
  (dynamic (former t))
) ;tm-menu

(tm-menu (focus-toggle-icons t)
  (:require (detached-note-context? t))
  ((check (balloon (icon "tm_small_textual.xpm") "Use custom note symbol")
     "v"
     (custom-note-context? (focus-tree))
   ) ;check
   (note-toggle-custom t)
  ) ;
  (dynamic (former t))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Possibility to rename titled environments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-toggle-menu t)
  (:require (titled-context? t))
  ((check "Named" "v" (titled-named? (focus-tree))) (titled-toggle-name t))
  (dynamic (former t))
) ;tm-menu

(tm-menu (focus-toggle-icons t)
  (:require (titled-context? t))
  ((check (balloon (icon "tm_small_textual.xpm") "Toggle name")
     "v"
     (titled-named? (focus-tree))
   ) ;check
   (titled-toggle-name t)
  ) ;
  (dynamic (former t))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Framed environments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-toggle-menu t)
  (:require (or (frame-context? t) (frame-titled-context? t)))
  ((check "Named" "v" (frame-titled? (focus-tree))) (frame-toggle-title t))
  (dynamic (former t))
) ;tm-menu

(tm-menu (focus-toggle-icons t)
  (:require (or (frame-context? t) (frame-titled-context? t)))
  ((check (balloon (icon "tm_small_textual.xpm") "Toggle name")
     "v"
     (frame-titled? (focus-tree))
   ) ;check
   (frame-toggle-title t)
  ) ;
  (dynamic (former t))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Renaming automatically generated sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-toggle-menu t)
  (:require (automatic-section-context? t))
  (dynamic (former t))
  ("Rename" (interactive automatic-section-rename))
) ;tm-menu

(tm-menu (focus-toggle-icons t)
  (:require (automatic-section-context? t))
  (dynamic (former t))
  ((balloon (icon "tm_small_textual.xpm") "Rename section")
   (interactive automatic-section-rename)
  ) ;
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Decorated tag
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (customizable-parameters t)
  (:require (tree-is? t 'marked))
  (list (list "marked-color" "Color"))
) ;tm-define

(tm-define (customizable-parameters t)
  (:require (tree-is? t 'nbsp))
  (list (list "nbsp-color" "Color"))
) ;tm-define

(tm-define (customizable-parameters t)
  (:require (and (tree-is? t 'with)
              (== (tree-arity t) 3)
              (== (tree->string (tree-ref t 0)) "color")
            ) ;and
  ) ;:require
  (list (list "color" "Color"))
) ;tm-define

(tm-define (customizable-parameters t)
  (:require (and (tree-is? t 'with)
              (== (tree-arity t) 3)
              (== (tree->string (tree-ref t 0)) "text-bg-color")
            ) ;and
  ) ;:require
  (list (list "text-bg-color" "Text background color"))
) ;tm-define

(tm-define (customizable-parameters-memo t)
  (:require (tree-is? t 'with))
  (customizable-parameters t)
) ;tm-define

(tm-menu (focus-hidden-icons t)
  (:require (and (tree-is? t 'with)
              (== (tree-arity t) 3)
              (in? (tree->string (tree-ref t 0)) (list "color" "text-bg-color"))
            ) ;and
  ) ;:require
) ;tm-menu
