;;  MIT License
;;  Copyright guenchi (c) 2018 - 2019
;;            Da Shen (c) 2024 - 2025
;;            (Jack) Yansong Li (c) 2025
;;  Permission is hereby granted, free of charge, to any person obtaining a copy
;;  of this software and associated documentation files (the "Software"), to deal
;;  in the Software without restriction, including without limitation the rights
;;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;;  copies of the Software, and to permit persons to whom the Software is
;;  furnished to do so, subject to the following conditions:
;;  The above copyright notice and this permission notice shall be included in all
;;  copies or substantial portions of the Software.
;;  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;;  SOFTWARE.

(define-library (guenchi json)
  (import (liii base)
    (liii chez)
    (liii alist)
    (liii list)
    (liii string)
    (liii unicode)
  ) ;import
  (export json-string-escape
    json-string-unescape
    string->json
    json->string
    json-ref
    json-ref*
    json-set
    json-set*
    json-push
    json-push*
    json-drop
    json-drop*
    json-reduce
    json-reduce*
  ) ;export
  (begin

    (define (json-string-escape str)
      (let ((out (open-output-string)))
        (write-char #\" out)
        (let loop
          ((i 0))
          (if (= i (string-length str))
            (begin
              (write-char #\" out)
              (get-output-string out)
            ) ;begin
            (let ((c (string-ref str i)))
              (case c
               ((#\") (display "\\\"" out))
               ((#\\) (display "\\\\" out))
               ((#\/) (display "\\/" out))
               ((#\backspace) (display "\\b" out))
               ((#\xc) (display "\\f" out))
               ((#\newline) (display "\\n" out))
               ((#\return) (display "\\r" out))
               ((#\tab) (display "\\t" out))
               (else (write-char c out))
              ) ;case
              (loop (+ i 1))
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (string-length-sum strings)
      (let loop
        ((o 0) (rest strings))
        (cond ((eq? '() rest) o)
              (else (loop (+ o (string-length (car rest)))
                      (cdr rest)
                    ) ;loop
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (fast-string-list-append strings)
      (let* ((output-length (string-length-sum strings)
             ) ;output-length
             (output (make-string output-length #\_))
             (fill 0)
            ) ;
        (let outer
          ((rest strings))
          (cond ((eq? '() rest) output)
                (else (let* ((s (car rest)) (n (string-length s)))
                        (let inner
                          ((i 0))
                          (cond ((= i n) 'done)
                                (else (string-set! output
                                        fill
                                        (string-ref s i)
                                      ) ;string-set!
                                  (set! fill (+ fill 1))
                                  (inner (+ i 1))
                                ) ;else
                          ) ;cond
                        ) ;let
                      ) ;let*
                  (outer (cdr rest))
                ) ;else
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    (define (handle-escape-char s end len)
      (let ((next-char (if (< (+ end 1) len)
                         (string-ref s (+ end 1))
                         #f
                       ) ;if
            ) ;next-char
           ) ;
        (case next-char
         ((#\") (values "\\\"" 2))
         ((#\\) (values "\\\\" 2))
         ((#\/) (values "/" 2))
         ((#\b) (values "\\b" 2))
         ((#\f) (values "\\f" 2))
         ((#\n) (values "\\n" 2))
         ((#\r) (values "\\r" 2))
         ((#\t) (values "\\t" 2))
         ((#\u)
          (let ((start-pos (+ end 2))
                (end-pos (+ end 6))
               ) ;
            (if (and (>= start-pos 0) (< end-pos len))
              (let ((hex-str (substring s start-pos end-pos)
                    ) ;hex-str
                   ) ;
                (let ((code-point (string->number hex-str 16))
                     ) ;
                  (when (not code-point)
                    (error 'parse-error
                      (string-append "Invalid HEX sequence "
                        hex-str
                      ) ;string-append
                    ) ;error
                  ) ;when
                  ;; 检查是否存在连续的两个 \u
                  (let ((next-u-pos (+ end 6)))
                    (if (and (< (+ next-u-pos 6) len)
                          (char=? (string-ref s next-u-pos) #\\)
                          (char=? (string-ref s (+ next-u-pos 1))
                            #\u
                          ) ;char=?
                        ) ;and
                      ;; 存在连续的两个 \u
                      (let ((next-hex-str (substring s
                                            (+ next-u-pos 2)
                                            (+ next-u-pos 6)
                                          ) ;substring
                            ) ;next-hex-str
                           ) ;
                        (let ((next-code-point (string->number next-hex-str 16)
                              ) ;next-code-point
                             ) ;
                          (when (not next-code-point)
                            (error 'parse-error
                              (string-append "Invalid HEX sequence "
                                next-hex-str
                              ) ;string-append
                            ) ;error
                          ) ;when
                          ;; 检查是否满足代理对条件
                          (if (and (>= code-point 55296)
                                (<= code-point 56319)
                                (>= next-code-point 56320)
                                (<= next-code-point 57343)
                              ) ;and
                            ;; 满足代理对条件，使用 unicode 模块计算码点并转换为字符串
                            (let ((surrogate-code-point (+ (* (- code-point 55296) 1024)
                                                          (- next-code-point 56320)
                                                          65536
                                                        ) ;+
                                  ) ;surrogate-code-point
                                 ) ;
                              (values (utf8->string (codepoint->utf8 surrogate-code-point)
                                      ) ;utf8->string
                                12
                              ) ;values
                            ) ;let
                            ;; 不满足代理对条件，仅对第一个 \u 进行转换
                            (values (utf8->string (codepoint->utf8 code-point)
                                    ) ;utf8->string
                              6
                            ) ;values
                          ) ;if
                        ) ;let
                      ) ;let
                      ;; 不存在连续的两个 \u，仅对第一个 \u 进行转换
                      (values (utf8->string (codepoint->utf8 code-point)
                              ) ;utf8->string
                        6
                      ) ;values
                    ) ;if
                  ) ;let
                ) ;let
              ) ;let
              ;; 索引无效，返回原字符
              (error 'parse-error
                (string-append "HEX sequence too short "
                  (substring s start-pos)
                ) ;string-append
              ) ;error
            ) ;if
          ) ;let
         ) ;
         (else (error 'parse-error
                 (string-append "Invalid escape char: "
                   (string next-char)
                 ) ;string-append
               ) ;error
         ) ;else
        ) ;case
      ) ;let
    ) ;define

    (define string->json
      (lambda (s)
        (read (open-input-string (let loop
                                   ((s s)
                                    (bgn 0)
                                    (end 0)
                                    (rst '())
                                    (len (string-length s))
                                    (quts? #f)
                                    (lst '(#t))
                                   ) ;
                                   (cond ((= end len)
                                          (fast-string-list-append (reverse rst))
                                         ) ;
                                         ((and quts?
                                            (char=? (string-ref s end) #\\)
                                            (< (+ end 1) len)
                                          ) ;and
                                          (let-values (((unescaped step)
                                                        (handle-escape-char s end len)
                                                       ) ;
                                                      ) ;
                                            (loop s
                                              (+ end step)
                                              (+ end step)
                                              (cons (string-append (substring s bgn end)
                                                      unescaped
                                                    ) ;string-append
                                                rst
                                              ) ;cons
                                              len
                                              quts?
                                              lst
                                            ) ;loop
                                          ) ;let-values
                                         ) ;
                                         ((and quts?
                                            (not (char=? (string-ref s end) #\"))
                                          ) ;and
                                          (loop s bgn (+ 1 end) rst len quts? lst)
                                         ) ;
                                         (else (case (string-ref s end)
                                                     ((#\{)
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                "(("
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        (cons #t lst)
                                                      ) ;loop
                                                     ) ;
                                                     ((#\})
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                "))"
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        (loose-cdr lst)
                                                      ) ;loop
                                                     ) ;
                                                     ((#\[)
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                "#("
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        (cons #f lst)
                                                      ) ;loop
                                                     ) ;
                                                     ((#\])
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                ")"
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        (loose-cdr lst)
                                                      ) ;loop
                                                     ) ;
                                                     ((#\:)
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                " . "
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        lst
                                                      ) ;loop
                                                     ) ;
                                                     ((#\,)
                                                      (loop s
                                                        (+ 1 end)
                                                        (+ 1 end)
                                                        (cons (string-append (substring s bgn end)
                                                                (if (loose-car lst) ")(" " ")
                                                              ) ;string-append
                                                          rst
                                                        ) ;cons
                                                        len
                                                        quts?
                                                        lst
                                                      ) ;loop
                                                     ) ;
                                                     ((#\")
                                                      (loop s
                                                        bgn
                                                        (+ 1 end)
                                                        rst
                                                        len
                                                        (not quts?)
                                                        lst
                                                      ) ;loop
                                                     ) ;
                                                     (else (loop s bgn (+ 1 end) rst len quts? lst)
                                                     ) ;else
                                               ) ;case
                                         ) ;else
                                   ) ;cond
                                 ) ;let
              ) ;open-input-string
        ) ;read
      ) ;lambda
    ) ;define
    (define json->string
      (lambda (json-scm)
        (define f
          (lambda (x)
            (cond ((string? x) (json-string-escape x))
                  ((number? x) (number->string x))
                  ((boolean? x) (if x "true" "false"))
                  ((symbol? x) (symbol->string x))
                  ((null? x) "{}")
                  (else (type-error "Unexpected x: " x))
            ) ;cond
          ) ;lambda
        ) ;define
        (define (delim x)
          (if (zero? x) "" ",")
        ) ;define
        (when (procedure? json-scm)
          (type-error "json->string: input must not be a procedure"
          ) ;type-error
        ) ;when
        (let loop
          ((lst json-scm)
           (x (if (vector? json-scm) "[" "{"))
          ) ;
          (if (vector? lst)
            (string-append x
              (let loop-v
                ((len (vector-length lst)) (n 0) (y ""))
                (if (< n len)
                  (let* ((k (vector-ref lst n))
                         (result (cond ((vector? k) (loop k "["))
                                       ((pair? k) (loop k "{"))
                                       (else (f k))
                                 ) ;cond
                         ) ;result
                        ) ;
                    (loop-v len
                      (+ n 1)
                      (string-append y (delim n) result)
                    ) ;loop-v
                  ) ;let*
                  (string-append y "]")
                ) ;if
              ) ;let
            ) ;string-append
            (let* ((d (car lst))
                   (k (loose-car d))
                   (v (loose-cdr d))
                  ) ;
              (when (not (list? d))
                (value-error d " must be a list")
              ) ;when
              (let ((len (length d)))
                (when (not (or (= len 0) (= len -1) (>= len 2))
                      ) ;not
                  (value-error d
                    " must be null, pair, or list with at least 2 elements"
                  ) ;value-error
                ) ;when
              ) ;let
              (if (null? (cdr lst))
                (if (null? d)
                  "{}"
                  (string-append x
                    (f k)
                    ":"
                    (cond ((null? v) "{}")
                          ((list? v) (loop v "{"))
                          ((vector? v) (loop v "["))
                          (else (f v))
                    ) ;cond
                    "}"
                  ) ;string-append
                ) ;if
                (loop (cdr lst)
                  (cond ((list? v)
                         (string-append x
                           (f k)
                           ":"
                           (loop v "{")
                           ","
                         ) ;string-append
                        ) ;
                        ((vector? v)
                         (string-append x
                           (f k)
                           ":"
                           (loop v "[")
                           ","
                         ) ;string-append
                        ) ;
                        (else (string-append x (f k) ":" (f v) ",")
                        ) ;else
                  ) ;cond
                ) ;loop
              ) ;if
            ) ;let*
          ) ;if
        ) ;let
      ) ;lambda
    ) ;define
    (define json-ref
      (lambda (x k)
        (define return
          (lambda (x)
            (if (symbol? x)
              (cond ((symbol=? x 'true) #t)
                    ((symbol=? x 'false) #f)
                    (else x)
              ) ;cond
              x
            ) ;if
          ) ;lambda
        ) ;define
        (if (vector? x)
          (return (vector-ref x k))
          (let loop
            ((x x) (k k))
            (if (null? x)
              '()
              (if (equal? (caar x) k)
                (return (cdar x))
                (loop (cdr x) k)
              ) ;if
            ) ;if
          ) ;let
        ) ;if
      ) ;lambda
    ) ;define
    (define (json-ref* j . keys)
      (let loop
        ((expr j) (keys keys))
        (if (null? keys)
          expr
          (loop (json-ref expr (car keys))
            (cdr keys)
          ) ;loop
        ) ;if
      ) ;let
    ) ;define
    (define json-set
      (lambda (x v p)
        (let ((x x)
              (v v)
              (p (if (procedure? p) p (lambda (x) p)))
             ) ;
          (if (vector? x)
            (list->vector (cond ((boolean? v)
                                 (if v
                                   (let l
                                     ((x (vector->alist x)) (p p))
                                     (if (null? x)
                                       '()
                                       (cons (p (cdar x)) (l (cdr x) p))
                                     ) ;if
                                   ) ;let
                                 ) ;if
                                ) ;
                                ((procedure? v)
                                 (let l
                                   ((x (vector->alist x)) (v v) (p p))
                                   (if (null? x)
                                     '()
                                     (if (v (caar x))
                                       (cons (p (cdar x)) (l (cdr x) v p))
                                       (cons (cdar x) (l (cdr x) v p))
                                     ) ;if
                                   ) ;if
                                 ) ;let
                                ) ;
                                (else (let l
                                        ((x (vector->alist x)) (v v) (p p))
                                        (if (null? x)
                                          '()
                                          (if (equal? (caar x) v)
                                            (cons (p (cdar x)) (l (cdr x) v p))
                                            (cons (cdar x) (l (cdr x) v p))
                                          ) ;if
                                        ) ;if
                                      ) ;let
                                ) ;else
                          ) ;cond
            ) ;list->vector
            (cond ((boolean? v)
                   (if v
                     (let l
                       ((x x) (p p))
                       (if (null? x)
                         '()
                         (cons (cons (caar x) (p (cdar x)))
                           (l (cdr x) p)
                         ) ;cons
                       ) ;if
                     ) ;let
                   ) ;if
                  ) ;
                  ((procedure? v)
                   (let l
                     ((x x) (v v) (p p))
                     (if (null? x)
                       '()
                       (if (v (caar x))
                         (cons (cons (caar x) (p (cdar x)))
                           (l (cdr x) v p)
                         ) ;cons
                         (cons (car x) (l (cdr x) v p))
                       ) ;if
                     ) ;if
                   ) ;let
                  ) ;
                  (else (let l
                          ((x x) (v v) (p p))
                          (if (null? x)
                            '()
                            (if (equal? (caar x) v)
                              (cons (cons v (p (cdar x)))
                                (l (cdr x) v p)
                              ) ;cons
                              (cons (car x) (l (cdr x) v p))
                            ) ;if
                          ) ;if
                        ) ;let
                  ) ;else
            ) ;cond
          ) ;if
        ) ;let
      ) ;lambda
    ) ;define
    (define (json-set* json k0 k1_or_v . ks_and_v)
      (if (null? ks_and_v)
        (json-set json k0 k1_or_v)
        (json-set json
          k0
          (lambda (x)
            (apply json-set*
              (cons x (cons k1_or_v ks_and_v))
            ) ;apply
          ) ;lambda
        ) ;json-set
      ) ;if
    ) ;define
    (define (json-push x k v)
      (if (vector? x)
        (if (= (vector-length x) 0)
          (vector v)
          (list->vector (let l
                          ((x (vector->alist x))
                           (k k)
                           (v v)
                           (b #f)
                          ) ;
                          (if (null? x)
                            (if b '() (cons v '()))
                            (if (equal? (caar x) k)
                              (cons v
                                (cons (cdar x) (l (cdr x) k v #t))
                              ) ;cons
                              (cons (cdar x) (l (cdr x) k v b))
                            ) ;if
                          ) ;if
                        ) ;let
          ) ;list->vector
        ) ;if
        (cons (cons k v) x)
      ) ;if
    ) ;define
    (define (json-push* json k0 v0 . rest)
      (if (null? rest)
        (json-push json k0 v0)
        (json-set json
          k0
          (lambda (x)
            (apply json-push*
              (cons x (cons v0 rest))
            ) ;apply
          ) ;lambda
        ) ;json-set
      ) ;if
    ) ;define
    (define json-drop
      (lambda (x v)
        (if (vector? x)
          (if (zero? (vector-length x))
            x
            (list->vector (cond ((procedure? v)
                                 (let l
                                   ((x (vector->alist x)) (v v))
                                   (if (null? x)
                                     '()
                                     (if (v (caar x))
                                       (l (cdr x) v)
                                       (cons (cdar x) (l (cdr x) v))
                                     ) ;if
                                   ) ;if
                                 ) ;let
                                ) ;
                                (else (let l
                                        ((x (vector->alist x)) (v v))
                                        (if (null? x)
                                          '()
                                          (if (equal? (caar x) v)
                                            (l (cdr x) v)
                                            (cons (cdar x) (l (cdr x) v))
                                          ) ;if
                                        ) ;if
                                      ) ;let
                                ) ;else
                          ) ;cond
            ) ;list->vector
          ) ;if
          (cond ((procedure? v)
                 (let l
                   ((x x) (v v))
                   (if (null? x)
                     '()
                     (if (v (caar x))
                       (l (cdr x) v)
                       (cons (car x) (l (cdr x) v))
                     ) ;if
                   ) ;if
                 ) ;let
                ) ;
                (else (let l
                        ((x x) (v v))
                        (if (null? x)
                          '()
                          (if (equal? (caar x) v)
                            (l (cdr x) v)
                            (cons (car x) (l (cdr x) v))
                          ) ;if
                        ) ;if
                      ) ;let
                ) ;else
          ) ;cond
        ) ;if
      ) ;lambda
    ) ;define
    (define json-drop*
      (lambda (json key . rest)
        (if (null? rest)
          (json-drop json key)
          (json-set json
            key
            (lambda (x)
              (apply json-drop* (cons x rest))
            ) ;lambda
          ) ;json-set
        ) ;if
      ) ;lambda
    ) ;define
    (define json-reduce
      (lambda (x v p)
        (if (vector? x)
          (list->vector (cond ((boolean? v)
                               (if v
                                 (let l
                                   ((x (vector->alist x)) (p p))
                                   (if (null? x)
                                     '()
                                     (cons (p (caar x) (cdar x))
                                       (l (cdr x) p)
                                     ) ;cons
                                   ) ;if
                                 ) ;let
                                 x
                               ) ;if
                              ) ;
                              ((procedure? v)
                               (let l
                                 ((x (vector->alist x)) (v v) (p p))
                                 (if (null? x)
                                   '()
                                   (if (v (caar x))
                                     (cons (p (caar x) (cdar x))
                                       (l (cdr x) v p)
                                     ) ;cons
                                     (cons (cdar x) (l (cdr x) v p))
                                   ) ;if
                                 ) ;if
                               ) ;let
                              ) ;
                              (else (let l
                                      ((x (vector->alist x)) (v v) (p p))
                                      (if (null? x)
                                        '()
                                        (if (equal? (caar x) v)
                                          (cons (p (caar x) (cdar x))
                                            (l (cdr x) v p)
                                          ) ;cons
                                          (cons (cdar x) (l (cdr x) v p))
                                        ) ;if
                                      ) ;if
                                    ) ;let
                              ) ;else
                        ) ;cond
          ) ;list->vector
          (cond ((boolean? v)
                 (if v
                   (let l
                     ((x x) (p p))
                     (if (null? x)
                       '()
                       (cons (cons (caar x) (p (caar x) (cdar x)))
                         (l (cdr x) p)
                       ) ;cons
                     ) ;if
                   ) ;let
                   x
                 ) ;if
                ) ;
                ((procedure? v)
                 (let l
                   ((x x) (v v) (p p))
                   (if (null? x)
                     '()
                     (if (v (caar x))
                       (cons (cons (caar x) (p (caar x) (cdar x)))
                         (l (cdr x) v p)
                       ) ;cons
                       (cons (car x) (l (cdr x) v p))
                     ) ;if
                   ) ;if
                 ) ;let
                ) ;
                (else (let l
                        ((x x) (v v) (p p))
                        (if (null? x)
                          '()
                          (if (equal? (caar x) v)
                            (cons (cons v (p v (cdar x)))
                              (l (cdr x) v p)
                            ) ;cons
                            (cons (car x) (l (cdr x) v p))
                          ) ;if
                        ) ;if
                      ) ;let
                ) ;else
          ) ;cond
        ) ;if
      ) ;lambda
    ) ;define
    (define (json-reduce* j v1 v2 . rest)
      (cond ((null? rest) (json-reduce j v1 v2))
            ((length=? 1 rest)
             (json-reduce j
               v1
               (lambda (x y)
                 (let* ((new-v1 v2) (p (last rest)))
                   (json-reduce y
                     new-v1
                     (lambda (n m) (p (list x n) m))
                   ) ;json-reduce
                 ) ;let*
               ) ;lambda
             ) ;json-reduce
            ) ;
            (else (json-reduce j
                    v1
                    (lambda (x y)
                      (let* ((new-v1 v2) (p (last rest)))
                        (apply json-reduce*
                          (append (cons y
                                    (cons new-v1 (drop-right rest 1))
                                  ) ;cons
                            (list (lambda (n m) (p (cons x n) m)))
                          ) ;append
                        ) ;apply
                      ) ;let*
                    ) ;lambda
                  ) ;json-reduce
            ) ;else
      ) ;cond
    ) ;define
  ) ;begin
) ;define-library
