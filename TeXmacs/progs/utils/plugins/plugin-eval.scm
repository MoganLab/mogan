
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : plugin-eval.scm
;; DESCRIPTION : Evaluation via plugins
;; COPYRIGHT   : (C) 1999-2009  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils plugins plugin-eval)
  (:use (utils library tree) (utils library cursor))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; evaluation + simplification of document fragments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (plugin-output-std-simplify name t)
  ;; (display* "Simplify " t "\n")
  (cond ((or (func? t 'document 0) (func? 'concat 0)) "")
        ((or (func? t 'document 1) (func? t 'concat 1))
         (plugin-output-simplify name (cadr t))
        ) ;
        ((and (or (func? t 'document) (func? t 'concat)) (in? (cadr t) '(""
                                                                         " "
                                                                         "  ")))
         (plugin-output-simplify name (cons (car t) (cddr t)))
        ) ;
        ((and (or (func? t 'document) (func? t 'concat)) (in? (cAr t) '(""
                                                                        " "
                                                                        "  ")))
         (plugin-output-simplify name (cDr t))
        ) ;
        ((match? t '(with "mode" "math" :%1))
         `(math ,(plugin-output-simplify name (cAr t)))
        ) ;
        ((func? t 'with) (rcons (cDr t) (plugin-output-simplify name (cAr t))))
        (else t)
  ) ;cond
) ;tm-define

(tm-define (plugin-output-simplify name t)
  ;; (display* "Simplify " t "\n")
  (plugin-output-std-simplify name t)
) ;tm-define

(tm-define (plugin-preprocess name ses t opts)
  ;; (display* "Preprocess " t ", " opts "\n")
  (if (null? opts)
    t
    (begin
      (if (and (== (car opts) :math-input) (plugin-supports-math-input-ref name))
        (set! t (plugin-math-input (list 'tuple name t)))
      ) ;if
      (plugin-preprocess name ses t (cdr opts))
    ) ;begin
  ) ;if
) ;tm-define

(tm-define (plugin-postprocess name ses r opts)
  ;; (display* "Postprocess " r ", " opts "\n")
  (if (null? opts)
    r
    (begin
      (if (== (car opts) :simplify-output) (set! r (plugin-output-simplify name r)))
      (plugin-postprocess name ses r (cdr opts))
    ) ;begin
  ) ;if
) ;tm-define

(tm-define (plugin-eval name ses t . opts)
  (with u
    (plugin-preprocess name ses t opts)
    ;; (display* "u= " u "\n")
    (with r
      (tree->stree (connection-eval name ses u))
      ;; (display* "r= " r "\n")
      (plugin-postprocess name ses r (cons :simplify-output opts))
    ) ;with
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; New connection management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define plugin-pending (make-ahash-table))

(define plugin-started (make-ahash-table))

(define plugin-prompts (make-ahash-table))

(define plugin-author (make-ahash-table))

;; pending-set
;; 设置指定插件会话的待处理任务队列。
;;
;; 语法
;; ----
;; (pending-set lan ses l)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; l : list
;; 新的待处理任务列表。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 注意
;; ----
;; 此函数直接操作内部 plugin-pending 哈希表，覆盖指定会话的原有队列。
;; 通常由 plugin-feed、plugin-next、plugin-cancel 等函数调用。

(define (pending-set lan ses l)
  (ahash-set! plugin-pending (list lan ses) l)
) ;define

;; pending-ref
;; 获取指定插件会话的待处理任务队列。
;;
;; 语法
;; ----
;; (pending-ref lan ses)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; 返回值
;; ----
;; list
;; 当前会话的待处理任务列表；若该会话无待处理任务，返回空列表 '()。
;;
;; 注意
;; ----
;; 此函数不会修改内部状态，返回的列表是 plugin-pending 哈希表中
;; 对应键的值，或通过 or 表达式回退的空列表。
(tm-define (pending-ref lan ses)
  (or (ahash-ref plugin-pending (list lan ses)) '())
) ;tm-define

(define (plugin-status lan ses)
  (if (!= lan "scheme") (connection-status lan ses) 2)
) ;define

(define (plugin-set-author lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (ahash-set! plugin-author (list lan ses) (fifth (caar l)))
    ) ;when
  ) ;with
) ;define

(define (plugin-start lan ses)
  (when (!= lan "scheme")
    (plugin-set-author lan ses)
    (connection-start lan ses)
  ) ;when
) ;define

(tm-define (plugin-write lan ses t mode)
  (ahash-set! plugin-started (list lan ses) (texmacs-time))
  (if (!= lan "scheme")
    (if (tm-func? t 'command 1)
      (connection-write-string lan ses (cadr t))
      (begin
        (plugin-set-author lan ses)
        (connection-write lan ses (stree->tree t))
      ) ;begin
    ) ;if
    (delayed (connection-notify-status lan ses 3)
      (with r
        (scheme-eval t mode)
        (if (not (func? r 'document)) (set! r (tree 'document r)))
        (connection-notify lan ses "output" r)
      ) ;with
      (connection-notify-status lan ses 2)
    ) ;delayed
  ) ;if
) ;tm-define

(define (plugin-do lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with status
        (plugin-status lan ses)
        (cond ((and (> (length (car l)) 2) (== (second (car l)) :start))
               (if (== status 0) (plugin-start lan ses) (plugin-next lan ses))
              ) ;
              ((== status 0)
               (with author
                 0
                 (when (!= lan "scheme")
                   (set! author (new-author))
                   (start-slave author)
                 ) ;when
                 (with p
                   (silent-encode :start noop '())
                   (set! p (cons (rcons (car p) author) (cdr p)))
                   (pending-set lan ses (cons p l))
                   (plugin-do lan ses)
                 ) ;with
               ) ;with
              ) ;
              (#t ((first (caar l)) lan ses))
        ) ;cond
      ) ;with
    ) ;when
  ) ;with
) ;define

;; plugin-next
;; 当前任务完成时调用其 next 回调，并从待处理队列中移除该任务，
;; 然后继续执行队列中的下一个任务。
;;
;; 语法
;; ----
;; (plugin-next lan ses)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 的待处理队列 l。
;; 2. 若队列非空：
;;    a. 调用当前任务回调列表中的 next 回调（第三个元素）。
;;    b. 将队列首元素移除。
;;    c. 调用 plugin-do 继续处理下一个任务。
;;
;; 注意
;; ----
;; 此函数通常在插件连接层报告任务完成时被调用，用于推进 pending 队列。
;; 若队列为空，则不做任何操作。
(tm-define (plugin-next lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      ((third (caar l)) lan ses)
      (pending-set lan ses (cdr l))
      (plugin-do lan ses)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (plugin-cancel lan ses dead?)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      ((fourth (caar l)) lan ses dead?)
      (pending-set lan ses (cdr l))
      (plugin-cancel lan ses dead?)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (plugin-prompt lan ses)
  (with p
    (ahash-ref plugin-prompts (list lan ses))
    (if p (tree-copy p) (string-append (upcase-first lan) "] "))
  ) ;with
) ;tm-define

(tm-define (plugin-timing lan ses)
  (with t (ahash-ref plugin-started (list lan ses)) (if t (- (texmacs-time) t) 0))
) ;tm-define

;; plugin-feed
;; 向指定插件会话的待处理队列中追加一个新任务，若队列为空则立即开始执行。
;;
;; 语法
;; ----
;; (plugin-feed lan ses do notify next cancel args)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称（如 "scheme"、"llm" 等）。
;;
;; ses : string
;; 会话标识字符串。
;;
;; do : procedure
;; 任务开始时的执行回调，签名 (do lan ses)。
;;
;; notify : procedure
;; 接收插件输出通知的回调，签名 (notify lan ses ch t)。
;; ch 为通道名（"output"、"error"、"prompt"、"input"），t 为数据树。
;;
;; next : procedure
;; 当前任务完成后的回调，签名 (next lan ses)。
;;
;; cancel : procedure
;; 任务被取消时的回调，签名 (cancel lan ses dead?)。
;; dead? 为 #t 表示插件进程已死亡，#f 表示被中断。
;;
;; args : list
;; 随任务携带的附加参数列表，通常包含输入数据及相关元信息。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 对应的待处理队列 l。
;; 2. 初始化 author 为 0；若 lan 不是 "scheme"，则创建新 author 并启动 slave。
;; 3. 将 do、notify、next、cancel、author 打包为回调列表 cb。
;; 4. 将 (cons cb args) 追加到待处理队列。
;; 5. 若原队列 l 为空，立即调用 plugin-do 开始执行。
;;
;; 注意
;; ----
;; 此函数是插件异步执行模型的核心入口，所有向插件提交计算需求的操作
;; 最终都通过它进入 pending 队列。回调函数的生命周期由 plugin-do、
;; plugin-next、plugin-cancel 管理。
(tm-define (plugin-feed lan ses do notify next cancel args)
  (with l
    (pending-ref lan ses)
    (with author
      0
      (when (!= lan "scheme")
        (set! author (new-author))
        (start-slave author)
      ) ;when
      (with cb
        (list do notify next cancel author)
        (pending-set lan ses (rcons l (cons cb args)))
        (if (null? l) (plugin-do lan ses))
      ) ;with
    ) ;with
  ) ;with
) ;tm-define

(tm-define (plugin-interrupt)
  (let* ((lan (get-env "prog-language")) (ses (get-env "prog-session")))
    (if (== (connection-status lan ses) 3) (connection-interrupt lan ses))
    (plugin-cancel lan ses #f)
  ) ;let*
) ;tm-define

(tm-define (plugin-stop)
  (let* ((lan (get-env "prog-language")) (ses (get-env "prog-session")))
    (if (!= (connection-status lan ses) 0) (connection-stop lan ses))
  ) ;let*
) ;tm-define

(define-public-macro (with-author a . body)
  (with old
    (gensym)
    `(if (not ,a)
       (begin ,@body)
       (with ,old
         (get-author)
         (set-author ,a)
         (with r (begin ,@body) (commit-changes) (set-author ,old) r)))
  ) ;with
) ;define-public-macro

