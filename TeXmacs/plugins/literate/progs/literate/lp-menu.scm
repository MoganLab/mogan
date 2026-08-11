
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : lp-menu.scm
;; DESCRIPTION : menus for literate programming
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (literate lp-menu) (:use (literate lp-build)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main literate programming menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind literate-menu
  (-> "First chunk"
   ("Scheme" (insert-new-chunk 'scm-chunk))
   ("C++" (insert-new-chunk 'cpp-chunk))
   ("Generic" (insert-new-chunk 'generic-chunk))
   ("Verbatim" (insert-new-chunk 'verbatim-chunk))
   ("Shell" (insert-new-chunk 'shell-chunk))
   ---
   (when (style-has? "json")
     ("JSON" (insert-new-chunk 'json-chunk))
   ) ;when
   (when (style-has? "elvish-lang")
     ("Elvish" (insert-new-chunk 'elvish-chunk))
   ) ;when
   (when (style-has? "goldfish-lang")
     ("Goldfish" (insert-new-chunk 'goldfish-chunk))
   ) ;when
   (when (style-has? "scala")
     ("Scala" (insert-new-chunk 'scala-chunk))
   ) ;when
   (when (style-has? "java")
     ("Java" (insert-new-chunk 'java-chunk))
   ) ;when
   (when (style-has? "python")
     ("Python" (insert-new-chunk 'python-chunk))
   ) ;when
  ) ;->
  (with all-chunks
    (get-all-chunks)
    (when (nnull? (search-chunk-types all-chunks))
      (if (null? (search-chunk-types all-chunks))
       ("Next chunk" (interactive insert-next-chunk))
      ) ;if
      (if (nnull? (search-chunk-types all-chunks))
        (-> "Next chunk"
          (for (name (search-chunk-types all-chunks))
           ((eval `(verbatim ,name)) (insert-next-chunk name))
          ) ;for
          ---
          ("Other" (interactive insert-next-chunk))
        ) ;->
      ) ;if
      (if (null? (search-chunk-types all-chunks)) ("Reference" (make 'chunk-ref)))
      (if (nnull? (search-chunk-types all-chunks))
        (-> "Reference"
          (for (name (search-chunk-types all-chunks))
           ((eval `(verbatim ,name)) (insert `(chunk-ref ,name)))
          ) ;for
          ---
          ("Other" (make 'chunk-ref))
        ) ;->
      ) ;if
    ) ;when
  ) ;with

  ---
  ("Invisible newline" (make 'folded-newline-before))
  ("Invisible opening" (make 'unfolded-opening))
  ("Invisible ending" (make 'unfolded-ending))
  (when (nnull? (search-appended-unfolded (buffer-tree)))
    ("Fold all" (fold-appended))
  ) ;when
  (when (nnull? (search-appended-folded (buffer-tree)))
    ("Unfold all" (unfold-appended))
  ) ;when
  ---
  (when (nnull? (search-chunk-types (get-all-chunks)))
    ("Build buffer" (lp-build-buffer))
    ("Build buffer in" (interactive lp-build-buffer-in))
  ) ;when
  ("Build directory" (lp-interactive-build-directory))
  ("Build directory in" (lp-interactive-build-directory-in))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Insert menu as extra top level menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind texmacs-extra-menu
  (former)
  (if (style-has? "literate-dtd") (=> "Literate" (link literate-menu)))
) ;menu-bind
