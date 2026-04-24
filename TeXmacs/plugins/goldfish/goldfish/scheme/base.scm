;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (scheme base)
  (export let-values
    define-values
    define-record-type
    eq?
    eqv?
    equal?
    =
    <
    >
    <=
    >=
    +
    -
    *
    /
    abs
    square
    exact
    inexact
    max
    min
    floor
    floor/
    s7-floor
    ceiling
    s7-ceiling
    truncate
    truncate/
    s7-truncate
    round
    s7-round
    floor-quotient
    floor-remainder
    gcd
    lcm
    s7-lcm
    modulo
    quotient
    remainder
    numerator
    denominator
    rationalize
    exact-integer-sqrt
    number->string
    string->number
    number?
    complex?
    real?
    rational?
    integer?
    exact?
    inexact?
    exact-integer?
    positive?
    negative?
    zero?
    odd?
    even?
    not
    boolean=?
    boolean?
    pair?
    cons
    car
    cdr
    set-car!
    set-cdr!
    caar
    cadr
    cdar
    cddr
    null?
    list?
    make-list
    list
    length
    append
    reverse
    list-tail
    list-ref
    list-set!
    memq
    memv
    member
    assq
    assv
    assoc
    list-copy
    map
    symbol?
    symbol=?
    string->symbol
    symbol->string
    char?
    char=?
    char<?
    char>?
    char<=?
    char>=?
    char->integer
    integer->char
    string?
    make-string
    string
    string-length
    string-ref
    string-set!
    string-copy
    string-append
    substring
    string-fill!
    string->list
    list->string
    string=?
    string<?
    string>?
    string<=?
    string>=?
    vector?
    make-vector
    vector
    vector-length
    vector-ref
    vector-set!
    vector->list
    list->vector
    vector->string
    string->vector
    vector-copy
    vector-copy!
    vector-fill!
    vector-append
    bytevector?
    make-bytevector
    bytevector
    bytevector-length
    bytevector-u8-ref
    bytevector-u8-set!
    bytevector-copy
    bytevector-append
    utf8->string
    string->utf8
    utf8-string-length
    bytevector-advance-utf8
    call-with-port
    port?
    binary-port?
    textual-port?
    input-port-open?
    output-port-open?
    open-binary-input-file
    open-binary-output-file
    close-port
    eof-object
    string-map
    vector-map
    string-for-each
    vector-for-each
    raise
    guard
    read-error?
    file-error?
  ) ;export
  (begin

    (define-macro (let-values vars . body)
      (if (and (pair? vars)
            (pair? (car vars))
            (null? (cdar vars))
          ) ;and
        `((lambda ,(caar vars) ,@body) ,(cadar vars))
        `(with-let (apply sublet (curlet) (list ,@(map (lambda (v) `((lambda ,(car v) (values ,@(map (lambda (name) (values (symbol->keyword name) name)) (let args->proper-list ((args (car v))) (cond ((symbol? args) (list args)) ((not (pair? args)) args) ((pair? (car args)) (cons (caar args) (args->proper-list (cdr args)))) (else (cons (car args) (args->proper-list (cdr args))))))))) ,(cadr v))) vars))) ,@body)
      ) ;if
    ) ;define-macro

    (define-macro (define-values vars expression)
      `(if (not (null? (quote ,vars))) (varlet (curlet) ((lambda ,vars (curlet)) ,expression)))
    ) ;define-macro

    (define-macro (define-record-type
                    type
                    make
                    ?
                    .
                    fields
                  ) ;
      (let ((obj (gensym))
            (typ (gensym))
            (args (map (lambda (field)
                         (values (list 'quote (car field))
                           (let ((par (memq (car field) (cdr make))))
                             (and (pair? par) (car par))
                           ) ;let
                         ) ;values
                       ) ;lambda
                    fields
                  ) ;map
            ) ;args
           ) ;
        `(begin (define (,? ,obj) (and (let? ,obj) (eq? (let-ref ,obj (quote ,typ)) (quote ,type)))) (define ,make (inlet (quote ,typ) (quote ,type) ,@args)) ,@(map (lambda (field) (when (pair? field) (if (null? (cdr field)) (values) (if (null? (cddr field)) `(define (,(cadr field) ,obj) (let-ref ,obj (quote ,(car field)))) `(begin (define (,(cadr field) ,obj) (let-ref ,obj (quote ,(car field)))) (define (,(caddr field) ,obj val) (let-set! ,obj (quote ,(car field)) val))))))) fields) (quote ,type))
      ) ;let
    ) ;define-macro

    (define exact inexact->exact)

    (define inexact exact->inexact)

    (define s7-max max)

    (define (max2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error
          "max: parameter must be real number"
        ) ;error
      ) ;when
      (if (or (inexact? x) (inexact? y))
        (inexact (s7-max x y))
        (s7-max x y)
      ) ;if
    ) ;define

    (define (max x . xs)
      (let loop
        ((current-max x) (remaining xs))
        (if (null? remaining)
          current-max
          (loop (max2 current-max (car remaining))
            (cdr remaining)
          ) ;loop
        ) ;if
      ) ;let
    ) ;define

    (define s7-min min)

    (define (min2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error
          "min: parameter must be real number"
        ) ;error
      ) ;when
      (if (or (inexact? x) (inexact? y))
        (inexact (s7-min x y))
        (s7-min x y)
      ) ;if
    ) ;define

    (define (min x . xs)
      (let loop
        ((current-min x) (remaining xs))
        (if (null? remaining)
          current-min
          (loop (min2 current-min (car remaining))
            (cdr remaining)
          ) ;loop
        ) ;if
      ) ;let
    ) ;define

    (define s7-floor floor)

    (define (floor x)
      (if (inexact? x)
        (inexact (s7-floor x))
        (s7-floor x)
      ) ;if
    ) ;define

    (define s7-ceiling ceiling)

    (define (ceiling x)
      (if (inexact? x)
        (inexact (s7-ceiling x))
        (s7-ceiling x)
      ) ;if
    ) ;define

    (define s7-truncate truncate)

    (define (truncate x)
      (if (inexact? x)
        (inexact (s7-truncate x))
        (s7-truncate x)
      ) ;if
    ) ;define

    (define s7-round round)

    (define (round x)
      (if (inexact? x)
        (inexact (s7-round x))
        (s7-round x)
      ) ;if
    ) ;define

    (define (floor-quotient x y)
      (floor (/ x y))
    ) ;define

    (define (floor/ x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'wrong-type-arg
          "floor/: parameters must be real numbers"
        ) ;error
      ) ;when
      (when (zero? y)
        (error 'division-by-zero
          "floor/: division by zero"
        ) ;error
      ) ;when
      (let ((q (floor (/ x y))) (r (modulo x y)))
        (values q r)
      ) ;let
    ) ;define

    (define (floor-remainder x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error
          "floor-remainder: parameters must be reals"
        ) ;error
      ) ;when
      (when (zero? y)
        (error 'division-by-zero
          "floor-remainder: division by zero"
        ) ;error
      ) ;when
      (modulo x y)
    ) ;define

    (define (truncate/ x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'wrong-type-arg
          "truncate/: parameters must be real numbers"
        ) ;error
      ) ;when
      (when (zero? y)
        (error 'division-by-zero
          "truncate/: division by zero"
        ) ;error
      ) ;when
      (let* ((q (truncate (/ x y)))
             (r (- x (* q y)))
            ) ;
        (values q r)
      ) ;let*
    ) ;define

    (define s7-modulo modulo)

    (define (modulo x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error
          "modulo: parameters must be reals"
        ) ;error
      ) ;when
      (when (zero? y)
        (error 'division-by-zero
          "modulo: division by zero"
        ) ;error
      ) ;when
      (s7-modulo x y)
    ) ;define

    (define s7-lcm lcm)

    (define (lcm2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error
          "lcm: parameters must be reals"
        ) ;error
      ) ;when
      (cond ((and (inexact? x) (exact? y))
             (inexact (s7-lcm (exact x) y))
            ) ;
            ((and (exact? x) (inexact? y))
             (inexact (s7-lcm x (exact y)))
            ) ;
            ((and (inexact? x) (inexact? y))
             (inexact (s7-lcm (exact x) (exact y)))
            ) ;
            (else (s7-lcm x y))
      ) ;cond
    ) ;define

    (define (lcm . args)
      (cond ((null? args) 1)
            ((null? (cdr args)) (lcm2 (car args) 1))
            ((null? (cddr args))
             (lcm2 (car args) (cadr args))
            ) ;
            (else (apply lcm
                    (cons (lcm (car args) (cadr args))
                      (cddr args)
                    ) ;cons
                  ) ;apply
            ) ;else
      ) ;cond
    ) ;define

    (define (square x)
      (* x x)
    ) ;define

    (define (exact-integer-sqrt n)
      (when (not (integer? n))
        (type-error "n must be an integer" n)
      ) ;when
      (when (< n 0)
        (value-error "n must be non-negative" n)
      ) ;when
      (let* ((a (sqrt n))
             (b (inexact->exact (floor a)))
             (square-b (square b))
            ) ;
        (if (= square-b n)
          (values b 0)
          (values b (- n square-b))
        ) ;if
      ) ;let*
    ) ;define

    (define exact-integer? integer?)

    (define (boolean=? obj1 obj2 . rest)
      (define (same-boolean obj rest)
        (if (null? rest)
          #t
          (and (equal? obj (car rest))
            (same-boolean obj (cdr rest))
          ) ;and
        ) ;if
      ) ;define
      (cond ((not (boolean? obj1)) #f)
            ((not (boolean? obj2)) #f)
            ((not (equal? obj1 obj2)) #f)
            (else (same-boolean obj1 rest))
      ) ;cond
    ) ;define

    (define (symbol=? sym1 sym2 . rest)
      (define (same-symbol sym rest)
        (if (null? rest)
          #t
          (and (eq? sym (car rest))
            (same-symbol sym (cdr rest))
          ) ;and
        ) ;if
      ) ;define
      (cond ((not (symbol? sym1)) #f)
            ((not (symbol? sym2)) #f)
            ((not (eq? sym1 sym2)) #f)
            (else (same-symbol sym1 rest))
      ) ;cond
    ) ;define

    (define bytevector byte-vector)

    (define bytevector? byte-vector?)

    (define make-bytevector
      make-byte-vector
    ) ;define

    (define bytevector-length length)

    (define bytevector-u8-ref
      byte-vector-ref
    ) ;define

    (define bytevector-u8-set!
      byte-vector-set!
    ) ;define

    (define* (bytevector-copy v
               (start 0)
               (end (bytevector-length v))
             ) ;bytevector-copy
      (if (or (< start 0)
            (> start end)
            (> end (bytevector-length v))
          ) ;or
        (error 'out-of-range "bytevector-copy")
      ) ;if
      (let ((new-v (make-bytevector (- end start)))
           ) ;
        (let loop
          ((i start) (j 0))
          (if (>= i end)
            new-v
            (begin
              (bytevector-u8-set! new-v
                j
                (bytevector-u8-ref v i)
              ) ;bytevector-u8-set!
              (loop (+ i 1) (+ j 1))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define*

    (define bytevector-append append)

    (define* (bytevector-advance-utf8 bv
               index
               (end (length bv))
             ) ;bytevector-advance-utf8
      (if (>= index end)
        index
        (let ((byte (bv index)))
          (cond
            ;; 1-byte sequence (0xxxxxxx)
            ((< byte 128) (+ index 1))

            ;; 2-byte sequence (110xxxxx 10xxxxxx)
            ((< byte 224)
             (if (>= (+ index 1) end)
               index
               (let ((next-byte (bv (+ index 1))))
                 (if (not (= (logand next-byte 192) 128))
                   index
                   (+ index 2)
                 ) ;if
               ) ;let
             ) ;if
            ) ;

            ;; 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            ((< byte 240)
             (if (>= (+ index 2) end)
               index
               (let ((next-byte1 (bv (+ index 1)))
                     (next-byte2 (bv (+ index 2)))
                    ) ;
                 (if (or (not (= (logand next-byte1 192) 128))
                       (not (= (logand next-byte2 192) 128))
                     ) ;or
                   index
                   (+ index 3)
                 ) ;if
               ) ;let
             ) ;if
            ) ;

            ;; 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            ((< byte 248)
             (if (>= (+ index 3) end)
               index
               (let ((next-byte1 (bv (+ index 1)))
                     (next-byte2 (bv (+ index 2)))
                     (next-byte3 (bv (+ index 3)))
                    ) ;
                 (if (or (not (= (logand next-byte1 192) 128))
                       (not (= (logand next-byte2 192) 128))
                       (not (= (logand next-byte3 192) 128))
                     ) ;or
                   index
                   (+ index 4)
                 ) ;if
               ) ;let
             ) ;if
            ) ;
            (else index)
          ) ;cond
        ) ;let
      ) ;if
    ) ;define*

    (define (utf8-string-length str)
      (let ((bv (string->byte-vector str))
            (N (string-length str))
           ) ;
        (if (zero? N)
          0
          (let loop
            ((pos 0) (cnt 0))
            (let ((next-pos (bytevector-advance-utf8 bv pos N)
                  ) ;next-pos
                 ) ;
              (cond ((= next-pos N) (+ cnt 1))
                    ((= next-pos pos)
                     (error 'value-error
                       "Invalid UTF-8 sequence at index: "
                       pos
                     ) ;error
                    ) ;
                    (else (loop next-pos (+ cnt 1)))
              ) ;cond
            ) ;let
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (utf8->string bv
               (start 0)
               (end (bytevector-length bv))
             ) ;utf8->string
      (if (or (< start 0)
            (> end (bytevector-length bv))
            (> start end)
          ) ;or
        (error 'out-of-range start end)
        (let loop
          ((pos start))
          (let ((next-pos (bytevector-advance-utf8 bv pos end)
                ) ;next-pos
               ) ;
            (cond ((= next-pos end)
                   (copy bv
                     (make-string (- end start))
                     start
                     end
                   ) ;copy
                  ) ;
                  ((= next-pos pos)
                   (error 'value-error
                     "Invalid UTF-8 sequence at index: "
                     pos
                   ) ;error
                  ) ;
                  (else (loop next-pos))
            ) ;cond
          ) ;let
        ) ;let
      ) ;if
    ) ;define*

    (define* (string->utf8 str (start 0) (end #t))
      (define (string->utf8-sub str start end)
        (let ((bv (string->byte-vector str))
              (N (string-length str))
             ) ;
          (let loop
            ((pos 0) (cnt 0) (start-pos 0))
            (let ((next-pos (bytevector-advance-utf8 bv pos N)
                  ) ;next-pos
                 ) ;
              (cond ((and (not (zero? start))
                       (zero? start-pos)
                       (= cnt start)
                     ) ;and
                     (loop next-pos (+ cnt 1) pos)
                    ) ;
                    ((and (integer? end) (= cnt end))
                     (copy bv
                       (make-byte-vector (- pos start-pos))
                       start-pos
                       pos
                     ) ;copy
                    ) ;
                    ((and end (= next-pos N))
                     (copy bv
                       (make-byte-vector (- N start-pos))
                       start-pos
                       N
                     ) ;copy
                    ) ;
                    ((= next-pos pos)
                     (error 'value-error
                       "Invalid UTF-8 sequence at index: "
                       pos
                     ) ;error
                    ) ;
                    (else (loop next-pos (+ cnt 1) start-pos)
                    ) ;else
              ) ;cond
            ) ;let
          ) ;let
        ) ;let
      ) ;define

      (when (not (string? str))
        (error 'type-error "str must be string")
      ) ;when
      (let ((N (utf8-string-length str)))
        (when (and (> N 0)
                (or (< start 0) (>= start N))
              ) ;and
          (error 'out-of-range
            (string-append "start must >= 0 and < "
              (number->string N)
            ) ;string-append
          ) ;error
        ) ;when
        (when (and (integer? end)
                (or (< end 0) (>= end (+ N 1)))
              ) ;and
          (error 'out-of-range
            (string-append "end must >= 0 and < "
              (number->string (+ N 1))
            ) ;string-append
          ) ;error
        ) ;when
        (when (and (integer? end) (> start end))
          (error 'out-of-range
            "start <= end failed"
            start
            end
          ) ;error
        ) ;when

        (if (and (integer? end) (= start end))
          (byte-vector)
          (string->utf8-sub str start end)
        ) ;if
      ) ;let
    ) ;define*

    (define (raise . args)
      (apply throw #t args)
    ) ;define

    (define-macro (guard results . body)
      `(let ((,(car results) (catch ,#t (lambda ,() ,@body) (lambda (type info) (if (pair? (*s7* (#_quote catches))) (lambda () (apply throw type info)) (car info)))))) (cond ,@(cdr results) (else (if (procedure? ,(car results)) (,(car results)) ,(car results)))))
    ) ;define-macro

    (define (read-error? obj)
      (eq? (car obj) 'read-error)
    ) ;define

    (define (file-error? obj)
      (eq? (car obj) 'io-error)
    ) ;define

    (define (call-with-port port proc)
      (let ((res (proc port)))
        (if res (close-port port))
        res
      ) ;let
    ) ;define

    (define (port? p)
      (or (input-port? p) (output-port? p))
    ) ;define

    (define textual-port? port?)

    (define binary-port? port?)

    (define (input-port-open? p)
      (not (port-closed? p))
    ) ;define

    (define (output-port-open? p)
      (not (port-closed? p))
    ) ;define

    (define (close-port p)
      (if (input-port? p)
        (close-input-port p)
        (close-output-port p)
      ) ;if
    ) ;define

    (define (eof-object)
      #<eof>
    ) ;define

    (define list-copy copy)

    (define (string-copy str . start_end)
      (cond ((null? start_end) (substring str 0))
            ((= (length start_end) 1)
             (substring str (car start_end))
            ) ;
            ((= (length start_end) 2)
             (substring str
               (car start_end)
               (cadr start_end)
             ) ;substring
            ) ;
            (else (error 'wrong-number-of-args))
      ) ;cond
    ) ;define

    (define (string-map p . args)
      (apply string (apply map p args))
    ) ;define

    (define string-for-each for-each)

    (define* (vector-copy v
               (start 0)
               (end (vector-length v))
             ) ;vector-copy
      (if (or (> start end)
            (> end (vector-length v))
          ) ;or
        (error 'out-of-range "vector-copy")
        (let ((new-v (make-vector (- end start))))
          (let loop
            ((i start) (j 0))
            (if (>= i end)
              new-v
              (begin
                (vector-set! new-v j (vector-ref v i))
                (loop (+ i 1) (+ j 1))
              ) ;begin
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define*

    (define (vector-map p . args)
      (apply vector (apply map p args))
    ) ;define

    (define vector-for-each for-each)

    (define vector-fill! fill!)

    (define* (vector-copy! to
               at
               from
               (start 0)
               (end (vector-length from))
             ) ;vector-copy!
      (if (or (< at 0)
            (> start (vector-length from))
            (< end 0)
            (> end (vector-length from))
            (> start end)
            (> (+ at (- end start))
              (vector-length to)
            ) ;>
          ) ;or
        (error 'out-of-range "vector-copy!")
        (let loop
          ((to-i at) (from-i start))
          (if (>= from-i end)
            to
            (begin
              (vector-set! to
                to-i
                (vector-ref from from-i)
              ) ;vector-set!
              (loop (+ to-i 1) (+ from-i 1))
            ) ;begin
          ) ;if
        ) ;let
      ) ;if
    ) ;define*

    (define* (vector->string v (start 0) end)
      (let ((stop (or end (length v))))
        (copy v
          (make-string (- stop start))
          start
          stop
        ) ;copy
      ) ;let
    ) ;define*

    (define* (string->vector s (start 0) end)
      (let ((stop (or end (length s))))
        (copy s
          (make-vector (- stop start))
          start
          stop
        ) ;copy
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library
