(define-library (liii vector)
  (import (scheme base)
    (srfi srfi-133)
    (srfi srfi-13)
  ) ;import
  (export vector-empty?
    vector-fold
    vector-fold-right
    vector-count
    vector-any
    vector-every
    vector-index
    vector-index-right
    vector-skip
    vector-skip-right
    vector-partition
    vector-swap!
    vector-reverse!
    vector-cumulate
    reverse-list->vector
    vector=
    vector-contains?
    vector-filter
    vector-contains?
    vector-take
    vector-drop
    vector-take-right
    vector-drop-right
    int-vector
    int-vector?
    make-int-vector
    int-vector-ref
    int-vector-set!
    complex-vector
    complex-vector?
    make-complex-vector
    complex-vector-ref
    complex-vector-set!
    float-vector
    float-vector?
    make-float-vector
    float-vector-ref
    float-vector-set!
  ) ;export
  (begin

    (define (vector-filter pred vec)
      (let* ((result-list (vector-fold (lambda (elem acc)
                                         (if (pred elem) (cons elem acc) acc)
                                       ) ;lambda
                            '()
                            vec
                          ) ;vector-fold
             ) ;result-list
             (result-length (length result-list))
             (result-vec (make-vector result-length))
            ) ;
        (let loop
          ((i (- result-length 1))
           (lst result-list)
          ) ;
          (if (null? lst)
            result-vec
            (begin
              (vector-set! result-vec i (car lst))
              (loop (- i 1) (cdr lst))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (vector-contains? vec elem . args)
      (let ((cmp (if (null? args) equal? (car args))
            ) ;cmp
           ) ;
        (not (not (vector-index (lambda (x) (cmp x elem))
                    vec
                  ) ;vector-index
             ) ;not
        ) ;not
      ) ;let
    ) ;define

    (define (vector-take vec n)
      (unless (vector? vec)
        (type-error "vector-take: first argument must be a vector"
          vec
        ) ;type-error
      ) ;unless
      (unless (integer? n)
        (type-error "vector-take: second argument must be an integer"
          n
        ) ;type-error
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) (vector))
              ((>= n len) vec)
              (else (vector-copy vec 0 n))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-drop vec n)
      (unless (vector? vec)
        (type-error "vector-drop: first argument must be a vector"
          vec
        ) ;type-error
      ) ;unless
      (unless (integer? n)
        (type-error "vector-drop: second argument must be an integer"
          n
        ) ;type-error
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) vec)
              ((>= n len) (vector))
              (else (vector-copy vec n))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-take-right vec n)
      (unless (vector? vec)
        (type-error "vector-take-right: first argument must be a vector"
          vec
        ) ;type-error
      ) ;unless
      (unless (integer? n)
        (type-error "vector-take-right: second argument must be an integer"
          n
        ) ;type-error
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) (vector))
              ((>= n len) vec)
              (else (vector-copy vec (- len n)))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-drop-right vec n)
      (unless (vector? vec)
        (type-error "vector-drop-right: first argument must be a vector"
          vec
        ) ;type-error
      ) ;unless
      (unless (integer? n)
        (type-error "vector-drop-right: second argument must be an integer"
          n
        ) ;type-error
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) vec)
              ((>= n len) (vector))
              (else (vector-copy vec 0 (- len n)))
        ) ;cond
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
