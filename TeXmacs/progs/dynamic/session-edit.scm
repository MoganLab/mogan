
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : session-edit.scm
;; DESCRIPTION : editing routines for sessions
;; COPYRIGHT   : (C) 2001--2009  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic session-edit)
  (:use (utils library tree)
    (utils library cursor)
    (utils plugins plugin-cmd)
    (dynamic session-drd)
    (dynamic fold-edit)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Where to find plug-in binaries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (set-manual-path p)
  (:synopsis* "Set path to plug-in binaries")
  (:argument p "Path")
  (:proposals p
    (if (cpp-has-preference? "manual path")
      (list (get-preference "manual path"))
      (list)
    ) ;if
  ) ;:proposals
  (if (== p "") (reset-preference "manual path") (set-preference "manual path" p))
  (restart-message)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style package rules for sessions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (style-category p)
  (:require (in? p (list "framed-session" "ring-session" "large-formulas")))
  :session-theme
) ;tm-define

(tm-define (style-category-precedes? x y)
  (:require (and (== x :session-theme) (in? y (map symbol->string (plugin-list))))
  ) ;:require
  #t
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Switches
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define session-math-input (make-ahash-table))

(define (session-key)
  (let* ((lan (get-env "prog-language")) (ses (get-env "prog-session")))
    (cons lan ses)
  ) ;let*
) ;define

(tm-define (session-math-input? . opts)
  (with key
    (if (< (length opts) 2) (session-key) (cons (car opts) (cadr opts)))
    (ahash-ref session-math-input key)
  ) ;with
) ;tm-define

