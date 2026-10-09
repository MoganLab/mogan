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
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (liii go)
  (import (scheme base)
    (scheme case-lambda)
    (scheme time)
    (liii base)
    (liii error)
    (liii hash-table)
    (liii queue)
    (liii syntax-case)
  ) ;import
  (export go go-call go-apply go-apply/source go-worker-count go-result
    go-result-recv! make-chan chan? chan-send! chan-recv! chan-try-recv!
    chan-try-send! chan-close! chan-closed? select make-context
    make-timeout-context context? context-done? context-cancel! context-channel
    spawn-fiber fiber-yield! fiber-scheduler-run! make-fiber-chan fiber-chan?
    fiber-send! fiber-recv! %set-scheduler-idle-return! %run-worker-task
    %drain-suspended!
  ) ;export
  (begin
    (define make-chan (case-lambda (() (g_make-chan 0)) ((cap) (g_make-chan cap))))

    (define (chan? obj)
      (g_chan? obj)
    ) ;define

    (define chan-send!
      (case-lambda
       ((ch val) (fiber-chan-send! ch val))
       ((ch val timeout-ms) (g_chan-send! ch val timeout-ms))
      ) ;case-lambda
    ) ;define

    (define chan-recv!
      (case-lambda
       ((ch) (fiber-chan-recv! ch))
       ((ch timeout-ms) (g_chan-recv! ch timeout-ms))
       ((ch timeout-ms default-val) (g_chan-recv! ch timeout-ms default-val))
      ) ;case-lambda
    ) ;define

    (define chan-try-recv!
      (case-lambda
       ((ch) (g_chan-try-recv! ch #f))
       ((ch default-val) (g_chan-try-recv! ch default-val))
      ) ;case-lambda
    ) ;define

    (define (chan-try-send! ch val)
      (g_chan-send! ch val 0)
    ) ;define

    (define (chan-close! ch)
      (g_chan-close! ch)
    ) ;define

    (define (chan-closed? ch)
      (g_chan-closed? ch)
    ) ;define

    (define (go-worker-count)
      (g_go-worker-count)
    ) ;define

    (define-record-type <go-context>
      (%make-context done-chan)
      context?
      (done-chan context-channel)
    ) ;define-record-type

    (define (make-context)
      (%make-context (make-chan 1))
    ) ;define

    (define (context-done? ctx)
      (chan-closed? (context-channel ctx))
    ) ;define

    (define (context-cancel! ctx)
      (when (not (context-done? ctx))
        (chan-close! (context-channel ctx))
      ) ;when
    ) ;define

    (define (make-timeout-context ms)
      ;; 使用 C++ 定时器到期关闭 done channel，不占用 worker 线程
      (let ((ctx (make-context)))
        (g_chan-timeout-close! (context-channel ctx) ms)
        ctx
      ) ;let
    ) ;define

    ;; -----------------------------------------------------------------------
    ;; M:N 混合协程调度器：单 Session 内基于 call/cc 的用户态轻量协程调度引擎
    ;; -----------------------------------------------------------------------

    (define *ready-queue* (make-list-queue (list)))
    (define *scheduler-return* #f)
    ;; 外层调度器返回 continuation 的保存栈：fiber-scheduler-run! 可嵌套
    ;; （worker 任务包 fiber 后，任务体内可再调调度器），收尾逃逸必须只重入
    ;; 最近一次、尚未返回的那层，否则会重入已结束的调度导致代码重放
    (define *scheduler-return-stack* '())
    (define *suspended-fibers* 0)
    ;; M:N 阶段二：真 channel 挂起的跨线程唤醒
    (define *gate* (g_make-gate))
    (define *watch-thunks* (make-hash-table))
    (define *next-watch-id* 0)
    (define *real-chan-suspended* 0)
    ;; 空闲策略：#t（默认，主会话）无可运行 fiber 且有真 channel 挂起时 park 等待；
    ;; #f（worker 会话）改为返回调用方，不阻塞物理线程，事件延迟到下次进入调度器
    (define *scheduler-idle-park* #t)

    (define (%set-scheduler-idle-return!)
      ;; worker 会话初始化时调用：调度器空闲时返回 C++ 层，永不 park
      (set! *scheduler-idle-park* #f)
    ) ;define

    (define (enqueue-fiber! thunk)
      (list-queue-add-back! *ready-queue* thunk)
    ) ;define

    (define (%gate-dispatch! id)
      ;; 唤醒登记在 id 上的挂起 fiber。thunk 调用 k 后控制流切走，不再返回，
      ;; 因此本函数返回仅发生在找不到 watcher（防御路径）的情形
      (let ((thunk (hash-table-ref *watch-thunks* id)))
        (if thunk (begin (hash-table-set! *watch-thunks* id #f) (thunk)))
      ) ;let
    ) ;define

    (define (schedule-next!)
      (if (list-queue-empty? *ready-queue*)
        (cond ((> *real-chan-suspended* 0)
               ;; 有 fiber 挂在真 channel 上：唤醒可能来自其他线程，先非阻塞 drain gate
               (let ((id (g_gate-try-wait *gate*)))
                 (if (integer? id)
                   (begin
                     (%gate-dispatch! id)
                     (schedule-next!)
                   ) ;begin
                   (if *scheduler-idle-park*
                     ;; 主会话：物理挂起等待新事件（保持既有行为）
                     (begin
                       (%gate-dispatch! (g_gate-wait *gate*))
                       (schedule-next!)
                     ) ;begin
                     ;; worker 会话：返回 C++ 层继续取下一个任务，事件延迟消费
                     (if *scheduler-return* (*scheduler-return* #t) #f)
                   ) ;if
                 ) ;if
               ) ;let
              ) ;
              ((> *suspended-fibers* 0)
               ;; 就绪队列空且仅存在 fiber-chan 挂起：全体死锁，报错而非静默退出
               (let ((n *suspended-fibers*))
                 (set! *suspended-fibers* 0)
                 (set! *scheduler-return* #f)
                 (error 'deadlock "all fibers are blocked on channel operations" n)
               ) ;let
              ) ;
              (else (if *scheduler-return* (*scheduler-return* #t) #f))
        ) ;cond
        (let ((next-thunk (list-queue-front *ready-queue*)))
          (list-queue-remove-front! *ready-queue*)
          (next-thunk)
        ) ;let
      ) ;if
    ) ;define

    (define (spawn-fiber thunk)
      (enqueue-fiber! (lambda () (thunk) (schedule-next!)))
    ) ;define

    (define (fiber-yield!)
      (call/cc (lambda (k) (enqueue-fiber! (lambda () (k #t))) (schedule-next!)))
    ) ;define

    (define (fiber-scheduler-run!)
      ;; 进入时保存外层返回 continuation；call/cc 无论从正常调度结束还是收尾
      ;; 逃逸（(*scheduler-return* #t)）返回，都顺序执行下方的恢复代码，把
      ;; *scheduler-return* 还给外层调度。不能用 dynamic-wind：s7 的 wind after
      ;; 在 continuation 逃逸重入时不执行。deadlock 走 error 逃逸（longjmp），
      ;; 不经过恢复代码，由 schedule-next! 的 deadlock 分支显式清理
      (set! *scheduler-return-stack*
        (cons *scheduler-return* *scheduler-return-stack*)
      ) ;set!
      (let ((r (call/cc (lambda (exit-k) (set! *scheduler-return* exit-k) (schedule-next!)))
            ) ;r
           ) ;
        (set! *scheduler-return* (car *scheduler-return-stack*))
        (set! *scheduler-return-stack* (cdr *scheduler-return-stack*))
        r
      ) ;let
    ) ;define

    (define-record-type <fiber-chan>
      (%make-fiber-chan buffer waiting-receivers)
      fiber-chan?
      (buffer %fch-buf %fch-set-buf!)
      (waiting-receivers %fch-recv %fch-set-recv!)
    ) ;define-record-type

    (define (make-fiber-chan)
      (%make-fiber-chan (make-list-queue (list)) (make-list-queue (list)))
    ) ;define

    (define (fiber-send! fch val)
      (let ((recv-waiters (%fch-recv fch)))
        (if (list-queue-empty? recv-waiters)
          (list-queue-add-back! (%fch-buf fch) val)
          (let ((receiver-k (list-queue-front recv-waiters)))
            (list-queue-remove-front! recv-waiters)
            (set! *suspended-fibers* (- *suspended-fibers* 1))
            (enqueue-fiber! (lambda () (receiver-k val)))
          ) ;let
        ) ;if
      ) ;let
      (fiber-yield!)
    ) ;define

    (define (fiber-recv! fch)
      (let ((buf (%fch-buf fch)))
        (if (list-queue-empty? buf)
          (call/cc (lambda (k)
                     (list-queue-add-back! (%fch-recv fch) k)
                     (set! *suspended-fibers* (+ *suspended-fibers* 1))
                     (schedule-next!)
                   ) ;lambda
          ) ;call/cc
          (let ((val (list-queue-front buf)))
            (list-queue-remove-front! buf)
            val
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; -----------------------------------------------------------------------
    ;; 真 channel 的 fiber 版操作（库内部，不导出）：挂起协程而非阻塞物理线程，
    ;; 是 chan-recv!/chan-send! 无 timeout 形态的实现。
    ;; 唤醒来源可以是同会话 fiber、其他 worker 线程或 C++ 定时器
    ;; -----------------------------------------------------------------------

    (define (fiber-suspend-on! register-watch . reused-id)
      ;; register-watch : (lambda (id tag) ...) 执行原子 try-or-watch，
      ;; 返回非 tag 表示立即完成，返回 tag 表示已登记 watcher。
      ;; reused-id：唤醒后重试时复用原 id——send 侧的 fiber↔fiber 交接
      ;; （recv watcher 直接取走载荷）按 id 记账完成状态，send 重试以同 id
      ;; 查询确认后才不重发载荷
      (let* ((id (if (null? reused-id) *next-watch-id* (car reused-id)))
             (tag (gensym "watch"))
             (result (register-watch id tag))
            ) ;
        (if (not (eq? result tag))
          result
          (begin
            (set! *next-watch-id* (+ id 1))
            (call/cc (lambda (k)
                       (set! *real-chan-suspended* (+ *real-chan-suspended* 1))
                       (hash-table-set! *watch-thunks*
                         id
                         (lambda () (set! *real-chan-suspended* (- *real-chan-suspended* 1)) (k #t))
                       ) ;hash-table-set!
                       (schedule-next!)
                     ) ;lambda
            ) ;call/cc
            ;; 被唤醒后重试整个操作（就绪事件可能被竞争者抢先消费），复用 id
            (fiber-suspend-on! register-watch id)
          ) ;begin
        ) ;if
      ) ;let*
    ) ;define

    (define (fiber-chan-recv! ch)
      ;; 真 channel 的 fiber 版接收：挂起协程而非阻塞物理线程
      (fiber-suspend-on! (lambda (id tag) (g_chan-recv-or-watch! ch *gate* id tag)))
    ) ;define

    (define (fiber-chan-send! ch val)
      ;; 真 channel 的 fiber 版发送：挂起协程而非阻塞物理线程
      (fiber-suspend-on! (lambda (id tag) (g_chan-send-or-watch! ch val *gate* id tag))
      ) ;fiber-suspend-on!
    ) ;define

    ;; worker 会话的任务执行入口：把任务包成 fiber 推进就绪队列并运行调度器。
    ;; 调度器空闲（无可运行 fiber）时返回，挂起的 fiber 留在会话，由后续任务
    ;; 再次进入调度器时经 gate 事件唤醒（任务级阻塞）。外层 catch 兜住
    ;; deadlock 等调度器错误。必须是【源码定义】而非 C 层拼接的同构表达式：
    ;; C 拼装表达式走求值器通用路径，其 continuation 栈布局与源码路径不同，
    ;; 收尾 ((*scheduler-return* v)) 逃逸恢复时 op 错位，参数会被误传入 list-ref
    (define (%run-worker-task thunk err-handler)
      (catch #t (lambda () (spawn-fiber thunk) (fiber-scheduler-run!)) err-handler)
    ) ;define

    (define (%drain-suspended!)
      ;; worker 空闲（gate fire 空唤醒）时消费 gate 事件：有真 channel 挂起或
      ;; 就绪 fiber 才进入调度器，唤醒挂起 fiber；fiber-chan-only 挂起不进入，
      ;; 避免误报 deadlock（那是新任务进入调度器时的检查）
      (if (or (> *real-chan-suspended* 0) (not (list-queue-empty? *ready-queue*)))
        (fiber-scheduler-run!)
        #f
      ) ;if
    ) ;define

    ;; 主会话当前已加载的 R7RS 库列表（用于构造 worker 代码的前导 import）
    (define (%go-active-libs)
      (catch #t
        (lambda ()
          (if (and (defined? '*r7rs-libraries*) (hash-table? *r7rs-libraries*))
            (map car *r7rs-libraries*)
            '()
          ) ;if
        ) ;lambda
        (lambda (t a) '())
      ) ;catch
    ) ;define

    ;; worker 代码前导：恢复 *load-path* 并 import 主会话的全部活动库
    (define (%go-worker-prelude libs)
      `((set! *load-path* (quote ,*load-path*))
        ,@(if (null? libs) '() `((import ,@libs))))
    ) ;define

    (define (%go-arg-names n)
      (let loop
        ((i 0))
        (if (= i n)
          '()
          (cons (string->symbol (string-append "g_arg" (number->string i)))
            (loop (+ i 1))
          ) ;cons
        ) ;if
      ) ;let
    ) ;define

    ;; (go (captured-vars ...) body ...) 在后台 worker 线程的独立 s7 会话中执行 body。
    ;; 注意：捕获变量只支持可序列化的数据类型（数字、字符串、符号、列表、vector、
    ;; bytevector、channel、let 等），不支持过程/闭包——传入函数会在 spawn 时
    ;; 抛 type-error。在 body 中直接引用全局函数名（如 car、display）即可，无需捕获。
    (define (go-call fn . args)
      (if (not (procedure? fn))
        (error 'type-error "go: target must be a procedure" fn)
      ) ;if
      (let ((src (procedure-source fn)))
        (if (pair? src)
          (let* ((libs (%go-active-libs))
                 (arg-names (%go-arg-names (length args)))
                 (code `(begin ,@(%go-worker-prelude libs) (,src ,@arg-names)))
                ) ;
            (g_go-spawn arg-names args code)
          ) ;let*
          (error 'type-error "go: cannot extract source code from procedure" fn)
        ) ;if
      ) ;let
    ) ;define

    (define %go-call go-call)

    (define-syntax go
      (lambda (stx)
        (syntax-case stx
          ()
          ((_ (fn arg ...)) (syntax (go-call fn arg ...)))
          (_ (error 'syntax-error
               "go: invalid syntax, expected (go (fn arg ...))"
               (syntax->datum stx)
             ) ;error
          ) ;_
        ) ;syntax-case
      ) ;lambda
    ) ;define-syntax

    ;; -----------------------------------------------------------------------
    ;; go-result：带结果回传的 go。worker 执行完毕（含异常路径）后把结果送入
    ;; 缓冲 1 的结果 channel，接收端不会因任务异常而永久死等。
    ;; 结果对象协议：(ok value) | (error tag args)
    ;; -----------------------------------------------------------------------

    (define *go-result-timeout-sentinel* (cons #f #f))

    (define (%go-result-unwrap r)
      (cond ((and (pair? r) (eq? (car r) 'ok) (pair? (cdr r))) (cadr r))
            ((and (pair? r) (eq? (car r) 'error) (pair? (cdr r)))
             (apply error (cadr r) (caddr r))
            ) ;
            (else (error 'type-error "go-result-recv!: invalid result object" r))
      ) ;cond
    ) ;define

    (define go-result-recv!
      (case-lambda
       ((ch) (%go-result-unwrap (chan-recv! ch)))
       ((ch timeout-ms)
        (let ((r (chan-recv! ch timeout-ms *go-result-timeout-sentinel*)))
          (if (eq? r *go-result-timeout-sentinel*)
            (error 'timeout-error
              "go-result-recv!: timed out waiting for result"
              timeout-ms
            ) ;error
            (%go-result-unwrap r)
          ) ;if
        ) ;let
       ) ;
      ) ;case-lambda
    ) ;define

    ;; 内层 catch 覆盖"返回值/异常参数不可序列化"的失败：降级为 stderr 报告
    ;; （*go-err-handler* 只在 worker 会话中定义，此处引用不会出现在主会话）
    (define (%go-result-spawn vars vals body rc-sym rc)
      (let ((libs (%go-active-libs)))
        (g_go-spawn (append vars (list rc-sym))
          (append vals (list rc))
          `(begin
             ,@(%go-worker-prelude libs)
             (catch ,#t
               (lambda ,() (chan-send! ,rc-sym (list 'ok (begin ,@body))))
               (lambda (tag args)
                 (catch ,#t
                   (lambda ,() (chan-send! ,rc-sym (list 'error tag args)))
                   (lambda (t2 a2) (*go-err-handler* t2 a2))))))
        ) ;g_go-spawn
      ) ;let
    ) ;define

    (define-syntax go-result
      (lambda (stx)
        (syntax-case stx
          ()
          ((_ first . rest)
           (let* ((rc (car (generate-temporaries '(#f))))
                  (first-datum (syntax->datum (syntax first)))
                  (is-vars? (and (list? first-datum) (every symbol? first-datum) (pair? (syntax rest)))
                  ) ;is-vars?
                 ) ;
             (let ((real-vars (if is-vars? (syntax first) '()))
                   (real-body (if is-vars? (syntax rest) (syntax (first . rest))))
                  ) ;
               (quasisyntax (let (((unsyntax rc) (make-chan 1)))
                              (%go-result-spawn '(unsyntax (syntax->datum real-vars))
                                (list (unsyntax-splicing (if is-vars? (syntax first) '())))
                                '(unsyntax (syntax->datum real-body))
                                '(unsyntax rc)
                                (unsyntax rc)
                              ) ;%go-result-spawn
                              (unsyntax rc)
                            ) ;let
               ) ;quasisyntax
             ) ;let
           ) ;let*
          ) ;
        ) ;syntax-case
      ) ;lambda
    ) ;define-syntax

    ;; -----------------------------------------------------------------------
    ;; go-apply：把 (apply f args) 投递到后台 worker，结果按 go-result 协议
    ;; 送入调用方指定的 channel。与 go/go-result 不同，f 的词法自由变量中
    ;; 的可序列化数据会自动捕获运输，无需显式列出。
    ;; -----------------------------------------------------------------------

    ;; 提取 lambda 源码参数列表中的参数名（默认参数 (name default) 取 name）
    (define (%go-param-names args)
      (cond ((null? args) '())
            ((symbol? args) (list args))
            ((pair? args)
             (let ((first (car args)))
               (cons (if (pair? first) (car first) first) (%go-param-names (cdr args)))
             ) ;let
            ) ;
            (else '())
      ) ;cond
    ) ;define

    (define (%go-serializable-data? val)
      (and (not (undefined? val))
        (not (procedure? val))
        (not (syntax? val))
        (not (macro? val))
      ) ;and
    ) ;define

    ;; 提取 src 的词法自由变量绑定：遍历源码中的符号，凡不是参数、
    ;; 尚未捕获且在 env 中已定义、值为可序列化数据的符号，都捕获运输。
    (define (%go-free-vars src env)
      (if (not (pair? src))
        '()
        (let* ((params (if (pair? (cdr src)) (%go-param-names (cadr src)) '()))
               (bindings '())
              ) ;
          (let walk
            ((x (cddr src)))
            (cond ((and (pair? x) (eq? (car x) 'quote)) #f)
                  ((symbol? x)
                   (when (and (not (memq x params)) (not (assq x bindings)) (defined? x env))
                     (catch #t
                       (lambda ()
                         (let ((val (let-ref env x)))
                           (when (%go-serializable-data? val)
                             (set! bindings (cons (cons x val) bindings))
                           ) ;when
                         ) ;let
                       ) ;lambda
                       (lambda (t a) #f)
                     ) ;catch
                   ) ;when
                  ) ;
                  ((pair? x) (walk (car x)) (walk (cdr x)))
                  ((vector? x) (for-each walk (vector->list x)))
            ) ;cond
          ) ;let
          bindings
        ) ;let*
      ) ;if
    ) ;define

    ;; go-apply 与 go-apply/source 的共享投递逻辑：把 (src arg ...) 包装为
    ;; 带异常兜底的 worker 代码，连同捕获的自由变量一起 spawn。
    (define (%go-ship-apply src env args ch)
      (let* ((libs (%go-active-libs))
             (captured (%go-free-vars src env))
             ;; 参数名用 gensym，避免与用户捕获的自由变量重名
             (arg-names (map (lambda (a) (gensym "go-arg")) args))
             (ch-sym (gensym "go-apply-ch"))
             (names (append arg-names (map car captured) (list ch-sym)))
             (vals (append args (map cdr captured) (list ch)))
             (code `(begin
                      ,@(%go-worker-prelude libs)
                      (catch ,#t
                        (lambda ,()
                          (chan-send! ,ch-sym (list 'ok (,src ,@arg-names))))
                        (lambda (tag args)
                          (catch ,#t
                            (lambda ,()
                              (chan-send! ,ch-sym (list 'error tag args)))
                            (lambda (t2 a2)
                              (chan-send! ,ch-sym
                                (list 'error tag (list (object->string args)))))))))
             ) ;code
            ) ;
        (g_go-spawn names vals code)
      ) ;let*
    ) ;define

    (define (go-apply f args ch)
      (unless (procedure? f)
        (type-error "go-apply: first argument must be a procedure" f)
      ) ;unless
      (unless (list? args)
        (type-error "go-apply: second argument must be a list" args)
      ) ;unless
      (unless (chan? ch)
        (type-error "go-apply: third argument must be a channel" ch)
      ) ;unless
      (let ((src (procedure-source f)))
        (unless (pair? src)
          (type-error "go-apply: cannot extract source code from procedure" f)
        ) ;unless
        (%go-ship-apply src (funclet f) args ch)
      ) ;let
    ) ;define

    ;; go-apply/source：go-apply 的源码级变体。src 是过程源码表达式（lambda
    ;; 列表，或解析为过程的全局符号），env 是自由变量的捕获环境（通常是
    ;; 定义点的 funclet 或其派生 inlet）。供需要把多个过程的来源组合成一个
    ;; worker 的调用方（如 (liii par) 的分块 worker）使用，避免在主会话
    ;; eval 构造包装过程。
    (define (go-apply/source src env args ch)
      (unless (or (pair? src) (symbol? src))
        (type-error "go-apply/source: first argument must be a procedure source" src)
      ) ;unless
      (unless (let? env)
        (type-error "go-apply/source: second argument must be an environment (let)" env)
      ) ;unless
      (unless (list? args)
        (type-error "go-apply/source: third argument must be a list" args)
      ) ;unless
      (unless (chan? ch)
        (type-error "go-apply/source: fourth argument must be a channel" ch)
      ) ;unless
      (%go-ship-apply src env args ch)
    ) ;define

    (define-syntax select
      (lambda (stx)
        (syntax-case stx
          ()
          ((_ clause ...)
           (let ((else-branch #f)
                 (timeout-branch #f)
                 (timeout-arrow #f)
                 (timeout-ms 0)
                 (raw-clauses (syntax (clause ...)))
                 (cases '())
                ) ;
             (for-each (lambda (clause-stx)
                         (let ((clause (syntax->datum clause-stx)))
                           (cond ((and (pair? clause) (eq? (car clause) 'else))
                                  (if else-branch
                                    (error 'syntax-error "select: multiple else clauses")
                                    (set! else-branch (cdr clause-stx))
                                  ) ;if
                                 ) ;
                                 ;; 形式 1: ((timeout ms) => proc) 或 ((timeout ms) body ...)
                                 ((and (pair? clause) (pair? (car clause)) (eq? (caar clause) 'timeout))
                                  (if (or timeout-branch timeout-arrow)
                                    (error 'syntax-error "select: multiple timeout clauses")
                                    (begin
                                      (if (or (null? (cdar clause)) (not (null? (cddar clause))))
                                        (error 'syntax-error "select: invalid timeout clause format" clause)
                                      ) ;if
                                      (set! timeout-ms (cadar clause-stx))
                                      (let ((rest (cdr clause-stx)))
                                        (if (and (pair? (syntax->datum rest)) (eq? (car (syntax->datum rest)) '=>))
                                          (begin
                                            (if (or (null? (cdr (syntax->datum rest)))
                                                  (not (null? (cddr (syntax->datum rest))))
                                                ) ;or
                                              (error 'syntax-error "select: malformed => in timeout clause" clause)
                                            ) ;if
                                            (set! timeout-arrow (cadr rest))
                                          ) ;begin
                                          (set! timeout-branch rest)
                                        ) ;if
                                      ) ;let
                                    ) ;begin
                                  ) ;if
                                 ) ;
                                 ;; 形式 2: (timeout ms => proc) 或 (timeout ms body ...)
                                 ((and (pair? clause) (eq? (car clause) 'timeout))
                                  (if (or timeout-branch timeout-arrow)
                                    (error 'syntax-error "select: multiple timeout clauses")
                                    (begin
                                      (if (null? (cdr clause))
                                        (error 'syntax-error "select: invalid timeout clause format" clause)
                                      ) ;if
                                      (set! timeout-ms (cadr clause-stx))
                                      (let ((rest (cddr clause-stx)))
                                        (if (and (pair? (syntax->datum rest)) (eq? (car (syntax->datum rest)) '=>))
                                          (begin
                                            (if (or (null? (cdr (syntax->datum rest)))
                                                  (not (null? (cddr (syntax->datum rest))))
                                                ) ;or
                                              (error 'syntax-error "select: malformed => in timeout clause" clause)
                                            ) ;if
                                            (set! timeout-arrow (cadr rest))
                                          ) ;begin
                                          (set! timeout-branch rest)
                                        ) ;if
                                      ) ;let
                                    ) ;begin
                                  ) ;if
                                 ) ;
                                 ((and (pair? clause) (pair? (car clause))) (set! cases (cons clause-stx cases)))
                                 (else (error 'syntax-error "select: invalid clause" clause))
                           ) ;cond
                         ) ;let
                       ) ;lambda
               raw-clauses
             ) ;for-each

             (if (and else-branch (or timeout-branch timeout-arrow))
               (error 'syntax-error "select: cannot specify both else and timeout clauses")
             ) ;if

             (set! cases (reverse cases))

             (let ((result-sym (car (generate-temporaries '(#f))))
                   (t0-sym (car (generate-temporaries '(#f))))
                   (pre-bindings '())
                   (parsed-cases '())
                  ) ;
               (for-each (lambda (c-stx)
                           (let* ((c (syntax->datum c-stx))
                                  (action-stx (car c-stx))
                                  (body-stx (cdr c-stx))
                                  (action (car c))
                                  (body (cdr c))
                                  (op (car action))
                                 ) ;
                             (cond ((eq? op 'chan-recv!)
                                    (let ((ch-sym (car (generate-temporaries '(#f)))))
                                      (set! pre-bindings (cons (list ch-sym (cadr action-stx)) pre-bindings))
                                      (if (and (pair? body) (eq? (car body) '=>))
                                        (set! parsed-cases
                                          (cons (list 'recv-arrow ch-sym (cadr body-stx)) parsed-cases)
                                        ) ;set!
                                        (set! parsed-cases
                                          (cons (list 'recv ch-sym (caddr action-stx) body-stx) parsed-cases)
                                        ) ;set!
                                      ) ;if
                                    ) ;let
                                   ) ;
                                   ((eq? op 'chan-send!)
                                    (let ((ch-sym (car (generate-temporaries '(#f))))
                                          (val-sym (car (generate-temporaries '(#f))))
                                         ) ;
                                      (set! pre-bindings
                                        (cons (list ch-sym (cadr action-stx))
                                          (cons (list val-sym (caddr action-stx)) pre-bindings)
                                        ) ;cons
                                      ) ;set!
                                      (set! parsed-cases (cons (list 'send ch-sym val-sym body-stx) parsed-cases))
                                    ) ;let
                                   ) ;
                                   (else (error 'syntax-error "select: unsupported channel operation" op))
                             ) ;cond
                           ) ;let*
                         ) ;lambda
                 cases
               ) ;for-each

               (set! pre-bindings (reverse pre-bindings))
               (set! parsed-cases (reverse parsed-cases))

               (let ((recv-cases '()) (send-cases '()))
                 (for-each (lambda (c)
                             (if (or (eq? (car c) 'recv) (eq? (car c) 'recv-arrow))
                               (set! recv-cases (append recv-cases (list c)))
                               (set! send-cases (append send-cases (list c)))
                             ) ;if
                           ) ;lambda
                   parsed-cases
                 ) ;for-each

                 (let ((recv-dispatch (let loop
                                        ((rcs recv-cases) (i 0) (acc '()))
                                        (if (null? rcs)
                                          (reverse acc)
                                          (let* ((rc (car rcs))
                                                 (kind (car rc))
                                                 (dispatch-expr (if (eq? kind 'recv-arrow)
                                                                  (quasisyntax ((unsyntax (caddr rc)) (vector-ref (unsyntax result-sym) 2)))
                                                                  (quasisyntax ((lambda ((unsyntax (caddr rc))) (unsyntax-splicing (cadddr rc)))
                                                                                (vector-ref (unsyntax result-sym) 2)
                                                                               ) ;
                                                                  ) ;quasisyntax
                                                                ) ;if
                                                 ) ;dispatch-expr
                                                ) ;
                                            (loop (cdr rcs)
                                              (+ i 1)
                                              (cons (quasisyntax (((unsyntax i)) (unsyntax dispatch-expr))) acc)
                                            ) ;loop
                                          ) ;let*
                                        ) ;if
                                      ) ;let
                       ) ;recv-dispatch
                       (send-dispatch (let loop
                                        ((scs send-cases) (i 0) (acc '()))
                                        (if (null? scs)
                                          (reverse acc)
                                          (let ((dispatch-expr (quasisyntax (begin (unsyntax-splicing (cadddr (car scs)))))))
                                            (loop (cdr scs)
                                              (+ i 1)
                                              (cons (quasisyntax (((unsyntax i)) (unsyntax dispatch-expr))) acc)
                                            ) ;loop
                                          ) ;let
                                        ) ;if
                                      ) ;let
                       ) ;send-dispatch
                      ) ;
                   (quasisyntax (let* ((unsyntax-splicing pre-bindings)
                                       (unsyntax-splicing (if timeout-arrow (list (list t0-sym (quasisyntax (current-jiffy)))) '())
                                       ) ;unsyntax-splicing
                                      ) ;
                                  (let (((unsyntax result-sym)
                                         (g_select (list (unsyntax-splicing (map cadr recv-cases)))
                                           (list (unsyntax-splicing (map (lambda (c) (quasisyntax (cons (unsyntax (cadr c)) (unsyntax (caddr c)))))
                                                                      send-cases
                                                                    ) ;map
                                                 ) ;unsyntax-splicing
                                           ) ;list
                                           (unsyntax (cond (else-branch 0)
                                                           ((or timeout-branch timeout-arrow) timeout-ms)
                                                           (else -1)
                                                     ) ;cond
                                           ) ;unsyntax
                                         ) ;g_select
                                        ) ;
                                       ) ;
                                    (if (not (unsyntax result-sym))
                                      (unsyntax (cond (else-branch (quasisyntax (begin (unsyntax-splicing else-branch))))
                                                      (timeout-arrow (quasisyntax (let ((elapsed-ms (inexact->exact (round (* 1000.0 (/ (- (current-jiffy) (unsyntax t0-sym)) (jiffies-per-second)))
                                                                                                                    ) ;round
                                                                                                    ) ;inexact->exact
                                                                                        ) ;elapsed-ms
                                                                                       ) ;
                                                                                    ((unsyntax timeout-arrow) elapsed-ms)
                                                                                  ) ;let
                                                                     ) ;quasisyntax
                                                      ) ;timeout-arrow
                                                      (timeout-branch (quasisyntax (begin (unsyntax-splicing timeout-branch))))
                                                      (else (quasisyntax (begin)))
                                                ) ;cond
                                      ) ;unsyntax
                                      (case (vector-ref (unsyntax result-sym) 0)
                                            (unsyntax-splicing (if (pair? recv-cases)
                                                                 (list (quasisyntax ((0)
                                                                                     (case (vector-ref (unsyntax result-sym) 1)
                                                                                           (unsyntax-splicing recv-dispatch)
                                                                                     ) ;case
                                                                                    ) ;
                                                                       ) ;quasisyntax
                                                                 ) ;list
                                                                 '()
                                                               ) ;if
                                            ) ;unsyntax-splicing
                                            (unsyntax-splicing (if (pair? send-cases)
                                                                 (list (quasisyntax ((1)
                                                                                     (case (vector-ref (unsyntax result-sym) 1)
                                                                                           (unsyntax-splicing send-dispatch)
                                                                                     ) ;case
                                                                                    ) ;
                                                                       ) ;quasisyntax
                                                                 ) ;list
                                                                 '()
                                                               ) ;if
                                            ) ;unsyntax-splicing
                                            (else (error 'fatal-error "select: unreachable"))
                                      ) ;case
                                    ) ;if
                                  ) ;let
                                ) ;let*
                   ) ;quasisyntax
                 ) ;let
               ) ;let
             ) ;let
           ) ;let
          ) ;
        ) ;syntax-case
      ) ;lambda
    ) ;define-syntax
  ) ;begin
) ;define-library
