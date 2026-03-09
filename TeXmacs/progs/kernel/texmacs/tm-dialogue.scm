
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

(texmacs-module (kernel texmacs tm-dialogue)
  (:use (kernel texmacs tm-define)))
(import (liii njson)
        (liii time)
        (liii list))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Questions with user interaction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (user-ask prompt cont)
  (tm-interactive cont
    (if (string? prompt)
        (list (build-interactive-arg prompt))
        (list prompt))))

(define-public (user-confirm prompt default cont)
  (let ((k (lambda (answ) (cont (yes? answ)))))
    (if default
        (user-ask (list prompt "question" (translate "yes") (translate "no")) k)
        (user-ask (list prompt "question" (translate "no") (translate "yes")) k))))

(define-public (user-simple-confirm prompt default cont)
    (let ((k (lambda (answ) (cont (yes? answ)))))
      (if default
          (user-ask (list prompt "question-no-cancel" (translate "yes") (translate "no")) k)
          (user-ask (list prompt "question-no-cancel" (translate "no") (translate "yes")) k))))

(define-public (user-url prompt type cont)
  (user-delayed (lambda () (choose-file cont prompt type))))

(define-public (user-delayed cont)
  (exec-delayed cont))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Delayed execution of commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (delayed-sub body)
  (cond ((or (npair? body) (nlist? (car body)) (not (keyword? (caar body))))
         `(lambda () ,@body #t))
        ((== (caar body) :pause)
         `(let* ((start (texmacs-time))
                 (proc ,(delayed-sub (cdr body))))
            (lambda ()
              (with left (- (+ start ,(cadar body)) (texmacs-time))
                (if (> left 0) left
                    (begin
                      (set! start (texmacs-time))
                      (proc)))))))
        ((== (caar body) :every)
         `(let* ((time (+ (texmacs-time) ,(cadar body)))
                 (proc ,(delayed-sub (cdr body))))
            (lambda ()
              (with left (- time (texmacs-time))
                (if (> left 0) left
                    (begin
                      (set! time (+ (texmacs-time) ,(cadar body)))
                      (proc)))))))
        ((== (caar body) :idle)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              (with left (- ,(cadar body) (idle-time))
                (if (> left 0) left
                    (proc))))))
        ((== (caar body) :refresh)
         (with sym (gensym)
           `(let* ((,sym #f)
                   (proc ,(delayed-sub (cdr body))))
              (lambda ()
                (if (!= ,sym (change-time)) 0
                    (with left (- ,(cadar body) (idle-time))
                      (if (> left 0) left
                          (begin
                            (set! ,sym (change-time))
                            (proc)))))))))
        ((== (caar body) :require)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              (if (not ,(cadar body)) 0
                  (proc)))))
        ((== (caar body) :while)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              (if (not ,(cadar body)) #t
                  (with left (proc)
                    (if (== left #t) 0 left))))))
        ((== (caar body) :clean)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              (with left (proc)
                (if (!= left #t) left
                    (begin ,(cadar body) #t))))))
        ((== (caar body) :permanent)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              (with left (proc)
                (if (!= left #t) left
                    (with next ,(cadar body)
                      (if (!= next #t) #t
                          0)))))))
        ((== (caar body) :do)
         `(with proc ,(delayed-sub (cdr body))
            (lambda ()
              ,(cadar body)
              (proc))))
        (else (delayed-sub (cdr body)))))

(define-public-macro (delayed . body)
  `(exec-delayed-pause ,(delayed-sub body)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Messages and feedback on the status bar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public message-serial 0)

(define-public (set-message-notify)
  (set! message-serial (+ message-serial 1)))

(define-public (recall-message-after len)
  (with current message-serial
    (delayed
      (:idle len)
      (when (== message-serial current)
        (recall-message)))))

(define-public (set-temporary-message left right len)
  (set-message-temp left right #t)
  (recall-message-after len))

(define-public (texmacs-banner)
  (with tmv (string-append "GNU TeXmacs " (texmacs-version))
    (delayed
     (set-message "Welcome to GNU TeXmacs" tmv)
     (delayed
     (:pause 5000)
     (set-message "GNU TeXmacs falls under the GNU general public license" tmv)
     (delayed
     (:pause 2500)
     (set-message "GNU TeXmacs comes without any form of legal warranty" tmv)
     (delayed
     (:pause 2500)
     (set-message
      "More information about GNU TeXmacs can be found in the Help->About menu"
      tmv)
     (delayed
     (:pause 2500)
     (set-message "" ""))))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interactive commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define interactive-arg-version 1)
(define interactive-arg-file "$TEXMACS_HOME_PATH/system/interactive.json")
(define interactive-arg-recent-file-path "$TEXMACS_HOME_PATH/system/recent-files.json")
(define interactive-arg-file-system
  (url->system (string->url interactive-arg-file)))
(define interactive-arg-recent-file-system
  (url->system (string->url interactive-arg-recent-file-path)))

(define (make-empty-state kind)
  (case kind
    ((interactive-arg)
     (let ((root (string->njson "{\"meta\":{},\"commands\":{}}")))
       (njson-set! root "meta" "version" interactive-arg-version)
       root))
    ((recent-file)
     (string->njson "{\"meta\":{\"version\":1,\"total\":0},\"files\":[]}"))
    (else
     (string->njson "{}"))))

(define interactive-arg-json
  (make-empty-state 'interactive-arg))


(define interactive-arg-recent-file-json
  (make-empty-state 'recent-file))

(define interactive-args-schema-v1
  (string->njson
   "{\"type\":\"object\",\"required\":[\"meta\",\"commands\"],\"properties\":{\"meta\":{\"type\":\"object\",\"required\":[\"version\"],\"properties\":{\"version\":{\"type\":\"integer\",\"minimum\":1}}},\"commands\":{\"type\":\"object\",\"additionalProperties\":{\"type\":\"array\",\"items\":{\"type\":\"object\",\"additionalProperties\":{\"type\":\"string\"}}}}}}"))

(define recent-files-schema-v1
  (string->njson
   "{\"type\":\"object\",\"required\":[\"meta\",\"files\"],\"properties\":{\"meta\":{\"type\":\"object\",\"required\":[\"version\",\"total\"],\"properties\":{\"version\":{\"type\":\"number\"},\"total\":{\"type\":\"integer\",\"minimum\":0}}},\"files\":{\"type\":\"array\",\"items\":{\"type\":\"object\",\"required\":[\"path\",\"name\",\"last_open\",\"open_count\",\"show\"],\"properties\":{\"path\":{\"type\":\"string\"},\"name\":{\"type\":\"string\"},\"last_open\":{\"type\":\"number\"},\"open_count\":{\"type\":\"number\"},\"show\":{\"type\":\"boolean\"}}}}}}"))

(define (njson-schema-valid? schema instance)
  (catch #t
    (lambda ()
      (let ((report (njson-schema-report schema instance)))
        (hash-table-ref report 'valid?)))
    (lambda args #f)))

(define (interactive-args-json-valid? interactive-args)
  (njson-schema-valid? interactive-args-schema-v1 interactive-args))

(define (interactive-command-learned command-name)
  (let-njson ((commands (njson-ref interactive-arg-json "commands")))
    (if (njson-contains-key? commands command-name)
        (let-njson ((items (njson-ref commands command-name)))
          (if (njson-array? items)
              (vector->list (njson->json items))
              '()))
        '())))

(define (set-interactive-command-learned command-name items)
  (let-njson ((payload (json->njson (list->vector items))))
    (njson-set! interactive-arg-json "commands" command-name payload)))

(define (remove-interactive-command-learned command-name)
  (let-njson ((commands (njson-ref interactive-arg-json "commands")))
    (when (njson-contains-key? commands command-name)
      (njson-drop! interactive-arg-json "commands" command-name))))

(define (recent-files-json-valid? recent-files)
  (njson-schema-valid? recent-files-schema-v1 recent-files))


#|
recent-files-remove-by-path
按路径从最近文件缓存中删除对应条目。

语法
----
(recent-files-remove-by-path path)

参数
----
path : string
    目标文件路径。用于在 `interactive-arg-recent-file-json` 的 `files`
    列表中定位要移除的记录。

返回值
----
unspecified
- 函数通过副作用更新全局变量 `interactive-arg-recent-file-json`。
- 若路径不存在，则不做任何修改。

逻辑
----
1. 调用 `recent-files-index-by-path` 查找 `path` 在 `files` 中的索引。
2. 若找到索引，调用 `njson-drop!` 删除该项。
3. 将 `meta.total` 减一（不低于 0）。
4. 将更新后的 JSON 结构回写到 `interactive-arg-recent-file-json`。
|#
(define-public (recent-files-remove-by-path path)
  (let ((idx (recent-files-index-by-path interactive-arg-recent-file-json path)))
    (when idx
      (let* ((total (njson-ref interactive-arg-recent-file-json "meta" "total"))
             (total (if (number? total) total 0))
             (new-total (if (<= total 0) 0 (- total 1))))
        (njson-drop! interactive-arg-recent-file-json "files" idx)
        (njson-set! interactive-arg-recent-file-json "meta" "total" new-total)))))



(define (recent-files-apply-lru recent-files limit)
  (let-njson ((files (njson-ref recent-files "files")))
    (let* ((n (njson-size files))
           (indexed
            (let loop ((i 0) (acc '()))
              (if (>= i n) acc
                  (let* ((t (njson-ref files i "last_open"))
                         (t (if (number? t) t 0)))
                    (loop (+ i 1) (cons (cons i t) acc))))))
           (sorted (sort indexed (lambda (a b) (> (cdr a) (cdr b))))))
      (let-njson ((new-files (string->njson "[]")))
        (let loop ((rank 0) (rest sorted))
          (when (pair? rest)
            (let* ((p (car rest))
                   (idx (car p))
                   (show? (< rank limit)))
              (let-njson ((item (njson-ref files idx)))
                (njson-set! item "show" show?)
                (njson-append! new-files item))
              (loop (+ rank 1) (cdr rest)))))
        (njson-set! recent-files "files" new-files)))
    recent-files))

(define (recent-files-add recent-files path name)
  (let-njson ((item (json->njson
                      `(("path" . ,path)
                        ("name" . ,name)
                        ("last_open" . ,(current-second))
                        ("open_count" . 1)
                        ("show" . #t)))))
    (njson-append! recent-files "files" item))
  (let* ((total (njson-ref recent-files "meta" "total"))
         (total (if (number? total) total 0)))
    (njson-set! recent-files "meta" "total" (+ total 1)))
  (recent-files-apply-lru recent-files 25))

(define (recent-files-set recent-files idx)
  (let* ((count* (njson-ref recent-files "files" idx "open_count"))
         (count* (if (number? count*) count* 0)))
    (njson-set! recent-files "files" idx "last_open" (current-second))
    (njson-set! recent-files "files" idx "open_count" (+ count* 1))
    (njson-set! recent-files "files" idx "show" #t)
    (recent-files-apply-lru recent-files 25)))



(define (recent-files-index-by-path recent-files path)
  (let-njson ((files (njson-ref recent-files "files")))
    (let loop ((i 0))
      (if (>= i (njson-size files))
          #f
          (if (equal? (njson-ref files i "path") path)
              i
              (loop (+ i 1)))))))

(define (recent-files-paths recent-files)
  (let-njson ((files (njson-ref recent-files "files")))
    (let loop ((i 0) (n (njson-size files)) (acc '()))
      (if (>= i n) (reverse acc)
          (loop (+ i 1) n
                (cons (list (cons "0" (njson-ref files i "path"))) acc))))))


(define (list-but l1 l2)
  (cond ((null? l1) l1)
        ((in? (car l1) l2) (list-but (cdr l1) l2))
        (else (cons (car l1) (list-but (cdr l1) l2)))))

(define (as-stree x)
  (cond ((tree? x) (tree->stree x))
        ((== x #f) "false")
        ((== x #t) "true")
        (else x)))

(define (interactive-key->string x)
  (cond ((string? x) x)
        ((symbol? x) (symbol->string x))
        ((number? x) (number->string x))
        (else (object->string x))))

(define (interactive-value->string x)
  (with y (as-stree x)
    (cond ((string? y) y)
          ((symbol? y) (symbol->string y))
          ((number? y) (number->string y))
          ((boolean? y) (if y "true" "false"))
          (else (object->string y)))))

(define (normalize-interactive-assoc assoc-t)
  (map (lambda (x)
         (cons (interactive-key->string (car x))
               (interactive-value->string (cdr x))))
       assoc-t))

(define-public (procedure-symbol-name fun)
  (cond ((symbol? fun) fun)
        ((string? fun) (string->symbol fun))
        ((and (procedure? fun) (procedure-name fun)) => identity)
        (else #f)))

(define-public (procedure-string-name fun)
  (and-with name (procedure-symbol-name fun)
    (symbol->string name)))

(define (recent-buffer-json file-path)
  (let* ((name (url->system (url-tail (system->url file-path))))
         (idx (recent-files-index-by-path interactive-arg-recent-file-json file-path)))
    (if idx
        (set! interactive-arg-recent-file-json
              (recent-files-set interactive-arg-recent-file-json idx))
        (set! interactive-arg-recent-file-json
              (recent-files-add interactive-arg-recent-file-json file-path name)))))


(define-public (learn-interactive fun assoc-t)
  "Learn interactive values for @fun"
  (set! assoc-t (normalize-interactive-assoc assoc-t))
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (let* ((name (symbol->string fun))
           (l1 (interactive-command-learned name))
           (l2 (cons assoc-t (list-but l1 (list assoc-t)))))
      (case fun
        ((recent-buffer)
          (recent-buffer-json (cdr (car (car l2)))))
        (else (set-interactive-command-learned name l2)))
      )))


#|
learned-interactive
读取交互命令已学习的参数候选值。

语法
----
(learned-interactive fun)

参数
----
fun : procedure | symbol | string
    目标命令。函数内部会先调用 `procedure-symbol-name` 归一化为符号。

返回值
----
list
- 当命令是 `recent-buffer` 时：返回最近文件路径列表，元素形如
  `(("0" . 文件路径))`。
- 其他命令：返回 `interactive-arg-json` 中为该命令记录的历史参数列表。
- 若无记录，返回空列表 `()`。

逻辑
----
1. 归一化：将 `fun` 转为符号名。
2. 分支：`recent-buffer` 走最近文件 JSON 缓存分支。
3. 默认：从 `interactive-arg-json` 读取命令历史，缺省为 `()`。
|#
(define-public (learned-interactive fun)
  "Return learned list of interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (case fun
    ((recent-buffer)
     (recent-files-paths interactive-arg-recent-file-json))
    (else
     (with name (procedure-string-name fun)
       (if (string? name)
           (interactive-command-learned name)
           '())))))




#|
forget-interactive
清除指定交互命令的已学习参数。

语法
----
(forget-interactive fun)

参数
----
fun : procedure | symbol | string
    目标命令。函数内部会先调用 `procedure-symbol-name` 归一化为符号。

返回值
----
unspecified
- 通过副作用修改全局状态。
- 若 `fun` 不能归一化为符号，则不执行清除操作。

逻辑
----
1. 归一化：将 `fun` 转为符号名。
2. 校验：仅当 `fun` 是符号时继续。
3. 分支清理：
   - `recent-buffer`：将最近文件列表重置为空向量 `#()`，并把计数清零。
   - 其他命令：从 `interactive-arg-json` 中删除对应键。
|#
(define-public (forget-interactive fun)
  "Forget interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (case fun
      ((recent-buffer)
       (njson-free interactive-arg-recent-file-json)
       (set! interactive-arg-recent-file-json
             (make-empty-state 'recent-file)))
      (else
       (with name (procedure-string-name fun)
         (when (string? name)
           (remove-interactive-command-learned name)))))))


(define (learned-interactive-arg fun nr)
  (let* ((l (learned-interactive fun))
         (arg (number->string nr))
         (extract (lambda (assoc-l) (assoc-ref assoc-l arg))))
    (map extract l)))

(define (compute-interactive-arg-text fun which)
  (with arg (property fun (list :argument which))
    (cond ((npair? arg) (upcase-first (symbol->string which)))
          ((and (string? (car arg)) (null? (cdr arg))) (car arg))
          ((string? (cadr arg)) (cadr arg))
          (else (upcase-first (symbol->string which))))))

(define (compute-interactive-arg-type fun which)
  (with arg (property fun (list :argument which))
    (cond ((or (npair? arg) (npair? (cdr arg))) "string")
          ((string? (car arg)) (car arg))
          ((symbol? (car arg)) (symbol->string (car arg)))
          (else "string"))))

(define (compute-interactive-arg-proposals fun which)
  (let* ((default (property fun (list :default which)))
         (proposals (property fun (list :proposals which)))
         (learned '()))
    (cond ((procedure? default) (list (default)))
          ((procedure? proposals) (proposals))
          (else '()))))

(define (compute-interactive-arg fun which)
  (cons (compute-interactive-arg-text fun which)
        (cons (compute-interactive-arg-type fun which)
              (compute-interactive-arg-proposals fun which))))

(define (compute-interactive-args-try-hard fun)
  (with src (procedure-source fun)
    (if (and (pair? src) (== (car src) 'lambda)
             (pair? (cdr src)) (list? (cadr src)))
        (map upcase-first (map symbol->string (cadr src)))
        '())))

(define (compute-interactive-arg-list fun l)
  (if (npair? l) (list)
      (cons (compute-interactive-arg fun (car l))
            (compute-interactive-arg-list fun (cdr l)))))

(tm-define (compute-interactive-args fun)
  (let* ((args (property fun :arguments))
         (syn* (property fun :synopsis*)))
    (cond ((not args)
           (compute-interactive-args-try-hard fun))
          ((and (not (side-tools?)) (list-1? syn*) (string? (car syn*)))
           (let* ((type (compute-interactive-arg-type fun (car args)))
                  (prop (compute-interactive-arg-proposals fun (car args)))
                  (tail (compute-interactive-arg-list fun (cdr args))))
             (cons (cons (car syn*) (cons type prop)) tail)))
          (else (compute-interactive-arg-list fun args)))))

(define (build-interactive-arg s)
  (cond ((string-ends? s ":") s)
        ((string-ends? s "?") s)
        (else (string-append s ":"))))

(tm-define (build-interactive-args fun l nr learned?)
  (cond ((null? l) l)
        ((string? (car l))
         (build-interactive-args
          fun (cons (list (car l) "string") (cdr l)) nr learned?))
        (else
         (let* ((name (build-interactive-arg (caar l)))
                (type (cadar l))
                (pl (cddar l))
                (ql pl)
                ;;(ql (if (null? pl) '("") pl))
                (ll (if learned? (learned-interactive-arg fun nr) '()))
                (rl (append ql (list-but ll ql)))
                (props (if (<= (length ql) 1) rl ql)))
           (cons (cons name (cons type props))
                 (build-interactive-args fun (cdr l) (+ nr 1) learned?))))))

(tm-define (tm-interactive-new fun args)
  ;;(display* "interactive " fun ", " args "\n")
  (if (side-tools?)
      (begin
        (tool-select :transient-bottom (list 'interactive-tool fun args))
        (delayed
          (:pause 500)
          (keyboard-focus-on "interactive-0")))
      (tm-interactive fun args)))

(tm-define (interactive fun . args)
  (:synopsis "Call @fun with interactively specified arguments @args")
  (:interactive #t)
  (lazy-define-force fun)
  (if (null? args) (set! args (compute-interactive-args fun)))
  (with fun-args (build-interactive-args fun args 0 #t)
    (tm-interactive-new fun fun-args)))

(tm-define (interactive-title fun)
  (let* ((val (property fun :synopsis))
         (name (procedure-name fun))
         (name* (and name (symbol->string name))))
    (or (and (list-1? val) (string? (car val)) (car val))
        (and name (string-append "Interactive command '" name* "'"))
        "Interactive command")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Store learned arguments from one session to another
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (save-learned)
  (njson->file interactive-arg-file-system interactive-arg-json)
  (njson->file interactive-arg-recent-file-system interactive-arg-recent-file-json))

(define (load-njson-with-fallback file valid? fallback-maker)
  (catch #t
    (lambda ()
      (let ((parsed (file->njson file)))
        (if (valid? parsed)
            parsed
            (begin
              (njson-free parsed)
              (fallback-maker)))))
    (lambda args
      (fallback-maker))))

(define (reload-state current-state file valid? fallback-maker)
  (njson-free current-state)
  (load-njson-with-fallback file valid? fallback-maker))

(define (retrieve-learned)
  (set! interactive-arg-json
        (reload-state interactive-arg-json
                      interactive-arg-file-system
                      interactive-args-json-valid?
                      (lambda () (make-empty-state 'interactive-arg))))
  (set! interactive-arg-recent-file-json
        (reload-state interactive-arg-recent-file-json
                      interactive-arg-recent-file-system
                      recent-files-json-valid?
                      (lambda () (make-empty-state 'recent-file)))))


(on-entry (retrieve-learned))
(on-exit (save-learned))
