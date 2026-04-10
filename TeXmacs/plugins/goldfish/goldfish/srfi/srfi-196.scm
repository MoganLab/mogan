;
; Copyright (C) 2020 Wolfgang Corcoran-Mathe
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to
; permit persons to whom the Software is furnished to do so, subject to
; the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
; SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;

(define-library (srfi srfi-196)
  (import (scheme base)
          (scheme case-lambda)
  ) ;import
  (export range numeric-range vector-range string-range range-append
          iota-range range? range=? range-length range-ref range-first
          range-last subrange range-segment range-split-at range-take
          range-take-right range-drop range-drop-right range-count
          range-map->list range-for-each range-fold range-fold-right
          range-any range-every range-filter->list range-remove->list
          range-reverse range-map->vector range-filter->vector
          range-remove->vector vector->range range->list range->vector
          range->string range->generator
  ) ;export
  (begin

    ;;; Utilities

    (define (exact-natural? x)
      (and (exact-integer? x) (not (negative? x)))
    ) ;define

    ; Find the least element of a list non-empty of naturals.
    (define (short-minimum ns)
      (let loop ((ns ns) (min-val +inf.0))
        (if (null? ns)
            min-val
            (let ((n (car ns)))
              (if (zero? n)
                  0
                  (loop (cdr ns) (if (< n min-val) n min-val))
              ) ;if
            ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;;; Range record type

    (define-record-type <range>
      (raw-range start-index length indexer complexity)
      range?
      (start-index range-start-index)
      (length range-length)
      (indexer range-indexer)
      (complexity range-complexity)
    ) ;define-record-type

    ; Maximum number of indexers to compose before vectorization
    (define %range-maximum-complexity 16)

    ; Returns an empty range which is otherwise identical to r.
    (define (%empty-range-from r)
      (raw-range (range-start-index r) 0 (range-indexer r) (range-complexity r))
    ) ;define

    (define (threshold? k)
      (> k %range-maximum-complexity)
    ) ;define

    (define (%range-valid-index? r index)
      (and (exact-natural? index)
           (< index (range-length r))
      ) ;and
    ) ;define

    (define (%range-valid-bound? r bound)
      (and (exact-natural? bound)
           (<= bound (range-length r))
      ) ;and
    ) ;define

    ;;; Constructors

    (define (range length indexer)
      (raw-range 0 length indexer 0)
    ) ;define

    (define numeric-range
      (case-lambda
        ((start end) (numeric-range start end 1))
        ((start end step)
         (let ((len (exact (ceiling (max 0 (/ (- end start) step))))))
           (raw-range 0 len (lambda (n) (+ start (* n step))) 0)
         ) ;let
        ) ;
      ) ;case-lambda
    ) ;define

    (define iota-range
      (case-lambda
        ((len) (iota-range len 0 1))
        ((len start) (iota-range len start 1))
        ((len start step)
         (raw-range 0
                    len
                    (cond ((and (zero? start) (= step 1)) (lambda (i) i))
                          ((= step 1) (lambda (i) (+ start i)))
                          ((zero? start) (lambda (i) (* step i)))
                          (else (lambda (i) (+ start (* step i))))
                    ) ;cond
                    0
         ) ;raw-range
        ) ;
      ) ;case-lambda
    ) ;define

    (define (vector-range vec)
      (raw-range 0 (vector-length vec) (lambda (i) (vector-ref vec i)) 0)
    ) ;define

    (define (string-range s)
      (vector-range (string->vector s))
    ) ;define

    (define (%range-maybe-vectorize r)
      (if (threshold? (range-complexity r))
          (vector-range (range->vector r))
          r
      ) ;if
    ) ;define

    ;;; Accessors

    (define (range-ref r index)
      ((range-indexer r) (+ index (range-start-index r)))
    ) ;define

    (define (%range-ref-no-check r index)
      ((range-indexer r) (+ index (range-start-index r)))
    ) ;define

    (define (range-first r)
      (%range-ref-no-check r 0)
    ) ;define

    (define (range-last r)
      (%range-ref-no-check r (- (range-length r) 1))
    ) ;define

    ;;; Predicates

    (define range=?
      (case-lambda
        ((equal ra rb)
         (%range=?-2 equal ra rb)
        ) ;
        ((equal . rs)
         (let ((ra (car rs)))
           (every (lambda (rb) (%range=?-2 equal ra rb)) (cdr rs))
         ) ;let
        ) ;
      ) ;case-lambda
    ) ;define

    (define (%range=?-2 equal ra rb)
      (or (eqv? ra rb)
          (let ((la (range-length ra)))
            (and (= la (range-length rb))
                 (if (zero? la)
                     #t
                     (let lp ((i 0))
                       (cond ((= i la) #t)
                             ((not (equal (range-ref ra i)
                                          (range-ref rb i))
                              ) ;not
                              #f
                             ) ;
                             (else (lp (+ i 1)))
                       ) ;cond
                     ) ;let
                 ) ;if
            ) ;and
          ) ;let
      ) ;or
    ) ;define

    ;;; Iteration

    (define (range-split-at r index)
      (cond ((= index 0) (values (%empty-range-from r) r))
            ((= index (range-length r)) (values r (%empty-range-from r)))
            (else
             (let ((indexer (range-indexer r)) (k (range-complexity r)))
               (values (raw-range (range-start-index r) index indexer k)
                       (raw-range index (- (range-length r) index) indexer k)
               ) ;values
             ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (subrange r start end)
      (if (and (zero? start) (= end (range-length r)))
          r
          (raw-range (+ (range-start-index r) start)
                     (- end start)
                     (range-indexer r)
                     (range-complexity r)
          ) ;raw-range
      ) ;if
    ) ;define

    (define (range-segment r k)
      (let
        ((len (range-length r))
         (%subrange-no-check
          (lambda (s e)
            (raw-range (+ (range-start-index r) s)
                       (- e s)
                       (range-indexer r)
                       (range-complexity r))
            ) ;raw-range
          ) ;lambda
         ) ;%subrange-no-check
        (let loop ((i 0) (result '()))
          (if (>= i len)
              (reverse result)
              (loop (+ i k) (cons (%subrange-no-check i (min len (+ i k))) result))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (range-take r count)
      (cond ((zero? count) (%empty-range-from r))
            ((= count (range-length r)) r)
            (else
             (raw-range (range-start-index r)
                        count
                        (range-indexer r)
                        (range-complexity r)
             ) ;raw-range
            ) ;else
      ) ;cond
    ) ;define

    (define (range-take-right r count)
      (cond ((zero? count) (%empty-range-from r))
            ((= count (range-length r)) r)
            (else
             (raw-range (+ (range-start-index r) (- (range-length r) count))
                        count
                        (range-indexer r)
                        (range-complexity r)
             ) ;raw-range
            ) ;else
      ) ;cond
    ) ;define

    (define (range-drop r count)
      (if (zero? count)
          r
          (raw-range (+ (range-start-index r) count)
                     (- (range-length r) count)
                     (range-indexer r)
                     (range-complexity r)
          ) ;raw-range
      ) ;if
    ) ;define

    (define (range-drop-right r count)
      (if (zero? count)
          r
          (raw-range (range-start-index r)
                     (- (range-length r) count)
                     (range-indexer r)
                     (range-complexity r)
          ) ;raw-range
      ) ;if
    ) ;define

    (define (range-count pred r . rs)
      (if (null? rs)
          (%range-fold-1 (lambda (c x) (if (pred x) (+ c 1) c)) 0 r)
          (apply range-fold
                 (lambda (c . xs)
                   (if (apply pred xs) (+ c 1) c)
                 ) ;lambda
                 0
                 r
                 rs
          ) ;apply
      ) ;if
    ) ;define

    (define (range-map->list proc r . rs)
      (if (null? rs)
          (%range-fold-right-1 (lambda (x res) (cons (proc x) res)) '() r)
          (apply range-fold-right
                 (lambda (x . xs-res) (cons (apply proc x (butlast xs-res)) (last xs-res)))
                 '()
                 r
                 rs
          ) ;apply
      ) ;if
    ) ;define

    (define (range-for-each proc r . rs)
      (if (null? rs)
          (let ((len (range-length r)))
            (let lp ((i 0))
              (cond ((= i len) (if #f #f))
                    (else (proc (%range-ref-no-check r i))
                          (lp (+ i 1))
                    ) ;else
              ) ;cond
            ) ;let
          ) ;let
          (let* ((rs* (cons r rs))
                 (len (short-minimum (map range-length rs*))))
            (let lp ((i 0))
              (cond ((= i len) (if #f #f))
                    (else
                     (apply proc (map (lambda (r)
                                        (%range-ref-no-check r i))
                                      rs*)
                     ) ;apply
                     (lp (+ i 1))
                    ) ;else
              ) ;cond
            ) ;let
          ) ;let*
      ) ;if
    ) ;define

    (define (%range-fold-1 proc nil r)
      (let ((len (range-length r)))
        (let lp ((i 0) (acc nil))
          (if (= i len)
              acc
              (lp (+ i 1) (proc acc (%range-ref-no-check r i)))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define range-fold
      (case-lambda
        ((proc nil r)
         (%range-fold-1 proc nil r)
        ) ;
        ((proc nil . rs)
         (let ((len (short-minimum (map range-length rs))))
           (let lp ((i 0) (acc nil))
             (if (= i len)
                 acc
                 (lp (+ i 1)
                     (apply proc acc (map (lambda (r)
                                            (%range-ref-no-check r i))
                                          rs)
                     ) ;apply
                 ) ;lp
             ) ;if
           ) ;let
         ) ;let
        ) ;
      ) ;case-lambda
    ) ;define

    (define (%range-fold-right-1 proc nil r)
      (let ((len (range-length r)))
        (let rec ((i 0))
          (if (= i len)
              nil
              (proc (%range-ref-no-check r i) (rec (+ i 1)))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define range-fold-right
      (case-lambda
        ((proc nil r)
         (%range-fold-right-1 proc nil r)
        ) ;
        ((proc nil . rs)
         (let ((len (short-minimum (map range-length rs))))
           (let rec ((i 0))
             (if (= i len)
                 nil
                 (apply proc
                        (append (map (lambda (r) (%range-ref-no-check r i)) rs)
                                (list (rec (+ i 1)))
                        ) ;append
                 ) ;apply
             ) ;if
           ) ;let
         ) ;let
        ) ;
      ) ;case-lambda
    ) ;define

    (define (range-any pred r . rs)
      (if (null? rs)
          (let ((len (range-length r)))
            (let lp ((i 0))
              (cond ((= i len) #f)
                    ((pred (%range-ref-no-check r i)))
                    (else (lp (+ i 1)))
              ) ;cond
            ) ;let
          ) ;let
          (let* ((rs* (cons r rs))
                 (len (short-minimum (map range-length rs*))))
            (let lp ((i 0))
              (cond ((= i len) #f)
                    ((apply pred (map (lambda (r) (%range-ref-no-check r i)) rs*)))
                    (else (lp (+ i 1)))
              ) ;cond
            ) ;let
          ) ;let*
      ) ;if
    ) ;define

    (define (range-every pred r . rs)
      (if (null? rs)
          (let ((len (range-length r)))
            (let lp ((i 0))
              (cond ((= i len) #t)
                    ((not (pred (%range-ref-no-check r i))) #f)
                    (else (lp (+ i 1)))
              ) ;cond
            ) ;let
          ) ;let
          (let* ((rs* (cons r rs))
                 (len (short-minimum (map range-length rs*))))
            (let lp ((i 0))
              (cond ((= i len) #t)
                    ((not (apply pred (map (lambda (r) (%range-ref-no-check r i)) rs*))) #f)
                    (else (lp (+ i 1)))
              ) ;cond
            ) ;let
          ) ;let*
      ) ;if
    ) ;define

    (define (range-filter->list pred r)
      (range-fold-right (lambda (x xs)
                          (if (pred x) (cons x xs) xs))
                        '()
                        r
      ) ;range-fold-right
    ) ;define

    (define (range-remove->list pred r)
      (range-fold-right (lambda (x xs)
                          (if (pred x) xs (cons x xs)))
                        '()
                        r
      ) ;range-fold-right
    ) ;define

    (define (range-reverse r)
      (%range-maybe-vectorize
       (raw-range (range-start-index r)
                  (range-length r)
                  (lambda (n)
                    ((range-indexer r) (- (range-length r) 1 n))
                  ) ;lambda
                  (+ 1 (range-complexity r))
       ) ;raw-range
      ) ;%range-maybe-vectorize
    ) ;define

    (define range-append
      (case-lambda
        (() (raw-range 0 0 (lambda (i) i) 0))
        ((r) r)
        ((ra rb)
         (let ((la (range-length ra))
               (lb (range-length rb)))
           (%range-maybe-vectorize
            (raw-range 0
                       (+ la lb)
                       (lambda (i)
                         (if (< i la)
                             (%range-ref-no-check ra i)
                             (%range-ref-no-check rb (- i la))
                         ) ;if
                       ) ;lambda
                       (+ 2 (range-complexity ra) (range-complexity rb))
            ) ;raw-range
           ) ;%range-maybe-vectorize
         ) ;let
        ) ;
        (rs
         (let ((lens (map range-length rs)))
           (%range-maybe-vectorize
            (raw-range 0
                       (apply + lens)
                       (lambda (i)
                         (let lp ((i i) (rs rs) (lens lens))
                           (if (< i (car lens))
                               (%range-ref-no-check (car rs) i)
                               (lp (- i (car lens)) (cdr rs) (cdr lens))
                           ) ;if
                         ) ;let
                       ) ;lambda
                       (+ (length rs) (apply + (map range-complexity rs)))
            ) ;raw-range
           ) ;%range-maybe-vectorize
         ) ;let
        ) ;rs
      ) ;case-lambda
    ) ;define

    ;;; Conversion

    (define (range->list r)
      (range-fold-right cons '() r)
    ) ;define

    (define (range->vector r)
      (let ((len (range-length r)))
        (let ((vec (make-vector len)))
          (let lp ((i 0))
            (cond ((= i len) vec)
                  (else
                   (vector-set! vec i (%range-ref-no-check r i))
                   (lp (+ i 1))
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (range->string r)
      (let ((res (make-string (range-length r))))
        (range-fold (lambda (i c) (string-set! res i c) (+ i 1)) 0 r)
        res
      ) ;let
    ) ;define

    (define (vector->range vec)
      (vector-range (vector-copy vec))
    ) ;define

    (define (range->generator r)
      (let ((i 0) (len (range-length r)))
        (lambda ()
          (if (>= i len)
              (eof-object)
              (begin
                (let ((v (%range-ref-no-check r i)))
                  (set! i (+ i 1))
                  v
                ) ;let
              ) ;begin
          ) ;if
        ) ;lambda
      ) ;let
    ) ;define

    ;;; Vector versions (not in original SRFI but useful)

    (define (range-map->vector proc r . rs)
      (if (null? rs)
          (let ((len (range-length r)))
            (let ((vec (make-vector len)))
              (let lp ((i 0))
                (cond ((= i len) vec)
                      (else
                       (vector-set! vec i (proc (%range-ref-no-check r i)))
                       (lp (+ i 1))
                      ) ;else
                ) ;cond
              ) ;let
            ) ;let
          ) ;let
          (let* ((rs* (cons r rs))
                 (len (short-minimum (map range-length rs*))))
            (let ((vec (make-vector len)))
              (let lp ((i 0))
                (cond ((= i len) vec)
                      (else
                       (vector-set! vec i (apply proc (map (lambda (r)
                                                             (%range-ref-no-check r i))
                                                           rs*))
                       ) ;vector-set!
                       (lp (+ i 1))
                      ) ;else
                ) ;cond
              ) ;let
            ) ;let
          ) ;let*
      ) ;if
    ) ;define

    (define (range-filter->vector pred r)
      (list->vector (range-filter->list pred r))
    ) ;define

    (define (range-remove->vector pred r)
      (list->vector (range-remove->list pred r))
    ) ;define

  ) ;begin
) ;define-library
