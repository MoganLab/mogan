;;  MIT License

;  Copyright guenchi (c) 2018 - 2019
;            Da Shen (c) 2024 - 2025
;            (Jack) Yansong Li (c) 2025
     
;  Permission is hereby granted, free of charge, to any person obtaining a copy
;  of this software and associated documentation files (the "Software"), to deal
;  in the Software without restriction, including without limitation the rights
;  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;  copies of the Software, and to permit persons to whom the Software is
;  furnished to do so, subject to the following conditions:
     
;  The above copyright notice and this permission notice shall be included in all
;  copies or substantial portions of the Software.
     
;  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;  SOFTWARE.

(define-library (guenchi json)
(import (liii base) (liii chez) (liii alist) (liii list) (liii string) (liii unicode))
(export
  json-string-escape json-string-unescape string->json json->string
  json-ref json-ref*
  json-set json-set* json-push json-push* json-drop json-drop* json-reduce json-reduce*)
(begin

(define (json-string-escape str)
  (let ((out (open-output-string)))
    (write-char #\" out) ; 起始引号
    (let loop ((i 0))    ; 尾递归遍历
      (if (= i (string-length str))
          (begin
            (write-char #\" out) ; 结束引号
            (get-output-string out))
          (let ((c (string-ref str i)))
            (case c
              ((#\")  (display "\\\"" out))
              ((#\\)  (display "\\\\" out))
              ((#\/)  (display "\\/" out))
              ((#\backspace) (display "\\b" out))
              ((#\xc) (display "\\f" out))
              ((#\newline) (display "\\n" out))
              ((#\return) (display "\\r" out))
              ((#\tab)    (display "\\t" out))
              (else (write-char c out)))
            (loop (+ i 1))))))) ; 尾递归调用


(define (string-length-sum strings)
  (let loop ((o 0)
             (rest strings))
    (cond
     ((eq? '() rest) o)
     (else
      (loop (+ o (string-length (car rest)))
            (cdr rest))))))

(define (fast-string-list-append strings)
  (let* ((output-length (string-length-sum strings))
         (output (make-string output-length #\_))
         (fill 0))
    (let outer ((rest strings))
      (cond
       ((eq? '() rest) output)
       (else
        (let* ((s (car rest))
               (n (string-length s)))
          (let inner ((i 0))
            (cond ((= i n) 'done)
                  (else
                   (string-set! output fill (string-ref s i))
                   (set! fill (+ fill 1))
                   (inner (+ i 1))))))
        (outer (cdr rest)))))))

(define (handle-escape-char s end len)
  (let ((next-char (if (< (+ end 1) len)
                       (string-ref s (+ end 1))
                       #f)))
    (case next-char
      ((#\")  ; 处理双引号
       (values "\\\"" 2))
      ((#\\)  ; 处理反斜杠
       (values "\\\\" 2))
      ((#\/)  ; 处理斜杠
       (values "/" 2))
      ((#\b)  ; 处理退格符
       (values "\\b" 2))
      ((#\f)  ; 处理换页符
       (values "\\f" 2))
      ((#\n)  ; 处理换行符
       (values "\\n" 2))
      ((#\r)  ; 处理回车符
       (values "\\r" 2))
      ((#\t)  ; 处理制表符
       (values "\\t" 2))
      ((#\u)  ; 处理 \u 转义字符
       (let ((start-pos (+ end 2))  ; \u 后的起始位置
             (end-pos (+ end 6)))   ; \u 后的结束位置
         (if (and (>= start-pos 0) (< end-pos len))  ; 检查索引是否有效
             (let ((hex-str (substring s start-pos end-pos)))  ; 提取 4 位十六进制数
               (let ((code-point (string->number hex-str 16)))  ; 将十六进制转换为整数
                 (when (not code-point)
                   (error 'parse-error (string-append "Invalid HEX sequence " hex-str)))
                 ;; 检查是否存在连续的两个 \u
                 (let ((next-u-pos (+ end 6)))  ; 下一个 \u 的起始位置
                   (if (and (< (+ next-u-pos 6) len)  ; 检查是否足够剩余字符
                            (char=? (string-ref s next-u-pos) #\\)
                            (char=? (string-ref s (+ next-u-pos 1)) #\u))
                       ;; 存在连续的两个 \u
                       (let ((next-hex-str (substring s (+ next-u-pos 2) (+ next-u-pos 6))))  ; 提取下一个 4 位十六进制数
                         (let ((next-code-point (string->number next-hex-str 16)))  ; 将十六进制转换为整数
                           (when (not next-code-point)
                             (error 'parse-error (string-append "Invalid HEX sequence " next-hex-str)))
                           ;; 检查是否满足代理对条件
                           (if (and (>= code-point #xD800) (<= code-point #xDBFF)  ; 高代理
                                    (>= next-code-point #xDC00) (<= next-code-point #xDFFF))  ; 低代理
                               ;; 满足代理对条件，使用 unicode 模块计算码点并转换为字符串
                               (let ((surrogate-code-point (+ (* (- code-point #xD800) #x400)
                                                              (- next-code-point #xDC00) #x10000)))  ; 计算码点
                                 (values (utf8->string (codepoint->utf8 surrogate-code-point)) 12))
                               ;; 不满足代理对条件，仅对第一个 \u 进行转换
                               (values (utf8->string (codepoint->utf8 code-point)) 6))))
                       ;; 不存在连续的两个 \u，仅对第一个 \u 进行转换
                       (values (utf8->string (codepoint->utf8 code-point)) 6)))))
             ;; 索引无效，返回原字符
             (error 'parse-error (string-append "HEX sequence too short " (substring s start-pos))))))
      (else
       (error 'parse-error (string-append "Invalid escape char: " (string next-char)))))))

(define string->json
  (lambda (s)
    (read (open-input-string
      (let loop
        ((s s) (bgn 0) (end 0) (rst '()) (len (string-length s)) (quts? #f) (lst '(#t)))
        (cond
          ((= end len)
           (fast-string-list-append (reverse rst)))
          ((and quts? (char=? (string-ref s end) #\\) (< (+ end 1) len))
           (let-values (((unescaped step) (handle-escape-char s end len)))
             (loop s (+ end step) (+ end step)
                   (cons (string-append (substring s bgn end) unescaped) rst)
                   len quts? lst)))
          ((and quts? (not (char=? (string-ref s end) #\")))
           (loop s bgn (+ 1 end) rst len quts? lst))
          (else
            (case (string-ref s end)
              ((#\{)
               (loop s (+ 1 end) (+ 1 end) 
                 (cons 
                   (string-append 
                     (substring s bgn end) "((" ) rst) len quts? (cons #t lst)))
              ((#\})
               (loop s (+ 1 end) (+ 1 end)
                 (cons
                   (string-append
                     (substring s bgn end) "))") rst) len quts? (loose-cdr lst)))
              ((#\[)
               (loop s (+ 1 end) (+ 1 end) 
                  (cons
                    (string-append 
                      (substring s bgn end) "#(") rst) len quts? (cons #f lst)))
              ((#\])
               (loop s (+ 1 end) (+ 1 end) 
                 (cons 
                   (string-append 
                     (substring s bgn end) ")") rst) len quts? (loose-cdr lst)))
              ((#\:)
               (loop s (+ 1 end) (+ 1 end) 
                 (cons 
                   (string-append 
                     (substring s bgn end) " . ") rst) len quts? lst))
              ((#\,)
               (loop s (+ 1 end) (+ 1 end) 
                 (cons 
                   (string-append 
                     (substring s bgn end) 
                     (if (loose-car lst) ")(" " ")) rst) len quts? lst))
              ((#\")
               (loop s bgn (+ 1 end) rst len (not quts?) lst))
              (else
               (loop s bgn (+ 1 end) rst len quts? lst))))))))))

(define json->string
  (lambda (json-scm)
    (define f
      (lambda (x)
        (cond                           
          ((string? x) (json-string-escape x))                        
          ((number? x) (number->string x))                             
          ((boolean? x) (if x "true" "false"))
          ((symbol? x) (symbol->string x))
          ((null? x) "{}")
          (else (type-error "Unexpected x: " x)))))

    (define (delim x)
      (if (zero? x) "" ","))
    
    (when (procedure? json-scm)
      (type-error "json->string: input must not be a procedure"))
    
    (let loop ((lst json-scm) (x (if (vector? json-scm) "[" "{")))
      (if (vector? lst)
          (string-append x 
            (let loop-v ((len (vector-length lst)) (n 0) (y ""))
              (if (< n len)
                  (let* ((k (vector-ref lst n))
                         (result (cond 
                                  ((vector? k) 
                                   (loop k "["))
                                  ((pair? k) 
                                   (loop k "{"))
                                  (else 
                                   (f k)))))
                    (loop-v len (+ n 1)
                      (string-append y (delim n) result)))
                  (string-append y "]"))))
          (let* ((d (car lst))
                 (k (loose-car d))
                 (v (loose-cdr d)))
            (when (not (list? d))
              (value-error d " must be a list"))
            (let ((len (length d)))
              (when (not (or (= len 0) (= len -1) (>= len 2)))
                    (value-error d " must be null, pair, or list with at least 2 elements")))
            
            (if (null? (cdr lst))
                (if (null? d)
                    "{}"
                    (string-append x (f k) ":"
                      (cond
                        ((null? v) "{}")
                        ((list? v) (loop v "{"))
                        ((vector? v) (loop v "["))
                        (else (f v)))
                      "}"))
                (loop (cdr lst)
                      (cond 
                        ((list? v) (string-append x (f k) ":" (loop v "{") ","))
                        ((vector? v) (string-append x (f k) ":" (loop v "[") ","))
                        (else (string-append x (f k) ":" (f v) ","))))))))))


(define json-ref
  (lambda (x k)
    (define return
      (lambda (x)
        (if (symbol? x)
            (cond
              ((symbol=? x 'true) #t)
              ((symbol=? x 'false) #f)
              (else x))
            x)))
    (if (vector? x)
        (return (vector-ref x k))
        (let loop ((x x) (k k))
          (if (null? x)
              '()
              (if (equal? (caar x) k)
                  (return (cdar x))
                  (loop (cdr x) k)))))))

(define (json-ref* j . keys)
  (let loop ((expr j) (keys keys))
    (if (null? keys)
        expr
        (loop (json-ref expr (car keys)) (cdr keys)))))

(define json-set
  (lambda (x v p)
    (let ((x x) (v v) (p (if (procedure? p) p (lambda (x) p))))
      (if (vector? x)
          (list->vector
            (cond 
              ((boolean? v)
                (if v
                  (let l ((x (vector->alist x))(p p))
                    (if (null? x)
                      '()
                      (cons (p (cdar x)) (l (cdr x) p))))))
              ((procedure? v)
                (let l ((x (vector->alist x))(v v)(p p))
                  (if (null? x)
                    '()
                    (if (v (caar x))
                      (cons (p (cdar x)) (l (cdr x) v p))
                      (cons (cdar x) (l (cdr x) v p))))))
              (else
                (let l ((x (vector->alist x))(v v)(p p))
                  (if (null? x)
                    '()
                    (if (equal? (caar x) v)
                      (cons (p (cdar x)) (l (cdr x) v p))
                      (cons (cdar x) (l (cdr x) v p))))))))
          (cond
            ((boolean? v)
              (if v
                (let l ((x x)(p p))
                  (if (null? x)
                    '()
                    (cons (cons (caar x) (p (cdar x)))(l (cdr x) p))))))
            ((procedure? v)
              (let l ((x x)(v v)(p p))
                (if (null? x)
                  '()
                  (if (v (caar x))
                    (cons (cons (caar x) (p (cdar x)))(l (cdr x) v p))
                    (cons (car x) (l (cdr x) v p))))))
            (else
              (let l ((x x)(v v)(p p))
                (if (null? x)
                  '()
                  (if (equal? (caar x) v)
                    (cons (cons v (p (cdar x)))(l (cdr x) v p))
                    (cons (car x) (l (cdr x) v p)))))))))))

(define (json-set* json k0 k1_or_v . ks_and_v)
  (if (null? ks_and_v)
      (json-set json k0 k1_or_v)
      (json-set json k0
        (lambda (x)
          (apply json-set* (cons x (cons k1_or_v ks_and_v)))))))

(define (json-push x k v)
  (if (vector? x)
      (if (= (vector-length x) 0)
          (vector v)
          (list->vector
            (let l ((x (vector->alist x)) (k k) (v v) (b #f))
              (if (null? x)
                  (if b '() (cons v '()))
                  (if (equal? (caar x) k)
                      (cons v (cons (cdar x) (l (cdr x) k v #t)))
                      (cons (cdar x) (l (cdr x) k v b)))))))
      (cons (cons k v) x)))

(define (json-push* json k0 v0 . rest)
  (if (null? rest)
      (json-push json k0 v0)
      (json-set json k0
        (lambda (x) (apply json-push* (cons x (cons v0 rest)))))))

(define json-drop
  (lambda (x v)
    (if (vector? x)
        (if (zero? (vector-length x))
            x
            (list->vector
             (cond
               ((procedure? v)
                (let l ((x (vector->alist x)) (v v))
                  (if (null? x)
                      '()
                      (if (v (caar x))
                          (l (cdr x) v)
                          (cons (cdar x) (l (cdr x) v))))))
               (else
                (let l ((x (vector->alist x)) (v v))
                  (if (null? x)
                      '()
                      (if (equal? (caar x) v)
                          (l (cdr x) v)
                          (cons (cdar x) (l (cdr x) v)))))))))
        (cond
          ((procedure? v)
           (let l ((x x) (v v))
             (if (null? x)
                 '()
                 (if (v (caar x))
                     (l (cdr x) v)
                     (cons (car x) (l (cdr x) v))))))
          (else
           (let l ((x x) (v v))
             (if (null? x)
                 '()
                 (if (equal? (caar x) v)
                     (l (cdr x) v)
                     (cons (car x) (l (cdr x) v))))))))))

(define json-drop*
  (lambda (json key . rest)
    (if (null? rest)
        (json-drop json key)
        (json-set json key
                  (lambda (x) (apply json-drop* (cons x rest)))))))

(define json-reduce
  (lambda (x v p)
    (if (vector? x)
        (list->vector
         (cond
           ((boolean? v)
            (if v
                (let l ((x (vector->alist x)) (p p))
                  (if (null? x)
                      '()
                      (cons (p (caar x) (cdar x)) (l (cdr x) p))))
                x))
           ((procedure? v)
            (let l ((x (vector->alist x)) (v v) (p p))
              (if (null? x)
                  '()
                  (if (v (caar x))
                      (cons (p (caar x) (cdar x)) (l (cdr x) v p))
                      (cons (cdar x) (l (cdr x) v p))))))
           (else
            (let l ((x (vector->alist x)) (v v) (p p))
              (if (null? x)
                  '()
                  (if (equal? (caar x) v)
                      (cons (p (caar x) (cdar x)) (l (cdr x) v p))
                      (cons (cdar x) (l (cdr x) v p))))))))
        (cond
          ((boolean? v)
           (if v
               (let l ((x x) (p p))
                 (if (null? x)
                     '()
                     (cons (cons (caar x) (p (caar x) (cdar x))) (l (cdr x) p))))
               x))
          ((procedure? v)
           (let l ((x x) (v v) (p p))
             (if (null? x)
                 '()
                 (if (v (caar x))
                     (cons (cons (caar x) (p (caar x) (cdar x))) (l (cdr x) v p))
                     (cons (car x) (l (cdr x) v p))))))
          (else
           (let l ((x x) (v v) (p p))
             (if (null? x)
                 '()
                 (if (equal? (caar x) v)
                     (cons (cons v (p v (cdar x))) (l (cdr x) v p))
                     (cons (car x) (l (cdr x) v p))))))))))

(define (json-reduce* j v1 v2 . rest)
  (cond
    ((null? rest) (json-reduce j v1 v2))
    ((length=? 1 rest)
     (json-reduce j v1
       (lambda (x y)
         (let* ((new-v1 v2)
                (p (last rest)))
          (json-reduce y new-v1
                       (lambda (n m) (p (list x n) m)))))))
    (else
     (json-reduce j v1
       (lambda (x y)
         (let* ((new-v1 v2)
                (p (last rest)))
          (apply json-reduce*
                 (append (cons y (cons new-v1 (drop-right rest 1)))
                         (list (lambda (n m) (p (cons x n) m)))))))))))

) ; end of begin
) ; end of define-library
