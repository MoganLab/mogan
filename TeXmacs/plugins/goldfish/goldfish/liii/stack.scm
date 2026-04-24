(define-library (liii stack)
  (export make-stack
    stack
    stack?
    stack-empty?
    stack-top
    stack-size
    stack-push!
    stack-pop!
    stack->list
    list->stack
    stack-map
    stack-map!
    stack-for-each
    stack-copy
  ) ;export
  (import (liii error))
  (begin

    (define-record-type stack
      (%make-stack elements)
      stack?
      (elements stack-elements
        stack-elements-set!
      ) ;elements
    ) ;define-record-type

    (define (make-stack . args)
      (if (null? args)
        (%make-stack '())
        (let ((arg (car args)))
          (if (list? arg)
            (%make-stack arg)
            (type-error (format #f
                          "make-stack in (liii stack): argument must be *list* type! **Got ~a**"
                          (object->string arg)
                        ) ;format
            ) ;type-error
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    (define (stack . elements)
      (%make-stack elements)
    ) ;define

    (define (stack-empty? s)
      (unless (stack? s)
        (type-error (format #f
                      "stack-empty? in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (null? (stack-elements s))
    ) ;define

    (define (stack-top s)
      (unless (stack? s)
        (type-error (format #f
                      "stack-top in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (if (stack-empty? s)
        (value-error "stack-top in (liii stack): stack is empty"
        ) ;value-error
        (car (stack-elements s))
      ) ;if
    ) ;define

    (define (stack-size s)
      (unless (stack? s)
        (type-error (format #f
                      "stack-size in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (length (stack-elements s))
    ) ;define

    (define (stack-push! s elem)
      (unless (stack? s)
        (type-error (format #f
                      "stack-push! in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (stack-elements-set! s
        (cons elem (stack-elements s))
      ) ;stack-elements-set!
      s
    ) ;define

    (define (stack-pop! s)
      (unless (stack? s)
        (type-error (format #f
                      "stack-pop! in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (if (stack-empty? s)
        (value-error "stack-pop! in (liii stack): stack is empty"
        ) ;value-error
        (let ((top (car (stack-elements s))))
          (stack-elements-set! s
            (cdr (stack-elements s))
          ) ;stack-elements-set!
          top
        ) ;let
      ) ;if
    ) ;define

    (define (stack->list s)
      (unless (stack? s)
        (type-error (format #f
                      "stack->list in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (stack-elements s)
    ) ;define

    (define (list->stack lst)
      (unless (list? lst)
        (type-error (format #f
                      "list->stack in (liii stack): argument *lst* must be *list* type! **Got ~a**"
                      (object->string lst)
                    ) ;format
        ) ;type-error
      ) ;unless
      (%make-stack lst)
    ) ;define

    (define (stack-map proc s)
      (unless (procedure? proc)
        (type-error (format #f
                      "stack-map in (liii stack): argument *proc* must be *procedure* type! **Got ~a**"
                      (object->string proc)
                    ) ;format
        ) ;type-error
      ) ;unless
      (unless (stack? s)
        (type-error (format #f
                      "stack-map in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (%make-stack (map proc (stack-elements s))
      ) ;%make-stack
    ) ;define

    (define (stack-map! proc s)
      (unless (procedure? proc)
        (type-error (format #f
                      "stack-map! in (liii stack): argument *proc* must be *procedure* type! **Got ~a**"
                      (object->string proc)
                    ) ;format
        ) ;type-error
      ) ;unless
      (unless (stack? s)
        (type-error (format #f
                      "stack-map! in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (stack-elements-set! s
        (map proc (stack-elements s))
      ) ;stack-elements-set!
      s
    ) ;define

    (define (stack-for-each proc s)
      (unless (procedure? proc)
        (type-error (format #f
                      "stack-for-each in (liii stack): argument *proc* must be *procedure* type! **Got ~a**"
                      (object->string proc)
                    ) ;format
        ) ;type-error
      ) ;unless
      (unless (stack? s)
        (type-error (format #f
                      "stack-for-each in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (for-each proc (stack-elements s))
    ) ;define

    (define (stack-copy s)
      (unless (stack? s)
        (type-error (format #f
                      "stack-copy in (liii stack): argument *s* must be *stack* type! **Got ~a**"
                      (object->string s)
                    ) ;format
        ) ;type-error
      ) ;unless
      (%make-stack (stack-elements s))
    ) ;define

  ) ;begin
) ;define-library
