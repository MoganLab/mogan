;;
;; Copyright (C) 2024-2026 The Goldfish Scheme Authors
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
    ;; R7RS 5: Program Structure
    define-values
    define-record-type
    ;; R7RS 6.1: Equivalence predicates
    eqv?
    eq?
    equal?
    ;; R7RS 6.2: Numbers
    ;; - 比较和算术
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
    ;; - 数值函数
    square
    exact
    inexact
    max
    min
    floor
    floor/
    ceiling
    truncate
    truncate/
    round
    floor-quotient
    floor-remainder
    gcd
    lcm
    modulo
    quotient
    remainder
    numerator
    denominator
    rationalize
    exact-integer-sqrt
    number->string
    string->number
    ;; - 类型判断
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
    ;; R7RS 6.3: Booleans
    not
    boolean=?
    boolean?
    ;; R7RS 6.4: list
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
    for-each
    ;; R7RS 6.5: Symbol
    symbol?
    symbol=?
    string->symbol
    symbol->string
    ;; R7RS 6.6: Characters
    char?
    char=?
    char<?
    char>?
    char<=?
    char>=?
    char->integer
    integer->char
    ;; R7RS 6.7: String
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
    ;; R7RS 6.8: Vector
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
    ;; R7RS 6.9: Bytevectors
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
    ;; Input and Output
    call-with-port
    call-with-input-file
    call-with-output-file
    input-port?
    output-port?
    port?
    binary-port?
    textual-port?
    input-port-open?
    output-port-open?
    current-input-port
    current-output-port
    current-error-port
    open-input-file
    open-output-file
    open-binary-input-file
    open-binary-output-file
    close-port
    close-input-port
    close-output-port
    open-input-string
    open-output-string
    get-output-string
    read-char
    peek-char
    read-line
    read-string
    read
    write-char
    newline
    flush-output-port
    eof-object?
    eof-object
    char-ready?
    with-input-from-file
    with-output-to-file
    ;; Control flow
    procedure?
    apply
    string-map
    vector-map
    string-for-each
    vector-for-each
    call-with-current-continuation
    call/cc
    values
    call-with-values
    dynamic-wind
    ;; Exception
    raise
    guard
    read-error?
    file-error?
  ) ;export
  (begin

    ;; 0-clause BSD
    ;; Bill Schottstaedt
    ;; from S7 source repo: r7rs.scm
    (define-macro (let-values vars . body)
      (if (and (pair? vars) (pair? (car vars)) (null? (cdar vars)))
        `((lambda ,(caar vars) ,@body) ,(cadar vars))
        `(with-let (apply sublet
                     (curlet)
                     (list ,@(map (lambda (v)
                                    `((lambda ,(car v)
                                        (values ,@(map (lambda (name)
                                                         (values (symbol->keyword name)
                                                           name))
                                                    (let args->proper-list
                                                      ((args (car v)))
                                                      (cond ((symbol? args)
                                                             (list args))
                                                            ((not (pair? args))
                                                             args)
                                                            ((pair? (car args))
                                                             (cons (caar args)
                                                               (args->proper-list (cdr args))))
                                                            (else (cons (car args)
                                                                    (args->proper-list (cdr args)))))))))
                                      ,(cadr v)))
                               vars)))
           ,@body)
      ) ;if
    ) ;define-macro

    (define-macro (define-values vars expression)
      `(if (not (null? (quote ,vars)))
         (varlet (curlet) ((lambda ,vars (curlet)) ,expression)))
    ) ;define-macro

    (define-macro (define-record-type type make ? . fields)
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
        `(begin
           (define (,? ,obj)
             (and (let? ,obj) (eq? (let-ref ,obj (quote ,typ)) (quote ,type))))
           (define ,make (inlet (quote ,typ) (quote ,type) ,@args))
           ,@(map (lambda (field)
                    (when (pair? field)
                      (if (null? (cdr field))
                        (values)
                        (if (null? (cddr field))
                          `(define (,(cadr field) ,obj)
                             (let-ref ,obj (quote ,(car field))))
                          `(begin
                             (define (,(cadr field) ,obj)
                               (let-ref ,obj (quote ,(car field))))
                             (define (,(caddr field) ,obj val)
                               (let-set! ,obj (quote ,(car field)) val)))))))
               fields)
           (quote ,type))
      ) ;let
    ) ;define-macro

    (define exact inexact->exact)

    (define inexact exact->inexact)

    (define (max2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error "max: parameter must be real number")
      ) ;when
      (if (or (inexact? x) (inexact? y)) (inexact (s7-max x y)) (s7-max x y))
    ) ;define

    (define (max x . xs)
      (let loop
        ((current-max x) (remaining xs))
        (if (null? remaining)
          current-max
          (loop (max2 current-max (car remaining)) (cdr remaining))
        ) ;if
      ) ;let
    ) ;define

    (define (min2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error "min: parameter must be real number")
      ) ;when
      (if (or (inexact? x) (inexact? y)) (inexact (s7-min x y)) (s7-min x y))
    ) ;define

    (define (min x . xs)
      (let loop
        ((current-min x) (remaining xs))
        (if (null? remaining)
          current-min
          (loop (min2 current-min (car remaining)) (cdr remaining))
        ) ;if
      ) ;let
    ) ;define

    (define (floor x)
      (if (inexact? x) (inexact (s7-floor x)) (s7-floor x))
    ) ;define

    (define (ceiling x)
      (if (inexact? x) (inexact (s7-ceiling x)) (s7-ceiling x))
    ) ;define

    (define (truncate x)
      (if (inexact? x) (inexact (s7-truncate x)) (s7-truncate x))
    ) ;define

    (define (round x)
      (if (inexact? x) (inexact (s7-round x)) (s7-round x))
    ) ;define

    (define (floor-quotient x y)
      (floor (/ x y))
    ) ;define

    (define (floor/ x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'wrong-type-arg "floor/: parameters must be real numbers")
      ) ;when
      (when (zero? y)
        (error 'division-by-zero "floor/: division by zero")
      ) ;when
      (let ((q (floor (/ x y))) (r (modulo x y)))
        (values q r)
      ) ;let
    ) ;define

    (define (floor-remainder x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error "floor-remainder: parameters must be reals")
      ) ;when
      (when (zero? y)
        (error 'division-by-zero "floor-remainder: division by zero")
      ) ;when
      (modulo x y)
    ) ;define

    (define (truncate/ x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'wrong-type-arg "truncate/: parameters must be real numbers")
      ) ;when
      (when (zero? y)
        (error 'division-by-zero "truncate/: division by zero")
      ) ;when
      (let* ((q (truncate (/ x y))) (r (- x (* q y))))
        (values q r)
      ) ;let*
    ) ;define

    (define (modulo x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error "modulo: parameters must be reals")
      ) ;when
      (when (zero? y)
        (error 'division-by-zero "modulo: division by zero")
      ) ;when
      (s7-modulo x y)
    ) ;define

    (define (lcm2 x y)
      (when (or (not (real? x)) (not (real? y)))
        (error 'type-error "lcm: parameters must be reals")
      ) ;when
      (cond ((and (inexact? x) (exact? y)) (inexact (s7-lcm (exact x) y)))
            ((and (exact? x) (inexact? y)) (inexact (s7-lcm x (exact y))))
            ((and (inexact? x) (inexact? y)) (inexact (s7-lcm (exact x) (exact y))))
            (else (s7-lcm x y))
      ) ;cond
    ) ;define

    (define (lcm . args)
      (cond ((null? args) 1)
            ((null? (cdr args)) (lcm2 (car args) 1))
            ((null? (cddr args)) (lcm2 (car args) (cadr args)))
            (else (apply lcm (cons (lcm (car args) (cadr args)) (cddr args))))
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
      (let* ((a (sqrt n)) (b (inexact->exact (floor a))) (square-b (square b)))
        (if (= square-b n) (values b 0) (values b (- n square-b)))
      ) ;let*
    ) ;define

    (define exact-integer? integer?)

    (define (boolean=? obj1 obj2 . rest)
      (define (same-boolean obj rest)
        (if (null? rest) #t (and (equal? obj (car rest)) (same-boolean obj (cdr rest))))
      ) ;define
      (cond ((not (boolean? obj1)) #f)
            ((not (boolean? obj2)) #f)
            ((not (equal? obj1 obj2)) #f)
            (else (same-boolean obj1 rest))
      ) ;cond
    ) ;define

    (define (symbol=? sym1 sym2 . rest)
      (define (same-symbol sym rest)
        (if (null? rest) #t (and (eq? sym (car rest)) (same-symbol sym (cdr rest))))
      ) ;define
      (cond ((not (symbol? sym1)) #f)
            ((not (symbol? sym2)) #f)
            ((not (eq? sym1 sym2)) #f)
            (else (same-symbol sym1 rest))
      ) ;cond
    ) ;define

    (define bytevector byte-vector)

    (define bytevector? byte-vector?)

    (define make-bytevector make-byte-vector)

    (define bytevector-length length)

    (define bytevector-u8-ref byte-vector-ref)

    (define bytevector-u8-set! byte-vector-set!)

    (define* (bytevector-copy v (start 0) (end (bytevector-length v)))
      (if (or (< start 0) (> start end) (> end (bytevector-length v)))
        (error 'out-of-range "bytevector-copy")
      ) ;if
      (let ((new-v (make-bytevector (- end start))))
        (let loop
          ((i start) (j 0))
          (if (>= i end)
            new-v
            (begin
              (bytevector-u8-set! new-v j (bytevector-u8-ref v i))
              (loop (+ i 1) (+ j 1))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define*

    (define bytevector-append append)

    (define* (bytevector-advance-utf8 bv index (end (length bv)))
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
                 (if (not (= (logand next-byte 192) 128)) index (+ index 2))
               ) ;let
             ) ;if
            ) ;

            ;; 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
            ((< byte 240)
             (if (>= (+ index 2) end)
               index
               (let ((next-byte1 (bv (+ index 1))) (next-byte2 (bv (+ index 2))))
                 (if (or (not (= (logand next-byte1 192) 128)) (not (= (logand next-byte2 192) 128)))
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
      (let ((bv (string->byte-vector str)) (N (string-length str)))
        (if (zero? N)
          0
          (let loop
            ((pos 0) (cnt 0))
            (let ((next-pos (bytevector-advance-utf8 bv pos N)))
              (cond ((= next-pos N) (+ cnt 1))
                    ((= next-pos pos) (error 'value-error "Invalid UTF-8 sequence at index: " pos))
                    (else (loop next-pos (+ cnt 1)))
              ) ;cond
            ) ;let
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (utf8->string bv (start 0) (end (bytevector-length bv)))
      (if (or (< start 0) (> end (bytevector-length bv)) (> start end))
        (error 'out-of-range start end)
        (let loop
          ((pos start))
          (let ((next-pos (bytevector-advance-utf8 bv pos end)))
            (cond ((= next-pos end) (copy bv (make-string (- end start)) start end))
                  ((= next-pos pos) (error 'value-error "Invalid UTF-8 sequence at index: " pos))
                  (else (loop next-pos))
            ) ;cond
          ) ;let
        ) ;let
      ) ;if
    ) ;define*

    (define* (string->utf8 str (start 0) (end #t))
      ;; start < end in this case
      (define (string->utf8-sub str start end)
        (let ((bv (string->byte-vector str)) (N (string-length str)))
          (let loop
            ((pos 0) (cnt 0) (start-pos 0))
            (let ((next-pos (bytevector-advance-utf8 bv pos N)))
              (cond ((and (not (zero? start)) (zero? start-pos) (= cnt start))
                     (loop next-pos (+ cnt 1) pos)
                    ) ;
                    ((and (integer? end) (= cnt end))
                     (copy bv (make-byte-vector (- pos start-pos)) start-pos pos)
                    ) ;
                    ((and end (= next-pos N))
                     (copy bv (make-byte-vector (- N start-pos)) start-pos N)
                    ) ;
                    ((= next-pos pos) (error 'value-error "Invalid UTF-8 sequence at index: " pos))
                    (else (loop next-pos (+ cnt 1) start-pos))
              ) ;cond
            ) ;let
          ) ;let
        ) ;let
      ) ;define

      (when (not (string? str))
        (error 'type-error "str must be string")
      ) ;when
      (let ((N (utf8-string-length str)))
        (when (and (> N 0) (or (< start 0) (>= start N)))
          (error 'out-of-range
            (string-append "start must >= 0 and < " (number->string N))
          ) ;error
        ) ;when
        (when (and (integer? end) (or (< end 0) (>= end (+ N 1))))
          (error 'out-of-range
            (string-append "end must >= 0 and < " (number->string (+ N 1)))
          ) ;error
        ) ;when
        (when (and (integer? end) (> start end))
          (error 'out-of-range "start <= end failed" start end)
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
      `(let ((,(car results)
              (catch ,#t
                (lambda ,() ,@body)
                (lambda (type info)
                  (if (pair? (*s7* 'catches))
                    (lambda () (apply throw type info))
                    (car info))))))
         (cond ,@(cdr results)
               (else (if (procedure? ,(car results))
                       (,(car results))
                       ,(car results)))))
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
      (if (input-port? p) (close-input-port p) (close-output-port p))
    ) ;define

    (define (eof-object)
      #<eof>
    ) ;define

    (define list-copy copy)

    (define (string-copy str . start_end)
      (cond ((null? start_end) (substring str 0))
            ((= (length start_end) 1) (substring str (car start_end)))
            ((= (length start_end) 2) (substring str (car start_end) (cadr start_end)))
            (else (error 'wrong-number-of-args))
      ) ;cond
    ) ;define

    (define (string-map p . args)
      (apply string (apply map p args))
    ) ;define

    (define string-for-each for-each)

    (define* (vector-copy v (start 0) (end (vector-length v)))
      (if (or (> start end) (> end (vector-length v)))
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

    (define* (vector-copy! to at from (start 0) (end (vector-length from)))
      (if (or (< at 0)
            (> start (vector-length from))
            (< end 0)
            (> end (vector-length from))
            (> start end)
            (> (+ at (- end start)) (vector-length to))
          ) ;or
        (error 'out-of-range "vector-copy!")
        (let loop
          ((to-i at) (from-i start))
          (if (>= from-i end)
            to
            (begin
              (vector-set! to to-i (vector-ref from from-i))
              (loop (+ to-i 1) (+ from-i 1))
            ) ;begin
          ) ;if
        ) ;let
      ) ;if
    ) ;define*

    ;; 0-clause BSD
    ;; Bill Schottstaedt
    ;; from S7 source repo: r7rs.scm
    (define* (vector->string v (start 0) end)
      (let ((stop (or end (length v))))
        (copy v (make-string (- stop start)) start stop)
      ) ;let
    ) ;define*

    ;; 0-clause BSD
    ;; Bill Schottstaedt
    ;; from S7 source repo: r7rs.scm
    (define* (string->vector s (start 0) end)
      (let ((stop (or end (length s))))
        (copy s (make-vector (- stop start)) start stop)
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library