(tm-define (connection-notify lan ses ch t)
  ;; (display* "Notify " lan ", " ses ", " ch ", " t "\n")
  (with-author (ahash-ref plugin-author (list lan ses))
    (with l
      (pending-ref lan ses)
      (when (nnull? l)
        (if (== ch "prompt") (ahash-set! plugin-prompts (list lan ses) (tree-copy t)))
        ((second (caar l)) lan ses ch t)
      ) ;when
    ) ;with
  ) ;with-author
) ;tm-define

(tm-define (connection-notify-status lan ses st)
  ;; (display* "Notify status " lan ", " ses ", " st "\n")
  (with-author (ahash-ref plugin-author (list lan ses))
    (when (== st 0)
      (ahash-remove! plugin-started (list lan ses))
      (ahash-remove! plugin-prompts (list lan ses))
      (plugin-cancel lan ses #t)
    ) ;when
    (when (== st 2)
      (plugin-next lan ses)
    ) ;when
  ) ;with-author
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Silent evaluation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (silent-encode in return opts)
  (list (list silent-do silent-notify silent-next silent-cancel)
    (if (tm? in) (tm->stree in) in)
    (tree 'document)
    (tree 'document)
    return
    opts
  ) ;list
) ;define

(define (silent-decode l)
  (list (second l) (third l) (fourth l) (fifth l) (sixth l))
) ;define

(define (silent-do lan ses)
  (with l
    (pending-ref lan ses)
    (with (in out err return opts)
      (silent-decode (car l))
      ;; (display* "Silent do " lan ", " ses ", " in "\n")
      (if (tree-empty? in) (plugin-next lan ses) (plugin-write lan ses in :silent))
    ) ;with
  ) ;with
) ;define

(define (silent-next lan ses)
  ;; (display* "Silent next " lan ", " ses "\n")
  (with l
    (pending-ref lan ses)
    (with (in out err return opts)
      (silent-decode (car l))
      ;; (display* "Silent return " (tm->stree out) ", " (tm->stree err) "\n")
      (return (cons (tm->stree out) (tm->stree err)))
    ) ;with
  ) ;with
) ;define

(define (var-tree-children t)
  (with r (tree-children t) (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r))
) ;define

(define (silent-output t u)
  (when (and (tm-func? t 'document) (tm-func? u 'document))
    (tree-insert! t (tree-arity t) (var-tree-children u))
  ) ;when
) ;define

(define (silent-notify lan ses ch t)
  ;; (display* "Silent notify " lan ", " ses ", " ch ", " t "\n")
  (with l
    (pending-ref lan ses)
    (with (in out err return opts)
      (silent-decode (car l))
      (cond ((== ch "output") (silent-output out t))
            ((== ch "error") (silent-output err t))
      ) ;cond
    ) ;with
  ) ;with
) ;define

(define (silent-cancel lan ses dead?)
  ;; (display* "Silent cancel " lan ", " ses ", " dead? "\n")
  (with l
    (pending-ref lan ses)
    (with (in out err return opts)
      (silent-decode (car l))
      (return (if dead? :dead :interrupted))
    ) ;with
  ) ;with
) ;define

(tm-define (silent-feed lan ses in return opts)
  (set! in (plugin-preprocess lan ses in opts))
  (with ret
    (lambda (x)
      (return (if (npair? x)
                x
                (cons (plugin-postprocess lan ses (car x) opts)
                  (plugin-postprocess lan ses (cdr x) opts)
                ) ;cons
              ) ;if
      ) ;return
    ) ;lambda
    (with x
      (silent-encode in ret opts)
      (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x)))
    ) ;with
  ) ;with
) ;tm-define

(tm-define (silent-feed* lan ses in return opts)
  (define (result-wrap x)
    (cond ((== x :dead) '(script-dead))
          ((== x :interrupted) '(script-interrupted))
          ((!= (tm-arity (cdr x)) 0) `(with ,"color" ,"red" ,(cdr x)))
          (else (car x))
    ) ;cond
  ) ;define

  (define (result-callback x)
    (let ((r1 (result-wrap x)))
      (if (tree? r1) (return r1) (return (stree->tree r1)))
    ) ;let
  ) ;define

  (silent-feed lan ses in result-callback opts)
) ;tm-define

(define (plugin-command-answer x)
  (if (tm-func? x 'document 1) (plugin-command-answer (cadr x)) x)
) ;define

(tm-define (plugin-command lan ses in return opts)
  (let* ((cmd `(command ,(format-command lan in)))
         (ret (lambda (x) (and (pair? x) (return (plugin-command-answer (car x))))))
         (x (silent-encode cmd ret opts))
        ) ;
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x)))
  ) ;let*
) ;tm-define
