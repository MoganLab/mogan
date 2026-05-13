(define-library (liii string-cursor)
  (export
    ;; Cursor operations
    string-cursor?
    string-cursor-start
    string-cursor-end
    string-cursor-next
    string-cursor-prev
    string-cursor-forward
    string-cursor-back
    string-cursor=?
    string-cursor<?
    string-cursor>?
    string-cursor<=?
    string-cursor>=?
    string-cursor-diff
    string-cursor->index
    string-index->cursor

    ;; Predicates
    string-null?
    string-every
    string-any

    ;; Constructors
    string-tabulate
    string-unfold
    string-unfold-right

    ;; Conversion
    string->list/cursors
    string->vector/cursors
    reverse-list->string
    string-join

    ;; Selection
    string-ref/cursor
    substring/cursors
    string-copy/cursors
    string-take
    string-drop
    string-take-right
    string-drop-right
    string-pad
    string-pad-right
    string-trim
    string-trim-right
    string-trim-both

    ;; Prefixes & suffixes
    string-prefix-length
    string-suffix-length
    string-prefix?
    string-suffix?

    ;; Searching
    string-index
    string-index-right
    string-skip
    string-skip-right
    string-contains
    string-contains-right

    ;; The whole string
    string-reverse
    string-concatenate
    string-concatenate-reverse
    string-fold
    string-fold-right
    string-for-each-cursor
    string-replicate
    string-count
    string-replace
    string-split
    string-filter
    string-remove
  ) ;export

  (import (scheme base)
    (scheme char)
    (liii base)
    (liii error)
    (liii list)
    (liii unicode)
  ) ;import

  (begin

    ;; ==== Internal data structures ====

    (define-record-type <string-offsets>
      (make-string-offsets bv positions)
      string-offsets?
      (bv string-offsets-bv)
      (positions string-offsets-positions)
    ) ;define-record-type

    (define-record-type <string-cursor>
      (make-string-cursor-raw offsets char-index)
      string-cursor?
      (offsets string-cursor-offsets)
      (char-index string-cursor-char-index)
    ) ;define-record-type

    ;; Pre-scan a UTF-8 bytevector to generate position vector
    (define (make-string-positions bv)
      (let ((len (bytevector-length bv)))
        (let loop
          ((pos 0) (result '(0)))
          (if (>= pos len)
            (list->vector (reverse result))
            (let ((next (bytevector-advance-utf8 bv pos len)))
              (loop next (cons next result))
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    ;; ==== Cursor operations ====

    (define (string-cursor-start str)
      (let* ((bv (string->utf8 str))
             (off (make-string-offsets bv (make-string-positions bv)))
            ) ;
        (make-string-cursor-raw off 0)
      ) ;let*
    ) ;define

    (define (string-cursor-end str)
      (let* ((bv (string->utf8 str))
             (off (make-string-offsets bv (make-string-positions bv)))
             (positions (string-offsets-positions off))
            ) ;
        (make-string-cursor-raw off (- (vector-length positions) 1))
      ) ;let*
    ) ;define

    (define (string-cursor-next str cursor)
      (let* ((c (if (string-cursor? cursor) cursor (string-index->cursor str cursor)))
             (off (string-cursor-offsets c))
             (char-idx (string-cursor-char-index c))
             (positions (string-offsets-positions off))
             (max-idx (- (vector-length positions) 1))
            ) ;
        (if (>= char-idx max-idx)
          (error 'value-error "string-cursor-next: already at end cursor")
          (let ((new-idx (+ char-idx 1)))
            (if (string-cursor? cursor) (make-string-cursor-raw off new-idx) new-idx)
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (string-cursor-prev str cursor)
      (let* ((c (if (string-cursor? cursor) cursor (string-index->cursor str cursor)))
             (off (string-cursor-offsets c))
             (char-idx (string-cursor-char-index c))
            ) ;
        (if (<= char-idx 0)
          (error 'value-error "string-cursor-prev: already at start cursor")
          (let ((new-idx (- char-idx 1)))
            (if (string-cursor? cursor) (make-string-cursor-raw off new-idx) new-idx)
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (string-cursor-forward str cursor nchars)
      (let* ((c (if (string-cursor? cursor) cursor (string-index->cursor str cursor)))
             (off (string-cursor-offsets c))
             (char-idx (string-cursor-char-index c))
             (positions (string-offsets-positions off))
             (max-idx (- (vector-length positions) 1))
             (new-idx (+ char-idx nchars))
            ) ;
        (if (or (< new-idx 0) (> new-idx max-idx))
          (error 'value-error "string-cursor-forward: result would be invalid cursor")
          (if (string-cursor? cursor) (make-string-cursor-raw off new-idx) new-idx)
        ) ;if
      ) ;let*
    ) ;define

    (define (string-cursor-back str cursor nchars)
      (string-cursor-forward str cursor (- nchars))
    ) ;define

    (define (string-cursor=? cursor1 cursor2)
      (let ((idx1 (if (string-cursor? cursor1) (string-cursor-char-index cursor1) cursor1))
            (idx2 (if (string-cursor? cursor2) (string-cursor-char-index cursor2) cursor2))
           ) ;
        (= idx1 idx2)
      ) ;let
    ) ;define

    (define (string-cursor<? cursor1 cursor2)
      (let ((idx1 (if (string-cursor? cursor1) (string-cursor-char-index cursor1) cursor1))
            (idx2 (if (string-cursor? cursor2) (string-cursor-char-index cursor2) cursor2))
           ) ;
        (< idx1 idx2)
      ) ;let
    ) ;define

    (define (string-cursor>? cursor1 cursor2)
      (let ((idx1 (if (string-cursor? cursor1) (string-cursor-char-index cursor1) cursor1))
            (idx2 (if (string-cursor? cursor2) (string-cursor-char-index cursor2) cursor2))
           ) ;
        (> idx1 idx2)
      ) ;let
    ) ;define

    (define (string-cursor<=? cursor1 cursor2)
      (let ((idx1 (if (string-cursor? cursor1) (string-cursor-char-index cursor1) cursor1))
            (idx2 (if (string-cursor? cursor2) (string-cursor-char-index cursor2) cursor2))
           ) ;
        (<= idx1 idx2)
      ) ;let
    ) ;define

    (define (string-cursor>=? cursor1 cursor2)
      (let ((idx1 (if (string-cursor? cursor1) (string-cursor-char-index cursor1) cursor1))
            (idx2 (if (string-cursor? cursor2) (string-cursor-char-index cursor2) cursor2))
           ) ;
        (>= idx1 idx2)
      ) ;let
    ) ;define

    (define (string-cursor-diff str start end)
      (validate-start-end start end)
      (let ((s-idx (if (string-cursor? start) (string-cursor-char-index start) start))
            (e-idx (if (string-cursor? end) (string-cursor-char-index end) end))
           ) ;
        (- e-idx s-idx)
      ) ;let
    ) ;define

    (define (string-cursor->index str cursor)
      (if (string-cursor? cursor) (string-cursor-char-index cursor) cursor)
    ) ;define

    (define (string-index->cursor str index)
      (if (string-cursor? index)
        index
        (let* ((bv (string->utf8 str))
               (off (make-string-offsets bv (make-string-positions bv)))
               (positions (string-offsets-positions off))
               (max-idx (- (vector-length positions) 1))
              ) ;
          (if (or (< index 0) (> index max-idx))
            (error 'value-error "string-index->cursor: index out of range")
            (make-string-cursor-raw off index)
          ) ;if
        ) ;let*
      ) ;if
    ) ;define

    ;; ==== Helper functions ====

    (define (cursor->index c)
      (if (string-cursor? c) (string-cursor-char-index c) c)
    ) ;define

    (define (validate-start-end start end)
      (let ((start-cursor? (string-cursor? start)) (end-cursor? (string-cursor? end)))
        (when (and (not start-cursor?) (not (integer? start)))
          (error 'type-error "start must be integer or cursor")
        ) ;when
        (when (and (not end-cursor?) (not (integer? end)))
          (error 'type-error "end must be integer or cursor")
        ) ;when
        (when (not (eq? start-cursor? end-cursor?))
          (error 'type-error "start and end must both be integer or both be cursor")
        ) ;when
        (let ((start-idx (if start-cursor? (string-cursor-char-index start) start))
              (end-idx (if end-cursor? (string-cursor-char-index end) end))
             ) ;
          (when (> start-idx end-idx)
            (error 'value-error "start must be <= end")
          ) ;when
          (when (< start-idx 0)
            (error 'value-error "start must be >= 0")
          ) ;when
          (when (< end-idx 0)
            (error 'value-error "end must be >= 0")
          ) ;when
        ) ;let
      ) ;let
    ) ;define

    (define (list->utf8-string chars)
      (let ((bvs (map (lambda (ch) (codepoint->utf8 (char->integer ch))) chars)))
        (if (null? bvs) "" (utf8->string (apply bytevector-append bvs)))
      ) ;let
    ) ;define

    ;; ==== Selection ====

    (define (string-ref/cursor str cursor)
      (let* ((c (if (string-cursor? cursor) cursor (string-index->cursor str cursor)))
             (off (string-cursor-offsets c))
             (bv (string-offsets-bv off))
             (pos (string-offsets-positions off))
             (idx (string-cursor-char-index c))
             (max-idx (- (vector-length pos) 1))
             (_ (when (>= idx max-idx)
                  (error 'value-error "string-ref/cursor: cursor at or past end of string")
                ) ;when
             ) ;_
             (start (vector-ref pos idx))
            ) ;
        (integer->char (utf8->codepoint-at bv start))
      ) ;let*
    ) ;define

    (define (substring/cursors str start end)
      (validate-start-end start end)
      (let* ((start-off (if (string-cursor? start)
                          (string-cursor-offsets start)
                          (let ((bv (string->utf8 str)))
                            (make-string-offsets bv (make-string-positions bv))
                          ) ;let
                        ) ;if
             ) ;start-off
             (end-off (if (string-cursor? end) (string-cursor-offsets end) start-off))
             (pos (string-offsets-positions start-off))
             (bv (string-offsets-bv start-off))
             (start-idx (if (string-cursor? start) (string-cursor-char-index start) start))
             (end-idx (if (string-cursor? end) (string-cursor-char-index end) end))
             (max-idx (- (vector-length pos) 1))
             (_ (when (> end-idx max-idx)
                  (error 'value-error "substring/cursors: end index out of range")
                ) ;when
             ) ;_
             (byte-start (vector-ref pos start-idx))
             (byte-end (vector-ref pos end-idx))
            ) ;
        (utf8->string (bytevector-copy bv byte-start byte-end))
      ) ;let*
    ) ;define

    (define (string-copy/cursors str . maybe-start+end)
      (let* ((bv (string->utf8 str))
             (off (make-string-offsets bv (make-string-positions bv)))
             (positions (string-offsets-positions off))
             (len (- (vector-length positions) 1))
             (end-c-raw (make-string-cursor-raw off len))
            ) ;
        (if (null? maybe-start+end)
          (substring/cursors str (make-string-cursor-raw off 0) end-c-raw)
          (let ((start (car maybe-start+end)) (rest (cdr maybe-start+end)))
            (let ((end (if (null? rest) (if (string-cursor? start) end-c-raw len) (car rest))))
              (substring/cursors str start end)
            ) ;let
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    ;; ==== String operations ====

    (define (string-take str nchars)
      (let ((end (string-index->cursor str nchars)))
        (substring/cursors str (string-cursor-start str) end)
      ) ;let
    ) ;define

    (define (string-drop str nchars)
      (let ((start (string-index->cursor str nchars)))
        (substring/cursors str start (string-cursor-end str))
      ) ;let
    ) ;define

    (define (string-take-right str nchars)
      (let* ((end (string-cursor-end str)) (start (string-cursor-back str end nchars)))
        (substring/cursors str start end)
      ) ;let*
    ) ;define

    (define (string-drop-right str nchars)
      (let* ((end (string-cursor-end str)) (new-end (string-cursor-back str end nchars)))
        (substring/cursors str (string-cursor-start str) new-end)
      ) ;let*
    ) ;define

    ;; ==== Predicates ====

    (define (string-null? str)
      (zero? (string-length str))
    ) ;define

    (define (string-every pred s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c))
          (if (string-cursor>=? cur end-c)
            #t
            (let ((result (pred (string-ref/cursor s cur))))
              (if result
                (let ((next (string-cursor-next s cur)))
                  (if (string-cursor>=? next end-c) result (loop next))
                ) ;let
                #f
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-any pred s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c))
          (if (string-cursor>=? cur end-c)
            #f
            (let ((result (pred (string-ref/cursor s cur))))
              (if result result (loop (string-cursor-next s cur)))
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Fold and iteration ====

    (define (string-fold kons knil s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((acc knil) (cur start-c))
          (if (string-cursor>=? cur end-c)
            acc
            (loop (kons (string-ref/cursor s cur) acc) (string-cursor-next s cur))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-fold-right kons knil s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        ;; Non-recursive implementation using iteration
        (let ((chars (let collect
                       ((cur start-c) (result '()))
                       (if (string-cursor>=? cur end-c)
                         result
                         (collect (string-cursor-next s cur) (cons (string-ref/cursor s cur) result))
                       ) ;if
                     ) ;let
              ) ;chars
             ) ;
          ;; chars is reversed: '(cn ... c2 c1)
          ;; Iterate from left to right to build fold-right result
          (let loop
            ((lst chars) (acc knil))
            (if (null? lst) acc (loop (cdr lst) (kons (car lst) acc)))
          ) ;let
        ) ;let
      ) ;let*
    ) ;define

    (define (string-for-each-cursor proc s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c))
          (when (string-cursor<? cur end-c)
            (proc cur)
            (loop (string-cursor-next s cur))
          ) ;when
        ) ;let
      ) ;let*
    ) ;define

    (define (string-count pred s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c) (count 0))
          (if (string-cursor>=? cur end-c)
            count
            (loop (string-cursor-next s cur)
              (if (pred (string-ref/cursor s cur)) (+ count 1) count)
            ) ;loop
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Searching ====

    (define (string-index s pred . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c))
          (if (string-cursor>=? cur end-c)
            end-c
            (if (pred (string-ref/cursor s cur)) cur (loop (string-cursor-next s cur)))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-index-right s pred . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (if (string-cursor=? start-c end-c)
          start-c
          (let loop
            ((cur (string-cursor-prev s end-c)))
            (cond ((pred (string-ref/cursor s cur)) (string-cursor-next s cur))
                  ((string-cursor=? cur start-c) start-c)
                  (else (loop (string-cursor-prev s cur)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (string-skip s pred . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (let loop
          ((cur start-c))
          (if (string-cursor>=? cur end-c)
            end-c
            (if (pred (string-ref/cursor s cur)) (loop (string-cursor-next s cur)) cur)
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-skip-right s pred . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end))
            ) ;
        (if (string-cursor=? start-c end-c)
          start-c
          (let loop
            ((cur (string-cursor-prev s end-c)))
            (if (pred (string-ref/cursor s cur))
              (if (string-cursor=? cur start-c) start-c (loop (string-cursor-prev s cur)))
              (string-cursor-next s cur)
            ) ;if
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    ;; ==== Trim and Pad ====

    (define* (string-trim s (pred char-whitespace?) (start 0) (end #t))
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let ((trimmed-start (string-skip s pred start-c end-c)))
          (substring/cursors s trimmed-start end-c)
        ) ;let
      ) ;let*
    ) ;define*

    (define* (string-trim-right s (pred char-whitespace?) (start 0) (end #t))
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let ((trimmed-end (string-skip-right s pred start-c end-c)))
          (substring/cursors s start-c trimmed-end)
        ) ;let
      ) ;let*
    ) ;define*

    (define* (string-trim-both s (pred char-whitespace?) (start 0) (end #t))
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let ((trimmed-start (string-skip s pred start-c end-c))
              (trimmed-end (string-skip-right s pred start-c end-c))
             ) ;
          (if (string-cursor>=? trimmed-start trimmed-end)
            ""
            (substring/cursors s trimmed-start trimmed-end)
          ) ;if
        ) ;let
      ) ;let*
    ) ;define*

    (define* (string-pad s len (char #\space) (start 0) (end #t))
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end-idx))
             (sub (substring/cursors s start-c end-c))
             (sub-len (string-cursor-diff sub (string-cursor-start sub) (string-cursor-end sub))
             ) ;sub-len
            ) ;
        (if (>= sub-len len)
          (string-take-right sub len)
          (string-append (make-string (- len sub-len) char) sub)
        ) ;if
      ) ;let*
    ) ;define*

    (define* (string-pad-right s len (char #\space) (start 0) (end #t))
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (string-index->cursor s start))
             (end-c (string-index->cursor s end-idx))
             (sub (substring/cursors s start-c end-c))
             (sub-len (string-cursor-diff sub (string-cursor-start sub) (string-cursor-end sub))
             ) ;sub-len
            ) ;
        (if (>= sub-len len)
          (string-take sub len)
          (string-append sub (make-string (- len sub-len) char))
        ) ;if
      ) ;let*
    ) ;define*

    ;; ==== Prefix and Suffix ====

    (define (string-prefix-length s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
             (off1 (string-cursor-offsets end1-c-raw))
             (pos1 (string-offsets-positions off1))
             (bv1 (string-offsets-bv off1))
             (off2 (string-cursor-offsets end2-c-raw))
             (pos2 (string-offsets-positions off2))
             (bv2 (string-offsets-bv off2))
            ) ;
        (let loop
          ((i start1-idx) (j start2-idx) (count 0))
          (if (or (>= i end1-idx) (>= j end2-idx))
            count
            (let* ((b1-start (vector-ref pos1 i))
                   (ch1 (integer->char (utf8->codepoint-at bv1 b1-start)))
                   (b2-start (vector-ref pos2 j))
                   (ch2 (integer->char (utf8->codepoint-at bv2 b2-start)))
                  ) ;
              (if (char=? ch1 ch2) (loop (+ i 1) (+ j 1) (+ count 1)) count)
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-suffix-length s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
             (off1 (string-cursor-offsets end1-c-raw))
             (pos1 (string-offsets-positions off1))
             (bv1 (string-offsets-bv off1))
             (off2 (string-cursor-offsets end2-c-raw))
             (pos2 (string-offsets-positions off2))
             (bv2 (string-offsets-bv off2))
            ) ;
        (let loop
          ((i (- end1-idx 1)) (j (- end2-idx 1)) (count 0))
          (if (or (< i start1-idx) (< j start2-idx))
            count
            (let* ((b1-start (vector-ref pos1 i))
                   (ch1 (integer->char (utf8->codepoint-at bv1 b1-start)))
                   (b2-start (vector-ref pos2 j))
                   (ch2 (integer->char (utf8->codepoint-at bv2 b2-start)))
                  ) ;
              (if (char=? ch1 ch2) (loop (- i 1) (- j 1) (+ count 1)) count)
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-prefix? s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
            ) ;
        (let ((len1 (- end1-idx start1-idx)))
          (and (<= len1 (- end2-idx start2-idx))
            (= (string-prefix-length s1 s2 start1-idx end1-idx start2-idx end2-idx) len1)
          ) ;and
        ) ;let
      ) ;let*
    ) ;define

    (define (string-suffix? s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
            ) ;
        (let ((len1 (- end1-idx start1-idx)))
          (and (<= len1 (- end2-idx start2-idx))
            (= (string-suffix-length s1 s2 start1-idx end1-idx start2-idx end2-idx) len1)
          ) ;and
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Contains ====

    (define (string-prefix-at? s1 s2 s1-pos s2-start s2-end off1 pos1 bv1 off2 pos2 bv2)
      ;; Check if s2[s2-start:s2-end] matches s1 at character position s1-pos
      ;; Uses pre-computed offsets for O(m) comparison without re-scanning
      (let loop
        ((i s1-pos) (j s2-start))
        (if (>= j s2-end)
          #t
          (let* ((b1-start (vector-ref pos1 i))
                 (ch1 (integer->char (utf8->codepoint-at bv1 b1-start)))
                 (b2-start (vector-ref pos2 j))
                 (ch2 (integer->char (utf8->codepoint-at bv2 b2-start)))
                ) ;
            (if (char=? ch1 ch2) (loop (+ i 1) (+ j 1)) #f)
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (string-contains s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
             (off1 (string-cursor-offsets end1-c-raw))
             (pos1 (string-offsets-positions off1))
             (bv1 (string-offsets-bv off1))
             (off2 (string-cursor-offsets end2-c-raw))
             (pos2 (string-offsets-positions off2))
             (bv2 (string-offsets-bv off2))
            ) ;
        (let ((s2-len (- end2-idx start2-idx)))
          (if (zero? s2-len)
            (string-index->cursor s1 start1-idx)
            (let loop
              ((i start1-idx))
              (if (> (+ i s2-len) end1-idx)
                #f
                (if (string-prefix-at? s1 s2 i start2-idx end2-idx off1 pos1 bv1 off2 pos2 bv2)
                  (string-index->cursor s1 i)
                  (loop (+ i 1))
                ) ;if
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-contains-right s1 s2 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (min (cursor->index end1) char-len1))
             (start2-idx (cursor->index start2))
             (end2-idx (min (cursor->index end2) char-len2))
             (off1 (string-cursor-offsets end1-c-raw))
             (pos1 (string-offsets-positions off1))
             (bv1 (string-offsets-bv off1))
             (off2 (string-cursor-offsets end2-c-raw))
             (pos2 (string-offsets-positions off2))
             (bv2 (string-offsets-bv off2))
            ) ;
        (let ((s2-len (- end2-idx start2-idx)))
          (if (zero? s2-len)
            (string-index->cursor s1 end1-idx)
            (let loop
              ((i (- end1-idx s2-len)))
              (if (< i start1-idx)
                #f
                (if (string-prefix-at? s1 s2 i start2-idx end2-idx off1 pos1 bv1 off2 pos2 bv2)
                  (string-index->cursor s1 i)
                  (loop (- i 1))
                ) ;if
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-concatenate string-list)
      (apply string-append string-list)
    ) ;define

    (define (string-concatenate-reverse string-list . maybe-final+end)
      (let* ((final (if (null? maybe-final+end) "" (car maybe-final+end)))
             (rest (if (null? maybe-final+end) '() (cdr maybe-final+end)))
             (end (if (null? rest)
                    (string-cursor-char-index (string-cursor-end final))
                    (car rest)
                  ) ;if
             ) ;end
             (end-idx (cursor->index end))
             (final-part (substring/cursors final 0 end-idx))
            ) ;
        (let* ((all-strings (reverse (cons final-part string-list)))
               (bvs (map string->utf8 all-strings))
              ) ;
          (if (null? bvs) "" (utf8->string (apply bytevector-append bvs)))
        ) ;let*
      ) ;let*
    ) ;define

    (define (string-reverse s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (min (cursor->index end) char-len))
             (start-c (string-index->cursor s start-idx))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let loop
          ((cur start-c) (result '()))
          (if (string-cursor>=? cur end-c)
            (list->utf8-string result)
            (loop (string-cursor-next s cur) (cons (string-ref/cursor s cur) result))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-replicate s from . maybe-to+start+end)
      (when (string-cursor? from)
        (error 'type-error "string-replicate: from cannot be a cursor")
      ) ;when
      (when (null? maybe-to+start+end)
        (error 'value-error "string-replicate: to argument is required")
      ) ;when
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (to (car maybe-to+start+end))
             (start (if (null? (cdr maybe-to+start+end)) 0 (cadr maybe-to+start+end)))
             (rest1 (if (null? (cdr maybe-to+start+end)) '() (cddr maybe-to+start+end)))
             (end (if (null? rest1) char-len (car rest1)))
             (_ (when (string-cursor? to)
                  (error 'type-error "string-replicate: to cannot be a cursor")
                ) ;when
             ) ;_
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (cursor->index end))
             (from-idx from)
             (to-idx to)
             (slen (- end-idx start-idx))
             (anslen (- to-idx from-idx))
             (start-c (string-index->cursor s start-idx))
             (source-chars (let loop
                             ((cur start-c) (n 0) (result '()))
                             (if (>= n slen)
                               (list->vector (reverse result))
                               (loop (string-cursor-next s cur)
                                 (+ n 1)
                                 (cons (string-ref/cursor s cur) result)
                               ) ;loop
                             ) ;if
                           ) ;let
             ) ;source-chars
            ) ;
        (when (> from-idx to-idx)
          (error 'value-error "string-replicate: from > to")
        ) ;when
        (cond ((zero? anslen) "")
              ((zero? slen) (error 'value-error "Cannot replicate empty substring"))
              (else (let loop
                      ((i 0) (result '()))
                      (if (>= i anslen)
                        (list->utf8-string (reverse result))
                        (let ((ch (vector-ref source-chars (modulo (+ from-idx i) slen))))
                          (loop (+ i 1) (cons ch result))
                        ) ;let
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    (define (string-replace s1 s2 start1 end1 . maybe-start+end)
      (let* ((end1-c-raw (string-cursor-end s1))
             (char-len1 (string-cursor-char-index end1-c-raw))
             (end2-c-raw (string-cursor-end s2))
             (char-len2 (string-cursor-char-index end2-c-raw))
             (start2 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end2 (if (null? rest) char-len2 (car rest)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (cursor->index start1))
             (end1-idx (cursor->index end1))
             (start2-idx (cursor->index start2))
             (end2-idx (cursor->index end2))
             (before (substring/cursors s1 0 start1-idx))
             (middle (substring/cursors s2 start2-idx end2-idx))
             (after (substring/cursors s1 end1-idx char-len1))
            ) ;
        (string-append before middle after)
      ) ;let*
    ) ;define

    (define (string-split s delimiter . args)
      (let* ((slen (string-cursor-char-index (string-cursor-end s)))
             (grammar (if (null? args) 'infix (car args)))
             (rest1 (if (null? args) '() (cdr args)))
             (limit (if (null? rest1) #f (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end (if (null? rest3) slen (car rest3)))
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (cursor->index end))
             (start-c (string-index->cursor s start-idx))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (cond ((= start-idx end-idx)
               (if (eq? grammar 'strict-infix)
                 (error 'value-error "empty string cannot be split with strict-infix grammar")
                 '()
               ) ;if
              ) ;
              ((string-null? delimiter)
               (let loop
                 ((cur start-c) (result '()) (n 0))
                 (cond ((string-cursor>=? cur end-c) (reverse result))
                       ((and limit (>= n limit))
                        (reverse (cons (substring/cursors s cur end-c) result))
                       ) ;
                       (else (loop (string-cursor-next s cur)
                               (cons (string (string-ref/cursor s cur)) result)
                               (+ n 1)
                             ) ;loop
                       ) ;else
                 ) ;cond
               ) ;let
              ) ;
              (else (let ((dlen (string-cursor-char-index (string-cursor-end delimiter))))
                      (define (finish r c)
                        (let ((rest-str (substring/cursors s c end-c)))
                          (if (and (eq? grammar 'suffix) (string-null? rest-str))
                            (reverse r)
                            (reverse (cons rest-str r))
                          ) ;if
                        ) ;let
                      ) ;define
                      (define (scan r c n)
                        (if (and limit (>= n limit))
                          (finish r c)
                          (let ((i (string-contains s delimiter c end-c)))
                            (if i
                              (let ((fragment (substring/cursors s c i)))
                                (if (and (= n 0) (eq? grammar 'prefix) (string-null? fragment))
                                  (scan r (string-cursor-forward s i dlen) (+ n 1))
                                  (scan (cons fragment r) (string-cursor-forward s i dlen) (+ n 1))
                                ) ;if
                              ) ;let
                              (finish r c)
                            ) ;if
                          ) ;let
                        ) ;if
                      ) ;define
                      (scan '() start-c 0)
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    (define (string-filter pred s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (cursor->index end))
             (start-c (string-index->cursor s start-idx))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let loop
          ((cur start-c) (result '()))
          (if (string-cursor>=? cur end-c)
            (list->utf8-string (reverse result))
            (let ((ch (string-ref/cursor s cur)))
              (if (pred ch)
                (loop (string-cursor-next s cur) (cons ch result))
                (loop (string-cursor-next s cur) result)
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-remove pred s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (cursor->index end))
             (start-c (string-index->cursor s start-idx))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let loop
          ((cur start-c) (result '()))
          (if (string-cursor>=? cur end-c)
            (list->utf8-string (reverse result))
            (let ((ch (string-ref/cursor s cur)))
              (if (pred ch)
                (loop (string-cursor-next s cur) result)
                (loop (string-cursor-next s cur) (cons ch result))
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Constructors ====

    (define (string-tabulate proc len)
      (let loop
        ((i 0) (result '()))
        (if (>= i len)
          (list->utf8-string (reverse result))
          (loop (+ i 1) (cons (proc i) result))
        ) ;if
      ) ;let
    ) ;define

    (define (string-unfold p f g seed . base+make-final)
      (let* ((base (if (null? base+make-final) "" (car base+make-final)))
             (rest (if (null? base+make-final) '() (cdr base+make-final)))
             (make-final (if (null? rest) (lambda (x) "") (car rest)))
            ) ;
        (let loop
          ((seed seed) (result '()))
          (if (p seed)
            (string-append base (list->utf8-string (reverse result)) (make-final seed))
            (loop (g seed) (cons (f seed) result))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-unfold-right p f g seed . base+make-final)
      (let* ((base (if (null? base+make-final) "" (car base+make-final)))
             (rest (if (null? base+make-final) '() (cdr base+make-final)))
             (make-final (if (null? rest) (lambda (x) "") (car rest)))
            ) ;
        (let loop
          ((seed seed) (result '()))
          (if (p seed)
            (string-append (make-final seed) (list->utf8-string result) base)
            (loop (g seed) (cons (f seed) result))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Conversion ====

    (define (string->list/cursors s . maybe-start+end)
      (let* ((end-c-raw (string-cursor-end s))
             (char-len (string-cursor-char-index end-c-raw))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-idx (cursor->index start))
             (end-idx (cursor->index end))
             (start-c (string-index->cursor s start-idx))
             (end-c (string-index->cursor s end-idx))
            ) ;
        (let loop
          ((cur start-c) (result '()))
          (if (string-cursor>=? cur end-c)
            (reverse result)
            (loop (string-cursor-next s cur) (cons (string-ref/cursor s cur) result))
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string->vector/cursors s . maybe-start+end)
      (list->vector (apply string->list/cursors s maybe-start+end))
    ) ;define

    (define (reverse-list->string clist)
      (list->utf8-string (reverse clist))
    ) ;define

    (define* (string-join string-list (delimiter " ") (grammar 'infix))
      (cond ((null? string-list)
             (if (eq? grammar 'strict-infix)
               (error 'value-error "string-join: empty list with strict-infix")
               ""
             ) ;if
            ) ;
            ((not (memq grammar '(infix strict-infix suffix prefix)))
             (error 'value-error "string-join: invalid grammar")
            ) ;
            (else (let ((del-bv (string->utf8 delimiter)) (str-bvs (map string->utf8 string-list)))
                    (define (interleave bvs)
                      (let loop
                        ((lst bvs))
                        (if (null? lst)
                          '()
                          (if (or (eq? grammar 'infix) (eq? grammar 'strict-infix))
                            (if (null? (cdr lst))
                              (list (car lst))
                              (cons (car lst) (cons del-bv (loop (cdr lst))))
                            ) ;if
                            (if (eq? grammar 'suffix)
                              (cons (car lst) (cons del-bv (loop (cdr lst))))
                              (if (eq? grammar 'prefix) (cons del-bv (cons (car lst) (loop (cdr lst)))) '())
                            ) ;if
                          ) ;if
                        ) ;if
                      ) ;let
                    ) ;define
                    (utf8->string (apply bytevector-append (interleave str-bvs)))
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define*

  ) ;begin
) ;define-library
