; SRFI-117: Mutable Queues based on lists
; Reference implementation
;
; SPDX-FileCopyrightText: 2017 Alex Shinn
;
; SPDX-License-Identifier: BSD-3-Clause
;
; Copyright (c) 2024 The Goldfish Scheme Authors
; Follow the same License as the original one

(define-library (srfi srfi-117)
  (export
    ; Constructors
    make-list-queue list-queue list-queue-copy
    list-queue-unfold list-queue-unfold-right
    ; Predicates
    list-queue? list-queue-empty?
    ; Accessors
    list-queue-front list-queue-back list-queue-list list-queue-first-last
    ; Mutators
    list-queue-add-front! list-queue-add-back!
    list-queue-remove-front! list-queue-remove-back!
    list-queue-remove-all! list-queue-set-list!
    ; Whole queue
    list-queue-append list-queue-append! list-queue-concatenate
    ; Mapping
    list-queue-map list-queue-map! list-queue-for-each
  ) ;export
  (import (liii base)
          (liii error)
          (liii list)
  ) ;import
  (begin

    ;;; The list-queue record
    ;;; The invariant is that either first is (the first pair of) a list
    ;;; and last is the last pair, or both of them are the empty list.

    (define-record-type <list-queue>
      (raw-make-list-queue first last)
      list-queue?
      (first get-first set-first!)
      (last get-last set-last!)
    ) ;define-record-type

    ;;; Helper function: return the last pair of a list
    (define (last-pair ls)
      (if (null? (cdr ls))
        ls
        (last-pair (cdr ls))
      ) ;if
    ) ;define

    ;;; Helper function: return the next to last pair of lis, or nil if there is none
    (define (penult-pair lis)
      (let lp ((lis lis))
        (cond
          ((null? lis) '())
          ((null? (cdr lis)) '())
          ((null? (cddr lis)) lis)
          (else (lp (cdr lis)))
        ) ;cond
      ) ;let
    ) ;define

    ;;; Helper function: map! for unary functions
    (define (map! f lis)
      (let lp ((lis lis))
        (if (pair? lis)
          (begin
            (set-car! lis (f (car lis)))
            (lp (cdr lis))
          ) ;begin
        ) ;if
      ) ;let
    ) ;define

    ;;; Constructors

    (define (make-list-queue list-arg . rest)
      (if (null? rest)
        (if (null? list-arg)
          (raw-make-list-queue '() '())
          (raw-make-list-queue list-arg (last-pair list-arg))
        ) ;if
        (raw-make-list-queue list-arg (car rest))
      ) ;if
    ) ;define

    (define (list-queue . objs)
      (make-list-queue objs)
    ) ;define

    (define (list-queue-copy list-queue)
      (make-list-queue (list-copy (get-first list-queue)))
    ) ;define

    ;;; Predicates

    (define (list-queue-empty? list-queue)
      (null? (get-first list-queue))
    ) ;define

    ;;; Accessors

    (define (list-queue-front list-queue)
      (if (list-queue-empty? list-queue)
        (error 'wrong-type-arg "list-queue-front: empty list-queue")
        (car (get-first list-queue))
      ) ;if
    ) ;define

    (define (list-queue-back list-queue)
      (if (list-queue-empty? list-queue)
        (error 'wrong-type-arg "list-queue-back: empty list-queue")
        (car (get-last list-queue))
      ) ;if
    ) ;define

    (define (list-queue-list list-queue)
      (get-first list-queue)
    ) ;define

    (define (list-queue-first-last list-queue)
      (values (get-first list-queue) (get-last list-queue))
    ) ;define

    ;;; Mutators

    (define (list-queue-add-front! list-queue elem)
      (let ((new-first (cons elem (get-first list-queue))))
        (if (list-queue-empty? list-queue)
          (set-last! list-queue new-first)
        ) ;if
        (set-first! list-queue new-first)
      ) ;let
    ) ;define

    (define (list-queue-add-back! list-queue elem)
      (let ((new-last (list elem)))
        (if (list-queue-empty? list-queue)
          (set-first! list-queue new-last)
          (set-cdr! (get-last list-queue) new-last)
        ) ;if
        (set-last! list-queue new-last)
      ) ;let
    ) ;define

    (define (list-queue-remove-front! list-queue)
      (if (list-queue-empty? list-queue)
        (error 'wrong-type-arg "list-queue-remove-front!: empty list-queue")
        (let* ((old-first (get-first list-queue))
               (elem (car old-first))
               (new-first (cdr old-first)))
          (if (null? new-first)
            (set-last! list-queue '())
          ) ;if
          (set-first! list-queue new-first)
          elem
        ) ;let*
      ) ;if
    ) ;define

    (define (list-queue-remove-back! list-queue)
      (if (list-queue-empty? list-queue)
        (error 'wrong-type-arg "list-queue-remove-back!: empty list-queue")
        (let* ((old-last (get-last list-queue))
               (elem (car old-last))
               (new-last (penult-pair (get-first list-queue))))
          (if (null? new-last)
            (set-first! list-queue '())
            (set-cdr! new-last '())
          ) ;if
          (set-last! list-queue new-last)
          elem
        ) ;let*
      ) ;if
    ) ;define

    (define (list-queue-remove-all! list-queue)
      (let ((result (get-first list-queue)))
        (set-first! list-queue '())
        (set-last! list-queue '())
        result
      ) ;let
    ) ;define

    (define (list-queue-set-list! list-queue first-list . rest)
      (if (null? rest)
        (begin
          (set-first! list-queue first-list)
          (if (null? first-list)
            (set-last! list-queue '())
            (set-last! list-queue (last-pair first-list))
          ) ;if
        ) ;begin
        (begin
          (set-first! list-queue first-list)
          (set-last! list-queue (car rest))
        ) ;begin
      ) ;if
    ) ;define

    ;;; Whole queue

    (define (list-queue-concatenate list-queues)
      (let ((result (list-queue)))
        (for-each
          (lambda (q)
            (for-each (lambda (elem) (list-queue-add-back! result elem)) (get-first q))
          ) ;lambda
          list-queues
        ) ;for-each
        result
      ) ;let
    ) ;define

    (define (list-queue-append . list-queues)
      (list-queue-concatenate list-queues)
    ) ;define

    ; Forcibly join two queues, destroying the second
    (define (list-queue-join! queue1 queue2)
      (set-cdr! (get-last queue1) (get-first queue2))
      (set-last! queue1 (get-last queue2))
    ) ;define

    (define (list-queue-append! . queues)
      (cond
        ((null? queues) (list-queue))
        ((null? (cdr queues)) (car queues))
        (else
          (for-each (lambda (q) (list-queue-join! (car queues) q)) (cdr queues))
          (car queues)
        ) ;else
      ) ;cond
    ) ;define

    ;;; Mapping

    (define (list-queue-map proc list-queue)
      (make-list-queue (map proc (get-first list-queue)))
    ) ;define

    (define (list-queue-map! proc list-queue)
      (map! proc (get-first list-queue))
    ) ;define

    (define (list-queue-for-each proc list-queue)
      (for-each proc (get-first list-queue))
    ) ;define

    ;;; Unfold

    (define (list-queue-unfold stop? mapper successor seed . rest)
      (let ((queue (if (null? rest) (list-queue) (car rest))))
        (list-queue-unfold* stop? mapper successor seed queue)
      ) ;let
    ) ;define

    (define (list-queue-unfold* stop? mapper successor seed queue)
      (let loop ((seed seed))
        (if (not (stop? seed))
          (list-queue-add-front! (loop (successor seed)) (mapper seed))
        ) ;if
        queue
      ) ;let
    ) ;define

    (define (list-queue-unfold-right stop? mapper successor seed . rest)
      (let ((queue (if (null? rest) (list-queue) (car rest))))
        (list-queue-unfold-right* stop? mapper successor seed queue)
      ) ;let
    ) ;define

    (define (list-queue-unfold-right* stop? mapper successor seed queue)
      (let loop ((seed seed))
        (if (not (stop? seed))
          (list-queue-add-back! (loop (successor seed)) (mapper seed))
        ) ;if
        queue
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
