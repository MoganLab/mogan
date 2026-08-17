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
    (liii error)
    (liii list)
    (liii string)
    (liii unicode)
  ) ;import
  (export json-string-escape json-ref json-ref* json-set json-set* json-push
    json-push* json-drop json-drop* json-reduce json-reduce*
  ) ;export
  (begin

    (define (json-string-escape str)
      (let ((out (open-output-string)))
        (write-char #\" out)
        (let* ((bv (string->utf8 str)) (len (bytevector-length bv)))
          (let loop
            ((i 0))
            (if (>= i len)
              (begin
                (write-char #\" out)
                (get-output-string out)
              ) ;begin
              (let ((next (bytevector-advance-utf8 bv i len)))
                (cond ((= next i) (loop (+ i 1)))
                      ((= next (+ i 1))
                       (let ((c (integer->char (bytevector-u8-ref bv i))))
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
                         (loop next)
                       ) ;let
                      ) ;
                      (else
                        ;; 多字节 UTF-8 字符，直接输出原始字节
                        (display (copy bv (make-string (- next i)) i next) out)
                        (loop next)
                      ) ;else
                ) ;cond
              ) ;let
            ) ;if
          ) ;let
        ) ;let*
      ) ;let
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
            (if (null? x) '() (if (equal? (caar x) k) (return (cdar x)) (loop (cdr x) k)))
          ) ;let
        ) ;if
      ) ;lambda
    ) ;define
    (define (json-ref* j . keys)
      (let loop
        ((expr j) (keys keys))
        (if (null? keys) expr (loop (json-ref expr (car keys)) (cdr keys)))
      ) ;let
    ) ;define
    (define json-set
      (lambda (x v p)
        (let ((x x) (v v) (p (if (procedure? p) p (lambda (x) p))))
          (if (vector? x)
            (list->vector (cond ((boolean? v)
                                 (if v
                                   (let l
                                     ((x (vector->alist x)) (p p))
                                     (if (null? x) '() (cons (p (cdar x)) (l (cdr x) p)))
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
                       (if (null? x) '() (cons (cons (caar x) (p (cdar x))) (l (cdr x) p)))
                     ) ;let
                   ) ;if
                  ) ;
                  ((procedure? v)
                   (let l
                     ((x x) (v v) (p p))
                     (if (null? x)
                       '()
                       (if (v (caar x))
                         (cons (cons (caar x) (p (cdar x))) (l (cdr x) v p))
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
                              (cons (cons v (p (cdar x))) (l (cdr x) v p))
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
          (lambda (x) (apply json-set* (cons x (cons k1_or_v ks_and_v))))
        ) ;json-set
      ) ;if
    ) ;define
    (define (json-push x k v)
      (if (vector? x)
        (if (= (vector-length x) 0)
          (vector v)
          (list->vector (let l
                          ((x (vector->alist x)) (k k) (v v) (b #f))
                          (if (null? x)
                            (if b '() (cons v '()))
                            (if (equal? (caar x) k)
                              (cons v (cons (cdar x) (l (cdr x) k v #t)))
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
        (json-set json k0 (lambda (x) (apply json-push* (cons x (cons v0 rest)))))
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
                                   (if (null? x) '() (if (v (caar x)) (l (cdr x) v) (cons (cdar x) (l (cdr x) v))))
                                 ) ;let
                                ) ;
                                (else (let l
                                        ((x (vector->alist x)) (v v))
                                        (if (null? x)
                                          '()
                                          (if (equal? (caar x) v) (l (cdr x) v) (cons (cdar x) (l (cdr x) v)))
                                        ) ;if
                                      ) ;let
                                ) ;else
                          ) ;cond
            ) ;list->vector
          ) ;if
          (cond ((procedure? v)
                 (let l
                   ((x x) (v v))
                   (if (null? x) '() (if (v (caar x)) (l (cdr x) v) (cons (car x) (l (cdr x) v))))
                 ) ;let
                ) ;
                (else (let l
                        ((x x) (v v))
                        (if (null? x)
                          '()
                          (if (equal? (caar x) v) (l (cdr x) v) (cons (car x) (l (cdr x) v)))
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
          (json-set json key (lambda (x) (apply json-drop* (cons x rest))))
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
                                   (if (null? x) '() (cons (p (caar x) (cdar x)) (l (cdr x) p)))
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
                                     (cons (p (caar x) (cdar x)) (l (cdr x) v p))
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
                                          (cons (p (caar x) (cdar x)) (l (cdr x) v p))
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
                     (if (null? x) '() (cons (cons (caar x) (p (caar x) (cdar x))) (l (cdr x) p)))
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
                       (cons (cons (caar x) (p (caar x) (cdar x))) (l (cdr x) v p))
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
                            (cons (cons v (p v (cdar x))) (l (cdr x) v p))
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
                   (json-reduce y new-v1 (lambda (n m) (p (list x n) m)))
                 ) ;let*
               ) ;lambda
             ) ;json-reduce
            ) ;
            (else (json-reduce j
                    v1
                    (lambda (x y)
                      (let* ((new-v1 v2) (p (last rest)))
                        (apply json-reduce*
                          (append (cons y (cons new-v1 (drop-right rest 1)))
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
