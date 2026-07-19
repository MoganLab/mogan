;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1144.scm
;; DESCRIPTION : 基准测试：define / define-public / tm-define 的调用开销对比
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   tm-define 展开后经 (varlet (rootlet) ...) 把函数放进 rootlet（哈希表），
;;   S7 无法缓存查找位置，每次调用都走哈希查找；define / define-public 把函数
;;   放进模块 let 环境，S7 用槽位向量访问。用 fib 30 的递归调用放大该差异。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1144
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1144))

(define fib-n 30)

(define (bench label thunk)
  (let* ((start (texmacs-time))
         (result (thunk))
         (elapsed (- (texmacs-time) start))
        ) ;
    (display "[1144] ")
    (display label)
    (display ": result=")
    (display result)
    (display " time=")
    (display elapsed)
    (display " ms")
    (newline)
  ) ;let*
) ;define

;; 1) let 内局部 define：S7 对局部绑定优化最好，作为下限参照
(define (bench-local-define)
  (let ()
    (define (fib n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2)))))
    (bench "local define " (lambda () (fib fib-n)))
  ) ;let
) ;define

;; 2) 模块顶层 define：函数进模块 let 环境
(define (fib-top n) (if (< n 2) n (+ (fib-top (- n 1)) (fib-top (- n 2)))))

(define (bench-top-define)
  (bench "top-level define" (lambda () (fib-top fib-n)))
) ;define

;; 3) define-public：define + export，与 define 同为模块 let 环境
(define-public (fib-pub n)
  (if (< n 2) n (+ (fib-pub (- n 1)) (fib-pub (- n 2))))
) ;define-public

(define (bench-define-public)
  (bench "define-public   " (lambda () (fib-pub fib-n)))
) ;define

;; 4) tm-define：无条件分支也经 (varlet (rootlet) ...) 进 rootlet
(tm-define (fib-tm n) (if (< n 2) n (+ (fib-tm (- n 1)) (fib-tm (- n 2)))))

(define (bench-tm-define)
  (bench "tm-define       " (lambda () (fib-tm fib-n)))
) ;define

(tm-define (test_1144)
  (bench-local-define)
  (bench-top-define)
  (bench-define-public)
  (bench-tm-define)
) ;tm-define
