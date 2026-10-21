;;
;; Copyright (C) 2024 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

(define-library (liii sse)
  (export <sse-event> make-sse-event sse-event? sse-event-event sse-event-data
    sse-event-id sse-event-retry sse-event-json read-sse-event make-sse-decoder
    sse-event->string write-sse-event
  ) ;export
  (import (scheme base) (liii error) (liii json) (liii string))
  (begin

    (define-record-type <sse-event>
      (%make-sse-event event data id retry)
      sse-event?
      (event sse-event-event)
      (data sse-event-data)
      (id sse-event-id)
      (retry sse-event-retry)
    ) ;define-record-type

    (define <sse-event> '<sse-event>)

    (define %default-event "message")

    (define (%normalize-event ev)
      (cond
       ((or (not ev) (and (string? ev) (string=? ev ""))) %default-event)
       ((string? ev) ev)
       (else (type-error "make-sse-event: event must be a string or #f"))
      ) ;cond
    ) ;define

    (define (make-sse-event . args)
      (let* ((n (length args))
             (ev-arg (if (> n 1) (car args) %default-event))
             (data (cond ((= n 1) (car args))
                         ((<= 2 n 4) (cadr args))
                         (else (type-error "make-sse-event: expected 1 to 4 arguments"))
                   ) ;cond
             ) ;data
             (id (and (> n 2) (caddr args)))
             (retry (and (= n 4) (cadddr args)))
             (ev (%normalize-event ev-arg))
            ) ;
        (unless (string? data)
          (type-error "make-sse-event: data must be a string")
        ) ;unless
        (when (and id (not (string? id)))
          (type-error "make-sse-event: id must be a string or #f")
        ) ;when
        (when
          (and retry (not (and (integer? retry) (exact? retry))))
          (type-error "make-sse-event: retry must be an integer or #f")
        ) ;when
        (when (and retry (< retry 0))
          (value-error "make-sse-event: retry must be non-negative")
        ) ;when
        (%make-sse-event ev data id retry)
      ) ;let*
    ) ;define

    (define (sse-event-json event)
      (unless (sse-event? event)
        (type-error "sse-event-json: expected <sse-event>")
      ) ;unless
      (string->json (sse-event-data event))
    ) ;define

    (define (strip-eol line)
      (string-remove-suffix (string-remove-suffix line "\n") "\r")
    ) ;define

    (define (parse-sse-line line)
      (let ((i (string-index line #\:)) (len (string-length line)))
        (cond ((not i) (values line ""))
              ((and (< (+ i 1) len) (char=? (string-ref line (+ i 1)) #\space))
               (values (substring line 0 i) (substring line (+ i 2) len))
              ) ;
              (else (values (substring line 0 i) (substring line (+ i 1) len)))
        ) ;cond
      ) ;let
    ) ;define

    (define (%parse-sse-retry val)
      (let ((n (string->number val 10)))
        (and (exact-integer? n) (>= n 0) n)
      ) ;let
    ) ;define

    ;; ; 事件累积状态机：feed-line! 逐行喂入，空行或 dispatch! 触发事件派发。
    ;; ; 拆出供 make-sse-decoder（chunk 流）与 read-sse-event（逐行）共用。
    (define (%make-sse-line-decoder on-event)
      (let ((cur-event %default-event)
            (cur-lines '())
            (cur-id #f)
            (cur-retry #f)
            (has-field #f)
           ) ;

        (define (dispatch!)
          (when has-field
            (let ((ev (make-sse-event cur-event
                        (string-join (reverse cur-lines) "\n")
                        cur-id
                        cur-retry
                      ) ;make-sse-event
                  ) ;ev
                 ) ;
              (set! cur-event %default-event)
              (set! cur-lines '())
              (set! cur-id #f)
              (set! cur-retry #f)
              (set! has-field #f)
              (on-event ev)
            ) ;let
          ) ;when
        ) ;define

        (define (feed-field! name val)
          (cond ((string=? name "data")
                 (set! cur-lines (cons val cur-lines))
                 (set! has-field #t)
                ) ;
                ((string=? name "event") (set! cur-event val) (set! has-field #t))
                ((string=? name "id")
                 (unless (string-index val #\null)
                   (set! cur-id val)
                   (set! has-field #t)
                 ) ;unless
                ) ;
                ((string=? name "retry")
                 (let ((n (%parse-sse-retry val)))
                   (when n
                     (set! cur-retry n)
                     (set! has-field #t)
                   ) ;when
                 ) ;let
                ) ;
          ) ;cond
        ) ;define

        (define (feed-line! line)
          (cond ((string=? line "") (dispatch!))
                ((char=? (string-ref line 0) #\:) #f)
                (else
                  (let-values (((name val) (parse-sse-line line)))
                    (feed-field! name val)
                  ) ;let-values
                ) ;else
          ) ;cond
        ) ;define

        (values feed-line! dispatch!)
      ) ;let
    ) ;define

    (define (make-sse-decoder on-event)
      (unless (procedure? on-event)
        (type-error "make-sse-decoder: on-event must be a procedure")
      ) ;unless
      (let-values (((feed-line! dispatch!) (%make-sse-line-decoder on-event)))
        ;; buffer 只保留未处理的尾部残行：每轮 feed-chunk! 提取完所有完整行后
        ;; 立即 compact，避免 string-append 在长流上退化为 O(n^2)
        (let ((buffer ""))

          (define (feed-chunk! chunk)
            (set! buffer (string-append buffer chunk))
            (let loop
              ((offset 0))
              (let ((nl (string-index buffer #\newline offset)))
                (if nl
                  (begin
                    (feed-line! (strip-eol (substring buffer offset nl)))
                    (loop (+ nl 1))
                  ) ;begin
                  (when (> offset 0)
                    (set! buffer (substring buffer offset (string-length buffer)))
                  ) ;when
                ) ;if
              ) ;let
            ) ;let
          ) ;define

          (lambda (chunk)
            (cond
             ((eof-object? chunk)
              (unless (string=? buffer "")
                (feed-line! (strip-eol buffer))
                (set! buffer "")
              ) ;unless
              (dispatch!)
             ) ;
             ((string? chunk) (feed-chunk! chunk))
             (else (type-error "sse-decoder: chunk must be a string or eof-object"))
            ) ;cond
          ) ;lambda
        ) ;let
      ) ;let-values
    ) ;define

    (define (%resolve-port who args default-port port?)
      (cond ((null? args) default-port)
            ((and (null? (cdr args)) (port? (car args))) (car args))
            (else (type-error (string-append who ": invalid port argument")))
      ) ;cond
    ) ;define

    (define (read-sse-event . args)
      (let ((port (%resolve-port "read-sse-event" args (current-input-port) input-port?))
            (result #f)
           ) ;
        (let-values (((feed-line! dispatch!) (%make-sse-line-decoder (lambda (ev) (set! result ev))))
                    ) ;
          ;; read-line 已去除 \n，逐行直接喂入状态机，无需拼接回 chunk 再重拆
          (let loop
            ()
            (let ((line (read-line port)))
              (cond ((eof-object? line) (dispatch!) (or result line))
                    (else (feed-line! (strip-eol line)) (or result (loop)))
              ) ;cond
            ) ;let
          ) ;let
        ) ;let-values
      ) ;let
    ) ;define

    (define (%write-sse-event-port event p)
      (let ((ev (sse-event-event event))
            (id (sse-event-id event))
            (retry (sse-event-retry event))
           ) ;
        (define (emit name val)
          (display name p)
          (display ": " p)
          (display val p)
          (newline p)
        ) ;define
        (when (not (string=? ev %default-event))
          (emit "event" ev)
        ) ;when
        (for-each (lambda (line) (emit "data" line))
          (string-split (sse-event-data event) "\n")
        ) ;for-each
        (when (string? id)
          (emit "id" id)
        ) ;when
        (when (and (integer? retry) (>= retry 0))
          (emit "retry" (number->string retry))
        ) ;when
        (newline p)
      ) ;let
    ) ;define

    (define (sse-event->string event)
      (unless (sse-event? event)
        (type-error "sse-event->string: expected <sse-event>")
      ) ;unless
      (let ((p (open-output-string)))
        (%write-sse-event-port event p)
        (get-output-string p)
      ) ;let
    ) ;define

    (define (write-sse-event event . args)
      (unless (sse-event? event)
        (type-error "write-sse-event: expected <sse-event>")
      ) ;unless
      (%write-sse-event-port event
        (%resolve-port "write-sse-event" args (current-output-port) output-port?)
      ) ;%write-sse-event-port
    ) ;define

  ) ;begin
) ;define-library
