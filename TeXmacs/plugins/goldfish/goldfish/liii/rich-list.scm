;
; Copyright (C) 2025 The Goldfish Scheme Authors
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

(define-library (liii rich-list)
  (import (liii list)
          (liii oop)
          (liii sort)
          (liii hash-table)
          (liii string)
          (rename (liii rich-option) (rich-option option) (rich-none none))
          (srfi srfi-8)
          (liii error)
  ) ;import
  (export rich-list)
  (begin


    (define-final-class rich-list ((data list?))

      (define (@range start end . step-and-args)
        (chain-apply (if (null? step-and-args) 
                         step-and-args 
                         (if (number? (car step-and-args))
                             (cdr step-and-args)
                             step-and-args)
                         ) ;if
          (let ((step-size 
                  (if (null? step-and-args) 
                      1
                      (if (number? (car step-and-args))
                          (car step-and-args)
                          1))
                      ) ;if
                  ) ;if
            (cond
              ((and (positive? step-size) (>= start end))
               (rich-list '())
              ) ;
              ((and (negative? step-size) (<= start end))
               (rich-list '())
              ) ;
              ((zero? step-size)
               (value-error "Step size cannot be zero")
              ) ;
              (else
               (let ((cnt (ceiling (/ (- end start) step-size))))
                 (rich-list (iota cnt start step-size))
               ) ;let
              ) ;else
            ) ;cond
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (@empty . args)
        (chain-apply args
          (rich-list (list ))
        ) ;chain-apply
      ) ;define

      (define (@concat lst1 lst2 . args)
        (chain-apply args
          (rich-list (append (lst1 :collect) (lst2 :collect)))
        ) ;chain-apply
      ) ;define

      (define (@fill n elem)
        (cond
          ((< n 0)
           (value-error "n cannot be negative")
          ) ;
          ((= n 0)
           (rich-list '())
          ) ;
          (else
            (rich-list (make-list n elem))
          ) ;else
        ) ;cond
      ) ;define

      (define (%collect) data)

      (define (%apply n)
        (list-ref data n)
      ) ;define

      (define (%find pred)
        (let loop ((lst data))
          (cond
            ((null? lst) (none))
            ((pred (car lst)) (option (car lst)))
            (else (loop (cdr lst)))
          ) ;cond
        ) ;let
      ) ;define

      (define (%find-last pred)
        (let ((reversed-list (reverse data)))  ; 先反转列表
          (let loop ((lst reversed-list))
            (cond
              ((null? lst) (none))  ; 遍历完未找到
              ((pred (car lst)) (option (car lst)))  ; 找到第一个匹配项（即原列表最后一个）
              (else (loop (cdr lst)))  ; 继续查找
            ) ;cond
          ) ;let
        ) ;let
      ) ;define

      (define (%head)
        (if (null? data)
            (error 'out-of-range "rich-list%head: list is empty")
            (car data)
        ) ;if
      ) ;define

      (define (%head-option)
        (if (null? data)
            (none)
            (option (car data))
        ) ;if
      ) ;define


      (define (%last)
        (if (null? data)
            (index-error "rich-list%last: empty list")
            (car (reverse data))
        ) ;if
      ) ;define

      (define (%last-option)
        (if (null? data)
            (none)
            (option (car (reverse data)))
        ) ;if
      ) ;define

      (define (%slice from until . args)
        (chain-apply args
          (let* ((len (length data))
                 (start (max 0 (min from len)))
                 (end (max 0 (min until len))))
            (if (< start end)
                (rich-list (take (drop data start) (- end start)))
                (rich-list '())
            ) ;if
          ) ;let*
        ) ;chain-apply
      ) ;define

      (define (%empty?)
        (null? data)
      ) ;define

      (define (%equals that)
        (let* ((l1 data)
               (l2 (that 'data))
               (len1 (length l1))
               (len2 (length l2)))
          (if (not (eq? len1 len2))
              #f
              (let loop ((left l1) (right l2))
                (cond ((null? left) #t)
                      ((not (class=? (car left) (car right))) #f)
                      (else (loop (cdr left) (cdr right)))
                ) ;cond
              ) ;let
          ) ;if
        ) ;let*
      ) ;define

      (define (%forall pred)
        (every pred data)
      ) ;define

      (define (%exists pred)
        (any pred data)
      ) ;define

      (define (%contains elem)
        (%exists (lambda (x) (equal? x elem)))
      ) ;define

      (define (%map x . args)
        (chain-apply args
          (rich-list (map x data))
        ) ;chain-apply
      ) ;define

      (define (%flat-map x . args)
        (chain-apply args
          (rich-list (flat-map x data))
        ) ;chain-apply
      ) ;define

      (define (%filter x . args)
        (chain-apply args
          (rich-list (filter x data))
        ) ;chain-apply
      ) ;define

      (define (%for-each x)
        (for-each x data)
      ) ;define

      (define (%reverse . args)
        (chain-apply args
          (rich-list (reverse data))
        ) ;chain-apply
      ) ;define
    
      (define (%take x . args)
        (chain-apply args
          (rich-list (list-take data x))
        ) ;chain-apply
      ) ;define

      (define (%drop x . args)
        (chain-apply args
          (rich-list (list-drop data x))
        ) ;chain-apply
      ) ;define

      (define (%take-right x . args)
        (chain-apply args
          (rich-list (list-take-right data x))
        ) ;chain-apply
      ) ;define

      (define (%drop-right x . args)
        (chain-apply args
          (rich-list (list-drop-right data x))
        ) ;chain-apply
      ) ;define

      (define (%count . xs)
        (cond ((null? xs) (length data))
              ((length=? 1 xs) (count (car xs) data))
              (else (error 'wrong-number-of-args "rich-list%count" xs))
        ) ;cond
      ) ;define

      (define (%length)
        (length data)
      ) ;define

      (define (%fold initial f)
        (fold f initial data)
      ) ;define

      (define (%fold-right initial f)
        (fold-right f initial data)
      ) ;define

      (define (%sort-with less-p . args)
        (chain-apply args
          (let ((sorted-data (list-stable-sort less-p data)))
            (rich-list sorted-data)
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%sort-by f . args)
        (chain-apply args
          (let ((sorted-data (list-stable-sort (lambda (x y) (< (f x) (f y))) data)))
            (rich-list sorted-data)
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%group-by func)
        (let ((group (make-hash-table)))
          (for-each
            (lambda (elem) 
              (let ((key (func elem)))
                (hash-table-update!/default
                  group
                  key
                  (lambda (current-list) (cons elem current-list))
                  '()
                ) ;hash-table-update!/default
              ) ;let
            ) ;lambda
            data
          ) ;for-each
          (hash-table-for-each 
            (lambda (k v) (hash-table-set! group k (reverse v))) 
            group
          ) ;hash-table-for-each
          (rich-hash-table group)
        ) ;let
      ) ;define

      (define (%sliding size . step-arg)
        (unless (integer? size) (type-error "rich-list%sliding: size must be an integer " size))
        (unless (> size 0) (value-error "rich-list%sliding: size must be a positive integer " size))

        (let ((N (length data)))
          (if (null? data)
              #()
              (let* ((is-single-arg-case (null? step-arg))
                     (step (if is-single-arg-case 1 (car step-arg))))

                (when (and (not is-single-arg-case)
                           (or (not (integer? step)) (<= step 0)))
                  (if (not (integer? step))
                      (type-error "rich-list%sliding: step must be an integer " step)
                      (value-error "rich-list%sliding: step must be a positive integer " step)
                  ) ;if
                ) ;when
          
                (if (and is-single-arg-case (< N size))
                    (vector data)
                    (let collect-windows ((current-list-segment data) (result-windows '()))
                      (cond
                        ((null? current-list-segment) (list->vector (reverse result-windows)))
                        ((and is-single-arg-case (< (length current-list-segment) size))
                         (list->vector (reverse result-windows))
                        ) ;
                        (else
                         (let* ((elements-to-take (if is-single-arg-case
                                                      size
                                                      (min size (length current-list-segment))))
                                (current-window (take current-list-segment elements-to-take))
                                (next-list-segment (if (>= step (length current-list-segment))
                                                       '()
                                                       (drop current-list-segment step)))
                                ) ;next-list-segment
                           (collect-windows next-list-segment
                                            (cons current-window result-windows)
                           ) ;collect-windows
                         ) ;let*
                        ) ;else
                      ) ;cond
                    ) ;let
                ) ;if
              ) ;let*
          ) ;if
        ) ;let
      ) ;define

      (define (%zip l . args)
        (chain-apply args
          (rich-list (apply map cons (list data l)))
        ) ;chain-apply
      ) ;define

      (define (%zip-with-index . args)
        (chain-apply args
          (let loop ((lst data) (idx 0) (result '()))
            (if (null? lst)
                (rich-list (reverse result))  
                (loop (cdr lst) 
                      (+ idx 1) 
                      (cons (cons idx (car lst)) result)
                ) ;loop
            ) ;if
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%distinct . args)
        (chain-apply args
          (let loop
            ((result '()) 
             (data data) 
             (ht (make-hash-table))
            ) ;
            (cond
              ((null? data) (rich-list (reverse result)))  
              (else
               (let ((elem (car data)))
                 (if (eq? (hash-table-ref ht elem) #f) 
                     (begin
                       (hash-table-set! ht elem #t)  
                       (loop (cons elem result) (cdr data) ht)
                     ) ;begin
                     (loop result (cdr data) ht)
                 ) ;if
               ) ;let
              ) ;else
            ) ;cond
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%reduce f)
        (if (null? data)
            (value-error "rich-list%reduce: empty list is not allowed to reduce")
            (reduce f '() data)
        ) ;if
      ) ;define

      (define (%reduce-option f)
        (if (null? data)
            (none)
            (option (reduce f '() data))
        ) ;if
      ) ;define

      (define (%take-while pred . args)
        (chain-apply args
          (let ((result (take-while pred data)))
            (rich-list result)
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%drop-while pred . args)
        (chain-apply args
          (let ((result (drop-while pred data)))
            (rich-list result)
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%index-where pred)
        (list-index pred data)
      ) ;define

      (define (%max-by f)
        (unless (procedure? f) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %max-by '(f) 'f "procedure" (object->string f)              
            ) ;format
          ) ;type-error
        ) ;unless
        (if (null? data)
            (value-error "rich-list%max-by: empty list is not allowed")
            (let loop ((rest (cdr data))
                       (max-elem (car data))
                       (max-val (let ((val (f (car data))))
                                 (unless (real? val)
                                   (type-error "rich-list%max-by: procedure must return real number but got"
                                     (object->string val)
                                   ) ;type-error
                                 ) ;unless
                                 val))
                       ) ;max-val
              (if (null? rest)
                  max-elem
                  (let* ((current (car rest))
                         (current-val (let ((val (f current)))
                                       (unless (real? val)
                                         (type-error "rich-list%max-by: procedure must return real number but got"
                                           (object->string val)
                                         ) ;type-error
                                       ) ;unless
                                       val))
                         ) ;current-val
                    (if (> current-val max-val)
                        (loop (cdr rest) current current-val)
                        (loop (cdr rest) max-elem max-val)
                    ) ;if
                  ) ;let*
              ) ;if
            ) ;let
        ) ;if
      ) ;define

      (define (%min-by f)
        (unless (procedure? f) 
          (type-error 
            (format #f "In funtion #<~a ~a>: argument *~a* must be *~a*!    **Got ~a**" 
              %min-by '(f) 'f "procedure" (object->string f)              
            ) ;format
          ) ;type-error
        ) ;unless
        (if (null? data)
            (value-error "rich-list%min-by: empty list is not allowed")
            (let loop ((rest (cdr data))
                       (min-elem (car data))
                       (min-val (let ((val (f (car data))))
                                  (unless (real? val)
                                    (type-error "rich-list%min-by: procedure must return real number but got"
                                      (object->string val)
                                    ) ;type-error
                                  ) ;unless
                                  val))
                       ) ;min-val
              (if (null? rest)
                  min-elem
                  (let* ((current (car rest))
                         (current-val (let ((val (f current)))
                                        (unless (real? val)
                                          (type-error "rich-list%min-by: procedure must return real number but got"
                                            (object->string val)
                                          ) ;type-error
                                        ) ;unless
                                        val))
                         ) ;current-val
                    (if (< current-val min-val)
                        (loop (cdr rest) current current-val)
                        (loop (cdr rest) min-elem min-val)
                    ) ;if
                  ) ;let*
              ) ;if
            ) ;let
        ) ;if
      ) ;define

      (define (%append l)
        (rich-list (append data l))
      ) ;define

      (define (%max-by-option f)
        (if (null? data)
            (none)
            (option (%max-by f))
        ) ;if
      ) ;define

      (define (%min-by-option f)
        (if (null? data)
            (none)
            (option (%min-by f))
        ) ;if
      ) ;define

      (define (%to-string)
        (object->string data)
      ) ;define

      (define (%make-string . xs)
        (define (parse-args xs)
          (cond
            ((null? xs) (values "" "" ""))
            ((length=? 1 xs)
             (let ((sep (car xs)))
               (if (string? sep)
                   (values "" sep "")
                   (type-error "rich-list%make-string: separator must be a string" sep)
               ) ;if
             ) ;let
            ) ;
            ((length=? 2 xs)
             (error 'wrong-number-of-args "rich-list%make-string: expected 0, 1, or 3 arguments, but got 2" xs)
            ) ;
            ((length=? 3 xs)
             (let ((start (car xs))
                   (sep (cadr xs))
                   (end (caddr xs)))
               (if (and (string? start) (string? sep) (string? end))
                   (values start sep end)
                   (error 'type-error "rich-list%make-string: prefix, separator, and suffix must be strings" xs)
               ) ;if
             ) ;let
            ) ;
            (else (error 'wrong-number-of-args "rich-list%make-string: expected 0, 1, or 3 arguments" xs))
          ) ;cond
        ) ;define

        (receive (start sep end) (parse-args xs)
          (let ((as-string (lambda (x) (if (string? x) x (object->string x)))))
            (string-append start (string-join (map as-string data) sep) end)
          ) ;let
        ) ;receive
      ) ;define

      (define (%to-vector)
        (list->vector data)
      ) ;define

      (define (%to-rich-vector)
        (rich-vector (list->vector data))
      ) ;define

    ) ;define-final-class


  ) ;begin
) ;define-library
