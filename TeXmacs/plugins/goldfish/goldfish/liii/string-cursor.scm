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

    ;; ==== Cursor representation ====
    ;; 核心原语由 src/liii_string_cursor.cpp 实现（g_* 函数）。
    ;; 游标表示为负整数 -(byte_offset+2)，即字节 0 对应 -2。
    ;; -1 不是合法游标，保留给"负索引"错误语义。

    (define (string-cursor? obj)
      (and (integer? obj) (< obj -1))
    ) ;define

    (define string-cursor-start g_string-cursor-start)
    (define string-cursor-end g_string-cursor-end)
    (define string-cursor-next g_string-cursor-next)
    (define string-cursor-prev g_string-cursor-prev)
    (define string-cursor-forward g_string-cursor-forward)
    (define string-cursor-back g_string-cursor-back)
    (define string-cursor=? g_string-cursor=?)
    (define string-cursor<? g_string-cursor<?)
    (define string-cursor>? g_string-cursor>?)
    (define string-cursor<=? g_string-cursor<=?)
    (define string-cursor>=? g_string-cursor>=?)
    (define string-ref/cursor g_string-ref/cursor)

    ;; ==== Helper functions ====

    ;; 将索引或游标统一转换为游标
    (define (as-cursor s x)
      (cond ((string-cursor? x) x)
            ((integer? x) (string-index->cursor s x))
            (else (error 'type-error "cursor argument must be integer or cursor"))
      ) ;cond
    ) ;define

    ;; 将索引或游标转换为游标，索引超出 char-len 时截断到 char-len
    (define (as-cursor-clamped s x char-len)
      (if (string-cursor? x) x (string-index->cursor s (min x char-len)))
    ) ;define

    (define (validate-start-end start end)
      (when (not (integer? start))
        (error 'type-error "start must be integer or cursor")
      ) ;when
      (when (not (integer? end))
        (error 'type-error "end must be integer or cursor")
      ) ;when
      (cond
        ;; 两者均为负整数：游标模式
        ((and (string-cursor? start) (string-cursor? end))
         (when (string-cursor>? start end)
           (error 'value-error "start must be <= end")
         ) ;when
        ) ;
        ;; start 为负、end 为非负：按旧索引语义报 value-error
        ((string-cursor? start) (error 'value-error "start must be >= 0"))
        ;; start 为非负、end 为负：视为游标与索引混用
        ((string-cursor? end)
         (error 'type-error "start and end must both be integer or both be cursor")
        ) ;
        ;; 两者均为非负整数：索引模式（-1 视为负索引）
        (else (when (> start end)
                (error 'value-error "start must be <= end")
              ) ;when
          (when (< start 0)
            (error 'value-error "start must be >= 0")
          ) ;when
          (when (< end 0)
            (error 'value-error "end must be >= 0")
          ) ;when
        ) ;else
      ) ;cond
    ) ;define

    (define (string-cursor-diff str start end)
      (validate-start-end start end)
      (g_string-cursor-diff str start end)
    ) ;define

    (define (string-cursor->index str cursor)
      (g_string-cursor->index str cursor)
    ) ;define

    (define (string-index->cursor str index)
      (g_string-index->cursor str index)
    ) ;define

    (define (list->utf8-string chars)
      (let ((bvs (map (lambda (ch) (codepoint->utf8 (char->integer ch))) chars)))
        (if (null? bvs) "" (utf8->string (apply bytevector-append bvs)))
      ) ;let
    ) ;define

    ;; ==== Cursor operations (with validation) ====

    (define (substring/cursors str start end)
      (validate-start-end start end)
      (g_substring/cursors str start end)
    ) ;define

    ;; ==== Selection ====

    (define (string-copy/cursors str . maybe-start+end)
      (let* ((end-c (string-cursor-end str))
             (start (if (null? maybe-start+end) (string-cursor-start str) (car maybe-start+end))
             ) ;start
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest)
                    (if (string-cursor? start) end-c (string-cursor->index str end-c))
                    (car rest)
                  ) ;if
             ) ;end
            ) ;
        (substring/cursors str start end)
      ) ;let*
    ) ;define

    ;; ==== String operations ====

    (define (string-take str nchars)
      (let ((end (string-cursor-forward str (string-cursor-start str) nchars)))
        (substring/cursors str (string-cursor-start str) end)
      ) ;let
    ) ;define

    (define (string-drop str nchars)
      (let ((start (string-cursor-forward str (string-cursor-start str) nchars)))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end-idx))
            ) ;
        (let ((trimmed-start (string-skip s pred start-c end-c)))
          (substring/cursors s trimmed-start end-c)
        ) ;let
      ) ;let*
    ) ;define*

    (define* (string-trim-right s (pred char-whitespace?) (start 0) (end #t))
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end-idx))
            ) ;
        (let ((trimmed-end (string-skip-right s pred start-c end-c)))
          (substring/cursors s start-c trimmed-end)
        ) ;let
      ) ;let*
    ) ;define*

    (define* (string-trim-both s (pred char-whitespace?) (start 0) (end #t))
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end-idx))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end-idx))
             (sub-len (string-cursor-diff s start-c end-c))
            ) ;
        (if (>= sub-len len)
          (string-take-right (substring/cursors s start-c end-c) len)
          (string-append (make-string (- len sub-len) char)
            (substring/cursors s start-c end-c)
          ) ;string-append
        ) ;if
      ) ;let*
    ) ;define*

    (define* (string-pad-right s len (char #\space) (start 0) (end #t))
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (end-idx (if (eq? end #t) char-len end))
             (_ (validate-start-end start end-idx))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end-idx))
             (sub-len (string-cursor-diff s start-c end-c))
            ) ;
        (if (>= sub-len len)
          (string-take (substring/cursors s start-c end-c) len)
          (string-append (substring/cursors s start-c end-c)
            (make-string (- len sub-len) char)
          ) ;string-append
        ) ;if
      ) ;let*
    ) ;define*

    ;; ==== Prefix and Suffix ====

    (define (string-prefix-length s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-c (as-cursor-clamped s1 start1 char-len1))
             (end1-c (as-cursor-clamped s1 end1 char-len1))
             (start2-c (as-cursor-clamped s2 start2 char-len2))
             (end2-c (as-cursor-clamped s2 end2 char-len2))
            ) ;
        (let loop
          ((i start1-c) (j start2-c) (count 0))
          (if (or (string-cursor>=? i end1-c) (string-cursor>=? j end2-c))
            count
            (if (char=? (string-ref/cursor s1 i) (string-ref/cursor s2 j))
              (loop (string-cursor-next s1 i) (string-cursor-next s2 j) (+ count 1))
              count
            ) ;if
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (string-suffix-length s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-c (as-cursor-clamped s1 start1 char-len1))
             (end1-c (as-cursor-clamped s1 end1 char-len1))
             (start2-c (as-cursor-clamped s2 start2 char-len2))
             (end2-c (as-cursor-clamped s2 end2 char-len2))
            ) ;
        (let loop
          ((i end1-c) (j end2-c) (count 0))
          (let ((i2 (if (string-cursor>? i start1-c) (string-cursor-prev s1 i) i))
                (j2 (if (string-cursor>? j start2-c) (string-cursor-prev s2 j) j))
               ) ;
            (if (or (string-cursor=? i i2) (string-cursor=? j j2))
              ;; 某一侧已无法后退（到 start），停止
              count
              (if (char=? (string-ref/cursor s1 i2) (string-ref/cursor s2 j2))
                (loop i2 j2 (+ count 1))
                count
              ) ;if
            ) ;if
          ) ;let
        ) ;let
      ) ;let*
    ) ;define

    (define (string-prefix? s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (min (if (string-cursor? start1) (string-cursor->index s1 start1) start1)
                           char-len1
                         ) ;min
             ) ;start1-idx
             (end1-idx (min (if (string-cursor? end1) (string-cursor->index s1 end1) end1) char-len1)
             ) ;end1-idx
             (start2-idx (min (if (string-cursor? start2) (string-cursor->index s2 start2) start2)
                           char-len2
                         ) ;min
             ) ;start2-idx
             (end2-idx (min (if (string-cursor? end2) (string-cursor->index s2 end2) end2) char-len2)
             ) ;end2-idx
            ) ;
        (let ((len1 (- end1-idx start1-idx)))
          (and (<= len1 (- end2-idx start2-idx))
            (= (string-prefix-length s1 s2 start1-idx end1-idx start2-idx end2-idx) len1)
          ) ;and
        ) ;let
      ) ;let*
    ) ;define

    (define (string-suffix? s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-idx (min (if (string-cursor? start1) (string-cursor->index s1 start1) start1)
                           char-len1
                         ) ;min
             ) ;start1-idx
             (end1-idx (min (if (string-cursor? end1) (string-cursor->index s1 end1) end1) char-len1)
             ) ;end1-idx
             (start2-idx (min (if (string-cursor? start2) (string-cursor->index s2 start2) start2)
                           char-len2
                         ) ;min
             ) ;start2-idx
             (end2-idx (min (if (string-cursor? end2) (string-cursor->index s2 end2) end2) char-len2)
             ) ;end2-idx
            ) ;
        (let ((len1 (- end1-idx start1-idx)))
          (and (<= len1 (- end2-idx start2-idx))
            (= (string-suffix-length s1 s2 start1-idx end1-idx start2-idx end2-idx) len1)
          ) ;and
        ) ;let
      ) ;let*
    ) ;define

    ;; ==== Contains ====

    ;; Check if s2[s2-start:s2-end] matches s1 at cursor s1-pos
    (define (string-prefix-at? s1 s2 s1-pos s2-start s2-end)
      (let loop
        ((i s1-pos) (j s2-start))
        (if (string-cursor>=? j s2-end)
          #t
          (if (char=? (string-ref/cursor s1 i) (string-ref/cursor s2 j))
            (loop (string-cursor-next s1 i) (string-cursor-next s2 j))
            #f
          ) ;if
        ) ;if
      ) ;let
    ) ;define

    (define (string-contains s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-c (as-cursor s1 start1))
             (end1-c (as-cursor-clamped s1 end1 char-len1))
             (start2-c (as-cursor s2 start2))
             (end2-c (as-cursor-clamped s2 end2 char-len2))
             (s2-len (string-cursor-diff s2 start2-c end2-c))
            ) ;
        (if (zero? s2-len)
          start1-c
          (if (< (string-cursor-diff s1 start1-c end1-c) s2-len)
            #f
            (let ((limit (string-cursor-back s1 end1-c s2-len)))
              (let loop
                ((i start1-c))
                (if (string-cursor>? i limit)
                  #f
                  (if (string-prefix-at? s1 s2 i start2-c end2-c)
                    i
                    (loop (string-cursor-next s1 i))
                  ) ;if
                ) ;if
              ) ;let
            ) ;let
          ) ;if
        ) ;if
      ) ;let*
    ) ;define

    (define (string-contains-right s1 s2 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start1 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest1 (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end1 (if (null? rest1) char-len1 (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start2 (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end2 (if (null? rest3) char-len2 (car rest3)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (start1-c (as-cursor s1 start1))
             (end1-c (as-cursor-clamped s1 end1 char-len1))
             (start2-c (as-cursor s2 start2))
             (end2-c (as-cursor-clamped s2 end2 char-len2))
             (s2-len (string-cursor-diff s2 start2-c end2-c))
            ) ;
        (if (zero? s2-len)
          end1-c
          (if (< (string-cursor-diff s1 start1-c end1-c) s2-len)
            #f
            (let ((limit (string-cursor-back s1 end1-c s2-len)))
              (let loop
                ((i limit))
                (cond ((string-cursor<? i start1-c) #f)
                      ((string-prefix-at? s1 s2 i start2-c end2-c) i)
                      ;; 已到 start，无法再后退
                      ((string-cursor=? i start1-c) #f)
                      (else (loop (string-cursor-prev s1 i)))
                ) ;cond
              ) ;let
            ) ;let
          ) ;if
        ) ;if
      ) ;let*
    ) ;define

    (define (string-concatenate string-list)
      (apply string-append string-list)
    ) ;define

    (define (string-concatenate-reverse string-list . maybe-final+end)
      (let* ((final (if (null? maybe-final+end) "" (car maybe-final+end)))
             (rest (if (null? maybe-final+end) '() (cdr maybe-final+end)))
             (end (if (null? rest)
                    (string-cursor->index final (string-cursor-end final))
                    (car rest)
                  ) ;if
             ) ;end
             (end-idx (if (string-cursor? end) (string-cursor->index final end) end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (when (null? maybe-to+start+end)
        (error 'value-error "string-replicate: to argument is required")
      ) ;when
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (to (car maybe-to+start+end))
             (start (if (null? (cdr maybe-to+start+end)) 0 (cadr maybe-to+start+end)))
             (rest1 (if (null? (cdr maybe-to+start+end)) '() (cddr maybe-to+start+end)))
             (end (if (null? rest1) char-len (car rest1)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
             (slen (string-cursor-diff s start-c end-c))
             (anslen (- to from))
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
        (when (> from to)
          (error 'value-error "string-replicate: from > to")
        ) ;when
        (cond ((zero? anslen) "")
              ((zero? slen) (error 'value-error "Cannot replicate empty substring"))
              (else (let loop
                      ((i 0) (result '()))
                      (if (>= i anslen)
                        (list->utf8-string (reverse result))
                        (let ((ch (vector-ref source-chars (modulo (+ from i) slen))))
                          (loop (+ i 1) (cons ch result))
                        ) ;let
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    (define (string-replace s1 s2 start1 end1 . maybe-start+end)
      (let* ((char-len1 (string-cursor->index s1 (string-cursor-end s1)))
             (char-len2 (string-cursor->index s2 (string-cursor-end s2)))
             (start2 (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end2 (if (null? rest) char-len2 (car rest)))
             (_ (validate-start-end start1 end1))
             (_ (validate-start-end start2 end2))
             (before (substring/cursors s1 (as-cursor s1 0) (as-cursor s1 start1)))
             (middle (substring/cursors s2 (as-cursor s2 start2) (as-cursor s2 end2)))
             (after (substring/cursors s1 (as-cursor s1 end1) (as-cursor s1 char-len1)))
            ) ;
        (string-append before middle after)
      ) ;let*
    ) ;define

    (define (string-split s delimiter . args)
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (grammar (if (null? args) 'infix (car args)))
             (rest1 (if (null? args) '() (cdr args)))
             (limit (if (null? rest1) #f (car rest1)))
             (rest2 (if (null? rest1) '() (cdr rest1)))
             (start (if (null? rest2) 0 (car rest2)))
             (rest3 (if (null? rest2) '() (cdr rest2)))
             (end (if (null? rest3) char-len (car rest3)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
            ) ;
        (cond ((= start end)
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
              (else (let ((dlen (string-cursor->index delimiter (string-cursor-end delimiter))))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
      (let* ((char-len (string-cursor->index s (string-cursor-end s)))
             (start (if (null? maybe-start+end) 0 (car maybe-start+end)))
             (rest (if (null? maybe-start+end) '() (cdr maybe-start+end)))
             (end (if (null? rest) char-len (car rest)))
             (_ (validate-start-end start end))
             (start-c (as-cursor s start))
             (end-c (as-cursor s end))
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
