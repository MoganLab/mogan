;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (srfi srfi-13)
  (import (liii base) (srfi srfi-1))
  (export string-null? string-copy string-join string-every string-any string-take string-take-right
          string-drop string-drop-right string-pad string-pad-right string-trim string-trim-right
          string-trim-both string-prefix? string-suffix? string-index string-index-right string-skip string-skip-right
          string-contains string-count string-upcase string-downcase string-fold string-fold-right
          string-for-each-index string-reverse string-tokenize)
  (begin

    (define (%string-from-range str start_end)
      (cond ((null-list? start_end) str)
            ((= (length start_end) 1) (substring str (car start_end)))
            ((= (length start_end) 2)
             (substring str (first start_end) (second start_end)))
            (else
             (error 'wrong-number-of-args "%string-from-range"))))

    (define (%make-criterion char/pred?)
      (cond ((char? char/pred?) (lambda (x) (char=? x char/pred?)))
            ((procedure? char/pred?) char/pred?)
            (else (error 'wrong-type-arg "%make-criterion"))))

    (define (string-join l . delim+grammer)
      (define (extract-params params-l)
        (cond ((null-list? params-l) (list "" 'infix))
              ((and (= (length params-l) 1)
                    (string? (car params-l)))
               (list (car params-l) 'infix))
              ((and (= (length params-l) 2)
                    (string? (first params-l))
                    (symbol? (second params-l)))
               params-l)
              ((> (length params-l) 2)
               (error 'wrong-number-of-args "optional params in string-join"))
              (else
               (error 'type-error "optional params in string-join"))))
      (define (string-join-sub l delim)
        (cond ((null-list? l) "")
              ((= (length l) 1) (car l))
              (else
               (string-append (car l) delim (string-join-sub (cdr l) delim)))))
      (let* ((params (extract-params delim+grammer))
             (delim (first params))
             (grammer (second params))
             (ret (string-join-sub l delim)))
        (case grammer
          ('infix ret)
          ('strict-infix
           (if (null-list? l)
               (error 'value-error "empty list not allowed")
               ret))
          ('suffix (if (null-list? l) "" (string-append ret delim)))
          ('prefix (if (null-list? l) "" (string-append delim ret)))
          (else (error 'value-error "invalid grammer")))))


    (define (string-null? str)
      (if (not (string? str))
          (error 'type-error "string-null?: expected string~%~S" str)
          (zero? (string-length str))))

    (define (string-every char/pred? str . start+end)
      (define (string-every-sub pred? str)
        (let loop ((i 0)
                   (len (string-length str)))
          (or (= i len)
              (and (pred? (string-ref str i))
                   (loop (+ i 1) len)))))
      (let ((str-sub (%string-from-range str start+end))
            (criterion (%make-criterion char/pred?)))
        (string-every-sub criterion str-sub)))

    (define (string-any char/pred? str . start+end)
      (define (string-any-sub pred? str)
        (let loop ((i 0)
                   (len (string-length str)))
          (if (= i len)
              #f
              (or (pred? (string-ref str i))
                  (loop (+ i 1) len)))))
      (let ((str_sub (%string-from-range str start+end))
            (criterion (%make-criterion char/pred?)))
        (string-any-sub criterion str_sub)))

    (define (string-take str k)
      (substring str 0 k))

    (define (string-take-right str k)
      (let ((N (string-length str)))
        (if (> k N)
            (error 'out-of-range "k must be <= N" k N))
        (substring str (- N k) N)))

    (define string-drop
      (lambda (str k)
        (unless (string? str)
          (error 'wrong-type-arg "str is not string?" str))
        (unless (integer? k)
          (error 'wrong-type-arg "k is not integer?" k))
        (when (< k 0)
          (error 'out-of-range "k must be non-negative" k))
        (let ((N (string-length str)))
          (if (> k N)
              (error 'out-of-range "k must be <= N" k N)
              (substring str k N)))))

    (define string-drop-right
      (lambda (str k)
        (unless (string? str)
          (error 'wrong-type-arg "str is not string?" str))
        (unless (integer? k)
          (error 'wrong-type-arg "k is not integer?" k))
        (when (< k 0)
          (error 'out-of-range "k must be non-negative" k))
        (let ((N (string-length str)))
          (if (> k N)
              (error 'out-of-range "k must be <= N" k N)
              (substring str 0 (- N k))))))

    (define (string-pad str len . char+start+end)
      (define (string-pad-sub str len ch)
        (let ((orig-len (string-length str)))
          (if (< len orig-len)
              (string-take-right str len)
              (string-append (make-string (- len orig-len) ch) str))))
      (cond ((null-list? char+start+end) (string-pad-sub str len #\space))
            ((list? char+start+end)
             (string-pad-sub (%string-from-range str (cdr char+start+end)) len (car char+start+end)))
            (else (error 'wrong-type-arg "string-pad"))))

    (define (string-pad-right str len . char+start+end)
      (define (string-pad-right-sub str len ch)
        (let ((orig-len (string-length str)))
          (if (< len orig-len)
              (string-take str len)
              (string-append str (make-string (- len orig-len) ch)))))
      (cond ((null-list? char+start+end) (string-pad-right-sub str len #\space))
            ((list? char+start+end)
             (string-pad-right-sub (%string-from-range str (cdr char+start+end)) len (car char+start+end)))
            (else (error 'wrong-type-arg "string-pad"))))

    (define (string-trim str . opt)
      (let ((predicate (cond ((null? opt) char-whitespace?)
                             ((char? (car opt)) (lambda (c) (char=? c (car opt))))
                             ((procedure? (car opt)) (car opt))
                             (else
                              (type-error "Invalid second argument: expected character or predicate"
                                          (car opt))))))
        (let* ((start (if (and (> (length opt) 1)
                               (number? (cadr opt)))
                          (cadr opt)
                          0))
               (end (if (and (> (length opt) 2)
                             (number? (caddr opt)))
                        (caddr opt)
                        (string-length str)))
               (str (substring str start end)))
          (let loop ((i 0)
                     (len (string-length str)))
            (if (or (>= i len)
                    (not (predicate (string-ref str i))))
                (substring str i len)
                (loop (+ i 1) len))))))

    (define (string-trim-right str . opt)
      (let ((predicate (cond ((null? opt) char-whitespace?)
                             ((char? (car opt)) (lambda (c) (char=? c (car opt))))
                             ((procedure? (car opt)) (car opt))
                             (else
                              (type-error "Invalid second argument: expected character or predicate"
                                          (car opt))))))
        (let* ((start (if (and (> (length opt) 1)
                               (number? (cadr opt)))
                          (cadr opt)
                          0))
               (end (if (and (> (length opt) 2)
                             (number? (caddr opt)))
                        (caddr opt)
                        (string-length str)))
               (str (substring str start end)))
          (let loop ((j (- (string-length str) 1)))
            (if (or (< j 0)
                    (not (predicate (string-ref str j))))
                (substring str 0 (+ j 1))
                (loop (- j 1)))))))

    (define (string-trim-both str . opt)
      (let ((predicate (cond ((null? opt) char-whitespace?)
                             ((char? (car opt)) (lambda (c) (char=? c (car opt))))
                             ((procedure? (car opt)) (car opt))
                             (else
                              (type-error "Invalid second argument: expected character or predicate"
                                          (car opt))))))
        (let* ((start (if (and (> (length opt) 1)
                               (number? (cadr opt)))
                          (cadr opt)
                          0))
               (end (if (and (> (length opt) 2)
                             (number? (caddr opt)))
                        (caddr opt)
                        (string-length str)))
               (str (substring str start end)))
          (let loop-left ((i 0)
                          (len (string-length str)))
            (if (or (>= i len)
                    (not (predicate (string-ref str i))))
                (let loop-right ((j (- len 1)))
                  (if (or (< j i)
                          (not (predicate (string-ref str j))))
                      (substring str i (+ j 1))
                      (loop-right (- j 1))))
                (loop-left (+ i 1) len))))))

    (define (string-prefix? prefix str)
      (let* ((prefix-len (string-length prefix))
             (str-len (string-length str)))
        (and (<= prefix-len str-len)
             (string=? prefix (substring str 0 prefix-len)))))

    (define (string-suffix? suffix str)
      (let* ((suffix-len (string-length suffix))
             (str-len (string-length str)))
        (and (<= suffix-len str-len)
             (string=? suffix (substring str (- str-len suffix-len) str-len)))))

    (define (string-index str char/pred? . start+end)
      (define (string-index-sub str pred?)
        (let loop ((i 0))
          (cond ((>= i (string-length str)) #f)
                ((pred? (string-ref str i)) i)
                (else (loop (+ i 1))))))
      (let* ((start (if (null-list? start+end)
                        0
                        (car start+end)))
             (str-sub (%string-from-range str start+end))
             (pred? (%make-criterion char/pred?))
             (ret (string-index-sub str-sub pred?)))
        (if ret (+ start ret) ret)))

    (define (string-index-right str char/pred? . start+end)
      (define (string-index-right-sub str pred?)
        (let loop ((i (- (string-length str) 1)))
          (cond ((< i 0) #f)
                ((pred? (string-ref str i)) i)
                (else (loop (- i 1))))))
      (let* ((start (if (null-list? start+end)
                        0
                        (car start+end)))
             (str-sub (%string-from-range str start+end))
             (pred? (%make-criterion char/pred?))
             (ret (string-index-right-sub str-sub pred?)))
        (if ret (+ start ret) ret)))

    (define (string-skip str char/pred? . start+end)
      (define (string-skip-sub str pred?)
        (let loop ((i 0))
          (cond ((>= i (string-length str)) #f)
                ((pred? (string-ref str i)) (loop (+ i 1)))
                (else i))))
      (let* ((start (if (null-list? start+end)
                        0
                        (car start+end)))
             (str-sub (%string-from-range str start+end))
             (pred? (%make-criterion char/pred?))
             (ret (string-skip-sub str-sub pred?)))
        (if ret (+ start ret) ret)))

    (define (string-skip-right str char/pred? . start+end)
      (define (string-skip-right-sub str pred?)
        (let loop ((i (- (string-length str) 1)))
          (cond ((< i 0) #f)
                ((pred? (string-ref str i)) (loop (- i 1)))
                (else i))))
      (let* ((start (if (null-list? start+end)
                        0
                        (car start+end)))
             (str-sub (%string-from-range str start+end))
             (pred? (%make-criterion char/pred?))
             (ret (string-skip-right-sub str-sub pred?)))
        (if ret (+ start ret) ret)))

    (define (string-contains str sub-str)
      (let loop ((i 0))
        (let ((len (string-length str))
              (sub-str-len (string-length sub-str)))
          (if (> i (- len sub-str-len))
              #f
              (if (string=? (substring str i (+ i sub-str-len)) sub-str)
                  #t
                  (loop (+ i 1)))))))

    (define (string-count str char/pred? . start+end)
      (when (not (string? str))
        (type-error
         "string-count: first parameter must be string"))
      (let ((str-sub (%string-from-range str start+end))
            (criterion (%make-criterion char/pred?)))
        (count criterion (string->list str-sub))))

    (define s7-string-upcase string-upcase)

    (define* (string-upcase str (start 0) (end (string-length str)))
      (let* ((left (substring str 0 start))
             (middle (substring str start end))
             (right (substring str end)))
        (string-append left (s7-string-upcase middle) right)))

    (define s7-string-downcase string-downcase)

    (define* (string-downcase str (start 0) (end (string-length str)))
      (let* ((left (substring str 0 start))
             (middle (substring str start end))
             (right (substring str end)))
        (string-append left (s7-string-downcase middle) right)))

    (define (string-reverse str . start+end)
      (cond ((null-list? start+end) (reverse str))
            ((= (length start+end) 1)
             (let ((start (first start+end)))
               (string-append (substring str 0 start) (reverse (substring str start)))))
            ((= (length start+end) 2)
             (let ((start (first start+end))
                   (end (second start+end)))
               (string-append (substring str 0 start) (reverse (substring str start end))
                              (substring str end))))
            (else (error 'wrong-number-of-args "string-reverse"))))

    (define (string-fold kons knil s . rest)
      (when (not (procedure? kons))
        (type-error
         "string-fold: first argument must be a procedure"))
      (when (not (string? s))
        (type-error
         "string-fold: second argument must be a string"))

      (let ((substr (%string-from-range s rest)))
        (let loop ((i 0)
                   (result knil))
          (if (= i (string-length substr))
              result
              (loop (+ i 1) (kons (string-ref substr i) result))))))

    (define (string-fold-right kons knil s . rest)
      (when (not (procedure? kons))
        (type-error
         "string-fold-right: first argument must be a procedure"))
      (when (not (string? s))
        (type-error
         "string-fold-right: second argument must be a string"))

      (let ((substr (%string-from-range s rest)))
        (let loop ((i (- (string-length substr) 1))
                   (result knil))
          (if (< i 0)
              result
              (loop (- i 1) (kons (string-ref substr i) result))))))

    (define (string-for-each-index proc str . start+end)
      (when (not (procedure? proc))
        (error 'type-error "string-for-each-index: first argument must be a procedure"))
      (when (not (string? str))
        (error 'type-error "string-for-each-index: expected a string"))
      (let ((substr (%string-from-range str start+end)))
        (let loop ((i 0)
                   (len (string-length substr))
                   (acc '()))
          (if (< i len)
              (loop (+ i 1) len (proc i (string-ref substr i) acc))
              (reverse acc)))))

    (define (string-tokenize str . char+start+end)
      (define (string-tokenize-sub str char)
        (define (tokenize-helper tokens cursor)
          (let ((sep-pos/false (string-index str char cursor)))
            (if (not sep-pos/false)
                (reverse (cons (substring str cursor) tokens))
                (let ((new-tokens (if (= cursor sep-pos/false)
                                      tokens
                                      (cons (substring str cursor sep-pos/false) tokens)))
                      (next-cursor (+ sep-pos/false 1)))
                  (tokenize-helper new-tokens next-cursor)))))
        (tokenize-helper '() 0))
      (cond ((null-list? char+start+end) (string-tokenize-sub str #\space))
            ((list? char+start+end)
             (string-tokenize-sub (%string-from-range str (cdr char+start+end))
               (car char+start+end)))
            (else (error 'wrong-type-arg "string-tokenize"))))))

