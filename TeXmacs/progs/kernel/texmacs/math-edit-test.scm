(texmacs-module (kernel texmacs math-edit-test)
  (:use (math math-edit) (utils library tree))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-inside-comment-or-balloon)
  (let* ((comment-folded (stree->tree '(folded-comment "1"
                                         "1"
                                         "comment"
                                         "author"
                                         "date"
                                         ""
                                         (document (para (equation* "x"))))
                         ) ;stree->tree
         ) ;comment-folded
         (comment-unfolded (stree->tree '(unfolded-comment "1"
                                           "1"
                                           "comment"
                                           "author"
                                           "date"
                                           ""
                                           (document (para (equation* "x"))))
                           ) ;stree->tree
         ) ;comment-unfolded
         (comment-mirror (stree->tree '(mirror-comment "1"
                                         "1"
                                         "comment"
                                         "author"
                                         "date"
                                         ""
                                         (document (para (equation* "x"))))
                         ) ;stree->tree
         ) ;comment-mirror
         (comment-carbon (stree->tree '(carbon-comment "1"
                                         "1"
                                         "comment"
                                         "author"
                                         "date"
                                         ""
                                         (document (para (equation* "x"))))
                         ) ;stree->tree
         ) ;comment-carbon
         (comment-nested (stree->tree '(nested-comment "1"
                                         "1"
                                         "comment"
                                         "author"
                                         "date"
                                         ""
                                         (document (para (equation* "x"))))
                         ) ;stree->tree
         ) ;comment-nested
         (balloon-hover (stree->tree '(hover-balloon "x" "tooltip")))
         (balloon-hover-star (stree->tree '(hover-balloon* "x" "tooltip")))
         (balloon-popup (stree->tree '(popup-balloon "x" "tooltip")))
         (balloon-popup-star (stree->tree '(popup-balloon* "x" "tooltip")))
         (balloon-focus (stree->tree '(focus-balloon "x" "tooltip")))
         (balloon-help (stree->tree '(help-balloon "x" "tooltip")))
         (normal-tree (stree->tree '(document (para (equation* "x")))))
        ) ;
    (check (inside-comment-or-balloon? comment-folded) => #t)
    (check (inside-comment-or-balloon? comment-unfolded) => #t)
    (check (inside-comment-or-balloon? comment-mirror) => #t)
    (check (inside-comment-or-balloon? comment-carbon) => #t)
    (check (inside-comment-or-balloon? comment-nested) => #t)
    (check (inside-comment-or-balloon? balloon-hover) => #t)
    (check (inside-comment-or-balloon? balloon-hover-star) => #t)
    (check (inside-comment-or-balloon? balloon-popup) => #t)
    (check (inside-comment-or-balloon? balloon-popup-star) => #t)
    (check (inside-comment-or-balloon? balloon-focus) => #t)
    (check (inside-comment-or-balloon? balloon-help) => #t)
    (check (inside-comment-or-balloon? normal-tree) => #f)
  ) ;let*
) ;define

(tm-define (regtest-math-edit) (test-inside-comment-or-balloon) (check-report))
