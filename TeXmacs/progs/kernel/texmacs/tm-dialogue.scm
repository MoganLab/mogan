
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
(import (liii json)
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

(define interactive-arg-table (make-ahash-table))


(define interactive-arg-recent-file-json
  `(("meta" . (("version" . 1)
                ("total" . 0)))
      ("files" . #())))

(define (recent-file-item-valid? item)
  (and (json-object? item)
       (string? (json-ref item "path"))
       (string? (json-ref item "name"))
       (number? (json-ref item "last_open"))
       (number? (json-ref item "open_count"))
       (boolean? (json-ref item "show"))))

(define (recent-files-json-valid? recent-files)
  (and (json-object? recent-files)
       (let* ((meta (json-ref recent-files "meta"))
              (files (json-ref recent-files "files"))
              (version (and (json-object? meta) (json-ref meta "version")))
              (total (and (json-object? meta) (json-ref meta "total"))))
         (and (json-object? meta)
              (number? version)
              (integer? total) (>= total 0)
              (vector? files)
              (every recent-file-item-valid? (vector->list files))))))


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
2. 若找到索引，调用 `json-drop` 删除该项。
3. 将 `meta.total` 减一（不低于 0）。
4. 将更新后的 JSON 结构回写到 `interactive-arg-recent-file-json`。
|#
(define-public (recent-files-remove-by-path path)
  (let ((idx (recent-files-index-by-path interactive-arg-recent-file-json path)))
    (when idx
      (let* ((total (json-ref interactive-arg-recent-file-json "meta" "total"))
             (total (if (number? total) total 0))
             (new-total (if (<= total 0) 0 (- total 1)))
             (r1 (json-drop interactive-arg-recent-file-json "files" idx)))
        (set! interactive-arg-recent-file-json
              (json-set r1 "meta" "total" new-total))))))



(define (recent-files-apply-lru recent-files limit)
  (let* ((files (json-ref recent-files "files"))
         (n (vector-length files))
         (indexed
          (let loop ((i 0) (acc '()))
            (if (>= i n) acc
                (let* ((item (vector-ref files i))
                       (t (json-ref item "last_open"))
                       (t (if (number? t) t 0)))
                  (loop (+ i 1) (cons (cons i t) acc))))))
         (sorted (sort indexed (lambda (a b) (> (cdr a) (cdr b))))))
    (if (<= n limit)
        (json-set recent-files "files"
                  (list->vector
                   (map (lambda (p) (vector-ref files (car p))) sorted)))
        (let* ((keep (take sorted limit))
               (drop (drop sorted limit))
               (new-files
                (list->vector
                 (append
                  (map (lambda (p)
                         (let* ((item (vector-ref files (car p))))
                           (json-set item "show" #t)))
                       keep)
                  (map (lambda (p)
                         (let* ((item (vector-ref files (car p))))
                           (json-set item "show" #f)))
                       drop)))))
          (json-set recent-files "files" new-files)))))

(define (recent-files-add recent-files path name)
  (let* ((files (json-ref recent-files "files"))
         (idx (vector-length files))
         (item `(("path" . ,path)
                 ("name" . ,name)
                 ("last_open" . ,(current-second))
                 ("open_count" . 1)
                 ("show" . #t)))
         (total (json-ref recent-files "meta" "total"))
         (total (if (number? total) total 0))
         (r1 (json-set
               (json-push recent-files "files" idx item)
               "meta" "total" (+ total 1))))
    (recent-files-apply-lru r1 25)))

(define (recent-files-set recent-files idx)
  (let* ((item (json-ref recent-files "files" idx))
         (path* (json-ref item "path"))
         (name* (json-ref item "name"))
         (count* (json-ref item "open_count"))
         (count* (if (number? count*) count* 0))
         (new-item `(("path" . ,path*)
                     ("name" . ,name*)
                     ("last_open" . ,(current-second))
                     ("open_count" . ,(+ count* 1))
                     ("show" . #t)))
         (r1 (json-set recent-files "files" idx new-item)))
    (recent-files-apply-lru r1 25)))



(define (recent-files-index-by-path recent-files path)
  (let ((files (json-ref recent-files "files")))
    (let loop ((i 0))
      (if (>= i (vector-length files))
          #f
          (let ((item (vector-ref files i)))
            (if (equal? (json-ref item "path") path)
                i
                (loop (+ i 1))))))))

(define (recent-files-paths recent-files)
  (let ((files (json-ref recent-files "files")))
    (map (lambda (item)
           (list (cons "0" (json-ref item "path"))))
         (vector->list files))))


(define (list-but l1 l2)
  (cond ((null? l1) l1)
        ((in? (car l1) l2) (list-but (cdr l1) l2))
        (else (cons (car l1) (list-but (cdr l1) l2)))))

(define (as-stree x)
  (cond ((tree? x) (tree->stree x))
        ((== x #f) "false")
        ((== x #t) "true")
        (else x)))

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
  (set! assoc-t (map (lambda (x) (cons (car x) (as-stree (cdr x)))) assoc-t))
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (let* ((l1 (or (ahash-ref interactive-arg-table fun) '()))
           (l2 (cons assoc-t (list-but l1 (list assoc-t)))))
      (case fun
        ((recent-buffer)
          (recent-buffer-json (cdr (car (car l2)))))
        (else (ahash-set! interactive-arg-table fun l2)))
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
- 其他命令：返回 `interactive-arg-table` 中为该命令记录的历史参数列表。
- 若无记录，返回空列表 `()`。

逻辑
----
1. 归一化：将 `fun` 转为符号名。
2. 分支：`recent-buffer` 走最近文件 JSON 缓存分支。
3. 默认：从 `interactive-arg-table` 读取命令历史，缺省为 `()`。
|#
(define-public (learned-interactive fun)
  "Return learned list of interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (case fun
    ((recent-buffer)
     (recent-files-paths interactive-arg-recent-file-json))
    (else
     (or (ahash-ref interactive-arg-table fun) '()))))




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
   - 其他命令：从 `interactive-arg-table` 中删除对应键。
|#
(define-public (forget-interactive fun)
  "Forget interactive values for @fun"
  (set! fun (procedure-symbol-name fun))
  (when (symbol? fun)
    (case fun
      ((recent-buffer)
       (set! interactive-arg-recent-file-json
             `(("meta" . (("version" . 1)
                          ("total" . 0)))
               ("files" . #()))))
      (else
       (ahash-remove! interactive-arg-table fun)))))


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
  (with l (ahash-table->list interactive-arg-table)
    (save-object "$TEXMACS_HOME_PATH/system/interactive.scm" l)
    (string-save
      (json->string interactive-arg-recent-file-json)
      (string->url "$TEXMACS_HOME_PATH/system/recent-files.json"))))

(define (ahash-set-2! t x)
  (with (key . l) x
    (with (form arg) key
      (with a (or (ahash-ref t form) '())
        (set! a (assoc-set! a arg l))
        (ahash-set! t form a)))))      

(define (rearrange-old x)
  (with (form . l) x
    (let* ((len (apply min (map length l)))
           (truncl (map (cut sublist <> 0 len) l))
           (sl (sort truncl (lambda (l1 l2) (< (car l1) (car l2)))))
           (nl (map (lambda (x) (cons (number->string (car x)) (cdr x))) sl))
           (build (lambda args (map cons (map car nl) args)))
           (r (apply map (cons build (map cdr nl)))))
      (cons form r))))

(define (decode-old l)
  (let* ((t (make-ahash-table))
         (setter (cut ahash-set-2! t <>)))
    (for-each setter l)
    (let* ((r (ahash-table->list t))
           (m (map rearrange-old r)))
      (list->ahash-table m))))

(define (retrieve-learned)
  (if (url-exists? "$TEXMACS_HOME_PATH/system/interactive.scm")
      (let* ((l (load-object "$TEXMACS_HOME_PATH/system/interactive.scm"))
             (old? (and (pair? l) (pair? (car l)) (list-2? (caar l))))
             (decode (if old? decode-old list->ahash-table)))
        (set! interactive-arg-table (decode l))))
  (when (url-exists? "$TEXMACS_HOME_PATH/system/recent-files.json")
      (set! interactive-arg-recent-file-json
            (catch #t
              (lambda ()
                (let ((recent-files
                       (string->json
                        (string-load
                         (string->url "$TEXMACS_HOME_PATH/system/recent-files.json")))))
                  (if (recent-files-json-valid? recent-files)
                      recent-files
                      `(("meta" . (("version" . 1)
                                    ("total" . 0)))
                        ("files" . #())))))
              (lambda args
                `(("meta" . (("version" . 1)
                              ("total" . 0)))
                  ("files" . #())))))))


(on-entry (retrieve-learned))
(on-exit (save-learned))
