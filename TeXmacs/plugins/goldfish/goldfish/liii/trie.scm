(define-library (liii trie)

  (export make-trie
    trie?
    trie-insert!
    trie-ref
    trie-ref*
    trie-value
    trie->list
  ) ;export
  (import (srfi srfi-1)
    (srfi srfi-2)
    (srfi srfi-9)
    (liii alist)
  ) ;import

  (begin
    (define-record-type :trie
      (make-trie* children value)
      trie?
      (children trie-children
        trie-children-set!
      ) ;children
      (value trie-value trie-value-set!)
    ) ;define-record-type

    (define (make-trie)
      (make-trie* (list) (list))
    ) ;define

    (define (trie-ref* trie key)
      (alist-ref/default (trie-children trie)
        key
        #f
      ) ;alist-ref/default
    ) ;define

    (define* (trie-ref trie key (default #f))
      (let loop
        ((node trie) (key key))
        (if (null? key)
          (if (null? (trie-value node))
            default
            (car (trie-value node))
          ) ;if
          (let ((child (trie-ref* node (car key))))
            (if child
              (loop child (cdr key))
              default
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define*

    (define (add-child! trie key child)
      (trie-children-set! trie
        (alist-cons key
          child
          (trie-children trie)
        ) ;alist-cons
      ) ;trie-children-set!
    ) ;define

    (define (trie-insert! trie key val)
      (let loop
        ((node trie) (key key))
        (if (null? key)
          (trie-value-set! node (list val))
          (let* ((ckey (car key))
                 (child (or (trie-ref* node ckey)
                          (let ((child (make-trie)))
                            (add-child! node ckey child)
                            child
                          ) ;let
                        ) ;or
                 ) ;child
                ) ;
            (loop child (cdr key))
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (trie->list trie)
      (cons (let loop
              ((trie trie))
              (map (lambda (child)
                     (cons (car child)
                       (trie->list (cdr child))
                     ) ;cons
                   ) ;lambda
                (trie-children trie)
              ) ;map
            ) ;let
        (trie-value trie)
      ) ;cons
    ) ;define

  ) ;begin
) ;define-library
