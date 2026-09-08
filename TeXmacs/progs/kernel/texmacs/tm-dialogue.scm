
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-dialogue.scm
;; DESCRIPTION : Interactive dialogues between Scheme and C++
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-dialogue) (:use (kernel texmacs tm-define)))
(import (liii json) (liii time) (liii list))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Questions with user interaction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (user-ask prompt cont)
  (tm-interactive cont
    (if (string? prompt) (list (build-interactive-arg prompt)) (list prompt))
  ) ;tm-interactive
) ;define-public

(define-public (user-confirm prompt default cont)
  (let ((k (lambda (answ) (cont (yes? answ)))))
    (if default
      (user-ask (list prompt "question" (translate "yes") (translate "no")) k)
      (user-ask (list prompt "question" (translate "no") (translate "yes")) k)
    ) ;if
  ) ;let
) ;define-public

(define-public (user-simple-confirm prompt default cont)
  (let ((k (lambda (answ) (cont (yes? answ)))))
    (if default
      (user-ask (list prompt "question-no-cancel" (translate "yes") (translate "no"))
        k
      ) ;user-ask
      (user-ask (list prompt "question-no-cancel" (translate "no") (translate "yes"))
        k
      ) ;user-ask
    ) ;if
  ) ;let
) ;define-public

(define-public (user-url prompt type cont)
  (user-delayed (lambda () (choose-file cont prompt type)))
) ;define-public