(tm-define (session-enable-math-input lan ses)
  (ahash-set! session-math-input (cons lan ses) #t)
) ;tm-define

(tm-define (toggle-session-math-input)
  (:synopsis "Toggle mathematical input in sessions")
  (:check-mark "v" session-math-input?)
  (ahash-set! session-math-input (session-key) (not (session-math-input?)))
  (with-innermost t field-context? (field-update-math t))
) ;tm-define


(define session-text-input (make-ahash-table))

(tm-define (session-text-input? . opts)
  (with key
    (if (< (length opts) 2) (session-key) (cons (car opts) (cadr opts)))
    (ahash-ref session-text-input key)
  ) ;with
) ;tm-define

(tm-define (session-enable-text-input lan ses)
  (ahash-set! session-text-input (cons lan ses) #t)
) ;tm-define

(tm-define (toggle-session-text-input)
  (:synopsis "Toggle text input in sessions")
  (:check-mark "v" session-text-input?)
  (ahash-set! session-text-input (session-key) (not (session-text-input?)))
  (with-innermost t field-context? (field-update-text t))
) ;tm-define


(define session-multiline-input (make-ahash-table))

(tm-define (session-multiline-input?)
  (ahash-ref session-multiline-input (session-key))
) ;tm-define

(tm-define (set-session-multiline-input lan ses set?)
  (ahash-set! session-multiline-input (cons lan ses) set?)
) ;tm-define

(tm-define (toggle-session-multiline-input)
  (:synopsis "Toggle multi-line input in sessions")
  (:check-mark "v" session-multiline-input?)
  (ahash-set! session-multiline-input
    (session-key)
    (not (session-multiline-input?))
  ) ;ahash-set!
) ;tm-define

(define session-output-timings (make-ahash-table))

(tm-define (session-output-timings?)
  (ahash-ref session-output-timings (session-key))
) ;tm-define

(tm-define (toggle-session-output-timings)
  (:synopsis "Toggle output of evaluation timings")
  (:check-mark "v" session-output-timings?)
  (ahash-set! session-output-timings
    (session-key)
    (not (session-output-timings?))
  ) ;ahash-set!
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Specific switches for Scheme sessions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define session-scheme-trees #t)

(tm-define (session-scheme-trees?) session-scheme-trees)

(tm-define (toggle-session-scheme-trees)
  (:synopsis "Toggle pretty tree output in scheme sessions")
  (:check-mark "v" session-scheme-trees?)
  (set! session-scheme-trees (not session-scheme-trees))
) ;tm-define

(define session-scheme-strees #f)

(tm-define (session-scheme-strees?) session-scheme-strees)

(tm-define (toggle-session-scheme-strees)
  (:synopsis "Toggle pretty scheme tree output in scheme sessions")
  (:check-mark "v" session-scheme-strees?)
  (set! session-scheme-strees (not session-scheme-strees))
) ;tm-define

(define session-scheme-math #f)

(tm-define (session-scheme-math?) session-scheme-math)

(tm-define (toggle-session-scheme-math)
  (:synopsis "Toggle pretty math output in scheme sessions")
  (:check-mark "v" session-scheme-math?)
  (set! session-scheme-math (not session-scheme-math))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Scheme sessions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (replace-newline s)
  (with l
    (string-tokenize-by-char s #\newline)
    (if (<= (length l) 1) s (tm->tree `(document ,@l)))
  ) ;with
) ;define

(define (var-object->string t)
  (with s
    (object->string t)
    (if (== s "#<unspecified>") "" (replace-newline (utf8->cork s)))
  ) ;with
) ;define

(define (eval-string-with-catch s)
  (catch #t
    (lambda () (eval (string->object s)))
    (lambda (key msg . err-msg)
      (let* ((msg (car err-msg))
             (args (cadr err-msg))
             (err-msg (if (list? args) (eval (apply format #f msg args)) msg))
            ) ;
        (stree->tree `(errput ,err-msg))
      ) ;let*
    ) ;lambda
  ) ;catch
) ;define

(define (error-tree? t)
  (and (tree? t) (tree-is? t 'errput))
) ;define

(tm-define (scheme-eval t mode)
  (let* ((s (texmacs->code t "SourceCode")) (r (eval-string-with-catch s)))
    (cond ((and (tree? r) (error-tree? r) (session-scheme-trees?)) (tree-copy r))
          ((and (tree? r) (session-scheme-trees?)) (tree 'text (tree-copy r)))
          ((and (tm? r)
             (== mode :silent)
             (or (session-scheme-trees?) (session-scheme-strees?))
           ) ;and
           (tree-copy (tm->tree r))
          ) ;
          ((and (tm? r) (session-scheme-strees?)) (tree 'text (tree-copy (tm->tree r))))
          ((session-scheme-math?)
           (with m
             (cas->stree r)
             (if (tm? m) (tree 'math (tm->tree m)) (var-object->string r))
           ) ;with
          ) ;
          ((string? r) (utf8->cork r))
          (else (var-object->string r))
    ) ;cond
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Low-level evaluation management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (session-encode in out next opts)
  (list (list session-do session-notify session-next session-cancel)
    in
    (tree->tree-pointer out)
    (tree->tree-pointer next)
    opts
  ) ;list
) ;define

(define (session-decode l)
  (list (second l)
    (tree-pointer->tree (third l))
    (tree-pointer->tree (fourth l))
    (fifth l)
  ) ;list
) ;define

(define (session-detach l)
  (tree-pointer-detach (third l))
  (tree-pointer-detach (fourth l))
) ;define

(define (session-coherent? out next)
  (and (field-or-output-context? (tree-ref out :up)) (field-context? next))
) ;define

(define (session-do lan ses)
  (with l
    (pending-ref lan ses)
    (with (in out next opts)
      (session-decode (car l))
      ;; (display* "Session do " lan ", " ses ", " in "\n")
      (if (or (and (tree-empty? in) (!= lan "r")) (not (session-coherent? out next)))
        (plugin-next lan ses)
        (begin
          (plugin-write lan ses in :session)
          (tree-set out :up 0 (plugin-prompt lan ses))
        ) ;begin
      ) ;if
    ) ;with
  ) ;with
) ;define

(define (session-next lan ses)
  ;; (display* "Session next " lan ", " ses "\n")
  (with l
    (pending-ref lan ses)
    (with (in out next opts)
      (session-decode (car l))
      (when (and (session-coherent? out next)
              (tm-func? out 'document)
              (tm-func? (tree-ref out :last) 'script-busy)
            ) ;and
        (let* ((dt (plugin-timing lan ses))
               (ts (if (< dt 1000)
                     (string-append (number->string dt) " msec")
                     (string-append (number->string (/ dt 1000.0)) " sec")
                   ) ;if
               ) ;ts
              ) ;
          (if (and (in? :timings opts) (>= dt 1))
            (tree-set (tree-ref out :last) `(timing ,ts))
            (tree-remove! out (- (tree-arity out) 1) 1)
          ) ;if
        ) ;let*
      ) ;when
      (when (and (session-coherent? out next) (tree-empty? out))
        (field-remove-output (tree-ref out :up))
      ) ;when
      (session-detach (car l))
    ) ;with
  ) ;with
) ;define

(define (var-tree-children t)
  (with r (tree-children t) (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r))
) ;define

;; tree-contains-label?
;; 递归检查 tree 中是否包含指定 label 的节点

(define (tree-contains-label? t label)
  (cond ((not (tree? t)) #f)
        ((eq? (tree-label t) label) #t)
        (else (let loop
                ((i 0) (n (tree-arity t)))
                (if (>= i n)
                  #f
                  (or (tree-contains-label? (tree-ref t i) label) (loop (+ i 1) n))
                ) ;if
              ) ;let
        ) ;else
  ) ;cond
) ;define

;; tree-remove-label-from-children!
;; 从 document 的直接子节点和 concat 子节点中移除指定 label 的节点

(define (tree-remove-label-from-children! t label)
  (when (tm-func? t 'document)
    (let loop
      ((i (- (tree-arity t) 1)))
      (when (>= i 0)
        (let ((child (tree-ref t i)))
          (cond ((eq? (tree-label child) label) (tree-remove! t i 1))
                ((tm-func? child 'concat)
                 (let sub-loop
                   ((j (- (tree-arity child) 1)))
                   (when (>= j 0)
                     (when (eq? (tree-label (tree-ref child j)) label)
                       (tree-remove! child j 1)
                     ) ;when
                     (sub-loop (- j 1))
                   ) ;when
                 ) ;let
                ) ;
                (else (noop))
          ) ;cond
        ) ;let
        (loop (- i 1))
      ) ;when
    ) ;let
  ) ;when
) ;define

;; tree-extract-reasoning-delta!
;; 从 tree 中递归提取所有 reasoning-delta 节点的文本，并清除这些节点
;; 返回提取的文本字符串

(define (tree-extract-reasoning-delta! t)
  (define (collect node)
    (cond ((not (tree? node)) "")
          ((eq? (tree-label node) 'reasoning-delta)
           (if (> (tree-arity node) 0) (or (tree->stree (tree-ref node 0)) "") "")
          ) ;
          (else (let loop
                  ((i 0) (n (tree-arity node)) (acc '()))
                  (if (>= i n)
                    (apply string-append (reverse acc))
                    (loop (+ i 1) n (cons (collect (tree-ref node i)) acc))
                  ) ;if
                ) ;let
          ) ;else
    ) ;cond
  ) ;define
  (let ((text (collect t)))
    (tree-remove-label-from-children! t 'reasoning-delta)
    text
  ) ;let
) ;define

;; session-find-last-unfolded-explain
;; 从 out 的位置 i-1 向前搜索 unfolded-explain

(define (session-find-last-unfolded-explain out i)
  (let loop
    ((k (- i 1)))
    (if (< k 0)
      #f
      (let ((child (tree-ref out k)))
        (cond ((tm-func? child 'unfolded-explain) child)
              ((tm-func? child 'concat)
               (let sub-loop
                 ((j 0) (n (tree-arity child)))
                 (if (>= j n)
                   (loop (- k 1))
                   (if (tm-func? (tree-ref child j) 'unfolded-explain)
                     (tree-ref child j)
                     (sub-loop (+ j 1) n)
                   ) ;if
                 ) ;if
               ) ;let
              ) ;
              (else (loop (- k 1)))
        ) ;cond
      ) ;let
    ) ;if
  ) ;let
) ;define

;; session-append-reasoning!
;; 追加 reasoning 文本到 out 中最后一个 unfolded-explain 的 document 中

(define (session-append-reasoning! out text)
  (when (tm-func? out 'document)
    (with i
      (tree-arity out)
      (if (and (> i 0) (tm-func? (tree-ref out (- i 1)) 'script-busy))
        (set! i (- i 1))
      ) ;if
      (with ue
        (session-find-last-unfolded-explain out i)
        (when ue
          (with body
            (tree-ref ue 1)
            (let doc-loop
              ((j 0))
              (when (< j (tree-arity body))
                (if (tm-func? (tree-ref body j) 'document)
                  (let* ((doc (tree-ref body j)) (content (if (tree? text) (tree->stree text) text)))
                    (when (and (string? content) (not (string-null? content)))
                      (let* ((cork-parts (string-split (cork->utf8 content) #\newline))
                             (parts (map utf8->cork cork-parts))
                            ) ;
                        (when (nnull? parts)
                          (let ((last-idx (- (tree-arity doc) 1)))
                            (when (>= last-idx 0)
                              (tree-set doc
                                last-idx
                                (string-append (or (tree->stree (tree-ref doc last-idx)) "") (car parts))
                              ) ;tree-set
                            ) ;when
                          ) ;let
                          (when (> (length parts) 1)
                            (tree-insert! doc (tree-arity doc) (cdr parts))
                          ) ;when
                        ) ;when
                      ) ;let*
                    ) ;when
                  ) ;let*
                  (doc-loop (+ j 1))
                ) ;if
              ) ;when
            ) ;let
          ) ;with
        ) ;when
      ) ;with
    ) ;with
  ) ;when
) ;define

;; session-fold-last-explain!
;; 折叠 out 中最后一个 unfolded-explain 为 folded-explain

(define (session-fold-last-explain! out)
  (when (tm-func? out 'document)
    (with i
      (tree-arity out)
      (if (and (> i 0) (tm-func? (tree-ref out (- i 1)) 'script-busy))
        (set! i (- i 1))
      ) ;if
      (with ue
        (session-find-last-unfolded-explain out i)
        (when ue
          (variant-set ue 'folded-explain)
        ) ;when
      ) ;with
    ) ;with
  ) ;when
) ;define

(define (session-output t u)
  (when (tm-func? t 'document)
    (with i
      (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy)) (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput)) (set! i (- i 1)))
      (when (tm-func? u 'document)
        (tree-insert! t i (var-tree-children u))
        (set-user-active #f)
      ) ;when
    ) ;with
  ) ;when
) ;define

(define (session-errput t u)
  (when (tm-func? t 'document)
    (with i
      (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy)) (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
        (set! i (- i 1))
        (tree-insert! t i '((errput (document))))
      ) ;if
      (session-output (tree-ref t i 0) u)
    ) ;with
  ) ;when
) ;define

(define (session-notify lan ses ch t)
  ;; (display* "Session notify " lan ", " ses ", " ch ", " t "\n")
  (with l
    (pending-ref lan ses)
    (with (in out next opts)
      (session-decode (car l))
      (when (session-coherent? out next)
        (cond ((== ch "output")
               (cond
                 ;; t 包含 reasoning-delta → 提取并追加到 unfolded-explain
                 ((tree-contains-label? t 'reasoning-delta)
                  (let* ((text (tree-extract-reasoning-delta! t))
                         (has-fold? (tree-contains-label? t 'fold-explain-reasoning))
                        ) ;
                    (when has-fold?
                      (tree-remove-label-from-children! t 'fold-explain-reasoning)
                    ) ;when
                    ;; 输出 t 中剩余的非 reasoning 内容
                    (when (> (tree-arity t) 0)
                      (session-output out t)
                    ) ;when
                    ;; 追加 reasoning 文本到 out 中的 unfolded-explain
                    (session-append-reasoning! out text)
                    ;; 如果同时有 fold 命令，折叠
                    (when has-fold?
                      (session-fold-last-explain! out)
                    ) ;when
                  ) ;let*
                 ) ;
                 ;; t 仅包含 fold-explain-reasoning → 直接折叠
                 ((tree-contains-label? t 'fold-explain-reasoning)
                  (session-fold-last-explain! out)
                 ) ;
                 ;; 正常输出
                 (else (session-output out t))
               ) ;cond
              ) ;
              ((== ch "error") (session-errput out t))
              ((== ch "prompt")
               (if (and (== (length l) 1) (tree-empty? (tree-ref next 1)))
                 (tree-set! next 0 (tree-copy t))
               ) ;if
              ) ;
              ((and (== ch "input") (null? (cdr l))) (tree-set! next 1 t))
        ) ;cond
      ) ;when
    ) ;with
  ) ;with
) ;define

(define (session-cancel lan ses dead?)
  ;; (display* "Session cancel " lan ", " ses ", " dead? "\n")
  (with l
    (pending-ref lan ses)
    (with (in out next opts)
      (session-decode (car l))
      (when (and (session-coherent? out next)
              (tm-func? out 'document)
              (tm-func? (tree-ref out :last) 'script-busy)
            ) ;and
        (tree-assign (tree-ref out :last)
          (if dead? '(script-dead) '(script-interrupted))
        ) ;tree-assign
      ) ;when
      (session-detach (car l))
    ) ;with
  ) ;with
) ;define

(tm-define (session-feed lan ses in out next opts)
  (set! in (plugin-preprocess lan ses in opts))
  (tree-assign! out '(document (script-busy)))
  (with x
    (session-encode in out next opts)
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x)))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Session contexts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (session-document-context? t)
  (and (tm-func? t 'document) (tm-func? (tree-ref t :up) 'session))
) ;tm-define

(tm-define (subsession-document-context? t)
  (or (and (tm-func? t 'document) (tm-func? (tree-ref t :up) 'session))
    (and (tm-func? t 'document)
      (tm-func? (tree-ref t :up) 'unfolded-subsession)
      (== (tree-index t) 1)
    ) ;and
  ) ;or
) ;tm-define

(tm-define field-tags
  '(input unfolded-io
     folded-io
     input-math
     unfolded-io-math
     folded-io-math
     input-text
     unfolded-io-text
     folded-io-text)
) ;tm-define

(tm-define (field-context? t)
  (and (tm? t) (tree-in? t field-tags) (tm-func? (tree-ref t :up) 'document))
) ;tm-define

(tm-define (field-or-output-context? t)
  (and (tm? t)
    (tree-in? t (cons 'output field-tags))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-folded-context? t)
  (and (tree-in? t '(folded-io folded-io-math folded-io-text))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-unfolded-context? t)
  (and (tree-in? t '(unfolded-io unfolded-io-math unfolded-io-text))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-prog-context? t)
  (and (tree-in? t '(input folded-io unfolded-io))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-math-context? t)
  (and (tree-in? t '(input-math folded-io-math unfolded-io-math))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-text-context? t)
  (and (tree-in? t '(input-text folded-io-text unfolded-io-text))
    (tm-func? (tree-ref t :up) 'document)
  ) ;and
) ;tm-define

(tm-define (field-input-context? t)
  (and (field-context? t) (== (tree-down-index t) 1))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style parameters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (field-parameters kind)
  (let* ((var (string-append (get-env "prog-language") "-" kind))
         (gen (string-append "generic-" kind))
        ) ;
    (search-parameters (if (style-has? var) var gen))
  ) ;let*
) ;define

(tm-define (standard-parameters l)
  (:require (== l "session"))
  (field-parameters "session")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "input"))
  (field-parameters "input")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "output"))
  (field-parameters "output")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "errput"))
  (field-parameters "errput")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "textput"))
  (field-parameters "textput")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (session-ready? . err-flag?)
  (with lan
    (get-env "prog-language")
    (or (== lan "scheme")
      (connection-defined? lan)
      (begin
        (if err-flag? (set-message `(concat ,"undefined plugin: "
                                      (verbatim ,lan)) ""))
        #f
      ) ;begin
    ) ;or
  ) ;with
) ;tm-define

(tm-define (session-status)
  (let* ((lan (get-env "prog-language")) (ses (get-env "prog-session")))
    (cond ((== lan "scheme") 2)
          ((not (connection-defined? lan)) 0)
          (else (connection-status lan ses))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (session-busy-message msg)
  (let* ((lan (get-env "prog-language")) (ses (get-env "prog-session")))
    (with l
      (pending-ref lan ses)
      (for-each (lambda (x)
                  (with (in out next opts)
                    (session-decode x)
                    (when (and (tm-func? out 'document) (tm-func? (tree-ref out :last) 'script-busy))
                      (tree-assign (tree-ref out :last) `(script-busy ,msg))
                    ) ;when
                  ) ;with
                ) ;lambda
        l
      ) ;for-each
    ) ;with
  ) ;let*
) ;tm-define

(tm-define (session-alive?) (> (session-status) 1))

(tm-define (session-supports-completions?)
  (and (session-alive?) (plugin-supports-completions? (get-env "prog-language")))
) ;tm-define

(tm-define (session-supports-input-done?)
  (and (session-alive?) (plugin-supports-input-done? (get-env "prog-language")))
) ;tm-define

(define (field-next* t forward?)
  (and-with u
    (tree-ref t (if forward? :next :previous))
    (cond ((field-context? u) u)
          ((tree-in? u '(folded-subsession unfolded-subsession)) #f)
          (else (field-next u forward?))
    ) ;cond
  ) ;and-with
) ;define

(define (field-next t forward?)
  (and-with u
    (tree-ref t (if forward? :next :previous))
    (if (field-context? u) u (field-next u forward?))
  ) ;and-with
) ;define

(define (field-extreme t last?)
  (with u
    (tree-ref t :up (if last? :last :first))
    (if (field-context? u) u (field-next u (not last?)))
  ) ;with
) ;define

(define (field-insert-output t)
  (cond ((tm-func? t 'input)
         (tree-insert! t 2 (list '(document)))
         (tree-assign-node! t 'unfolded-io)
        ) ;
        ((tm-func? t 'input-math)
         (tree-insert! t 2 (list '(document)))
         (tree-assign-node! t 'unfolded-io-math)
        ) ;
        ((tm-func? t 'input-text)
         (tree-insert! t 2 (list '(document)))
         (tree-assign-node! t 'unfolded-io-text)
        ) ;
  ) ;cond
) ;define

(define (field-remove-output t)
  (cond ((or (tm-func? t 'folded-io) (tm-func? t 'unfolded-io))
         (tree-assign-node! t 'input)
         (tree-remove! t 2 1)
        ) ;
        ((or (tm-func? t 'folded-io-math) (tm-func? t 'unfolded-io-math))
         (tree-assign-node! t 'input-math)
         (tree-remove! t 2 1)
        ) ;
        ((or (tm-func? t 'folded-io-text) (tm-func? t 'unfolded-io-text))
         (tree-assign-node! t 'input-text)
         (tree-remove! t 2 1)
        ) ;
        ((tm-func? t 'output)
         (with p
           (tree-ref t :up)
           (when (tree-is? p 'document)
             (tree-remove! p (tree-index t) 1)
           ) ;when
         ) ;with
        ) ;
  ) ;cond
) ;define

(define (field-update-math t)
  (if (session-math-input?)
    (when (field-prog-context? t)
      (if (tm-func? t 'input)
        (tree-assign-node! t 'input-math)
        (begin
          (tree-assign-node! t 'folded-io-math)
          (tree-assign (tree-ref t 1) '(document ""))
        ) ;begin
      ) ;if
    ) ;when
    (when (field-math-context? t)
      (if (tm-func? t 'input-math)
        (tree-assign-node! t 'input)
        (begin
          (tree-assign-node! t 'folded-io)
          (tree-assign (tree-ref t 1) '(document ""))
        ) ;begin
      ) ;if
    ) ;when
  ) ;if
) ;define

(define (field-update-text t)
  (if (session-text-input?)
    (when (field-prog-context? t)
      (if (tm-func? t 'input)
        (tree-assign-node! t 'input-text)
        (begin
          (tree-assign-node! t 'folded-io-text)
          (tree-assign (tree-ref t 1) '(document ""))
        ) ;begin
      ) ;if
    ) ;when
    (when (field-math-context? t)
      (if (tm-func? t 'input-text)
        (tree-assign-node! t 'input)
        (begin
          (tree-assign-node! t 'folded-io)
          (tree-assign (tree-ref t 1) '(document ""))
        ) ;begin
      ) ;if
    ) ;when
  ) ;if
) ;define

(define (field-create t p forward?)
  (let* ((d (tree-ref t :up))
         (i (+ (tree-index t) (if forward? 1 0)))
         (l (cond ((session-math-input?) 'input-math)
                  ((session-text-input?) 'input-text)
                  (else 'input)
            ) ;cond
         ) ;l
         (b `(,l ,p (document "")))
        ) ;
    (tree-insert d i (list b))
    (tree-ref d i)
  ) ;let*
) ;define

(define (session-forall-sub fun t)
  (for (u (tree-children t))
    (when (field-context? u)
      (fun u)
    ) ;when
    (when (and (tm-func? u 'unfolded-subsession) (tm-func? (tree-ref u 1) 'document))
      (session-forall-sub fun (tree-ref u 1))
    ) ;when
  ) ;for
) ;define

(define (session-forall-find-doc body)
  (cond ((not (tree? body)) #f)
        ((tm-func? body 'session)
         (with d (tree-ref body 2) (if (tree-is? d 'document) d #f))
        ) ;
        ((tm-func? body 'document)
         (let loop
           ((i 0))
           (if (>= i (tree-arity body))
             #f
             (or (session-forall-find-doc (tree-ref body i)) (loop (+ i 1)))
           ) ;if
         ) ;let
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (session-forall fun)
  (let ((t (or (tree-innermost subsession-document-context?)
             (session-forall-find-doc (buffer-get-body (current-buffer)))
           ) ;or
        ) ;t
       ) ;
    (when t
      (session-forall-sub fun t)
    ) ;when
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Processing input
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (make-session lan ses)
  (:synopsis "Insert session")
  (:argument lan "Language")
  (:argument ses "Session identifier")
  (let* ((ban '(output (document "")))
         (l (cond ((session-math-input? lan ses) 'input-math)
                  ((session-text-input? lan ses) 'input-text)
                  (else 'input)
            ) ;cond
         ) ;l
         (p (plugin-prompt lan ses))
         (in `(,l (document ,p) (document "")))
         (s `(session ,lan ,ses (document ,ban ,in)))
        ) ;
    (insert-go-to s '(2 1 1 0 0))
    (with-innermost t
      field-input-context?
      (with u
        (tree-ref t :previous 0)
        (if (url-exists? (url-unix "$TEXMACS_STYLE_PATH" (string-append lan ".ts")))
          (add-style-package lan)
        ) ;if
        (if (not (has-style-package? "framed-session"))
          (add-style-package "framed-session")
        ) ;if
        (session-feed lan ses :start u t '())
      ) ;with
    ) ;with-innermost
  ) ;let*
) ;tm-define

(define (input-options t)
  (with opts
    '()
    (when (session-output-timings?)
      (set! opts (cons :timings opts))
    ) ;when
    (when (field-math-context? t)
      (set! opts (cons :math-input opts))
    ) ;when
    opts
  ) ;with
) ;define

(define (field-process-input t)
  (when (session-ready? #t)
    (field-insert-output t)
    (cond ((tm-func? t 'folded-io) (tree-assign-node! t 'unfolded-io))
          ((tm-func? t 'folded-io-math) (tree-assign-node! t 'unfolded-io-math))
          ((tm-func? t 'folded-io-text) (tree-assign-node! t 'unfolded-io-text))
    ) ;cond
    (let* ((lan (get-env "prog-language"))
           (ses (get-env "prog-session"))
           (p (plugin-prompt lan ses))
           (in (tree->stree (tree-ref t 1)))
           (out (tree-ref t 2))
           (opts (input-options t))
          ) ;
      (with u
        (or (field-next* t #t) (field-create t p #t))
        (session-feed lan ses in out u opts)
        (tree-go-to u 1 :end)
        (set-user-active #f)
      ) ;with
    ) ;let*
  ) ;when
) ;define

(define (kbd-enter-sub t done?)
  (if (in? done? (list #f "#f"))
    (insert-return)
    (delayed (:idle 1) (session-evaluate))
  ) ;if
) ;define

(tm-define (kbd-enter t shift?)
  (:require (field-input-context? t))
  (cond ((xor (session-multiline-input?) shift?) (insert-return))
        ((session-supports-input-done?)
         (let* ((lan (get-env "prog-language"))
                (ses (get-env "prog-session"))
                (opts (input-options t))
                (st (tree->stree (tree-ref t 1)))
                (pre (plugin-preprocess lan ses st opts))
                (in (plugin-serialize lan pre))
                (rew (if (string-ends? in "\n") (string-drop-right in 1) in))
                (cmd (string-append "(input-done? " (string-quote rew) ")"))
                (ret (lambda (done?) (kbd-enter-sub t done?)))
               ) ;
           (plugin-command lan ses cmd ret '())
         ) ;let*
        ) ;
        (else (session-evaluate))
  ) ;cond
) ;tm-define

(tm-define (session-evaluate)
  (with-innermost t field-input-context? (field-process-input t))
) ;tm-define

(tm-define (session-evaluate-all)
  (session-forall (lambda (t) (when (not (tree-empty? (tree-ref t 1))) (field-process-input t)))
  ) ;session-forall
) ;tm-define

(tm-define (session-evaluate-above)
  (with-innermost me
    field-input-context?
    (session-forall (lambda (t)
                      (when (not (tree-empty? (tree-ref t 1)))
                        (when (path-inf? (tree->path t) (tree->path me))
                          (field-process-input t)
                        ) ;when
                      ) ;when
                    ) ;lambda
    ) ;session-forall
  ) ;with-innermost
) ;tm-define

(tm-define (session-evaluate-below)
  (with-innermost me
    field-input-context?
    (session-forall (lambda (t)
                      (when (not (tree-empty? (tree-ref t 1)))
                        (when (path-inf-eq? (tree->path me) (tree->path t))
                          (field-process-input t)
                        ) ;when
                      ) ;when
                    ) ;lambda
    ) ;session-forall
  ) ;with-innermost
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard editing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-horizontal t forwards?)
  (:require (field-context? t))
  (with move
    (if forwards? go-right go-left)
    (go-to-remain-inside move field-context? 1)
  ) ;with
) ;tm-define

(tm-define (kbd-extremal t forwards?)
  (:require (field-context? t))
  (with move
    (if forwards? go-end-line go-start-line)
    (go-to-remain-inside move field-context? 1)
  ) ;with
) ;tm-define

(define (field-go-to-previous)
  (with-innermost t
    field-context?
    (if (== t (field-extreme t #f))
      (go-up)
      (begin
        (with u
          (tree-ref t :previous)
          (if (and u (field-context? u))
            (tree-go-to u 1 :end)
            (go-to-previous-tag-same-argument field-tags)
          ) ;if
        ) ;with
        (go-start-line)
      ) ;begin
    ) ;if
  ) ;with-innermost
) ;define

(define (field-go-to-next)
  (with-innermost t
    field-context?
    (if (== t (field-extreme t #t))
      (go-down)
      (begin
        (with u
          (tree-ref t :next)
          (if (and u (field-context? u))
            (tree-go-to u 1 :start)
            (go-to-next-tag-same-argument field-tags)
          ) ;if
        ) ;with
        (go-end-line)
      ) ;begin
    ) ;if
  ) ;with-innermost
) ;define

(define (field-go-up)
  (with p
    (cursor-path)
    (go-to-remain-inside go-up field-context? 1)
    (when (== (cursor-path) p)
      (field-go-to-previous)
    ) ;when
  ) ;with
) ;define

(define (field-go-down)
  (with p
    (cursor-path)
    (go-to-remain-inside go-down field-context? 1)
    (when (== (cursor-path) p)
      (field-go-to-next)
    ) ;when
  ) ;with
) ;define

(tm-define (kbd-vertical t downwards?)
  (:require (field-context? t))
  (if downwards? (field-go-down) (field-go-up))
) ;tm-define

(tm-define (kbd-incremental t downwards?)
  (:require (field-context? t))
  (for (n 0 5) (if downwards? (field-go-to-next) (field-go-to-previous)))
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:require (field-input-context? t))
  (cond ((and (tree-cursor-at? t 1 :start) (not forwards?)) (noop))
        ((and (tree-cursor-at? t 1 :end) forwards?) (noop))
        (else (remove-text forwards?))
  ) ;cond
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:require (and (field-input-context? t) (selection-active-any?)))
  (clipboard-cut "nowhere")
  (clipboard-clear "nowhere")
) ;tm-define

(tm-define (kbd-variant t forwards?)
  (:require (and (field-context? t) (session-supports-completions?)))
  (let* ((lan (get-env "prog-language"))
         (ses (get-env "prog-session"))
         (cmd (session-complete-command t))
         (ret (lambda (x) (when x (custom-complete (tm->tree x)))))
        ) ;
    (when (!= cmd "")
      (plugin-command lan ses cmd ret '())
    ) ;when
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured keyboard movements
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (document-context? t)
  (:require (and (tree-is? t 'document) (field-input-context? (tree-ref t :up))))
  #f
) ;tm-define

(tm-define (traverse-horizontal t forwards?)
  (:require (field-input-context? t))
  (with move
    (if forwards? go-to-next-word go-to-previous-word)
    (go-to-remain-inside move field-context? 1)
  ) ;with
) ;tm-define

(tm-define (traverse-vertical t downwards?)
  (:require (field-input-context? t))
  (if downwards? (field-go-down) (field-go-up))
) ;tm-define

(tm-define (traverse-extremal t forwards?)
  (:require (field-input-context? t))
  (with move (if forwards? field-go-down field-go-up) (go-to-repeat move))
) ;tm-define

(tm-define (traverse-incremental t downwards?)
  (:require (field-input-context? t))
  (if downwards? (field-go-down) (field-go-up))
) ;tm-define

(tm-define (structured-horizontal t forwards?)
  (:require (field-input-context? t))
  (noop)
) ;tm-define

(tm-define (structured-vertical t downwards?)
  (:require (field-input-context? t))
  (with move
    (if downwards? field-go-down field-go-up)
    (go-to-remain-inside move 'session)
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Fold and unfold
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (alternate-toggle t)
  (:require (field-unfolded-context? t))
  (with i
    (tree-down-index t)
    (variant-set t (ahash-ref alternate-table (tree-label t)))
    (if (== i 2) (tree-go-to t 1 :end))
  ) ;with
) ;tm-define

(tm-define (alternate-toggle t)
  (:require (field-folded-context? t))
  (variant-set t (ahash-ref alternate-table (tree-label t)))
) ;tm-define

(tm-define (field-fold t)
  (when (field-unfolded-context? t)
    (alternate-toggle t)
  ) ;when
) ;tm-define

(tm-define (field-unfold t)
  (when (field-folded-context? t)
    (alternate-toggle t)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Field management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (field-insert t* forwards?)
  (and-with t
    (tree-search-upwards t* field-input-context?)
    (let* ((lan (get-env "prog-language"))
           (ses (get-env "prog-session"))
           (p (plugin-prompt lan ses))
           (t (field-create t p forwards?))
          ) ;
      (tree-go-to t 1 :end)
    ) ;let*
  ) ;and-with
) ;tm-define

(tm-define (field-insert-text t* forward?)
  (and-with t
    (tree-search-upwards t* field-input-context?)
    (let* ((d (tree-ref t :up))
           (i (+ (tree-index t) (if forward? 1 0)))
           (b '(textput (document "")))
          ) ;
      (tree-insert d i (list b))
      (tree-go-to d i 0 :start)
    ) ;let*
  ) ;and-with
) ;tm-define

(tm-define (field-remove-banner t*)
  (and-with t
    (tree-search-upwards t* session-document-context?)
    (when (tm-func? (tree-ref t 0) 'output)
      (tree-remove! t 0 1)
    ) ;when
  ) ;and-with
) ;tm-define

(tm-define (field-remove-extreme t* last?)
  (and-with t
    (tree-search-upwards t* field-input-context?)
    (with u
      (field-extreme t last?)
      (with v
        (field-next t (not last?))
        (if (and (== u t) v) (tree-go-to v 1 :end))
        (if (or (!= u t) v) (tree-remove (tree-ref u :up) (tree-index u) 1))
      ) ;with
    ) ;with
  ) ;and-with
) ;tm-define

(tm-define (field-remove t* forwards?)
  (and-with t
    (tree-search-upwards t* field-input-context?)
    (if forwards?
      (with u
        (field-next t #t)
        (if u
          (begin
            (tree-remove (tree-ref t :up) (tree-index t) 1)
            (tree-go-to u 1 :start)
          ) ;begin
          (field-remove-extreme t #t)
        ) ;if
      ) ;with
      (with u
        (field-next* t #f)
        (if u (tree-remove (tree-ref u :up) (tree-index u) 1) (field-remove-banner t))
      ) ;with
    ) ;if
  ) ;and-with
) ;tm-define

(tm-define (structured-insert-horizontal t forwards?)
  (:require (field-input-context? t))
  (if forwards? (field-insert-fold t))
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (field-input-context? t))
  (field-insert t downwards?)
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (:require (field-input-context? t))
  (field-remove t forwards?)
) ;tm-define

(tm-define (structured-remove-vertical t forwards?)
  (:require (field-input-context? t))
  (field-remove t forwards?)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Session management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (session-clear-all) (session-forall field-remove-output))

(tm-define (session-fold-all) (session-forall field-fold))

(tm-define (session-unfold-all) (session-forall field-unfold))

(define (session-collect-fields-sub t)
  (let ((result '()))
    (for (u (tree-children t))
      (when (field-context? u)
        (set! result (append result (list u)))
      ) ;when
      (when (and (tm-func? u 'unfolded-subsession) (tm-func? (tree-ref u 1) 'document))
        (set! result (append result (session-collect-fields-sub (tree-ref u 1))))
      ) ;when
    ) ;for
    result
  ) ;let
) ;define

(define (session-unfold-last-n-sub n)
  (let ((t (or (tree-innermost subsession-document-context?)
             (session-forall-find-doc (buffer-get-body (current-buffer)))
           ) ;or
        ) ;t
       ) ;
    (when t
      (let* ((fields (session-collect-fields-sub t)) (total (length fields)))
        (for (i (.. (max 0 (- total n)) total)) (field-unfold (list-ref fields i)))
      ) ;let*
    ) ;when
  ) ;let
) ;define

(tm-define (session-unfold-last-n n)
  (session-fold-all)
  (session-unfold-last-n-sub n)
) ;tm-define

(tm-define (field-insert-fold t*)
  (and-with t
    (tree-search-upwards t* field-input-context?)
    (tree-set! t `(unfolded-subsession (document "") (document ,t)))
    (tree-go-to t 0 :end)
  ) ;and-with
) ;tm-define

(tm-define (session-split)
  (with-innermost t
    session-document-context?
    (let* ((u (tree-ref t :up))
           ;; session
           (v (tree-ref u :up))
           ;; document
           (i (+ (tree-down-index t) 1))
           (j (tree-index u))
           (lan (tree-ref u 0))
           (ses (tree-ref u 1))
          ) ;
      (when (< i (tree-arity t))
        (tree-remove! u 0 2)
        (tree-split! u 0 i)
        (tree-split! v j 1)
        (tree-insert (tree-ref v j) 0 `(,lan ,ses))
        (tree-insert (tree-ref v (+ j 1)) 0 `(,lan ,ses))
        (tree-insert v (+ j 1) '((document "")))
        (tree-go-to v (+ j 1) :end)
      ) ;when
    ) ;let*
  ) ;with-innermost
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Copy and paste
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (session-selection-one? t)
  (and (tree-in? t
         (cons* 'output 'textput 'folded-subsession 'unfolded-subsession field-tags)
       ) ;tree-in?
    (tree-up t)
    (session-document-context? (tree-up t))
  ) ;and
) ;define

(define (session-selection?)
  (with l (selection-trees) (and (nnull? l) (forall? session-selection-one? l)))
) ;define

(tm-define (clipboard-cut which)
  (:require (session-selection?))
  (let* ((l (selection-trees))
         (doc (tree-up (car l)))
         (ses (tree-up doc))
         (i (tree-index (car l)))
         (j (tree-index (cAr l)))
         (k (- (+ j 1) i))
         (n (tree-arity doc))
        ) ;
    (clipboard-copy which)
    (if (= k n)
      (tree-cut ses)
      (let* ((sel `(session ,@(cDr (tm-children ses)) (document ,@l))))
        (clipboard-set which sel)
        (tree-remove doc i k)
        (with next
          (tree-ref doc (min i (- n (+ k 1))))
          (cond ((field-context? next) (tree-go-to next 1 :start))
                ((tree-in? next '(output textput)) (tree-go-to next 0 :start))
                ((tree-in? next '(folded-subsession unfolded-subsession))
                 (tree-go-to next 0 :start)
                ) ;
                (else (tree-go-to next :start))
          ) ;cond
        ) ;with
      ) ;let*
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (clipboard-copy which)
  (:require (session-selection?))
  (let* ((l (selection-trees))
         (doc (tree-up (car l)))
         (ses (tree-up doc))
         (sel `(session ,@(cDr (tm-children ses)) (document ,@l)))
        ) ;
    (clipboard-set which sel)
  ) ;let*
) ;tm-define

(tm-define (inside-subsession-context? t)
  (and (tree-in? t '(folded-subsession unfolded-subsession))
    (== (tree-arity t) 2)
    (cursor-inside? (tree-ref t 1))
  ) ;and
) ;tm-define

(tm-define (clipboard-paste which)
  (:require (and (inside? 'session)
              (tm-ref (clipboard-get which) 1)
              (tree-is? (tm-ref (clipboard-get which) 1) 'session)
            ) ;and
  ) ;:require
  (let* ((ses (tree-innermost 'session))
         (sub (tree-innermost inside-subsession-context?))
         (ins (tree-ref (clipboard-get which) 1))
        ) ;
    (when (and (== (tree-arity ses) 3)
            (== (tree-arity ins) 3)
            (tm-equal? (tm-ref ses 0) (tm-ref ins 0))
          ) ;and
      (let* ((doc (if sub (tree-ref sub 1) (tree-ref ses 2)))
             (ext (tree-ref ins 2))
             (i (tree-down-index doc))
            ) ;
        (if (== (cursor-path) (tree->path doc i :end))
          (begin
            (tree-insert doc (+ i 1) (tree-children ext))
            (tree-go-to doc (+ i (tree-arity ext)) :end)
          ) ;begin
          (tree-insert doc i (tree-children ext))
        ) ;if
      ) ;let*
    ) ;when
  ) ;let*
) ;tm-define
