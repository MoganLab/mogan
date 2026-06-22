(define-library (liii string)
  (export string-null?
    string-join
    string-every
    string-any
    string-take
    string-take-right
    string-drop
    string-drop-right
    string-pad
    string-pad-right
    string-trim
    string-trim-left
    string-trim-right
    string-trim-both
    string-index
    string-index-right
    string-skip
    string-skip-right
    string-contains
    string-count
    string-fold
    string-fold-right
    string-for-each-index
    string-reverse
    string-tokenize
    string-starts?
    string-contains?
    string-ends?
    string-split
    string-replace
    string-remove-prefix
    string-remove-suffix
    pyfmt
    string-position
    char-position
  ) ;export
  (import (except (srfi srfi-13) string-replace)
    (scheme base)
    (liii base)
    (liii error)
    (liii unicode)
  ) ;import
  (begin

    ;; ; string-trim-left: 从字符串左侧移除空白字符
    ;; ; 基于 SRFI-13 的 string-trim 实现
    (define string-trim-left string-trim)

    (define (string-starts? str prefix)
      (if (and (string? str) (string? prefix))
        (string-prefix? prefix str)
        (type-error "string-starts? parameter is not a string")
      ) ;if
    ) ;define

    (define string-contains?
      (typed-lambda ((str string?) (sub-str string?)) (string-contains str sub-str))
    ) ;define

    (define (string-split str sep)
      (define (split-characters input)
        (let ((input-len (utf8-string-length input)))
          (let loop
            ((i 0) (parts '()))
            (if (= i input-len)
              (reverse parts)
              (loop (+ i 1) (cons (utf8-substring input i (+ i 1)) parts))
            ) ;if
          ) ;let
        ) ;let
      ) ;define

      (when (not (string? str))
        (type-error "string-split: first parameter must be string")
      ) ;when

      (let* ((sep-str (cond ((string? sep) sep)
                            ((char? sep) (string sep))
                            (else (type-error "string-split: second parameter must be string or char"))
                      ) ;cond
             ) ;sep-str
             (str-len (string-length str))
             (sep-len (string-length sep-str))
            ) ;
        (if (zero? sep-len)
          (split-characters str)
          (let loop
            ((search-start 0) (parts '()))
            (let ((next-pos (string-position sep-str str search-start)))
              (if next-pos
                (loop (+ next-pos sep-len) (cons (substring str search-start next-pos) parts))
                (reverse (cons (substring str search-start str-len) parts))
              ) ;if
            ) ;let
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (string-replace str old new . rest)
      (when (> (length rest) 1)
        (error 'wrong-number-of-args "string-replace: too many arguments")
      ) ;when
      (unless (string? str)
        (type-error "string-replace: str must be a string")
      ) ;unless
      (unless (string? old)
        (type-error "string-replace: old must be a string")
      ) ;unless
      (unless (string? new)
        (type-error "string-replace: new must be a string")
      ) ;unless
      (let ((count (if (null? rest) -1 (car rest))))
        (unless (integer? count)
          (type-error "string-replace: count must be an integer")
        ) ;unless
        (let ((str-len (string-length str)) (old-len (string-length old)))
          (cond ((zero? count) (string-copy str))
                ((zero? old-len)
                 (if (zero? str-len)
                   new
                   (let* ((max-inserts (+ str-len 1))
                          (remaining (if (negative? count) max-inserts (min count max-inserts)))
                         ) ;
                     (let loop
                       ((i 0) (acc '()) (r remaining))
                       (cond ((and (= i str-len) (> r 0)) (apply string-append (reverse (cons new acc))))
                             ((= i str-len) (apply string-append (reverse acc)))
                             ((zero? r) (apply string-append (reverse (cons (substring str i str-len) acc))))
                             (else (loop (+ i 1) (cons (substring str i (+ i 1)) (cons new acc)) (- r 1)))
                       ) ;cond
                     ) ;let
                   ) ;let*
                 ) ;if
                ) ;
                (else (let ((remaining (if (negative? count) -1 count)))
                        (let loop
                          ((search-start 0) (parts '()) (r remaining))
                          (let ((next-pos (string-position old str search-start)))
                            (if (and next-pos (not (zero? r)))
                              (loop (+ next-pos old-len)
                                (cons new (cons (substring str search-start next-pos) parts))
                                (- r 1)
                              ) ;loop
                              (if (null? parts)
                                (string-copy str)
                                (apply string-append
                                  (reverse (cons (substring str search-start str-len) parts))
                                ) ;apply
                              ) ;if
                            ) ;if
                          ) ;let
                        ) ;let
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (string-ends? str suffix)
      (if (and (string? str) (string? suffix))
        (string-suffix? suffix str)
        (type-error "string-ends? parameter is not a string")
      ) ;if
    ) ;define

    (define string-remove-prefix
      (typed-lambda ((str string?) (prefix string?))
        (if (string-prefix? prefix str) (substring str (string-length prefix)) str)
      ) ;typed-lambda
    ) ;define

    (define string-remove-suffix
      (typed-lambda ((str string?) (suffix string?))
        (if (string-suffix? suffix str)
          (substring str 0 (- (string-length str) (string-length suffix)))
          (string-copy str)
        ) ;if
      ) ;typed-lambda
    ) ;define

    (define (pyfmt format-string . plist)
      (unless (string? format-string)
        (type-error "pyfmt: first parameter must be string")
      ) ;unless

      ;; 将 plist 转为以字符串为键的 alist
      (define (plist->salist plist)
        (let loop
          ((p plist) (result '()))
          (cond ((null? p) (reverse result))
                ((not (pair? (cdr p))) (type-error "pyfmt: plist requires key-value pairs"))
                (else (let ((key (car p)) (val (cadr p)))
                        (loop (cddr p)
                          (cons (cons (cond ((keyword? key) (symbol->string (keyword->symbol key)))
                                            ((symbol? key) (symbol->string key))
                                            ((string? key) key)
                                            (else (type-error "pyfmt: key must be keyword, symbol or string"))
                                      ) ;cond
                                  val
                                ) ;cons
                            result
                          ) ;cons
                        ) ;loop
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let
      ) ;define

      (define (lookup-pair key alist)
        (assoc key alist equal?)
      ) ;define

      (let ((salist (plist->salist plist)) (len (string-length format-string)))
        (let loop
          ((i 0) (parts '()))
          (if (>= i len)
            (apply string-append (reverse parts))
            (let ((pos (string-position "%(" format-string i)))
              (if (and pos (>= pos i))
                (let ((end-pos (string-position ")" format-string (+ pos 2))))
                  (if (and end-pos (> end-pos (+ pos 2)))
                    (let* ((key (substring format-string (+ pos 2) end-pos))
                           (type-pos (+ end-pos 1))
                           (has-type? (< type-pos len))
                           (type-char (if has-type? (string-ref format-string type-pos) #\s))
                           (placeholder-end (if has-type? (+ type-pos 1) (+ end-pos 1)))
                           (placeholder (substring format-string pos placeholder-end))
                           (pair (lookup-pair key salist))
                           (val (and pair (cdr pair)))
                           (val-str (cond ((not pair) placeholder)
                                          ((char=? type-char #\d)
                                           (if (number? val)
                                             (number->string val)
                                             (type-error "pyfmt: %(key)d requires number")
                                           ) ;if
                                          ) ;
                                          (else (if (string? val) val (format #f "~a" val)))
                                    ) ;cond
                           ) ;val-str
                          ) ;
                      (loop placeholder-end
                        (cons val-str (cons (substring format-string i pos) parts))
                      ) ;loop
                    ) ;let*
                    (loop len (cons (substring format-string i len) parts))
                  ) ;if
                ) ;let
                (loop len (cons (substring format-string i len) parts))
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
