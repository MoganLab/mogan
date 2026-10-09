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
;;

(define-library (liii par)
  (import (scheme base) (liii error) (liii go) (srfi srfi-133))
  (export par-for-each par-map par-filter vector-par-for-each vector-par-map
    vector-par-filter
  ) ;export
  (begin
    ;; (liii go) worker 结果信封协议：(ok value) 或 (error sym irritants)
    (define (%par-result-error? res)
      (and (pair? res) (eq? (car res) 'error))
    ) ;define

    (define (%par-rethrow! err)
      (apply error (cadr err) (caddr err))
    ) ;define

    ;; Join Barrier：从 ch 收满 count 个信封，全部完成后重抛首个异常
    (define (%par-join! ch count)
      (let loop
        ((i 0) (first-error #f))
        (if (= i count)
          (when first-error
            (%par-rethrow! first-error)
          ) ;when
          (let ((res (chan-recv! ch)))
            (loop (+ i 1) (or first-error (and (%par-result-error? res) res)))
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define %par-builtin-predicates
      '(even? odd? zero? positive? negative? number? string? symbol? boolean?
         char? null? pair? vector? integer? real? rational? exact? inexact?)
    ) ;define

    ;; 提取 f 的源码：lambda 直接取 procedure-source；白名单内的内置谓词
    ;; （无源码可取）按其打印名重建等价的 lambda 源码；其余抛 type-error。
    (define (%par-proc-source caller f)
      (let ((src (procedure-source f))
            (fail (lambda ()
                    (type-error (string-append (symbol->string caller)
                                  ": cannot extract source code from procedure"
                                ) ;string-append
                      f
                    ) ;type-error
                  ) ;lambda
            ) ;fail
           ) ;
        (if (pair? src)
          src
          (let ((str (object->string f)))
            (if (and (not (string=? str "")) (not (char=? (string-ref str 0) #\#)))
              (let ((sym (string->symbol str)))
                (if (memq sym %par-builtin-predicates) `(lambda (x) (,sym x)) (fail))
              ) ;let
              (fail)
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; 把 f 的整个 funclet 链摊平进一个新 inlet，并附加 extra-bindings
    ;; （如 worker 源码引用的 res-ch），作为 go-apply/source 的自由变量
    ;; 捕获环境。
    (define (%par-capture-env f . extra-bindings)
      (let ((e (apply inlet extra-bindings)))
        (let loop
          ((cur (funclet f)))
          (when (and (let? cur) (not (eq? cur (rootlet))))
            (varlet e cur)
            (loop (outlet cur))
          ) ;when
        ) ;let
        e
      ) ;let
    ) ;define

    ;; 把 vec 切成 p = min(n, workers) 块，每块为 (vector k sub)：
    ;; k 是块序号，sub 是该块的元素副本。
    (define (%par-vector-chunks vec workers)
      (let* ((n (vector-length vec))
             (p (min n (max 1 workers)))
             (base (quotient n p))
             (rem (remainder n p))
             (chunks (make-vector p))
            ) ;
        (let loop
          ((k 0) (start 0))
          (if (= k p)
            chunks
            (let* ((size (if (< k rem) (+ base 1) base)) (end (+ start size)))
              (vector-set! chunks k (vector k (vector-copy vec start end)))
              (loop (+ k 1) end)
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    ;; 把 worker-src 按 chunk 派发到 worker 池：每个 chunk 一个
    ;; go-apply/source，结果信封进 done-ch。
    (define (%par-spawn-chunks worker-src env chunks done-ch)
      (let ((p (vector-length chunks)))
        (let loop
          ((k 0))
          (when (< k p)
            (go-apply/source worker-src env (list (vector-ref chunks k)) done-ch)
            (loop (+ k 1))
          ) ;when
        ) ;let
      ) ;let
    ) ;define

    ;; map/filter 的公共骨架：派发 worker-src（源码内须把 (cons 块序号 子向量)
    ;; 结果送入捕获环境中的 res-ch），收敛全部 chunk 后按块序号归位并顺序拼接。
    (define (%par-vector-collect worker-src f vec workers)
      (let* ((chunks (%par-vector-chunks vec workers))
             (p (vector-length chunks))
             (res-ch (make-chan p))
             (done-ch (make-chan p))
             (env (%par-capture-env f 'res-ch res-ch))
            ) ;
        (%par-spawn-chunks worker-src env chunks done-ch)
        (%par-join! done-ch p)
        (let ((ordered (make-vector p)))
          (let loop
            ((i 0))
            (when (< i p)
              (let ((r (chan-recv! res-ch)))
                (vector-set! ordered (car r) (cdr r))
                (loop (+ i 1))
              ) ;let
            ) ;when
          ) ;let
          (vector-concatenate (vector->list ordered))
        ) ;let
      ) ;let*
    ) ;define

    (define (vector-par-for-each f vec . opt)
      (unless (procedure? f)
        (type-error "vector-par-for-each: first argument must be a procedure" f)
      ) ;unless
      (unless (vector? vec)
        (type-error "vector-par-for-each: second argument must be a vector" vec)
      ) ;unless
      (unless (zero? (vector-length vec))
        (let* ((workers (if (pair? opt) (car opt) (go-worker-count)))
               (chunks (%par-vector-chunks vec workers))
               (done-ch (make-chan (vector-length chunks)))
               (src-f (%par-proc-source 'vector-par-for-each f))
               (env (%par-capture-env f))
               (worker-src `(lambda (chunk)
                              (vector-for-each ,src-f (vector-ref chunk 1))
                              ,#t))
              ) ;
          (%par-spawn-chunks worker-src env chunks done-ch)
          (%par-join! done-ch (vector-length chunks))
        ) ;let*
      ) ;unless
    ) ;define

    (define (vector-par-map f vec . opt)
      (unless (procedure? f)
        (type-error "vector-par-map: first argument must be a procedure" f)
      ) ;unless
      (unless (vector? vec)
        (type-error "vector-par-map: second argument must be a vector" vec)
      ) ;unless
      (if (zero? (vector-length vec))
        #()
        (let ((src-f (%par-proc-source 'vector-par-map f)))
          (%par-vector-collect `(lambda (chunk)
                                  (chan-send! res-ch
                                    (cons (vector-ref chunk 0)
                                      (vector-map ,src-f (vector-ref chunk 1))))
                                  ,#t)
            f
            vec
            (if (pair? opt) (car opt) (go-worker-count))
          ) ;%par-vector-collect
        ) ;let
      ) ;if
    ) ;define

    (define (vector-par-filter pred vec . opt)
      (unless (procedure? pred)
        (type-error "vector-par-filter: first argument must be a procedure" pred)
      ) ;unless
      (unless (vector? vec)
        (type-error "vector-par-filter: second argument must be a vector" vec)
      ) ;unless
      (if (zero? (vector-length vec))
        #()
        (let ((src-pred (%par-proc-source 'vector-par-filter pred)))
          (%par-vector-collect `(lambda (chunk)
                                  (let ((sub (vector-ref chunk 1)))
                                    (let loop
                                      ((j 0) (acc '()))
                                      (if (= j (vector-length sub))
                                        (begin
                                          (chan-send! res-ch
                                            (cons (vector-ref chunk 0)
                                              (list->vector (reverse acc))))
                                          #t)
                                        (let ((val (vector-ref sub j)))
                                          (loop (+ j 1)
                                            (if (,src-pred val)
                                              (cons val acc)
                                              acc)))))))
            pred
            vec
            (if (pair? opt) (car opt) (go-worker-count))
          ) ;%par-vector-collect
        ) ;let
      ) ;if
    ) ;define

    (define (par-for-each f l)
      (unless (procedure? f)
        (type-error "par-for-each: first argument must be a procedure" f)
      ) ;unless
      (unless (list? l)
        (type-error "par-for-each: second argument must be a list" l)
      ) ;unless
      (unless (null? l)
        (let* ((n (length l)) (done-ch (make-chan n)))
          (for-each (lambda (elem) (go-apply f (list elem) done-ch)) l)
          (%par-join! done-ch n)
        ) ;let*
      ) ;unless
    ) ;define

    (define (par-map f l)
      (unless (procedure? f)
        (type-error "par-map: first argument must be a procedure" f)
      ) ;unless
      (unless (list? l)
        (type-error "par-map: second argument must be a list" l)
      ) ;unless
      (let ((chans (map (lambda (_) (make-chan 1)) l)))
        (for-each (lambda (elem ch) (go-apply f (list elem) ch)) l chans)
        ;; Join Barrier：按序读取每个专属通道，收满所有结果
        (let loop
          ((chs chans) (results '()) (first-error #f))
          (if (null? chs)
            (if first-error (%par-rethrow! first-error) (reverse results))
            (let ((res (chan-recv! (car chs))))
              (if (%par-result-error? res)
                (loop (cdr chs) results (or first-error res))
                (loop (cdr chs) (cons (cadr res) results) first-error)
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (par-filter pred l)
      (unless (procedure? pred)
        (type-error "par-filter: first argument must be a procedure" pred)
      ) ;unless
      (unless (list? l)
        (type-error "par-filter: second argument must be a list" l)
      ) ;unless
      (if (null? l)
        '()
        (let ((flags (par-map pred l)))
          (let loop
            ((elems l) (fs flags) (acc '()))
            (if (null? elems)
              (reverse acc)
              (loop (cdr elems) (cdr fs) (if (car fs) (cons (car elems) acc) acc))
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define
  ) ;begin
) ;define-library
