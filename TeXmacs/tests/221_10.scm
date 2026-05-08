;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 221_10.scm
;; DESCRIPTION : Unit tests for structured insert up/down in lists
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-edit))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-list-structured-insert-end-index)
  (let* ((itemize-doc
          (tm->tree '(document
                       (concat (item) "a")
                       (itemize (document (concat (item) "a.1")))
                       (concat (item) "b"))))
         (enumerate-doc
          (tm->tree '(document
                       (concat (item) "1")
                       (enumerate-numeric
                         (document (concat (item) "1.1")))
                       (concat (item) "2"))))
         (mixed-doc
          (tm->tree '(document
                       (concat (item) "a")
                       (enumerate (document (concat (item) "1")))
                       (concat (item) "b"))))
         (plain-doc
          (tm->tree '(document
                       (concat (item) "a")
                       (concat (item) "b"))))
         (user-doc
          (tm->tree '(document
                       (item)
                       (concat (item) " ddd"))))
         (continued-doc
         (tm->tree '(document
                       (item)
                       (item)
                       (concat (item) "ddd")
                       "ddd"
                       (item)))))
    (check (list-item-end-index itemize-doc 0 'itemize) => 2)
    (check (list-item-end-index enumerate-doc 0 'enumerate) => 2)
    (check (list-item-end-index mixed-doc 0 'itemize) => 2)
    (check (list-item-end-index plain-doc 0 'itemize) => 1)
    (check (list-item-insert-index user-doc 1 'enumerate #f) => 1)
    (check (list-item-insert-index user-doc 1 'enumerate #t) => 2)
    (check (list-item-insert-index continued-doc 2 'enumerate #f) => 2)
    (check (list-item-insert-index continued-doc 2 'enumerate #t) => 4)))

(define (insert-blank-list-item doc pos)
  (tree->stree
   (tree-insert doc pos (list (blank-list-item-stree 'enumerate)))))

(define (remove-list-item-at doc item-index downwards?)
  (tree->stree
   (remove-list-item-range
    doc
    (list-item-remove-range doc item-index 'enumerate downwards?))))

(define (test-list-structured-insert-tree-shape)
  (let* ((up-doc
          (tm->tree '(document
                       (item)
                       (concat (item) " ddd"))))
         (down-doc
          (tm->tree '(document
                       (item)
                       (concat (item) " ddd"))))
         (nested-doc
          (tm->tree '(document
                       (concat (item) "a")
                       (enumerate
                         (document (concat (item) "a.1")))
                       (concat (item) "b"))))
         (continued-doc
          (tm->tree '(document
                       (item)
                       (item)
                       (concat (item) "ddd")
                       "ddd"
                       (item))))
         (bare-item-doc
          (tm->tree '(document
                       (item)
                       (concat (item) "ddd")
                       (item)))))
    (check (insert-blank-list-item
            up-doc
            (list-item-insert-index up-doc 1 'enumerate #f))
           => '(document (item) (item) (concat (item) " ddd")))
    (check (insert-blank-list-item
            down-doc
            (list-item-insert-index down-doc 1 'enumerate #t))
           => '(document (item) (concat (item) " ddd") (item)))
    (check (insert-blank-list-item
            nested-doc
            (list-item-insert-index nested-doc 0 'enumerate #t))
           => '(document
                 (concat (item) "a")
                 (enumerate (document (concat (item) "a.1")))
                 (item)
                 (concat (item) "b")))
    (check (insert-blank-list-item
            continued-doc
            (list-item-insert-index continued-doc 2 'enumerate #t))
           => '(document
                 (item)
                 (item)
                 (concat (item) "ddd")
                 "ddd"
                 (item)
                 (item)))
    (check (insert-blank-list-item
            bare-item-doc
            (list-item-insert-index bare-item-doc 0 'enumerate #t))
           => '(document
                 (item)
                 (item)
                 (concat (item) "ddd")
                 (item)))))

(define (test-list-structured-insert-blank-item)
  (check (blank-list-item-stree 'enumerate) => '(item))
  (check (blank-list-item-stree 'description) => '(item* "")))

(define (test-list-structured-remove-tree-shape)
  (let* ((remove-up-doc
          (tm->tree '(document
                       (item)
                       (concat (item) "ddd")
                       "more"
                       (item))))
         (remove-down-doc
          (tm->tree '(document
                       (item)
                       (concat (item) "ddd")
                       "more"
                       (item))))
         (nested-doc
          (tm->tree '(document
                       (concat (item) "a")
                       (enumerate
                         (document (concat (item) "a.1")))
                       (concat (item) "b")
                       (concat (item) "c"))))
         (bare-item-doc
          (tm->tree '(document
                       (item)
                       (concat (item) "ddd")
                       (item))))
         (only-list-doc
          (tm->tree '(document
                       (enumerate
                         (document (concat (item) "a"))))))))
    (check (list-item-remove-range remove-up-doc 1 'enumerate #f)
           => '(0 1))
    (check (list-item-remove-range remove-down-doc 1 'enumerate #t)
           => '(1 3))
    (check (remove-list-item-at remove-up-doc 1 #f)
           => '(document (concat (item) "ddd") "more" (item)))
    (check (remove-list-item-at remove-down-doc 1 #t)
           => '(document (item) (item)))
    (check (list-item-remove-range nested-doc 0 'enumerate #t)
           => '(0 2))
    (check (remove-list-item-at nested-doc 0 #t)
           => '(document
                 (concat (item) "b")
                 (concat (item) "c")))
    (check (remove-list-item-at bare-item-doc 0 #t)
           => '(document
                 (concat (item) "ddd")
                 (item)))
    (check (remove-list-item-at only-list-doc 0 #t)
           => '(document ""))))

(tm-define (test_221_10)
  (test-list-structured-insert-end-index)
  (test-list-structured-insert-tree-shape)
  (test-list-structured-insert-blank-item)
  (test-list-structured-remove-tree-shape)
  (check-report))
