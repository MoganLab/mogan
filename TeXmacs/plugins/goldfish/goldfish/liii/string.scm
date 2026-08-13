(define-library (liii string)
  (export string-null? string-join string-every string-any string-take
    string-take-right string-drop string-drop-right string-pad string-pad-right
    string-trim string-trim-left string-trim-right string-trim-both string-index
    string-index-right string-skip string-skip-right string-contains
    string-count string-fold string-fold-right string-for-each-index
    string-reverse string-tokenize string-starts? string-contains? string-ends?
    string-split string-replace string-remove-prefix string-remove-suffix pyfmt
    string-position char-position
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

    (define string-starts? g_string-starts?)

    (define string-contains?
      (typed-lambda ((str string?) (sub-str string?)) (string-contains str sub-str))
    ) ;define

    (define string-split g_string-split)

    (define string-replace g_string-replace)

    (define string-ends? g_string-ends?)

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
