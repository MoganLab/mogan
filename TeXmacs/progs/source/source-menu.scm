
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : source-menu.scm
;; DESCRIPTION : menus for inserting macro-language markup
;;               used for writing style files in source mode.
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (source source-menu)
  (:use (source source-edit) (source macro-widgets))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Transformational markup for the macro language
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind source-define-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Assign") (make 'assign))
    ((verbatim "With") (make 'with 3))
    ((verbatim "Value") (make 'value))
  ) ;when
) ;menu-bind

(menu-bind source-macro-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Macro") (make 'macro))
    ((verbatim "Argument") (make 'arg))
    ((verbatim "Compound") (make 'compound))
    ((verbatim "Extern") (make 'extern))
    ---
    ((verbatim "Long macro") (make 'xmacro))
    ((verbatim "Get label") (make 'get-label))
    ((verbatim "Get arity") (make 'get-arity))
    ((verbatim "Map arguments") (make 'map-args))
  ) ;when
) ;menu-bind

(menu-bind source-quote-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Evaluate") (make 'eval))
    ---
    ((verbatim "Quote") (make 'quote))
    ((verbatim "Quasi") (make 'quasi))
    ((verbatim "Quasiquote") (make 'quasi-quote))
    ((verbatim "Unquote") (make 'unquote))
    ---
    ((verbatim "Unevaluated value") (make 'quote-value))
    ((verbatim "Unevaluated argument") (make 'quote-arg))
  ) ;when
) ;menu-bind

(menu-bind source-flow-menu
  (when (not (selection-active-non-small?))
    ((verbatim "If") (make 'if))
    ((verbatim "Case") (make 'case))
    ((verbatim "While") (make 'while))
    ((verbatim "For each") (make 'for-each))
  ) ;when
) ;menu-bind

(menu-bind source-transformational-menu
  (-> "Definition" (link source-define-menu))
  (-> "Macro" (link source-macro-menu))
  (-> "Evaluation" (link source-quote-menu))
  (-> "Control flow" (link source-flow-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Executable markup for the macro language
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind source-arithmetic-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Plus") (make 'plus))
    ((verbatim "Minus") (make 'minus))
    ((verbatim "Times") (make 'times))
    ((verbatim "Over") (make 'over))
    ((verbatim "Div") (make 'div))
    ((verbatim "Mod") (make 'mod))
  ) ;when
) ;menu-bind

(menu-bind source-text-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Merge") (make 'merge))
    ((verbatim "Length") (make 'length))
    ((verbatim "Range") (make 'range))
    ((verbatim "Number") (make 'number))
    ((verbatim "Today") (make 'date 0))
    ((verbatim "Formatted date") (make 'date))
    ((verbatim "Translate::language") (make 'translate))
    ((verbatim "Find file") (make 'find-file))
  ) ;when
) ;menu-bind

(menu-bind source-tuple-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Tuple?") (make 'is-tuple))
    ((verbatim "Merge") (make 'merge))
    ((verbatim "Length") (make 'length))
    ((verbatim "Range") (make 'range))
    ((verbatim "Look up") (make 'look-up))
  ) ;when
) ;menu-bind

(menu-bind source-condition-menu
  (when (not (selection-active-non-small?))
    ((verbatim "Not") (make 'not))
    ((verbatim "And") (make 'and))
    ((verbatim "Or") (make 'or))
    ((verbatim "Exclusive or") (make 'xor))
    ---
    ((verbatim "Equal") (make 'equal))
    ((verbatim "Not equal") (make 'unequal))
    ((verbatim "Less") (make 'less))
    ((verbatim "Less or equal") (make 'lesseq))
    ((verbatim "Greater") (make 'greater))
    ((verbatim "Greater or equal") (make 'greatereq))
  ) ;when
) ;menu-bind

(menu-bind source-executable-menu
  (-> "Arithmetic" (link source-arithmetic-menu))
  (-> "Text" (link source-text-menu))
  (-> "Tuple" (link source-tuple-menu))
  (-> "Condition" (link source-condition-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Presentation of source code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind source-activation-menu
 ("Activate" (make-mod-active 'active*))
 ("Activate once" (make-mod-active 'active))
 (when (or (not (in-source?)) (inside? 'active) (inside? 'active*))
   ("Deactivate" (make-mod-active 'inactive*))
   ("Deactivate once" (make-mod-active 'inactive))
 ) ;when
) ;menu-bind

(menu-bind source-layout-menu
 ("Compact" (make-style-with "src-compact" "all"))
 ("Stretched" (make-style-with "src-compact" "none"))
 ---
 ("Apply macro" (make-mod-active 'style-only*))
 ("Apply macro once" (make-mod-active 'style-only))
) ;menu-bind

(menu-bind source-presentation-menu
  (-> "Activation" (link source-activation-menu))
  (-> "Presentation" (link source-layout-menu))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main menu for editing source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind source-macros-menu
 ("New macro" (open-macro-editor "" :global))
 (when (can-create-context-macro?)
   ("Create context macro" (create-context-macro "" :global))
 ) ;when
 (when (can-create-table-macro?)
   ("Create table macro" (create-table-macro "" :global))
 ) ;when
 ---
 ("Edit macros" (open-macros-editor :global))
 ("Edit preamble" (toggle-preamble-mode))
 ---
 ("Extract style file" (extract-style-file #t))
 ("Extract style package" (extract-style-file #f))
) ;menu-bind

(menu-bind source-menu
  (link source-transformational-menu)
  ---
  (link source-executable-menu)
  ---
  (link source-presentation-menu)
  ---
  (link source-macros-menu)
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The icon bar for editing source files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind source-icons
  (=> (balloon (icon "tm_assign.xpm") "Set or get environment variables")
    (link source-define-menu)
  ) ;=>
  (=> (balloon (icon "tm_lambda.xpm") "Write a macro") (link source-macro-menu))
  (=> (balloon (icon "tm_prime.xpm") "Control the evaluation of expressions")
    (link source-quote-menu)
  ) ;=>
  (=> (balloon (icon "tm_ctrl_flow.xpm") "Insert a control flow instruction")
    (link source-flow-menu)
  ) ;=>
  /
  (=> (balloon (icon "tm_three.xpm") "Insert a numerical operation")
    (link source-arithmetic-menu)
  ) ;=>
  (=> (balloon (icon "tm_equal.xpm") "Insert a condition")
    (link source-condition-menu)
  ) ;=>
  (=> (balloon (icon "tm_textual.xpm") "Insert a textual operation")
    (link source-text-menu)
  ) ;=>
  (=> (balloon (icon "tm_tuple.xpm") "Insert an operation on tuples")
    (link source-tuple-menu)
  ) ;=>
  /

  ((balloon (icon "tm_activate.xpm") "Activate") (make-mod-active 'active*))
  ((balloon (icon "tm_stretch.xpm") "Stretch")
   (make-style-with "src-compact" "none")
  ) ;
  ((balloon (icon "tm_compact.xpm") "Compactify")
   (make-style-with "src-compact" "all")
  ) ;
  /
  (link text-format-icons)
) ;menu-bind