(define-public (user-delayed cont) (exec-delayed cont))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Delayed execution of commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (delayed-sub body)
  (cond ((or (npair? body) (nlist? (car body)) (not (keyword? (caar body))))
         `(lambda ,() ,@body ,#t)
        ) ;
        ((== (caar body) :pause)
         `(let* ((start (texmacs-time)) (proc ,(delayed-sub (cdr body))))
            (lambda ,()
              (with left
                (- (+ start ,(cadar body)) (texmacs-time))
                (if (> left 0) left (begin (set! start (texmacs-time)) (proc))))))
        ) ;
        ((== (caar body) :every)
         `(let* ((time (+ (texmacs-time) ,(cadar body)))
                 (proc ,(delayed-sub (cdr body))))
            (lambda ,()
              (with left
                (- time (texmacs-time))
                (if (> left 0)
                  left
                  (begin (set! time (+ (texmacs-time) ,(cadar body))) (proc))))))
        ) ;
        ((== (caar body) :idle)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,()
              (with left
                (- ,(cadar body) (idle-time))
                (if (> left 0) left (proc)))))
        ) ;
        ((== (caar body) :refresh)
         (with sym
           (gensym)
           `(let* ((,sym ,#f) (proc ,(delayed-sub (cdr body))))
              (lambda ,()
                (if (!= ,sym (change-time))
                  ,0
                  (with left
                    (- ,(cadar body) (idle-time))
                    (if (> left 0)
                      left
                      (begin (set! ,sym (change-time)) (proc)))))))
         ) ;with
        ) ;
        ((== (caar body) :require)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,() (if (not ,(cadar body)) ,0 (proc))))
        ) ;
        ((== (caar body) :while)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,()
              (if (not ,(cadar body))
                ,#t
                (with left (proc) (if (== left #t) 0 left)))))
        ) ;
        ((== (caar body) :clean)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,()
              (with left
                (proc)
                (if (!= left #t) left (begin ,(cadar body) ,#t)))))
        ) ;
        ((== (caar body) :permanent)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,()
              (with left
                (proc)
                (if (!= left #t)
                  left
                  (with next ,(cadar body) (if (!= next #t) #t 0))))))
        ) ;
        ((== (caar body) :do)
         `(with proc
            ,(delayed-sub (cdr body))
            (lambda ,() ,(cadar body) (proc)))
        ) ;
        (else (delayed-sub (cdr body)))
  ) ;cond
) ;define-public

(define-public-macro (delayed . body) `(exec-delayed-pause ,(delayed-sub body)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Messages and feedback on the status bar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public message-serial 0)

(define-public (set-message-notify) (set! message-serial (+ message-serial 1)))

(define-public (recall-message-after len)
  (with current
    message-serial
    (delayed (:idle len) (when (== message-serial current) (recall-message)))
  ) ;with
) ;define-public

(define-public (set-temporary-message left right len)
  (set-message-temp left right #t)
  (recall-message-after len)
) ;define-public

(define-public (texmacs-banner)
  (with tmv
    (string-append "GNU TeXmacs " (texmacs-version))
    (delayed (set-message "Welcome to GNU TeXmacs" tmv)
      (delayed (:pause 5000)
        (set-message "GNU TeXmacs falls under the GNU general public license" tmv)
        (delayed (:pause 2500)
          (set-message "GNU TeXmacs comes without any form of legal warranty" tmv)
          (delayed (:pause 2500)
            (set-message "More information about GNU TeXmacs can be found in the Help->About menu"
              tmv
            ) ;set-message
            (delayed (:pause 2500) (set-message "" ""))
          ) ;delayed
        ) ;delayed
      ) ;delayed
    ) ;delayed
  ) ;with
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interactive commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define interactive-arg-version 1)

(define-public interactive-arg-file
  "$TEXMACS_HOME_PATH/system/interactive.json"
) ;define-public

(define-public interactive-arg-recent-file-path
  "$TEXMACS_HOME_PATH/system/recent-files.json"
) ;define-public

(define legacy-interactive-arg-file "$TEXMACS_HOME_PATH/system/interactive.scm")

(define interactive-arg-migration-marker-v1
  "$TEXMACS_HOME_PATH/system/interactive.scm->v1"
) ;define

(define recent-files-migration-marker-v1
  "$TEXMACS_HOME_PATH/system/recent-files.scm->v1"
) ;define

(define-public (make-empty-state kind)
  (case kind
   ((interactive-arg)
    `((,"meta" (,"version" . ,interactive-arg-version)) ("commands" ()))
   ) ;
   ((recent-file) '(("meta" ("version" . 1) ("total" . 0)) ("files" . #())))
   (else '(()))
  ) ;case
) ;define-public

(define interactive-arg-json (make-empty-state 'interactive-arg))

(define interactive-arg-recent-file-json (make-empty-state 'recent-file))

(define (interactive-arg-item-valid? item)
  (and (json-object? item)
    (let ((keys (json-keys item)))
      (and (every string? keys) (every (lambda (k) (string? (json-ref item k))) keys))
    ) ;let
  ) ;and
) ;define

(define-public (interactive-args-json-valid? interactive-args)
  (and (json-object? interactive-args)
    (let* ((meta (json-ref interactive-args "meta"))
           (commands (json-ref interactive-args "commands"))
           (version (and (json-object? meta) (json-ref meta "version")))
          ) ;
      (and (json-object? meta)
        (integer? version)
        (>= version 1)
        (json-object? commands)
        (every string? (json-keys commands))
        (every (lambda (cmd)
                 (let ((items (json-ref commands cmd)))
                   (and (vector? items) (every interactive-arg-item-valid? (vector->list items)))
                 ) ;let
               ) ;lambda
          (json-keys commands)
        ) ;every
      ) ;and
    ) ;let*
  ) ;and
) ;define-public

(define (interactive-command-learned command-name)
  (let* ((commands (json-ref interactive-arg-json "commands"))
         (items (and (json-object? commands) (json-ref commands command-name)))
        ) ;
    (if (vector? items) (vector->list items) '())
  ) ;let*
) ;define

(define (set-interactive-command-learned command-name items)
  (let* ((commands (json-ref interactive-arg-json "commands"))
         (commands (if (json-object? commands) commands '(())))
         (payload (list->vector items))
         (commands* (if (json-contains-key? commands command-name)
                      (json-set commands command-name payload)
                      (json-push commands command-name payload)
                    ) ;if
         ) ;commands*
        ) ;
    (set! interactive-arg-json (json-set interactive-arg-json "commands" commands*))
  ) ;let*
) ;define

(define (remove-interactive-command-learned command-name)
  (let* ((commands (json-ref interactive-arg-json "commands"))
         (commands (if (or (json-object? commands) (null? commands)) commands '(())))
         (commands* (if (json-contains-key? commands command-name)
                      (let ((res (json-drop commands command-name)))
                        (if (null? res) '(()) res)
                      ) ;let
                      commands
                    ) ;if
         ) ;commands*
        ) ;
    (set! interactive-arg-json (json-set interactive-arg-json "commands" commands*))
  ) ;let*
) ;define

(define (recent-file-item-valid? item)
  (and (json-object? item)
    (string? (json-ref item "path"))
    (string? (json-ref item "name"))
    (number? (json-ref item "last_open"))
    (number? (json-ref item "open_count"))
    (boolean? (json-ref item "show"))
  ) ;and
) ;define

(define-public (recent-files-json-valid? recent-files)
  (and (json-object? recent-files)
    (let* ((meta (json-ref recent-files "meta"))
           (files (json-ref recent-files "files"))
           (version (and (json-object? meta) (json-ref meta "version")))
           (total (and (json-object? meta) (json-ref meta "total")))
          ) ;
      (and (json-object? meta)
        (number? version)
        (integer? total)
        (>= total 0)
        (vector? files)
        (every recent-file-item-valid? (vector->list files))
      ) ;and
    ) ;let*
  ) ;and
) ;define-public

;; recent-files-remove-by-path
;; 按路径从最近文件缓存中删除对应条目。
;;
;; 语法
;; ----
;; (recent-files-remove-by-path path)
;;
;; 参数
;; ----
;; path : string
;;    目标文件路径。用于在 `interactive-arg-recent-file-json` 的 `files`
;;    列表中定位要移除的记录。
;;
;; 返回值
;; ----
;; unspecified
;; - 函数通过副作用更新全局变量 `interactive-arg-recent-file-json`。
;; - 若路径不存在，则不做任何修改。
;;
;; 逻辑
;; ----
;; 1. 调用 `recent-files-index-by-path` 查找 `path` 在 `files` 中的索引。
;; 2. 若找到索引，调用 `json-drop` 删除该项。
;; 3. 将 `meta.total` 减一（不低于 0）。
;; 4. 将更新后的 JSON 结构回写到 `interactive-arg-recent-file-json`。
(define-public (recent-files-remove-by-path path)
  (let ((idx (recent-files-index-by-path interactive-arg-recent-file-json path)))
    (when idx
      (let* ((total (json-ref interactive-arg-recent-file-json "meta" "total"))
             (total (if (number? total) total 0))
             (new-total (if (<= total 0) 0 (- total 1)))
             (r1 (json-drop interactive-arg-recent-file-json "files" idx))
            ) ;
        (set! interactive-arg-recent-file-json (json-set r1 "meta" "total" new-total))
      ) ;let*
    ) ;when
  ) ;let
) ;define-public



(define (recent-files-apply-lru recent-files limit)
  (let* ((files (json-ref recent-files "files"))
         (n (if (vector? files) (vector-length files) 0))
         (indexed (let loop
                    ((i 0) (acc '()))
                    (if (>= i n)
                      acc
                      (let* ((item (vector-ref files i))
                             (t (json-ref item "last_open"))
                             (t (if (number? t) t 0))
                            ) ;
                        (loop (+ i 1) (cons (cons i t) acc))
                      ) ;let*
                    ) ;if
                  ) ;let
         ) ;indexed
         (sorted (sort indexed
                   (lambda (a b) (if (== (cdr a) (cdr b)) (> (car a) (car b)) (> (cdr a) (cdr b))))
                 ) ;sort
         ) ;sorted
         (new-files (list->vector (let loop
                                    ((rank 0) (rest sorted))
                                    (if (null? rest)
                                      '()
                                      (let* ((idx (caar rest)) (item (vector-ref files idx)) (show? (< rank limit)))
                                        (cons (json-set item "show" show?) (loop (+ rank 1) (cdr rest)))
                                      ) ;let*
                                    ) ;if
                                  ) ;let
                    ) ;list->vector
         ) ;new-files
        ) ;
    (json-set recent-files "files" new-files)
  ) ;let*
) ;define

(define (recent-files-add recent-files path name)
  (let* ((files (json-ref recent-files "files"))
         (idx (if (vector? files) (vector-length files) 0))
         (item `((,"path" . ,path)
                 (,"name" . ,name)
                 (,"last_open" . ,(time-second (current-time)))
                 (,"open_count" . ,1)
                 (,"show" . ,#t))
         ) ;item
         (total (json-ref recent-files "meta" "total"))
         (total (if (number? total) total 0))
         (r1 (json-set (json-push recent-files "files" idx item) "meta" "total" (+ total 1))
         ) ;r1
        ) ;
    (recent-files-apply-lru r1 25)
  ) ;let*
) ;define

(define (recent-files-set recent-files idx)
  (let* ((item (json-ref recent-files "files" idx))
         (path* (json-ref item "path"))
         (name* (json-ref item "name"))
         (count* (json-ref item "open_count"))
         (count* (if (number? count*) count* 0))
         (new-item `((,"path" . ,path*)
                     (,"name" . ,name*)
                     (,"last_open" . ,(time-second (current-time)))
                     (,"open_count" . ,(+ count* 1))
                     (,"show" . ,#t))
         ) ;new-item
         (r1 (json-set recent-files "files" idx new-item))
        ) ;
    (recent-files-apply-lru r1 25)
  ) ;let*
) ;define



;; 路径归一为 url->system 规范形后再比较。Windows 下写入方分隔符不统一：
;; scheme 侧 url->system 记录 '\'，C++ 启动页 QDir::fromNativeSeparators 传入
;; '/'，裸字符串 equal? 会失配（如按路径移除失效）。经 system->url →
;; url->system 往返统一分隔符；tmfs:// 等非 default 根的 URL 往返后原样保持。
(define-public (recent-files-canonical-path p) (url->system (system->url p)))

(define (recent-files-index-by-path recent-files path)
  (let* ((key (recent-files-canonical-path path))
         (files (json-ref recent-files "files"))
        ) ;
    (if (not (vector? files))
      #f
      (let loop
        ((i 0))
        (if (>= i (vector-length files))
          #f
          (if (equal? (recent-files-canonical-path (json-ref (vector-ref files i) "path"))
                key
              ) ;equal?
            i
            (loop (+ i 1))
          ) ;if
        ) ;if
      ) ;let
    ) ;if
  ) ;let*
) ;define

(define (recent-files-paths recent-files)
  (let ((files (json-ref recent-files "files")))
    (if (not (vector? files))
      '()
      (map (lambda (item) (list (cons "0" (json-ref item "path"))))
        (vector->list files)
      ) ;map
    ) ;if
  ) ;let
) ;define


(define (list-but l1 l2)
  (cond ((null? l1) l1)
        ((in? (car l1) l2) (list-but (cdr l1) l2))
        (else (cons (car l1) (list-but (cdr l1) l2)))
  ) ;cond
) ;define

(define (as-stree x)
  (cond ((tree? x) (tree->stree x))
        ((== x #f) "false")
        ((== x #t) "true")
        (else x)
  ) ;cond
) ;define

(define (interactive-key->string x)
  (cond ((string? x) x)
        ((symbol? x) (symbol->string x))
        ((number? x) (number->string x))
        (else (object->string x))
  ) ;cond
) ;define

(define (interactive-value->string x)
  (with y
    (as-stree x)
    (cond ((string? y) y)
          ((symbol? y) (symbol->string y))
          ((number? y) (number->string y))
          ((boolean? y) (if y "true" "false"))
          (else (object->string y))
    ) ;cond
  ) ;with
) ;define

(define (normalize-interactive-assoc assoc-t)
  (map (lambda (x)
         (cons (interactive-key->string (car x)) (interactive-value->string (cdr x)))
       ) ;lambda
    assoc-t
  ) ;map
) ;define

(define-public (procedure-symbol-name fun)
  (cond ((symbol? fun) fun)
        ((string? fun) (string->symbol fun))
        ((and (procedure? fun) (procedure-name fun)) => identity)
        (else #f)
  ) ;cond
) ;define-public

(define-public (procedure-string-name fun)
  (and-with name (procedure-symbol-name fun) (symbol->string name))
) ;define-public

;; 增加/刷新一条最近文件（显式 name）：命中则 touch（last_open/open_count），
;; 否则新增并跑 LRU。本地 buffer（name 由 url-tail 推导）与云文档（name 为文档
;; 显示名）共用——云文档 path 形如 tmfs://collab/<doc_id>，由 collab-record-recent
;; 写入。原 recent-buffer-json 的 name 推导逻辑保留在此处委托。
(define-public (recent-files-learn file-path name)
  (let ((idx (recent-files-index-by-path interactive-arg-recent-file-json file-path)))
    (if idx
      (set! interactive-arg-recent-file-json
        (recent-files-set interactive-arg-recent-file-json idx)
      ) ;set!
      (set! interactive-arg-recent-file-json
        (recent-files-add interactive-arg-recent-file-json file-path name)
      ) ;set!
    ) ;if
  ) ;let
) ;define-public

;; 按 path 反查已存 name（菜单渲染云文档标题用，云 URL 的 url-tail 是 UUID 非标题）；
;; 未命中返回 #f，调用方自行回退。
(define-public (recent-files-get-name file-path)
  (let ((idx (recent-files-index-by-path interactive-arg-recent-file-json file-path)))
    (and idx (json-ref interactive-arg-recent-file-json "files" idx "name"))
  ) ;let
) ;define-public

(define (recent-buffer-json file-path)
  (recent-files-learn file-path (url->system (url-tail (system->url file-path))))
) ;define


(define-public (learn-interactive fun assoc-t)
  "Learn interactive values for @fun"
  (set! assoc-t (normalize-interactive-assoc assoc-t))
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (let* ((name (symbol->string fun))
           (l1 (interactive-command-learned name))
           (l2 (cons assoc-t (list-but l1 (list assoc-t))))
          ) ;
      (case fun
       ((recent-buffer) (recent-buffer-json (cdr (car (car l2)))))
       (else (set-interactive-command-learned name l2))
      ) ;case
    ) ;let*
  ) ;when
) ;define-public


;; learned-interactive
;; 读取交互命令已学习的参数候选值。
;;
;; 语法
;; ----
;; (learned-interactive fun)
;;
;; 参数
;; ----
;; fun : procedure | symbol | string
;;    目标命令。函数内部会先调用 `procedure-symbol-name` 归一化为符号。
;;
;; 返回值
;; ----
;; list
;; - 当命令是 `recent-buffer` 时：返回最近文件路径列表，元素形如
;;  `(("0" . 文件路径))`。
;; - 其他命令：返回 `interactive-arg-json` 中为该命令记录的历史参数列表。
;; - 若无记录，返回空列表 `()`。
;;
;; 逻辑
;; ----
;; 1. 归一化：将 `fun` 转为符号名。
;; 2. 分支：`recent-buffer` 走最近文件 JSON 缓存分支。
;; 3. 默认：从 `interactive-arg-json` 读取命令历史，缺省为 `()`。
(define-public (learned-interactive fun)
  "Return learned list of interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (case fun
   ((recent-buffer) (recent-files-paths interactive-arg-recent-file-json))
   (else (with name
           (procedure-string-name fun)
           (if (string? name) (interactive-command-learned name) '())
         ) ;with
   ) ;else
  ) ;case
) ;define-public




;; forget-interactive
;; 清除指定交互命令的已学习参数。
;;
;; 语法
;; ----
;; (forget-interactive fun)
;;
;; 参数
;; ----
;; fun : procedure | symbol | string
;;    目标命令。函数内部会先调用 `procedure-symbol-name` 归一化为符号。
;;
;; 返回值
;; ----
;; unspecified
;; - 通过副作用修改全局状态。
;; - 若 `fun` 不能归一化为符号，则不执行清除操作。
;;
;; 逻辑
;; ----
;; 1. 归一化：将 `fun` 转为符号名。
;; 2. 校验：仅当 `fun` 是符号时继续。
;; 3. 分支清理：
;;   - `recent-buffer`：将最近文件列表重置为空向量 `#()`，并把计数清零。
;;   - 其他命令：从 `interactive-arg-json` 中删除对应键。
(define-public (forget-interactive fun)
  "Forget interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (case fun
     ((recent-buffer)
      (set! interactive-arg-recent-file-json (make-empty-state 'recent-file))
     ) ;
     (else (with name
             (procedure-string-name fun)
             (when (string? name)
               (remove-interactive-command-learned name)
             ) ;when
           ) ;with
     ) ;else
    ) ;case
  ) ;when
) ;define-public


(define (learned-interactive-arg fun nr)
  (let* ((l (learned-interactive fun))
         (arg (number->string nr))
         (extract (lambda (assoc-l) (assoc-ref assoc-l arg)))
        ) ;
    (map extract l)
  ) ;let*
) ;define

(define (compute-interactive-arg-text fun which)
  (with arg
    (property fun (list :argument which))
    (cond ((npair? arg) (upcase-first (symbol->string which)))
          ((and (string? (car arg)) (null? (cdr arg))) (car arg))
          ((string? (cadr arg)) (cadr arg))
          (else (upcase-first (symbol->string which)))
    ) ;cond
  ) ;with
) ;define

(define (compute-interactive-arg-type fun which)
  (with arg
    (property fun (list :argument which))
    (cond ((or (npair? arg) (npair? (cdr arg))) "string")
          ((string? (car arg)) (car arg))
          ((symbol? (car arg)) (symbol->string (car arg)))
          (else "string")
    ) ;cond
  ) ;with
) ;define

(define (compute-interactive-arg-proposals fun which)
  (let* ((default (property fun (list :default which)))
         (proposals (property fun (list :proposals which)))
         (learned '())
        ) ;
    (cond ((procedure? default) (list (default)))
          ((procedure? proposals) (proposals))
          (else '())
    ) ;cond
  ) ;let*
) ;define

(define (compute-interactive-arg fun which)
  (cons (compute-interactive-arg-text fun which)
    (cons (compute-interactive-arg-type fun which)
      (compute-interactive-arg-proposals fun which)
    ) ;cons
  ) ;cons
) ;define

(define (compute-interactive-args-try-hard fun)
  (with src
    (procedure-source fun)
    (if (and (pair? src) (== (car src) 'lambda) (pair? (cdr src)) (list? (cadr src)))
      (map upcase-first (map symbol->string (cadr src)))
      '()
    ) ;if
  ) ;with
) ;define

(define (compute-interactive-arg-list fun l)
  (if (npair? l)
    (list)
    (cons (compute-interactive-arg fun (car l))
      (compute-interactive-arg-list fun (cdr l))
    ) ;cons
  ) ;if
) ;define

(tm-define (compute-interactive-args fun)
  (let* ((args (property fun :arguments)) (syn* (property fun :synopsis*)))
    (cond ((not args) (compute-interactive-args-try-hard fun))
          ((and (not (side-tools?)) (list-1? syn*) (string? (car syn*)))
           (let* ((type (compute-interactive-arg-type fun (car args)))
                  (prop (compute-interactive-arg-proposals fun (car args)))
                  (tail (compute-interactive-arg-list fun (cdr args)))
                 ) ;
             (cons (cons (car syn*) (cons type prop)) tail)
           ) ;let*
          ) ;
          (else (compute-interactive-arg-list fun args))
    ) ;cond
  ) ;let*
) ;tm-define

(define (build-interactive-arg s)
  (cond ((string-ends? s ":") s)
        ((string-ends? s "?") s)
        (else (string-append s ":"))
  ) ;cond
) ;define

(tm-define (build-interactive-args fun l nr learned?)
  (cond ((null? l) l)
        ((string? (car l))
         (build-interactive-args fun (cons (list (car l) "string") (cdr l)) nr learned?)
        ) ;
        (else (let* ((name (build-interactive-arg (caar l)))
                     (type (cadar l))
                     (pl (cddar l))
                     (ql pl)
                     ;; (ql (if (null? pl) '("") pl))
                     (ll (if learned? (learned-interactive-arg fun nr) '()))
                     (rl (append ql (list-but ll ql)))
                     (props (if (<= (length ql) 1) rl ql))
                    ) ;
                (cons (cons name (cons type props))
                  (build-interactive-args fun (cdr l) (+ nr 1) learned?)
                ) ;cons
              ) ;let*
        ) ;else
  ) ;cond
) ;tm-define

(tm-define (tm-interactive-new fun args)
  ;; (display* "interactive " fun ", " args "\n")
  (if (side-tools?)
    (begin
      (tool-select :transient-bottom (list 'interactive-tool fun args))
      (delayed (:pause 500) (keyboard-focus-on "interactive-0"))
    ) ;begin
    (tm-interactive fun args)
  ) ;if
) ;tm-define

(tm-define (interactive fun . args)
  (:synopsis "Call @fun with interactively specified arguments @args")
  (:interactive #t)
  (lazy-define-force fun)
  (if (null? args) (set! args (compute-interactive-args fun)))
  (with fun-args
    (build-interactive-args fun args 0 #t)
    (tm-interactive-new fun fun-args)
  ) ;with
) ;tm-define

(tm-define (interactive-title fun)
  (let* ((val (property fun :synopsis))
         (name (procedure-name fun))
         (name* (and name (symbol->string name)))
        ) ;
    (or (and (list-1? val) (string? (car val)) (car val))
      (and name (string-append "Interactive command '" name* "'"))
      "Interactive command"
    ) ;or
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Store learned arguments from one session to another
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (save-learned)
  (string-save (json->string interactive-arg-json)
    (string->url interactive-arg-file)
  ) ;string-save
  (string-save (json->string interactive-arg-recent-file-json)
    (string->url interactive-arg-recent-file-path)
  ) ;string-save
) ;define

;; 立即把最近文件状态落盘（仅 recent-files.json，C++ 启动页直接读该文件取 name）。
;; recent-files-learn 只更新内存态、默认 on-exit 才 flush；云文档记录后若不立即落盘，
;; 同会话内启动页读到的 name 会回退为 UUID（doc_id）。join/create 频次低，每次写一次
;; 小 JSON 可接受，且提升崩溃后的最近列表持久性。
(define-public (recent-files-save)
  (string-save (json->string interactive-arg-recent-file-json)
    (string->url interactive-arg-recent-file-path)
  ) ;string-save
) ;define-public

(define-public (load-json-with-fallback file valid? fallback-maker)
  (if (url-exists? file)
    (catch #t
      (lambda ()
        (let ((parsed (string->json (string-load (string->url file)))))
          (if (valid? parsed) parsed (fallback-maker))
        ) ;let
      ) ;lambda
      (lambda args (fallback-maker))
    ) ;catch
    (fallback-maker)
  ) ;if
) ;define-public

(define (write-migration-marker marker-file)
  (string-save "migrated\n" (string->url marker-file))
) ;define

(define (legacy-scm-interactive-assoc-valid? assoc-t)
  (and (list? assoc-t) (list-and (map pair? assoc-t)))
) ;define

(define (normalize-legacy-scm-interactive-items items)
  (list-filter (map (lambda (assoc-t)
                      (and (legacy-scm-interactive-assoc-valid? assoc-t)
                        (normalize-interactive-assoc assoc-t)
                      ) ;and
                    ) ;lambda
                 items
               ) ;map
    (lambda (x) x)
  ) ;list-filter
) ;define

(define (interactive-item-exists? item items)
  (list-and (map (lambda (existing) (not (equal? existing item))) items))
) ;define

(define (merge-legacy-scm-interactive-items existing-items legacy-items)
  (let loop
    ((legacy legacy-items) (merged existing-items))
    (if (null? legacy)
      merged
      (with item
        (car legacy)
        (if (interactive-item-exists? item merged)
          (loop (cdr legacy) (append merged (list item)))
          (loop (cdr legacy) merged)
        ) ;if
      ) ;with
    ) ;if
  ) ;let
) ;define

(define (legacy-scm-interactive-command-name key)
  (and-with sym (procedure-symbol-name key) (symbol->string sym))
) ;define

(define (legacy-scm-recent-buffer-key? key)
  (== (procedure-symbol-name key) 'recent-buffer)
) ;define

(define (legacy-scm-ahash-set-2! t x)
  (with (key . l)
    x
    (with (form arg)
      key
      (with a
        (or (ahash-ref t form) '())
        (set! a (assoc-set! a arg l))
        (ahash-set! t form a)
      ) ;with
    ) ;with
  ) ;with
) ;define

(define (legacy-scm-rearrange-old-interactive x)
  (with (form . l)
    x
    (let ((lengths (map length l)))
      (if (or (null? lengths) (<= (apply min lengths) 0))
        (cons form '())
        (let* ((len (apply min lengths))
               (truncl (map (cut sublist <> 0 len) l))
               (sl (sort truncl (lambda (l1 l2) (< (car l1) (car l2)))))
               (nl (map (lambda (y) (cons (number->string (car y)) (cdr y))) sl))
               (build (lambda args (map cons (map car nl) args)))
               (r (apply map (cons build (map cdr nl))))
              ) ;
          (cons form r)
        ) ;let*
      ) ;if
    ) ;let
  ) ;with
) ;define

(define (decode-legacy-scm-interactive-old l)
  (let* ((t (make-ahash-table)) (setter (cut legacy-scm-ahash-set-2! t <>)))
    (for-each setter l)
    (let* ((r (ahash-table->list t)) (m (map legacy-scm-rearrange-old-interactive r)))
      (list->ahash-table m)
    ) ;let*
  ) ;let*
) ;define

(define (load-legacy-scm-interactive-table)
  (and (url-exists? legacy-interactive-arg-file)
    (catch #t
      (lambda ()
        (let* ((loaded (load-object legacy-interactive-arg-file))
               (old? (and (pair? loaded) (pair? (car loaded)) (list-2? (caar loaded))))
               (decode (if old? decode-legacy-scm-interactive-old list->ahash-table))
              ) ;
          (and (list? loaded) (decode loaded))
        ) ;let*
      ) ;lambda
      (lambda args #f)
    ) ;catch
  ) ;and
) ;define

(define (import-legacy-scm-interactive-commands! legacy-table)
  (for-each (lambda (entry)
              (with (key . items)
                entry
                (when (and (not (legacy-scm-recent-buffer-key? key)) (list? items))
                  (and-with name
                    (legacy-scm-interactive-command-name key)
                    (let* ((existing (interactive-command-learned name))
                           (normalized (normalize-legacy-scm-interactive-items items))
                           (merged (merge-legacy-scm-interactive-items existing normalized))
                          ) ;
                      (when (not (equal? merged existing))
                        (set-interactive-command-learned name merged)
                      ) ;when
                    ) ;let*
                  ) ;and-with
                ) ;when
              ) ;with
            ) ;lambda
    (ahash-table->list legacy-table)
  ) ;for-each
) ;define

(define (legacy-scm-recent-path assoc-t)
  (or (assoc-ref assoc-t "0") (assoc-ref assoc-t 0))
) ;define

(define (recent-files-min-last-open recent-files)
  (let ((files (json-ref recent-files "files")))
    (if (or (not (vector? files)) (<= (vector-length files) 0))
      #f
      (let loop
        ((i 1)
         (min-t (let ((t (json-ref (vector-ref files 0) "last_open")))
                  (if (number? t) t 0)
                ) ;let
         ) ;min-t
        ) ;
        (if (>= i (vector-length files))
          min-t
          (let* ((t (json-ref (vector-ref files i) "last_open")) (t (if (number? t) t 0)))
            (loop (+ i 1) (min min-t t))
          ) ;let*
        ) ;if
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (append-recent-file-entry recent-files path last-open)
  (let* ((name (url->system (url-tail (system->url path))))
         (item `((,"path" . ,path)
                 (,"name" . ,name)
                 (,"last_open" . ,last-open)
                 (,"open_count" . ,1)
                 (,"show" . ,#t))
         ) ;item
         (files (json-ref recent-files "files"))
         (idx (if (vector? files) (vector-length files) 0))
         (total (json-ref recent-files "meta" "total"))
         (total (if (number? total) total 0))
        ) ;
    (json-set (json-push recent-files "files" idx item) "meta" "total" (+ total 1))
  ) ;let*
) ;define

(define (import-legacy-scm-recent-files! legacy-items)
  (let* ((min-open (recent-files-min-last-open interactive-arg-recent-file-json))
         (base (if (number? min-open) (- min-open 1) (time-second (current-time))))
        ) ;
    (let loop
      ((items legacy-items) (rank 0) (seen '()))
      (if (null? items)
        (set! interactive-arg-recent-file-json
          (recent-files-apply-lru interactive-arg-recent-file-json 25)
        ) ;set!
        (let* ((assoc-t (car items))
               (path (and (legacy-scm-interactive-assoc-valid? assoc-t)
                       (legacy-scm-recent-path assoc-t)
                     ) ;and
               ) ;path
              ) ;
          (if (or (not (string? path))
                (== path "")
                (in? path seen)
                (recent-files-index-by-path interactive-arg-recent-file-json path)
              ) ;or
            (loop (cdr items) (+ rank 1) seen)
            (begin
              (set! interactive-arg-recent-file-json
                (append-recent-file-entry interactive-arg-recent-file-json path (- base rank))
              ) ;set!
              (loop (cdr items) (+ rank 1) (cons path seen))
            ) ;begin
          ) ;if
        ) ;let*
      ) ;if
    ) ;let
  ) ;let*
) ;define

(define (maybe-import-legacy-scm-interactive-state)
  (let ((need-interactive? (not (url-exists? interactive-arg-migration-marker-v1)))
        (need-recent? (not (url-exists? recent-files-migration-marker-v1)))
       ) ;
    (when (and (or need-interactive? need-recent?)
            (url-exists? legacy-interactive-arg-file)
          ) ;and
      (and-with legacy-table
        (load-legacy-scm-interactive-table)
        (when need-interactive?
          (import-legacy-scm-interactive-commands! legacy-table)
        ) ;when
        (when need-recent?
          (with recent-items
            (or (ahash-ref legacy-table 'recent-buffer)
              (ahash-ref legacy-table "recent-buffer")
              '()
            ) ;or
            (when (list? recent-items)
              (import-legacy-scm-recent-files! recent-items)
            ) ;when
          ) ;with
        ) ;when
        (save-learned)
        (when need-interactive?
          (write-migration-marker interactive-arg-migration-marker-v1)
        ) ;when
        (when need-recent?
          (write-migration-marker recent-files-migration-marker-v1)
        ) ;when
      ) ;and-with
    ) ;when
  ) ;let
) ;define

(define (retrieve-learned)
  (set! interactive-arg-json
    (load-json-with-fallback interactive-arg-file
      interactive-args-json-valid?
      (lambda () (make-empty-state 'interactive-arg))
    ) ;load-json-with-fallback
  ) ;set!
  (set! interactive-arg-recent-file-json
    (load-json-with-fallback interactive-arg-recent-file-path
      recent-files-json-valid?
      (lambda () (make-empty-state 'recent-file))
    ) ;load-json-with-fallback
  ) ;set!
  (maybe-import-legacy-scm-interactive-state)
) ;define


(on-entry (retrieve-learned))
(on-exit (save-learned))
