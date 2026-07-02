
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : poster-edit.scm
;; DESCRIPTION : extra edit routines for posters
;; COPYRIGHT   : (C) 2018  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (various poster-edit)
  (:use (various poster-drd) (generic document-edit))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style package rules for poster
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (poster-themes)
  (list "boring-white"
    "dark"
    "blackboard"
    "bluish"
    "dark-vador"
    "granite"
    "ice"
    "manila-paper"
    "metal"
    "pale-blue"
    "pine"
    "reddish"
    "ridged-paper"
    "rough-paper"
    "xperiment"
  ) ;list
) ;tm-define

(tm-define (poster-title-styles)
  (list "plain-poster-title" "framed-poster-title" "topless-poster-title")
) ;tm-define

(tm-define (current-poster-theme)
  (with l
    (get-style-list)
    (or (list-find l (cut in? <> (poster-themes))) "boring-white")
  ) ;with
) ;tm-define

(tm-define (current-poster-title-style)
  (with l
    (get-style-list)
    (or (list-find l (cut in? <> (poster-title-styles))) "framed-poster-title")
  ) ;with
) ;tm-define

(tm-define (style-category p)
  (:require (and (in-poster?) (in? p (poster-themes))))
  :poster-theme
) ;tm-define

(tm-define (style-category p)
  (:require (in? p (poster-title-styles)))
  :poster-title-style
) ;tm-define

(tm-define (style-category-precedes? x y)
  (:require (and (== x "alt-colors") (== y :poster-theme)))
  #t
) ;tm-define

(tm-define (style-category-precedes? x y)
  (:require (and (== x :poster-theme)
              (in? y (list :poster-title-style :theorem-decorations))
            ) ;and
  ) ;:require
  #t
) ;tm-define

(tm-define (style-includes? x y)
  (:require (and (== x "poster") (in? y (list "boring-white" "framed-poster-title")))
  ) ;:require
  #t
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page size and orientation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (poster-sizes)
  (list "a0" "a1" "a2" "a3" "a4")
) ;define

(define (poster-size-styles)
  (map (cut string-append <> "-poster") (poster-sizes))
) ;define

(tm-define (default-page-type)
  (:mode in-poster?)
  (former)
  (set-style-list (list-difference (get-style-list) (poster-size-styles)))
) ;tm-define

(tm-define (init-page-type s)
  (:mode in-poster?)
  (former s)
  (let* ((del (poster-size-styles))
         (ins (if (and (!= s "a0") (in? s (poster-sizes)))
                (list (string-append s "-poster"))
                (list)
              ) ;if
         ) ;ins
        ) ;
    (set-style-list (append (list-difference (get-style-list) del) ins))
  ) ;let*
) ;tm-define

(tm-define (init-default-page-orientation)
  (:mode in-poster?)
  (former)
  (with l
    (list "portrait-poster" "landscape-poster")
    (set-style-list (list-difference (get-style-list) l))
  ) ;with
) ;tm-define

(tm-define (init-page-orientation s)
  (:mode in-poster?)
  (former s)
  (let* ((del (list "portrait-poster" "landscape-poster"))
         (ins (if (== s "landscape") (list "landscape-poster") (list)))
        ) ;
    (set-style-list (append (list-difference (get-style-list) del) ins))
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Blocks
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (poster-block-context? t) (tree-in? t (poster-block-tag-list)))

(tm-define (titled-block-context? t) (tree-in? t (titled-block-tag-list)))

(tm-define (untitled-block-context? t) (tree-in? t (untitled-block-tag-list)))

(tm-define (parameter-choice-list l)
  (:require (in? l (list "framed-shape" "alternate-shape")))
  (parameter-choice-list "ornament-shape")
) ;tm-define

(tm-define (parameter-choice-list l)
  (:require (in? l (list "framed-title-style" "alternate-title-style")))
  (parameter-choice-list "ornament-title-style")
) ;tm-define

(tm-define (make-poster-title) (make 'poster-title) (make 'with-title-font))

(tm-define (insert-same-block t below?)
  (tree-go-to t (if below? :end :start))
  (cond ((== (tree-arity t) 1) (insert-go-to `(,(tree-label t) (document "")) '(0
                                                                                0
                                                                                0)))
        ((== (tree-arity t) 2)
         (insert-go-to `(,(tree-label t) ,"" (document "")) '(0 0))
        ) ;
  ) ;cond
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (poster-block-context? t))
  (insert-same-block t downwards?)
) ;tm-define

(tm-define (kbd-control-enter t shift?)
  (:require (poster-block-context? t))
  (insert-same-block t #t)
) ;tm-define

(define (toggle-titled l)
  (cond ((== l 'plain-block) 'plain-titled-block)
        ((== l 'framed-block) 'framed-titled-block)
        ((== l 'alternate-block) 'alternate-titled-block)
        ((== l 'plain-titled-block) 'plain-block)
        ((== l 'framed-titled-block) 'framed-block)
        ((== l 'alternate-titled-block) 'alternate-block)
        (else l)
  ) ;cond
) ;define

(tm-define (test-block-titled? . args) (titled-block-context? (focus-tree)))
(tm-define (block-toggle-titled t)
  (:check-mark "v" test-block-titled?)
  (cond ((titled-block-context? t)
         (tree-assign-node! t (toggle-titled (tree-label t)))
         (tree-remove! t 0 1)
         (tree-go-to t 0 :start)
        ) ;
        ((untitled-block-context? t)
         (tree-assign-node! t (toggle-titled (tree-label t)))
         (tree-insert! t 0 '(""))
         (tree-go-to t 0 0)
        ) ;
  ) ;cond
) ;tm-define

(tm-define (block-wide? t)
  (and-with f
    (tree-search-upwards t poster-block-context?)
    (and-with w
      (tree-up f)
      (and (tree-is? w 'with)
        (== (tree-arity w) 3)
        (tm-equal? (tree-ref w 0) "par-columns")
        (tm-equal? (tree-ref w 1) "1")
      ) ;and
    ) ;and-with
  ) ;and-with
) ;tm-define

(tm-define (test-block-wide? . args) (block-wide? (focus-tree)))
(tm-define (block-toggle-wide t)
  (:check-mark "v" test-block-wide?)
  (and-with f
    (tree-search-upwards t poster-block-context?)
    (and-with w
      (tree-up f)
      (if (block-wide? t)
        (tree-set w (tree-ref w 2))
        (tree-set f `(with ,"par-columns" ,"1" ,f))
      ) ;if
    ) ;and-with
  ) ;and-with
) ;tm-define

(tm-define (kbd-enter t shift?)
  (:require (tree-is? t 'poster-title))
  (when (not shift?)
    (tree-go-to t :end)
    (insert-go-to '(framed-titled-block "" (document "")) '(0 0))
  ) ;when
) ;tm-define
