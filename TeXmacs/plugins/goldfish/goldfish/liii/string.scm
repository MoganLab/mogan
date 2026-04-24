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
    string-upcase
    string-downcase
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
        (type-error "string-starts? parameter is not a string"
        ) ;type-error
      ) ;if
    ) ;define

    (define string-contains?
      (typed-lambda ((str string?) (sub-str string?))
        (string-contains str sub-str)
      ) ;typed-lambda
    ) ;define

    (define (string-split str sep)
      (define (split-characters input)
        (let ((input-len (utf8-string-length input)))
          (let loop
            ((i 0) (parts '()))
            (if (= i input-len)
              (reverse parts)
              (loop (+ i 1)
                (cons (utf8-substring input i (+ i 1))
                  parts
                ) ;cons
              ) ;loop
            ) ;if
          ) ;let
        ) ;let
      ) ;define

      (when (not (string? str))
        (type-error "string-split: first parameter must be string"
        ) ;type-error
      ) ;when

      (let* ((sep-str (cond ((string? sep) sep)
                            ((char? sep) (string sep))
                            (else (type-error "string-split: second parameter must be string or char"
                                  ) ;type-error
                            ) ;else
                      ) ;cond
             ) ;sep-str
             (str-len (string-length str))
             (sep-len (string-length sep-str))
            ) ;
        (if (zero? sep-len)
          (split-characters str)
          (let loop
            ((search-start 0) (parts '()))
            (let ((next-pos (string-position sep-str
                              str
                              search-start
                            ) ;string-position
                  ) ;next-pos
                 ) ;
              (if next-pos
                (loop (+ next-pos sep-len)
                  (cons (substring str search-start next-pos)
                    parts
                  ) ;cons
                ) ;loop
                (reverse (cons (substring str search-start str-len)
                           parts
                         ) ;cons
                ) ;reverse
              ) ;if
            ) ;let
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (string-replace str old new . rest)
      (when (> (length rest) 1)
        (error 'wrong-number-of-args
          "string-replace: too many arguments"
        ) ;error
      ) ;when
      (unless (string? str)
        (type-error "string-replace: str must be a string"
        ) ;type-error
      ) ;unless
      (unless (string? old)
        (type-error "string-replace: old must be a string"
        ) ;type-error
      ) ;unless
      (unless (string? new)
        (type-error "string-replace: new must be a string"
        ) ;type-error
      ) ;unless
      (let ((count (if (null? rest) -1 (car rest)))
           ) ;
        (unless (integer? count)
          (type-error "string-replace: count must be an integer"
          ) ;type-error
        ) ;unless
        (let ((str-len (string-length str))
              (old-len (string-length old))
             ) ;
          (cond ((zero? count) (string-copy str))
                ((zero? old-len)
                 (if (zero? str-len)
                   new
                   (let* ((max-inserts (+ str-len 1))
                          (remaining (if (negative? count)
                                       max-inserts
                                       (min count max-inserts)
                                     ) ;if
                          ) ;remaining
                         ) ;
                     (let loop
                       ((i 0) (acc '()) (r remaining))
                       (cond ((and (= i str-len) (> r 0))
                              (apply string-append
                                (reverse (cons new acc))
                              ) ;apply
                             ) ;
                             ((= i str-len)
                              (apply string-append (reverse acc))
                             ) ;
                             ((zero? r)
                              (apply string-append
                                (reverse (cons (substring str i str-len) acc)
                                ) ;reverse
                              ) ;apply
                             ) ;
                             (else (loop (+ i 1)
                                     (cons (substring str i (+ i 1))
                                       (cons new acc)
                                     ) ;cons
                                     (- r 1)
                                   ) ;loop
                             ) ;else
                       ) ;cond
                     ) ;let
                   ) ;let*
                 ) ;if
                ) ;
                (else (let ((remaining (if (negative? count) -1 count)
                            ) ;remaining
                           ) ;
                        (let loop
                          ((search-start 0)
                           (parts '())
                           (r remaining)
                          ) ;
                          (let ((next-pos (string-position old str search-start)
                                ) ;next-pos
                               ) ;
                            (if (and next-pos (not (zero? r)))
                              (loop (+ next-pos old-len)
                                (cons new
                                  (cons (substring str search-start next-pos)
                                    parts
                                  ) ;cons
                                ) ;cons
                                (- r 1)
                              ) ;loop
                              (if (null? parts)
                                (string-copy str)
                                (apply string-append
                                  (reverse (cons (substring str search-start str-len)
                                             parts
                                           ) ;cons
                                  ) ;reverse
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
        (type-error "string-ends? parameter is not a string"
        ) ;type-error
      ) ;if
    ) ;define

    (define string-remove-prefix
      (typed-lambda ((str string?) (prefix string?))
        (if (string-prefix? prefix str)
          (substring str (string-length prefix))
          str
        ) ;if
      ) ;typed-lambda
    ) ;define

    (define string-remove-suffix
      (typed-lambda ((str string?) (suffix string?))
        (if (string-suffix? suffix str)
          (substring str
            0
            (- (string-length str)
              (string-length suffix)
            ) ;-
          ) ;substring
          (string-copy str)
        ) ;if
      ) ;typed-lambda
    ) ;define

  ) ;begin
) ;define-library
