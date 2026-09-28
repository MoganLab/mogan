;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
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

(define-library (liii goldfmt scan)
  (export scan scan-string scan-source-string scan-file stem-mode?
    call-with-stem-mode
  ) ;export
  (import (liii base)
    (liii path)
    (liii raw-string)
    (liii string)
    (liii syntax)
    (liii unicode)
    (liii list)
    (liii goldfmt record)
    (scheme char)
    (rename (liii goldfmt tokenize)
      (tokenize source-tokenize)
      (tokens->string source-tokens->string)
    ) ;rename
  ) ;import
  (begin
    (define %stem-mode? #f)
    (define (stem-mode?)
      %stem-mode?
    ) ;define
    (define (call-with-stem-mode thunk)
      (let-temporarily ((%stem-mode? #t)) (thunk))
    ) ;define
    (define (write-to-string value)
      (let ((port (open-output-string)))
        (let-temporarily (((*s7* 'print-length) 9223372036854775807))
          (write value port)
        ) ;let-temporarily
        (get-output-string port)
      ) ;let
    ) ;define
    (define (fast-string-join strings)
      (let ((out (open-output-string)))
        (let loop
          ((rest strings))
          (if (null? rest)
            (get-output-string out)
            (begin
              (display (car rest) out)
              (loop (cdr rest))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define
    (define (raw-string-form? datum)
      (and (pair? datum)
        (eq? (car datum) '*raw-string*)
        (pair? (cdr datum))
        (string? (cadr datum))
        (pair? (cddr datum))
        (string? (caddr datum))
        (null? (cdddr datum))
      ) ;and
    ) ;define
    (define (char-literal-form? datum)
      (and (pair? datum)
        (eq? (car datum) '*char-literal*)
        (pair? (cdr datum))
        (string? (cadr datum))
        (pair? (cddr datum))
        (char? (caddr datum))
        (null? (cdddr datum))
      ) ;and
    ) ;define
    (define (trim-right-spaces str)
      (if
        (string-every (lambda (c) (or (char=? c #\space) (char=? c #\tab))) str)
        str
        (utf8-string-trim-right str)
      ) ;if
    ) ;define
    (define (atom? x)
      (or (symbol? x)
        (number? x)
        (string? x)
        (raw-string-literal? x)
        (char-literal? x)
        (boolean? x)
        (char? x)
        (null? x)
        (vector? x)
        (byte-vector? x)
        (eof-object? x)
        (syntax? x)
        (eq? 'undefined? (type-of x))
        (unspecified? x)
        (procedure? x)
      ) ;or
    ) ;define
    (define (scan datum)
      (scan-datum (normalize-datum datum) 0)
    ) ;define
    (define (scan-datum datum depth)
      (cond
       ((raw-string-form? datum)
        (make-atom
          :depth depth
          :value
          (make-raw-string-literal :source (cadr datum) :value (caddr datum))
        ) ;make-atom
       ) ;
       ((char-literal-form? datum)
        (make-atom
          :depth depth
          :value (make-char-literal :source (cadr datum) :value (caddr datum))
        ) ;make-atom
       ) ;
       ((dotted-tail? datum) (scan-datum (dotted-tail-form datum) depth))
       ((atom? datum) (make-atom :depth depth :value datum))
       (else (scan-list datum depth))
      ) ;cond
    ) ;define
    (define (tag? x)
      (or (symbol? x) (and (syntax? x) #t))
    ) ;define
    (define (tag->string x)
      (if (symbol? x) (symbol->string x) (object->string x #f))
    ) ;define
    (define (procedure-name value)
      (and (procedure? value) (object->string value #f))
    ) ;define
    (define (procedure-name=? value name)
      (let ((proc-name (procedure-name value)))
        (and proc-name (string=? proc-name name))
      ) ;let
    ) ;define
    (define (internal-list-values-form? value)
      (and (pair? value) (procedure-name=? (car value) "#_list-values"))
    ) ;define
    (define (internal-apply-values-form? value)
      (and (pair? value) (procedure-name=? (car value) "#_apply-values"))
    ) ;define
    (define (unquote-apply-values-form? value)
      (and (pair? value)
        (eq? (car value) 'unquote)
        (pair? (cdr value))
        (null? (cddr value))
        (internal-apply-values-form? (cadr value))
      ) ;and
    ) ;define
    (define (internal-list-star-form? value)
      (and (pair? value)
        (let ((name (procedure-name (car value))))
          (and name
            (or (string=? name "<list*>") (string=? name "#_list") (string=? name "list*"))
          ) ;and
        ) ;let
      ) ;and
    ) ;define
    (define (quote-syntax? value)
      (or (eq? value 'quote)
        (and (syntax? value) (string=? (object->string value #f) "#_quote"))
      ) ;or
    ) ;define
    (define (internal-quote-builder-form? value)
      (and (internal-list-values-form? value)
        (pair? (cdr value))
        (pair? (cddr value))
        (null? (cdddr value))
        (quote-syntax? (cadr value))
      ) ;and
    ) ;define
    (define (make-dotted-list head-items tail)
      (if (null? head-items)
        tail
        (cons (car head-items) (make-dotted-list (cdr head-items) tail))
      ) ;if
    ) ;define
    (define (normalize-quasiquote-item datum)
      (cond ((quote-form? datum) (normalize-datum (cadr datum)))
            ((internal-quote-builder-form? datum)
             (list 'quote (normalize-quasiquote-item (caddr datum)))
            ) ;
            ((internal-list-values-form? datum) (normalize-quasiquote-list datum))
            ((internal-list-star-form? datum) (normalize-quasiquote-dotted-list datum))
            ((internal-apply-values-form? datum)
             (list 'unquote-splicing (normalize-datum (cadr datum)))
            ) ;
            (else (list 'unquote (normalize-datum datum)))
      ) ;cond
    ) ;define
    (define (normalize-quasiquote-list datum)
      (map normalize-quasiquote-item (cdr datum))
    ) ;define
    (define (normalize-quasiquote-dotted-list datum)
      (let* ((args (cdr datum))
             (prefix-form (if (pair? args) (car args) '()))
             (tail-form (if (pair? (cdr args)) (cadr args) '()))
             (head-items
               (cond ((internal-list-values-form? prefix-form)
                      (map normalize-quasiquote-item (cdr prefix-form))
                     ) ;
                     ((null? prefix-form) '())
                     (else (list (normalize-quasiquote-item prefix-form)))
               ) ;cond
             ) ;head-items
             (tail
               (let ((item (normalize-quasiquote-item tail-form)))
                 (if (unquote-form? item) (make-dotted-tail item) item)
               ) ;let
             ) ;tail
            ) ;
        (make-dotted-list head-items tail)
      ) ;let*
    ) ;define
    (define (explicit-quasiquote-form? datum)
      (and (pair? datum)
        (eq? (car datum) 'quasiquote)
        (pair? (cdr datum))
        (null? (cddr datum))
      ) ;and
    ) ;define
    (define (split-pair-list datum)
      (let loop
        ((current datum) (elems '()))
        (cond
         ((pair? current) (loop (cdr current) (cons (car current) elems)))
         ((null? current) (values (reverse elems) '()))
         (else (values (reverse elems) current))
        ) ;cond
      ) ;let
    ) ;define
    (define (normalize-explicit-qq-template datum)
      (cond ((unquote-form? datum) (normalize-datum datum))
            ((pair? datum)
             (call-with-values (lambda () (split-pair-list datum))
               (lambda (elems tail)
                 (let ((len (length elems)))
                   (cond
                    ((and (null? tail) (>= len 3) (eq? (list-ref elems (- len 2)) 'unquote))
                     (make-dotted-list (map normalize-explicit-qq-template (take elems (- len 2)))
                       (make-dotted-tail
                         (list 'unquote (normalize-datum (list-ref elems (- len 1))))
                       ) ;make-dotted-tail
                     ) ;make-dotted-list
                    ) ;
                    ((null? tail) (map normalize-explicit-qq-template elems))
                    (else (make-dotted-list (map normalize-explicit-qq-template elems)
                            (normalize-explicit-qq-template tail)
                          ) ;make-dotted-list
                    ) ;else
                   ) ;cond
                 ) ;let
               ) ;lambda
             ) ;call-with-values
            ) ;
            (else datum)
      ) ;cond
    ) ;define
    (define (normalize-datum datum)
      (cond ((raw-string-form? datum)
             (make-raw-string-literal :source (cadr datum) :value (caddr datum))
            ) ;
            ((char-literal-form? datum)
             (make-char-literal :source (cadr datum) :value (caddr datum))
            ) ;
            ((quote-form? datum)
             (if (stem-mode?)
               (list 'quote (normalize-datum (cadr datum)))
               (list (car datum) (normalize-datum (cadr datum)))
             ) ;if
            ) ;
            ((and (not (stem-mode?)) (explicit-quasiquote-form? datum))
             (list 'quasiquote (normalize-explicit-qq-template (cadr datum)))
            ) ;
            ((internal-list-values-form? datum)
             (list 'quasiquote (normalize-quasiquote-list datum))
            ) ;
            ((internal-list-star-form? datum)
             (list 'quasiquote (normalize-quasiquote-dotted-list datum))
            ) ;
            ((unquote-apply-values-form? datum)
             (list 'unquote-splicing (normalize-datum (cadr (cadr datum))))
            ) ;
            ((internal-apply-values-form? datum)
             (list 'unquote-splicing (normalize-datum (cadr datum)))
            ) ;
            ((pair? datum)
             (cons (normalize-datum (car datum)) (normalize-datum (cdr datum)))
            ) ;
            ((byte-vector? datum)
             (let ((result (make-byte-vector (vector-length datum) 0)))
               (let loop
                 ((i 0))
                 (if (>= i (vector-length datum))
                   result
                   (begin
                     (byte-vector-set! result i (normalize-datum (vector-ref datum i)))
                     (loop (+ i 1))
                   ) ;begin
                 ) ;if
               ) ;let
             ) ;let
            ) ;
            ((vector? datum) (list->vector (map normalize-datum (vector->list datum))))
            (else datum)
      ) ;cond
    ) ;define
    (define (s7-internal-form? lst)
      (and (pair? lst)
        (not (null? lst))
        (syntax? (car lst))
        (let ((name (object->string (car lst) #f)))
          (or (string=? name "#_list-values") (string=? name "#_list"))
        ) ;let
      ) ;and
    ) ;define
    (define (dotted-list-elements lst)
      (let loop
        ((current lst) (result '()))
        (cond
         ((pair? current) (loop (cdr current) (cons (car current) result)))
         ((null? current) (reverse result))
         (else
           (reverse (cons current (cons (string->symbol ".") result)))
         ) ;else
        ) ;cond
      ) ;let
    ) ;define
    (define (scan-list lst depth)
      (let* ((first (car lst)) (rest (cdr lst)))
        (cond
         ((and (not (stem-mode?)) (quote-form? lst))
          (make-env
            :tag-name (if (syntax? first) "#_quote" (symbol->string first))
            :depth    depth
            :children (vector)
            :value    lst
          ) ;make-env
         ) ;
         ((s7-internal-form? lst)
          (make-env
            :tag-name (object->string first #f)
            :depth    depth
            :children (vector)
            :value    lst
          ) ;make-env
         ) ;
         (else
           (let ((children-list (if (dotted-list? lst) (dotted-list-elements lst) (if (tag? first) rest lst))
                 ) ;children-list
                 (has-tag? (and (not (dotted-list? lst)) (tag? first)))
                ) ;
             (let* ((scanned-children
                      (map (lambda (child) (scan-datum child (+ depth 1))) children-list)
                    ) ;scanned-children
                    (children-vec (list->vector scanned-children))
                   ) ;
               (if has-tag?
                 (make-env
                   :tag-name (tag->string first)
                   :depth    depth
                   :children children-vec
                   :value    lst
                 ) ;make-env
                 (make-env :tag-name "" :depth depth :children children-vec :value lst)
               ) ;if
             ) ;let*
           ) ;let
         ) ;else
        ) ;cond
      ) ;let*
    ) ;define
    (define (string-prefix-at? str start prefix)
      (let ((prefix-len (string-length prefix)) (str-len (string-length str)))
        (and (<= (+ start prefix-len) str-len)
          (let loop
            ((i 0))
            (or (>= i prefix-len)
              (and (char=? (string-ref str (+ start i)) (string-ref prefix i)) (loop (+ i 1)))
            ) ;or
          ) ;let
        ) ;and
      ) ;let
    ) ;define
    (define (find-raw-string-literal-end str body-start delimiter)
      (let ((needle (string-append "\"" delimiter "\"")) (str-len (string-length str)))
        (let loop
          ((i body-start))
          (cond
           ((> (+ i (string-length needle)) str-len) #f)
           ((string-prefix-at? str i needle) (+ i (string-length needle)))
           (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define
    (define (read-raw-string-literal-value literal)
      (let ((value (read (open-input-string literal))))
        (if (string? value)
          value
          (error 'value-error "rewrite-raw-string-literals: expected raw string literal")
        ) ;if
      ) ;let
    ) ;define
    (define (reader-delimiter? c)
      (or (char=? c #\space)
        (char=? c #\tab)
        (char=? c #\newline)
        (char=? c #\return)
        (char=? c #\()
        (char=? c #\))
        (char=? c #\[)
        (char=? c #\])
        (char=? c #\")
        (char=? c #\;)
        (char=? c #\')
        (char=? c #\`)
        (char=? c #\,)
      ) ;or
    ) ;define
    (define (hex-digit? c)
      (or (char-numeric? c)
        (and (char>=? c #\a) (char<=? c #\f))
        (and (char>=? c #\A) (char<=? c #\F))
      ) ;or
    ) ;define
    (define (find-char-literal-end source start)
      (let ((len (string-length source)))
        (if (>= start len)
          #f
          (let loop
            ((i (+ start 1)))
            (if (>= i len)
              len
              (let ((c (string-ref source i)))
                (if (reader-delimiter? c) i (loop (+ i 1)))
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define
    (define (rewrite-reader-literals source)
      (let ((len (string-length source)))
        (let loop
          ((i 0)
           (in-string #f)
           (escaped #f)
           (in-line-comment #f)
           (in-block-comment #f)
           (result '())
          ) ;
          (if (>= i len)
            (fast-string-join (reverse! result))
            (let ((c (string-ref source i))
                  (next-c (if (< (+ i 1) len) (string-ref source (+ i 1)) #\nul))
                 ) ;
              (cond
                (in-line-comment (loop (+ i 1) #f #f (not (char=? c #\newline)) #f (cons (string c) result))
                ) ;in-line-comment
                (in-block-comment
                  (if (and (char=? c #\|) (char=? next-c #\#))
                    (loop (+ i 2) #f #f #f #f (cons "|#" (cons (string c) result)))
                    (loop (+ i 1) #f #f #f #t (cons (string c) result))
                  ) ;if
                ) ;in-block-comment
                (in-string
                  (cond
                    (escaped (loop (+ i 1) #t #f #f #f (cons (string c) result)))
                    ((char=? c #\\) (loop (+ i 1) #t #t #f #f (cons (string c) result)))
                    ((char=? c #\") (loop (+ i 1) #f #f #f #f (cons (string c) result)))
                    (else (loop (+ i 1) #t #f #f #f (cons (string c) result)))
                  ) ;cond
                ) ;in-string
                ((and (char=? c #\;) (char=? next-c #\;))
                 (loop (+ i 2) #f #f #t #f (cons ";;" result))
                ) ;
                ((and (char=? c #\#) (char=? next-c #\|))
                 (loop (+ i 2) #f #f #f #t (cons "#|" result))
                ) ;
                ((char=? c #\") (loop (+ i 1) #t #f #f #f (cons (string c) result)))
                ((and (char=? c #\#) (char=? next-c #\"))
                 (let ((delimiter-end (string-index source #\" (+ i 2))))
                   (if delimiter-end
                     (let* ((delimiter (substring source (+ i 2) delimiter-end))
                            (literal-end (find-raw-string-literal-end source (+ delimiter-end 1) delimiter))
                           ) ;
                       (if literal-end
                         (let* ((literal (substring source i literal-end))
                                (value (read-raw-string-literal-value literal))
                                (rewritten (string-append "(*raw-string* "
                                             (write-to-string literal)
                                             " "
                                             (write-to-string value)
                                             ")"
                                           ) ;string-append
                                ) ;rewritten
                               ) ;
                           (loop literal-end #f #f #f #f (cons rewritten result))
                         ) ;let*
                         (loop (+ i 1) #f #f #f #f (cons (string c) result))
                       ) ;if
                     ) ;let*
                     (loop (+ i 1) #f #f #f #f (cons (string c) result))
                   ) ;if
                 ) ;let
                ) ;
                ((and (char=? c #\#) (char=? next-c #\\))
                 (let ((literal-end (find-char-literal-end source (+ i 2))))
                   (if literal-end
                     (let* ((literal (substring source i literal-end))
                            (value (read (open-input-string literal)))
                            (rewritten (string-append "(*char-literal* " (write-to-string literal) " " literal ")")
                            ) ;rewritten
                           ) ;
                       (if (char? value)
                         (loop literal-end #f #f #f #f (cons rewritten result))
                         (loop (+ i 1) #f #f #f #f (cons (string c) result))
                       ) ;if
                     ) ;let*
                     (loop (+ i 1) #f #f #f #f (cons (string c) result))
                   ) ;if
                 ) ;let
                ) ;
                (else (loop (+ i 1) #f #f #f #f (cons (string c) result)))
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define
    (define (scan-string-fast str)
      (let ((port (open-input-string str)))
        (let loop
          ((results '()))
          (let ((datum (read port)))
            (if (eof-object? datum)
              (list->vector (reverse results))
              (loop (cons (scan-datum (normalize-datum datum) 0) results))
            ) ;if
          ) ;let
        ) ;let
      ) ;let
    ) ;define
    (define (needs-rewrite-reader-literals? str)
      (let ((len (string-length str)))
        (let loop
          ((i 0))
          (cond ((>= i len) #f)
                ((and (char=? (string-ref str i) #\#)
                   (< (+ i 1) len)
                   (or (char=? (string-ref str (+ i 1)) #\") (char=? (string-ref str (+ i 1)) #\\))
                 ) ;and
                 #t
                ) ;
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define
    (define (scan-string str)
      (if (needs-rewrite-reader-literals? str)
        (scan-string-fast (rewrite-reader-literals str))
        (scan-string-fast str)
      ) ;if
    ) ;define
    (define (whitespace-char? c)
      (or (char=? c #\space)
        (char=? c #\tab)
        (char=? c #\newline)
        (char=? c #\return)
      ) ;or
    ) ;define
    (define (inside-string? str pos)
      (let loop
        ((i 0) (in-string #f) (escaped #f))
        (cond ((>= i pos) in-string)
              ((>= i (string-length str)) in-string)
              (else
                (let ((c (string-ref str i)))
                  (cond (escaped (loop (+ i 1) in-string #f))
                        ((char=? c #\\) (loop (+ i 1) in-string #t))
                        ((char=? c #\") (loop (+ i 1) (not in-string) #f))
                        (else (loop (+ i 1) in-string #f))
                  ) ;cond
                ) ;let
              ) ;else
        ) ;cond
      ) ;let
    ) ;define
    (define (inside-raw-string? str pos)
      (let loop
        ((i 0) (in-raw-string #f) (raw-delimiter ""))
        (cond ((>= i pos) in-raw-string)
              ((>= i (string-length str)) in-raw-string)
              ((and (< (+ i 1) (string-length str))
                 (char=? (string-ref str i) #\#)
                 (char=? (string-ref str (+ i 1)) #\")
               ) ;and
               (let ((end-pos (string-index str #\" (+ i 2))))
                 (if end-pos
                   (let ((delimiter (substring str (+ i 2) end-pos)))
                     (loop (+ end-pos 1) #t delimiter)
                   ) ;let
                   (loop (+ i 2) #t "")
                 ) ;if
               ) ;let
              ) ;
              (in-raw-string
                (let ((del-len (string-length raw-delimiter)))
                  (if
                    (and (<= (+ i del-len) (string-length str))
                      (string=? (substring str i (+ i del-len)) raw-delimiter)
                      (< (+ i del-len) (string-length str))
                      (char=? (string-ref str (+ i del-len)) #\")
                    ) ;and
                    (loop (+ i del-len 1) #f "")
                    (loop (+ i 1) #t raw-delimiter)
                  ) ;if
                ) ;let
              ) ;in-raw-string
              (else (loop (+ i 1) #f ""))
        ) ;cond
      ) ;let
    ) ;define
    (define (find-real-comment-start line)
      (let loop
        ((pos 0))
        (cond
         ((>= pos (- (string-length line) 1)) #f)
         ((and (char=? (string-ref line pos) #\;)
            (char=? (string-ref line (+ pos 1)) #\;)
            (not (inside-string? line pos))
            (not (inside-raw-string? line pos))
          ) ;and
          pos
         ) ;
         (else (loop (+ pos 1)))
        ) ;cond
      ) ;let
    ) ;define
    (define (comment-line? line)
      (let ((comment-pos (find-real-comment-start line)))
        (if comment-pos
          (let ((prefix (substring line 0 comment-pos)))
            (if (or (string-null? prefix) (string-every whitespace-char? prefix))
              comment-pos
              #f
            ) ;if
          ) ;let
          #f
        ) ;if
      ) ;let
    ) ;define
    (define (extract-comment-content line comment-pos)
      (trim-right-spaces (substring line (+ comment-pos 2)))
    ) ;define
    (define (escape-comment-content content)
      (let loop
        ((chars (string->list content)) (result '()))
        (if (null? chars)
          (list->string (reverse result))
          (let ((c (car chars)))
            (cond
             ((char=? c #\\) (loop (cdr chars) (cons #\\ (cons #\\ result))))
             ((char=? c #\") (loop (cdr chars) (cons #\" (cons #\\ result))))
             (else (loop (cdr chars) (cons c result)))
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define
    (define (tokenize content)
      (let* ((lines (string-split content #\newline)) (tokens '()) (blank-line-count 0))
        (for-each
          (lambda (line)
            (let ((comment-pos (comment-line? line)))
              (cond ((or (string-null? line) (string-every whitespace-char? line))
                     (set! blank-line-count (+ blank-line-count 1))
                    ) ;
                    (comment-pos
                      (when (> blank-line-count 0)
                        (set! tokens (cons (cons 'newline blank-line-count) tokens))
                        (set! blank-line-count 0)
                      ) ;when
                      (let ((content (extract-comment-content line comment-pos)))
                        (set! tokens (cons (cons 'comment content) tokens))
                      ) ;let
                    ) ;comment-pos
                    (else
                      (when (> blank-line-count 0)
                        (set! tokens (cons (cons 'newline blank-line-count) tokens))
                        (set! blank-line-count 0)
                      ) ;when
                      (set! tokens (cons (cons 'code line) tokens))
                    ) ;else
              ) ;cond
            ) ;let
          ) ;lambda
          lines
        ) ;for-each
        (reverse tokens)
      ) ;let*
    ) ;define
    (define (tokens->string tokens)
      (string-join
        (map
          (lambda (token)
            (let ((type (car token)) (content (cdr token)))
              (cond ((eq? type 'comment)
                     (string-append "(*comment* \"" (escape-comment-content content) "\")")
                    ) ;
                    ((eq? type 'newline) (string-append "(*newline* " (number->string content) ")"))
                    (else content)
              ) ;cond
            ) ;let
          ) ;lambda
          tokens
        ) ;map
        "\n"
      ) ;string-join
    ) ;define
    (define (scan-source-string raw-content)
      (let* ((scanned (source-tokenize raw-content))
             (leading-blanks
               (let loop
                 ((i 0) (count 0))
                 (if (>= i (string-length raw-content))
                   count
                   (let ((c (string-ref raw-content i)))
                     (cond ((char=? c #\newline) (loop (+ i 1) (+ count 1)))
                           ((char=? c #\return) (loop (+ i 1) count))
                           ((or (char=? c #\space) (char=? c #\tab)) (loop (+ i 1) count))
                           (else count)
                     ) ;cond
                   ) ;let
                 ) ;if
               ) ;let
             ) ;leading-blanks
             (tokens-with-leading (if (> leading-blanks 0) (cons (cons 'newline leading-blanks) scanned) scanned)
             ) ;tokens-with-leading
             (tokens
               (if
                 (and (not (null? tokens-with-leading))
                   (> (string-length raw-content) 0)
                   (char=? (string-ref raw-content (- (string-length raw-content) 1)) #\newline)
                 ) ;and
                 (append tokens-with-leading (list (cons 'newline 1)))
                 tokens-with-leading
               ) ;if
             ) ;tokens
             (processed-content (source-tokens->string tokens))
            ) ;
        (scan-string processed-content)
      ) ;let*
    ) ;define
    (define (scan-file path)
      (scan-source-string (path-read-text path))
    ) ;define
  ) ;begin
) ;define-library
