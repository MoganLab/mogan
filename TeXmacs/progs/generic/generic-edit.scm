
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-edit.scm
;; DESCRIPTION : Generic editing routines
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-edit)
  (:use (utils library tree)
    (utils library cursor)
    (utils edit variants)
    (utils misc tooltip)
    (bibtex bib-complete)
    (source macro-search)
    (telemetry telemetry-track)
  ) ;:use
) ;texmacs-module

(import (liii http))

(tm-define (generic-context? t) #t)
;; overridden in, e.g., graphics mode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic cursor movements via the keyboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-horizontal t forwards?)
  (and-with p (tree-outer t) (kbd-horizontal p forwards?))
) ;tm-define

(tm-define (kbd-vertical t downwards?)
  (and-with p (tree-outer t) (kbd-vertical p downwards?))
) ;tm-define

(tm-define (kbd-extremal t forwards?)
  (and-with p (tree-outer t) (kbd-extremal p forwards?))
) ;tm-define

(tm-define (kbd-incremental t downwards?)
  (and-with p (tree-outer t) (kbd-incremental p downwards?))
) ;tm-define

(tm-define (kbd-horizontal t forwards?)
  (:require (tree-is-buffer? t))
  (with move
    (lambda () (if forwards? (go-right) (go-left)))
    (go-to-next-such-that move generic-context?)
  ) ;with
) ;tm-define

(tm-define (kbd-vertical t downwards?)
  (:require (tree-is-buffer? t))
  (with move
    (lambda () (if downwards? (go-down) (go-up)))
    (go-to-next-such-that move generic-context?)
  ) ;with
) ;tm-define

(tm-define (kbd-extremal t forwards?)
  (:require (tree-is-buffer? t))
  (with move
    (lambda () (if forwards? (go-end-line) (go-start-line)))
    (go-to-next-such-that move generic-context?)
  ) ;with
) ;tm-define

(tm-define (kbd-incremental t downwards?)
  (:require (tree-is-buffer? t))
  (with move
    (lambda () (if downwards? (go-page-down) (go-page-up)))
    (go-to-next-such-that move generic-context?)
  ) ;with
) ;tm-define

(tm-define (kbd-left) (kbd-horizontal (focus-tree) #f))
(tm-define (kbd-right) (kbd-horizontal (focus-tree) #t))
(tm-define (kbd-up) (kbd-vertical (focus-tree) #f))
(tm-define (kbd-down) (kbd-vertical (focus-tree) #t))
(tm-define (kbd-start-line) (kbd-extremal (focus-tree) #f))
(tm-define (kbd-end-line) (kbd-extremal (focus-tree) #t))
(tm-define (kbd-page-up) (kbd-incremental (focus-tree) #f))
(tm-define (kbd-page-down) (kbd-incremental (focus-tree) #t))

(tm-define (kbd-select r) (select-from-shift-keyboard) (r) (select-from-cursor))

(tm-define (kbd-select-if-active r) (r) (select-from-cursor-if-active))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic editing via the keyboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (insert-return) (insert-raw-return))

(tm-define (kbd-space-bar t shift?)
  (and-with p (tree-outer t) (kbd-space-bar p shift?))
) ;tm-define

(tm-define (kbd-enter t shift?)
  (and-with p (tree-outer t) (kbd-enter p shift?))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Algorithm macro enter key navigation (only inside listing)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define algo-macro-tags
  '(algo-if algo-else-if
     algo-else
     algo-while
     algo-for
     algo-for-all
     algo-for-each
     algo-repeat
     algo-loop
     algo-procedure
     algo-function
     algo-if-else-if)
) ;define

(define algo-no-cond-macros
  '(algo-else algo-loop algo-body algo-begin algo-inputs algo-outputs)
) ;define

(define (in-listing-context? t)
  (and t (tree-search-upwards t (lambda (n) (tree-in? n '(listing)))))
) ;define

;; Fast path: only check direct parent and grandparent.
;; Algorithm macro arguments are rarely nested more than 3 levels deep.

(define (find-algo-macro-ancestor t)
  "Find the nearest algo-macro ancestor of t, or t itself"
  (cond ((not t) #f)
        ((tree-in? t algo-macro-tags) t)
        (else (let ((p (tree-outer t)))
                (cond ((not p) #f)
                      ((tree-in? p algo-macro-tags) p)
                      (else (let ((gp (tree-outer p)))
                              (cond ((not gp) #f)
                                    ((tree-in? gp algo-macro-tags) gp)
                                    (else #f)
                              ) ;cond
                            ) ;let
                      ) ;else
                ) ;cond
              ) ;let
        ) ;else
  ) ;cond
) ;define

(define (cursor-in-algo-macro-first-param? t)
  (and (in-listing-context? t)
    (with macro
      (find-algo-macro-ancestor t)
      (let* ((path (cursor-path)) (macro-path (tree->path macro)))
        (and macro-path
          (> (length path) (length macro-path))
          (let ((param-index (list-ref path (length macro-path))))
            (and (integer? param-index) (== param-index 0) (> (tree-arity macro) 1))
          ) ;let
        ) ;and
      ) ;let*
    ) ;with
  ) ;and
) ;define

(define (is-end-relative-path? t path)
  "Check if path (relative to t) points to the end or :after of t"
  (cond ((null? path) #f)
        ((== path '(:end)) #t)
        ((and (== (length path) 1)
           (integer? (car path))
           (if (tree-atomic? t)
             (== (car path) (string-length (tree->string t)))
             (== (car path) (tree-arity t))
           ) ;if
         ) ;and
         #t
        ) ;
        ((and (> (length path) 1) (integer? (car path)) (< (car path) (tree-arity t)))
         (with child
           (tm-ref t (car path))
           (and child (is-end-relative-path? child (cdr path)))
         ) ;with
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (cursor-in-algo-macro-condition-end? t)
  "Check if cursor is at the end of the condition (first param) of algo-macro t"
  (and (tree-in? t algo-macro-tags)
    (>= (tree-arity t) 2)
    (let* ((cond-idx 0) (path (cursor-path)) (t-path (tree->path t)))
      (and t-path
        (> (length path) (length t-path))
        (== (list-ref path (length t-path)) cond-idx)
        (let ((cond-arg (tm-ref t cond-idx)))
          (and cond-arg
            (let ((cond-path (tree->path cond-arg)))
              (and cond-path
                (>= (length path) (length cond-path))
                (is-end-relative-path? cond-arg (list-tail path (length cond-path)))
              ) ;and
            ) ;let
          ) ;and
        ) ;let
      ) ;and
    ) ;let*
  ) ;and
) ;define

(define (cursor-in-algo-macro-body-end? t)
  "Check if cursor is at the end of the body (last param) of algo-macro t"
  (and (tree-in? t algo-macro-tags)
    (>= (tree-arity t) 1)
    (let* ((body-idx (- (tree-arity t) 1)) (path (cursor-path)) (t-path (tree->path t)))
      (and t-path
        (> (length path) (length t-path))
        (== (list-ref path (length t-path)) body-idx)
        (let ((body (tm-ref t body-idx)))
          (and body
            (let ((body-path (tree->path body)))
              (and body-path
                (or (== path (append body-path '(:end)))
                  (and (> (tree-arity body) 0)
                    (with last-child
                      (tm-ref body (- (tree-arity body) 1))
                      (and last-child
                        (with lc-path
                          (tree->path last-child)
                          (and lc-path
                            (> (length path) (length lc-path))
                            (let ((rel-path (list-tail path (length lc-path))))
                              (or (== (cAr path) :end) (is-end-relative-path? last-child rel-path))
                            ) ;let
                          ) ;and
                        ) ;with
                      ) ;and
                    ) ;with
                  ) ;and
                ) ;or
              ) ;and
            ) ;let
          ) ;and
        ) ;let
      ) ;and
    ) ;let*
  ) ;and
) ;define

(define (cursor-in-algo-macro-body-empty-end? t)
  "Check if the cursor is in the empty last line/paragraph of the body of algo-macro t"
  (and (cursor-in-algo-macro-body-end? t)
    (let* ((body-idx (- (tree-arity t) 1)) (body (tm-ref t body-idx)))
      (and body
        (tree-is? body 'document)
        (> (tree-arity body) 1)
        (let* ((last-idx (- (tree-arity body) 1)) (last-child (tm-ref body last-idx)))
          (and (tree-empty? last-child)
            (let ((path (cursor-path)) (lc-path (tree->path last-child)))
              (and path lc-path (list-starts? path lc-path))
            ) ;let
          ) ;and
        ) ;let*
      ) ;and
    ) ;let*
  ) ;and
) ;define

(define (is-start-relative-path? t path)
  "Check if path (relative to t) points to the start of t"
  (cond ((null? path) #t)
        ((== path '(:start)) #t)
        ((and (== (length path) 1) (integer? (car path)) (== (car path) 0)) #t)
        ((and (> (length path) 1) (integer? (car path)) (== (car path) 0))
         (with child (tm-ref t 0) (and child (is-start-relative-path? child (cdr path))))
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (cursor-in-algo-macro-first-arg-start? t)
  "Check if cursor is at the start of the first argument of algo-macro t"
  (and (tree-in? t algo-macro-tags)
    (>= (tree-arity t) 1)
    (let* ((first-idx 0) (path (cursor-path)) (t-path (tree->path t)))
      (and t-path
        (> (length path) (length t-path))
        (== (list-ref path (length t-path)) first-idx)
        (let ((first-arg (tm-ref t first-idx)))
          (and first-arg
            (let ((arg-path (tree->path first-arg)))
              (and arg-path
                (>= (length path) (length arg-path))
                (is-start-relative-path? first-arg (list-tail path (length arg-path)))
              ) ;and
            ) ;let
          ) ;and
        ) ;let
      ) ;and
    ) ;let*
  ) ;and
) ;define

(define (cursor-at-algo-macro-start? t)
  "Check if the cursor is at the start of algo-macro t"
  (and (tree-in? t algo-macro-tags)
    (in-listing-context? t)
    (with t-path (tree->path t) (and t-path (== (cursor-path) t-path)))
  ) ;and
) ;define

(define (cursor-in-algo-macro-body-first-line? t)
  "Check if cursor is on the first line of the body of a no-cond algo-macro t"
  (and (tree-in? t algo-no-cond-macros)
    (in-listing-context? t)
    (let* ((body-idx 0) (path (cursor-path)) (t-path (tree->path t)))
      (and t-path
        (> (length path) (length t-path))
        (== (list-ref path (length t-path)) body-idx)
        (let ((body (tm-ref t body-idx)))
          (and body
            (let ((body-path (tree->path body)))
              (and body-path
                (if (tree-is? body 'document)
                  (and (> (tree-arity body) 0) (list-starts? path (append body-path '(0))))
                  #t
                ) ;if
              ) ;and
            ) ;let
          ) ;and
        ) ;let
      ) ;and
    ) ;let*
  ) ;and
) ;define

(tm-define (kbd-horizontal t forwards?)
  (:require (and (not forwards?)
              (tree-in? t algo-macro-tags)
              (in-listing-context? t)
              (or (cursor-in-algo-macro-first-arg-start? t) (cursor-at-algo-macro-start? t))
            ) ;and
  ) ;:require
  (with t-path
    (tree->path t)
    (and t-path
      (with parent
        (tree-up t)
        (let* ((parent-path (cDr t-path)) (t-index (cAr t-path)))
          (if (> t-index 0)
            (let ((sibling (tm-ref parent (- t-index 1))))
              (if (and sibling (tree-in? sibling algo-macro-tags))
                (begin
                  (display* "kbd-h backwards -> go-to sibling body end\n")
                  (tree-go-to sibling (- (tree-arity sibling) 1) :end)
                ) ;begin
                (begin
                  (display* "kbd-h backwards -> go-to sibling paragraph\n")
                  (go-to (tree->path sibling))
                ) ;begin
              ) ;if
            ) ;let
            (begin
              (display* "kbd-h backwards -> go-to start of parent\n")
              (go-to parent-path)
            ) ;begin
          ) ;if
        ) ;let*
      ) ;with
    ) ;and
  ) ;with
) ;tm-define

(tm-define (kbd-horizontal t forwards?)
  (:require (and forwards?
              (tree-in? t algo-no-cond-macros)
              (in-listing-context? t)
              (cursor-at-algo-macro-start? t)
            ) ;and
  ) ;:require
  (display* "kbd-h forwards at start -> go-to body\n")
  (tree-go-to t 0)
) ;tm-define

(tm-define (kbd-vertical t downwards?)
  (:require (and (not downwards?)
              (tree-in? t algo-no-cond-macros)
              (in-listing-context? t)
              (or (cursor-in-algo-macro-body-first-line? t) (cursor-at-algo-macro-start? t))
            ) ;and
  ) ;:require
  (with t-path
    (tree->path t)
    (and t-path
      (with parent
        (tree-up t)
        (let* ((parent-path (cDr t-path)) (t-index (cAr t-path)))
          (if (> t-index 0)
            (let ((sibling (tm-ref parent (- t-index 1))))
              (if (and sibling (tree-in? sibling algo-macro-tags))
                (begin
                  (display* "kbd-v upwards -> go-to sibling body end\n")
                  (tree-go-to sibling (- (tree-arity sibling) 1) :end)
                ) ;begin
                (begin
                  (display* "kbd-v upwards -> go-to sibling paragraph\n")
                  (go-to (tree->path sibling))
                ) ;begin
              ) ;if
            ) ;let
            (begin
              (display* "kbd-v upwards -> go-to start of parent\n")
              (go-to parent-path)
            ) ;begin
          ) ;if
        ) ;let*
      ) ;with
    ) ;and
  ) ;with
) ;tm-define

(tm-define (kbd-vertical t downwards?)
  (:require (and downwards?
              (tree-in? t algo-no-cond-macros)
              (in-listing-context? t)
              (cursor-at-algo-macro-start? t)
            ) ;and
  ) ;:require
  (display* "kbd-v downwards at start -> go-to body\n")
  (tree-go-to t 0)
) ;tm-define

(tm-define (kbd-horizontal t forwards?)
  (:require (and forwards?
              (tree-in? t algo-macro-tags)
              (in-listing-context? t)
              (or (cursor-in-algo-macro-body-end? t) (cursor-in-algo-macro-condition-end? t))
            ) ;and
  ) ;:require
  (cond ((cursor-in-algo-macro-body-end? t)
         (with t-path
           (tree->path t)
           (and t-path
             (with parent
               (tree-up t)
               (let* ((parent-path (cDr t-path)) (t-index (cAr t-path)))
                 (if (< (+ 1 t-index) (tree-arity parent))
                   (let ((sibling (tm-ref parent (+ 1 t-index))))
                     (if (and sibling (tree-in? sibling algo-macro-tags))
                       (begin
                         (display* "kbd-h body-end -> go-to sibling body\n")
                         (tree-go-to sibling 0)
                       ) ;begin
                       (begin
                         (display* "kbd-h body-end -> go-to sibling paragraph\n")
                         (go-to (tree->path sibling))
                       ) ;begin
                     ) ;if
                   ) ;let
                   (begin
                     (display* "kbd-h body-end -> go-to end of parent\n")
                     (go-to (rcons parent-path (+ 1 t-index)))
                   ) ;begin
                 ) ;if
               ) ;let*
             ) ;with
           ) ;and
         ) ;with
        ) ;
        ((cursor-in-algo-macro-condition-end? t)
         (display* "kbd-h cond-end -> go-to body\n")
         (tree-go-to t 1)
        ) ;
  ) ;cond
) ;tm-define

(tm-define (kbd-enter t shift?)
  (:require (and (not shift?) (cursor-in-algo-macro-first-param? t)))
  (with macro (find-algo-macro-ancestor t) (tree-go-to macro 0 :end) (go-right))
) ;tm-define

(tm-define (kbd-enter t shift?)
  (:require (and (not shift?)
              (in-listing-context? t)
              (with macro
                (find-algo-macro-ancestor t)
                (and macro (cursor-in-algo-macro-body-empty-end? macro))
              ) ;with
            ) ;and
  ) ;:require
  (with macro
    (find-algo-macro-ancestor t)
    (with t-path
      (tree->path macro)
      (and t-path
        (with parent
          (tree-up macro)
          (and parent
            (let* ((parent-path (cDr t-path))
                   (t-index (cAr t-path))
                   (body-idx (- (tree-arity macro) 1))
                   (body (tm-ref macro body-idx))
                   (last-idx (- (tree-arity body) 1))
                  ) ;
              (display* "kbd-enter body-empty-end -> remove empty line & insert sibling\n")
              (tree-remove! body last-idx 1)
              (tree-insert! parent (+ 1 t-index) '((concat "")))
              (go-to (rcons parent-path (+ 1 t-index)))
            ) ;let*
          ) ;and
        ) ;with
      ) ;and
    ) ;with
  ) ;with
) ;tm-define

(tm-define (kbd-control-enter t shift?)
  (and-with p (tree-outer t) (kbd-control-enter p shift?))
) ;tm-define

(tm-define (kbd-alternate-enter t shift?)
  (and-with p (tree-outer t) (kbd-alternate-enter p shift?))
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (and-with p (tree-outer t) (kbd-remove p forwards?))
) ;tm-define

(tm-define (kbd-variant t forwards?)
  (and-with p (tree-outer t) (kbd-variant p forwards?))
) ;tm-define

(tm-define (kbd-space-bar t shift?) (:require (tree-is-buffer? t)) (insert " "))

(tm-define (kbd-enter t shift?) (:require (tree-is-buffer? t)) (insert-return))

(tm-define (kbd-control-enter t shift?) (:require (tree-is-buffer? t)) (noop))

(tm-define (kbd-alternate-enter t shift?) (:require (tree-is-buffer? t)) (noop))

(tm-define (kbd-remove t forwards?)
  (:require (tree-is-buffer? t))
  (remove-text forwards?)
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:require (and (in-source?)
              (not (in-source-mode?))
              ;; 不在源码编辑或者导言区编辑模式
              (not (with-any-selection?))
            ) ;and
  ) ;:require
  (remove-text forwards?)
  (source-complete-try)
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:require (and (tree-is-buffer? t) (with-any-selection?)))
  (clipboard-cut "nowhere")
  (clipboard-clear "nowhere")
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:mode complete-mode?)
  (remove-text forwards?)
  (kbd-variant (focus-tree) #t)
) ;tm-define

(tm-define (kbd-remove t forwards?)
  (:require (at-image-start?))
  (let ((image (any-image-context?)))
    (tree-cut image)
  ) ;let
) ;tm-define

(tm-define (kbd-variant t forwards?)
  (:require (tree-is-buffer? t))
  (if (and (not (complete-try?)) forwards?)
    (set-message (translate "Use \\space (eg. 1cm) in order to insert a blank with specified width"
                 ) ;translate
      "tab"
    ) ;set-message
  ) ;if
) ;tm-define

;; 辅助函数：定义 enumerate-tag-list

(define (enumerate-tag-list)
  '(enumerate enumerate-numeric
     enumerate-numeric-bracket
     enumerate-roman
     enumerate-roman-bracket
     enumerate-roman-paren
     enumerate-Roman
     enumerate-alpha
     enumerate-alpha-bracket
     enumerate-alpha-full-paren
     enumerate-Alpha
     enumerate-circle
     enumerate-hanzi
     enumerate-numeric-paren)
) ;define

;; 辅助函数：定义 itemize-tag-list

(define (itemize-tag-list)
  '(itemize itemize-dot itemize-minus itemize-arrow)
) ;define

;; 辅助函数：定义 description-tag-list

(define (description-tag-list)
  '(description description-compact
     description-aligned
     description-dash
     description-long
     description-paragraphs)
) ;define

;; 辅助函数：检查是否在有序列表环境中

(define (in-enumerate-context?)
  (not (not (tree-search-upwards (focus-tree)
              (lambda (node) (tree-in? node (enumerate-tag-list)))
            ) ;tree-search-upwards
       ) ;not
  ) ;not
) ;define

;; 辅助函数：检查是否在无序列表环境中

(define (in-itemize-context?)
  (not (not (tree-search-upwards (focus-tree)
              (lambda (node) (tree-in? node (itemize-tag-list)))
            ) ;tree-search-upwards
       ) ;not
  ) ;not
) ;define

;; 辅助函数：检查是否在描述列表环境中

(define (in-description-context?)
  (not (not (tree-search-upwards (focus-tree)
              (lambda (node) (tree-in? node (description-tag-list)))
            ) ;tree-search-upwards
       ) ;not
  ) ;not
) ;define

;; 辅助函数：获取当前实际列表的精确标签，保留 itemize-dot 等变体样式

(define (get-current-list-label item)
  (and-with list-node
    (tree-search-upwards item list-node?)
    (tree-label list-node)
  ) ;and-with
) ;define

;; 辅助函数：查找包含 item 的 concat 包装和真正的 item list

(define (find-item-wrapper-and-list item)
  (let ((wrapper #f) (item-list #f))
    (let loop
      ((current (tree-outer item)))
      (if (tree-is? current 'concat)
        (begin
          (set! wrapper current)
          (loop (tree-outer current))
        ) ;begin
        (set! item-list current)
      ) ;if
    ) ;let
    (values wrapper item-list)
  ) ;let
) ;define

;; 辅助函数：提取 item 内容（处理 concat 包装）

(define (extract-item-content wrapper)
  (if (and wrapper (> (tree-arity wrapper) 1))
    (tree-copy (tree-ref wrapper 1))
    #f
  ) ;if
) ;define

;; 辅助函数：在列表中移除 item（处理 concat 包装）

(define (remove-item-from-list item wrapper item-list)
  (if wrapper
    ;; 如果有 wrapper，移除整个 wrapper
    (let ((wrapper-index (tree-index wrapper)))
      (tree-remove! item-list wrapper-index 1)
    ) ;let
    ;; 否则移除单个 item
    (let ((item-index (tree-index item)))
      (tree-remove! item-list item-index 1)
    ) ;let
  ) ;if
) ;define

(define (list-item-node? t)
  (or (tree-is? t 'item)
    (tree-is? t 'item*)
    (and (tree-is? t 'concat)
      (> (tree-arity t) 0)
      (or (tree-is? (tree-ref t 0) 'item) (tree-is? (tree-ref t 0) 'item*))
    ) ;and
  ) ;or
) ;define

(define (list-item-marker-node? t)
  (or (tree-is? t 'item) (tree-is? t 'item*))
) ;define

(define (list-item-wrapper-node? t)
  (and (tree-is? t 'concat)
    (> (tree-arity t) 0)
    (list-item-marker-node? (tree-ref t 0))
  ) ;and
) ;define

(define (list-document-child t)
  (let loop
    ((child t) (parent (and t (tree-up t))))
    (cond ((not parent) #f)
          ((tree-is? parent 'document) child)
          (else (loop parent (tree-up parent)))
    ) ;cond
  ) ;let
) ;define

(define (list-item-marker-at node)
  (cond ((list-item-marker-node? node) node)
        ((list-item-wrapper-node? node) (tree-ref node 0))
        (else #f)
  ) ;cond
) ;define

(tm-define (current-list-item-index-from-nearest-left t)
  (and-with child
    (list-document-child t)
    (let* ((doc (tree-up child))
           (list-node (and doc (tree-up doc)))
           (pos (tree-index child))
          ) ;
      (and doc
        list-node
        pos
        (list-node? list-node)
        (find-previous-item-index doc (+ pos 1))
      ) ;and
    ) ;let*
  ) ;and-with
) ;tm-define

(tm-define (current-list-item-marker-from-nearest-left t)
  (and-with child
    (list-document-child t)
    (let* ((doc (tree-up child))
           (item-index (current-list-item-index-from-nearest-left t))
          ) ;
      (and doc item-index (list-item-marker-at (tree-ref doc item-index)))
    ) ;let*
  ) ;and-with
) ;tm-define

;; Tab/Shift+Tab should work both on the item marker and inside its content.
;; When the cursor is in a concat-wrapped item, return the marker at index 0.
;; Content after a marker still belongs to that logical item until the next
;; marker, so fall back to the nearest item on the left in the same document.

(define (current-list-item-marker)
  (let* ((focus (focus-tree))
         (cursor (cursor-tree))
         (item? (lambda (t) (or (list-item-marker-node? t) (list-item-wrapper-node? t))))
         (item (or (tree-search-upwards cursor item?)
                 (tree-search-upwards focus item?)
                 (current-list-item-marker-from-nearest-left cursor)
                 (current-list-item-marker-from-nearest-left focus)
               ) ;or
         ) ;item
         (marker (and item (or (list-item-marker-at item) item)))
        ) ;
    marker
  ) ;let*
) ;define

(define (list-tab-context-match? forwards?)
  (let* ((item (current-list-item-marker))
         (enum? (in-enumerate-context?))
         (itemize? (in-itemize-context?))
         (description? (in-description-context?))
         (match? (and item
                   (or (and (or enum? itemize?) (tree-is? item 'item))
                     (and description? (tree-is? item 'item*))
                   ) ;or
                 ) ;and
         ) ;match?
        ) ;
    match?
  ) ;let*
) ;define

(define (list-item-cursor-target item wrapper)
  (if (and wrapper (not (cursor-inside? item))) 'content 'marker)
) ;define

(define (list-item-cursor-state item wrapper)
  (let* ((base (or wrapper item))
         (target (list-item-cursor-target item wrapper))
         (relative-path (and base (tree-cursor-path base)))
        ) ;
    (list target relative-path)
  ) ;let*
) ;define

(define (go-to-list-item-relative-path moved-item relative-path)
  (and relative-path
    (nnull? relative-path)
    (with p
      (apply tree->path (cons moved-item relative-path))
      (and p (begin (go-to p) #t))
    ) ;with
  ) ;and
) ;define

(define (go-to-moved-list-item moved-item cursor-state parent pos)
  (let* ((cursor-target (car cursor-state)) (relative-path (cadr cursor-state)))
    (or (go-to-list-item-relative-path moved-item relative-path)
      (cond ((and (== cursor-target 'content)
               (tree-is? moved-item 'concat)
               (> (tree-arity moved-item) 1)
             ) ;and
             (tree-go-to moved-item 1 :end)
            ) ;
            ((tree-is? moved-item 'concat) (tree-go-to moved-item 0 :end))
            (else (tree-go-to parent pos :end))
      ) ;cond
    ) ;or
  ) ;let*
) ;define

(define (list-node? t)
  (and t (tree-in? t (list-tag-list)))
) ;define

(define (list-family label)
  (cond ((in? label (enumerate-tag-list)) 'enumerate)
        ((in? label (itemize-tag-list)) 'itemize)
        ((in? label (description-tag-list)) 'description)
        (else label)
  ) ;cond
) ;define

(define (same-list-family? lhs rhs)
  (and lhs rhs (== (list-family lhs) (list-family rhs)))
) ;define

(tm-define (list-structured-insert-context?)
  (and-with item
    (current-list-item-marker)
    (or (in? (tree-label item) '(item item*))
      (in? (list-family (get-current-list-label item))
        '(enumerate itemize description)
      ) ;in?
    ) ;or
  ) ;and-with
) ;tm-define

(define (find-previous-item-index item-list start)
  (let loop
    ((i (- start 1)))
    (cond ((< i 0) #f)
          ((list-item-node? (tree-ref item-list i)) i)
          (else (loop (- i 1)))
    ) ;cond
  ) ;let
) ;define

(define (find-following-list-index item-list start end list-type)
  (let loop
    ((i (+ start 1)))
    (cond ((>= i end) #f)
          ((and (list-node? (tree-ref item-list i))
             (same-list-family? (tree-label (tree-ref item-list i)) list-type)
           ) ;and
           i
          ) ;
          (else (loop (+ i 1)))
    ) ;cond
  ) ;let
) ;define

(define (append-strees-to-document doc strees)
  (if (null? strees) doc (tree-insert doc (tree-arity doc) strees))
) ;define

(tm-define (blank-list-item-stree list-type)
  (if (== list-type 'description) '(item* "") '(item))
) ;tm-define

(tm-define (list-item-end-index item-list item-index list-type)
  (let loop
    ((i (+ item-index 1)))
    (cond ((>= i (tree-arity item-list)) i)
          ((list-item-node? (tree-ref item-list i)) i)
          ((and (list-node? (tree-ref item-list i))
             (same-list-family? (tree-label (tree-ref item-list i)) list-type)
           ) ;and
           (loop (+ i 1))
          ) ;
          (else (loop (+ i 1)))
    ) ;cond
  ) ;let
) ;tm-define

(tm-define (list-item-insert-index item-list item-index list-type downwards?)
  (if downwards? (list-item-end-index item-list item-index list-type) item-index)
) ;tm-define

(tm-define (list-item-remove-range item-list item-index list-type downwards?)
  (if downwards?
    (list item-index (list-item-end-index item-list item-index list-type))
    (and-with start
      (find-previous-item-index item-list item-index)
      (list start item-index)
    ) ;and-with
  ) ;if
) ;tm-define

(tm-define (remove-list-item-range item-list range)
  (and range (tree-remove item-list (car range) (- (cadr range) (car range))))
) ;tm-define

(tm-define (document-empty-for-list? doc)
  (let loop
    ((i 0))
    (cond ((>= i (tree-arity doc)) #t)
          ((list-item-node? (tree-ref doc i)) #f)
          (else (loop (+ i 1)))
    ) ;cond
  ) ;let
) ;tm-define

;; 在有序和无序列表中实现缩进功能
(tm-define (kbd-variant t forwards?)
  (:require (and forwards? (list-tab-context-match? forwards?)))

  (let ((item (current-list-item-marker)))
    ;; 查找包装和列表
    (call-with-values (lambda () (find-item-wrapper-and-list item))
      (lambda (wrapper item-list)
        (if (and item item-list)
          (let* ((item-index (if wrapper (tree-index wrapper) (tree-index item)))
                 (cursor-state (list-item-cursor-state item wrapper))
                 (list-type (list-family (or (get-current-list-label item) 'enumerate)))
                 (item-stree (tree->stree (if wrapper wrapper item)))
                 (next-index (+ item-index 1))
                 (attached-sublist-idx (and (< next-index (tree-arity item-list))
                                         (let ((next-node (tree-ref item-list next-index)))
                                           ;; 当前 item 后面如果紧跟同一大类的子列表，缩进时一并并入目标子列表。
                                           (and (list-node? next-node)
                                             (same-list-family? (tree-label next-node) list-type)
                                             next-index
                                           ) ;and
                                         ) ;let
                                       ) ;and
                 ) ;attached-sublist-idx
                ) ;
            (if (> item-index 0)
              (let* ((prev-item-index (find-previous-item-index item-list item-index))
                     (target-sublist-idx (and prev-item-index
                                           ;; 优先复用前一个 item 已有的同一大类子列表，避免制造相邻碎片列表。
                                           (find-following-list-index item-list prev-item-index item-index list-type)
                                         ) ;and
                     ) ;target-sublist-idx
                    ) ;
                (when prev-item-index
                  ;; 当前一个 item 还没有子列表时，才在当前 item 前插入一个空子列表。
                  (when (not target-sublist-idx)
                    (set! item-list
                      (tree-insert item-list item-index (list `(,list-type
                                                                (document))))
                    ) ;set!
                    (set! target-sublist-idx item-index)
                    (set! item-index (+ item-index 1))
                    ;; 新插入的空子列表会让原 item 和其后置子列表整体右移一位。
                    (if attached-sublist-idx (set! attached-sublist-idx (+ attached-sublist-idx 1)))
                  ) ;when

                  (let* ((target-sublist (tree-ref item-list target-sublist-idx))
                         (target-doc (tree-ref target-sublist 0))
                         (target-pos (tree-arity target-doc))
                         (attached-items (if attached-sublist-idx
                                           (map (lambda (i)
                                                  (tree->stree (tree-copy (tree-ref (tree-ref (tree-ref item-list attached-sublist-idx) 0) i))
                                                  ) ;tree->stree
                                                ) ;lambda
                                             (iota (tree-arity (tree-ref (tree-ref item-list attached-sublist-idx) 0)))
                                           ) ;map
                                           '()
                                         ) ;if
                         ) ;attached-items
                        ) ;
                    ;; 目标子列表依次接收：当前 item，以及它后面原来挂着的同类型子列表内容。
                    (set! target-doc
                      (append-strees-to-document target-doc (append (list item-stree) attached-items))
                    ) ;set!
                    ;; 从右往左删除，避免索引漂移。
                    (when attached-sublist-idx
                      (set! item-list (tree-remove! item-list attached-sublist-idx 1))
                    ) ;when
                    (set! item-list (tree-remove! item-list item-index 1))
                    ;; 优先恢复原光标在 item 内部的相对位置。
                    (let ((moved-item (tree-ref target-doc target-pos)))
                      (go-to-moved-list-item moved-item cursor-state target-doc target-pos)
                    ) ;let
                  ) ;let*
                ) ;when
              ) ;let*
            ) ;if
          ) ;let*
        ) ;if
      ) ;lambda
    ) ;call-with-values
  ) ;let
) ;tm-define

;; 在有序和无序列表中实现反缩进功能
;;
;; 处理逻辑：
;; 当用户按 Shift+Tab 时，将当前 item 从当前子列表中移出到外层列表。
;; 如果当前列表已经是最外层列表，则不再继续反缩进。
;;
;; 两种 Case：
;; 1. NOT first item: 当前 item 前面还有其他 items
;;    - 保留原 sublist（因为前面还有 items）
;;    - 从当前 sublist 的 document 中移除当前 item 和后续 items
;;    - 在 parent-list 中 sublist 之后插入当前 item
;;    - 如有后续 items，在当前 item 后面重建一个同类型 sublist
;;
;; 2. FIRST item: 当前 item 是第一个，后面可能有 items
;;    - 从当前 sublist 的 document 中移除当前 item（保留后续 items）
;;    - 从 parent-list 中移除整个 sublist
;;    - 在 parent-list 中原 sublist 位置插入当前 item
;;    - 如有后续 items，在当前 item 后面重建一个同类型 sublist
;;
(tm-define (kbd-variant t forwards?)
  (:require (and (not forwards?) (list-tab-context-match? forwards?)))

  (let* ((item (current-list-item-marker))
         (item-stree (tree->stree item))
         (wrapper #f)
         (doc (tree-outer item))
         (cursor-state #f)
        ) ;

    ;; 处理 concat 包装
    (when (tree-is? doc 'concat)
      (set! wrapper doc)
      (set! doc (tree-outer wrapper))
      (set! item-stree (tree->stree wrapper))
    ) ;when
    (set! cursor-state (list-item-cursor-state item wrapper))

    ;; 仅在 item 直接位于列表 document 中时才执行反缩进。
    (when (tree-is? doc 'document)
      (let* ((sublist (tree-outer doc))
             (parent-list (if sublist (tree-outer sublist) #f))
             ;; 只有在当前子列表外面还能找到另一层列表时，才允许继续反缩进；
             ;; 这样最外层列表会直接止住。
             (outer-list (and parent-list (tree-search-upwards parent-list list-node?)))
            ) ;

        (when (and parent-list outer-list)
          (let* ((sublist-idx (tree-index sublist))
                 (doc-arity (tree-arity doc))
                 (item-idx (if wrapper (tree-index wrapper) (tree-index item)))
                 (items-before-count item-idx)
                 (items-after-count (- doc-arity item-idx 1))
                ) ;

            ;; 先保存当前 item 后面的所有兄弟节点，后面需要重建 trailing sublist。
            (with items-after-stree
              (if (> items-after-count 0)
                (map (lambda (i) (tree->stree (tree-copy (tree-ref doc (+ item-idx 1 i)))))
                  (iota items-after-count)
                ) ;map
                '()
              ) ;if

              ;; 两个 case 都会得到同样的结果：更新后的 parent-list 和插入位置。
              (let* ((current-doc (tree-ref sublist 0))
                     (current-parent (tree-outer sublist))
                     (item-insert-pos #f)
                    ) ;

                ;; 先处理“原 sublist 要保留还是删除”这部分差异。
                (if (> items-before-count 0)
                  ;; Case 1: 不是第一个 item，保留 sublist
                  (begin
                    ;; 从 doc 中移除当前 item 和后续 items
                    (let loop
                      ((i (- doc-arity 1)) (cd current-doc))
                      (when (>= i item-idx)
                        (set! cd (tree-remove! cd i 1))
                        (loop (- i 1) cd)
                      ) ;when
                    ) ;let
                    ;; 重新获取 current-doc（修改后）
                    (set! current-doc (tree-ref sublist 0))
                    ;; item 插入位置：sublist 之后
                    (set! item-insert-pos (+ sublist-idx 1))
                  ) ;begin

                  ;; Case 2: 是第一个 item，删除 sublist
                  (begin
                    ;; 从 doc 中移除当前 item
                    (set! current-doc (tree-remove! current-doc item-idx 1))
                    ;; 从 parent-list 中移除 sublist
                    (set! current-parent (tree-remove! current-parent sublist-idx 1))
                    ;; item 插入位置：原 sublist 位置
                    (set! item-insert-pos sublist-idx)
                  ) ;begin
                ) ;if

                ;; 共同逻辑：在 parent-list 中插入当前 item
                (set! current-parent
                  (tree-insert current-parent item-insert-pos (list item-stree))
                ) ;set!

                ;; 如有后续 items，则在当前 item 后面重建一个同类型 sublist。
                (when (> (length items-after-stree) 0)
                  (let ((new-sublist-stree `(,(tree-label sublist)
                                             (document ,@items-after-stree)))
                        (sublist-pos (+ item-insert-pos 1))
                       ) ;
                    (set! current-parent
                      (tree-insert current-parent sublist-pos (list new-sublist-stree))
                    ) ;set!
                  ) ;let
                ) ;when

                ;; 共同逻辑：移动光标到新插入的 item
                (with moved-item
                  (tree-ref current-parent item-insert-pos)
                  ;; 根据 moved-item 类型决定光标位置
                  (go-to-moved-list-item moved-item cursor-state current-parent item-insert-pos)
                ) ;with
              ) ;let*
            ) ;with
          ) ;let*
        ) ;when
      ) ;let*
    ) ;when
  ) ;let*
) ;tm-define


(tm-define (kbd-variant t forwards?)
  (:require (and (tree-in? t '(label reference pageref eqref smart-ref)) (cursor-inside? t))
  ) ;:require
  (if (complete-try?) (noop))
) ;tm-define

(tm-define (bib-cite-context? t)
  (and (tree-in? t '(cite nocite cite-detail)) (cursor-inside? t))
) ;tm-define

(tm-define (kbd-variant t forwards?)
  (:require (and (not (supports-db?)) (bib-cite-context? t)))
  (with u
    (current-bib-file #t)
    (with ttxt
      (tree-ref t (cADr (cursor-path)))
      (if (or (url-none? u) (not ttxt))
        (set-message "No completions" "You must add a bibliography file")
        (custom-complete (tm->tree (citekey-completions u ttxt)))
      ) ;if
    ) ;with
  ) ;with
) ;tm-define

(tm-define (kbd-alternate-variant t forwards?)
  (and-with p (tree-outer t) (kbd-alternate-variant p forwards?))
) ;tm-define

(tm-define (kbd-alternate-variant t forwards?)
  (:require (tree-is-buffer? t))
  (make-htab "5mm")
) ;tm-define

(tm-define (kbd-space) (kbd-space-bar (focus-tree) #f))
(tm-define (kbd-shift-space) (kbd-space-bar (focus-tree) #t))
(tm-define (kbd-return) (kbd-enter (focus-tree) #f))
(tm-define (kbd-shift-return) (kbd-enter (focus-tree) #t))
(tm-define (kbd-control-return) (kbd-control-enter (focus-tree) #f))
(tm-define (kbd-shift-control-return) (kbd-control-enter (focus-tree) #t))
(tm-define (kbd-alternate-return) (kbd-alternate-enter (focus-tree) #f))
(tm-define (kbd-shift-alternate-return) (kbd-alternate-enter (focus-tree) #t))
(tm-define (kbd-backspace) (kbd-remove (focus-tree) #f))
(tm-define (kbd-delete) (kbd-remove (focus-tree) #t))
(tm-define (kbd-tab) (kbd-variant (focus-tree) #t))
(tm-define (kbd-shift-tab) (kbd-variant (focus-tree) #f))
(tm-define (kbd-alternate-tab) (kbd-alternate-variant (focus-tree) #t))
(tm-define (kbd-shift-alternate-tab) (kbd-alternate-variant (focus-tree) #f))
(tm-define (kbd-copy) (clipboard-copy "primary"))
(tm-define (kbd-cut) (clipboard-cut "primary"))
(tm-define (kbd-paste)
  (clipboard-paste "primary")
  (when (chat-input-buffer? (current-buffer-url))
    (qt-chat-notify-input-height)
  ) ;when
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "paste")
  ) ;when
) ;tm-define
(tm-define (kbd-paste-verbatim) (clipboard-paste-import "verbatim" "primary"))
(tm-define (kbd-cancel) (clipboard-clear "primary"))

;; ocr-paste
;; 剪贴板中的内容是图像时，OCR并插入已识别的内容到当前光标处
;;
;; 语法
;; ----
;; (ocr-paste)
(tm-define (ocr-paste)
  (when (not (defined? 'ocr-to-latex-by-cursor))
    (use-modules (liii ocr))
  ) ;when
  (with data
    (parse-texmacs-snippet (tree->string (tree-ref (clipboard-get "primary") 1)))
    (when (tree-is? (tree-ref data 0) 'image)
      (ocr-to-latex-by-cursor data)
    ) ;when
  ) ;with
) ;tm-define

;; image-and-ocr-paste
;; 剪贴板中的内容是图像时，OCR并插入图像和已识别的内容到当前光标处。图像和已识别的内容通过回车键分隔。
;;
;; 语法
;; ----
;; (image-and-ocr-paste)
(tm-define (image-and-ocr-paste)
  (with data
    (parse-texmacs-snippet (tree->string (tree-ref (clipboard-get "primary") 1)))
    (when (tree-is? (tree-ref data 0) 'image)
      (kbd-paste)
      (kbd-return)
      (when (not (defined? 'ocr-to-latex-by-cursor))
        (use-modules (liii ocr))
      ) ;when
      (ocr-to-latex-by-cursor data)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (paste-as-html)
  (with source-format
    (qt-clipboard-format)
    (if (string=? source-format "html")
      (let* ((fm (format-determine (qt-clipboard-text) "verbatim")))
        (cond ((string=? fm "html") (clipboard-paste-import "html" "primary"))
              ((string=? fm "latex") (clipboard-paste-import "latex" "primary"))
              ((string=? fm "verbatim") (kbd-paste))
              ((string=? fm "markdown") (paste-as-markdown))
        ) ;cond
      ) ;let*
      (clipboard-paste-import "html" "primary")
    ) ;if
  ) ;with
) ;tm-define

(tm-define (paste-as-markdown)
  (if (community-stem?)
    (begin
      (clipboard-paste-import "verbatim" "primary")
      (kbd-return)
      (let* ((latex-code (string-load (unix->url "$TEXMACS_PATH/plugins/account/data/md.tex"))
             ) ;latex-code
             (parsed-latex (parse-latex latex-code))
             (texmacs-latex (latex->texmacs parsed-latex))
            ) ;
        (insert texmacs-latex)
      ) ;let*
    ) ;begin
    (clipboard-paste-import "markdown" "primary")
  ) ;if
) ;tm-define

;; paste-as-texmacs
;; 期望以texmacs格式粘贴
;;
;; 语法
;; (paste-as-texmacs)

(tm-define (paste-as-texmacs)
  (when (not (defined? 'ocr-to-latex-by-cursor))
    (use-modules (liii ocr))
  ) ;when
  (with img-tree
    (tree-ref (clipboard-get "primary") 1)
    (cond ((tree-is? img-tree 'image) (ocr-to-latex-by-cursor img-tree))
          ((and (tree-is? img-tree 'with) (not (null? (tree-ref img-tree 2))))
           (let* ((sub-img-tree (tree-ref img-tree 2)))
             (when (tree-is? sub-img-tree 'image)
               (ocr-to-latex-by-cursor img-tree)
             ) ;when
           ) ;let*
          ) ;
    ) ;cond
  ) ;with
) ;tm-define

;; smart-format-paste
;; 智能格式粘贴，对剪贴板格式进行检测，按照对应格式进行粘贴
;;
;; 语法
;; (smart-format-paste)

(tm-define (smart-format-paste)
  (with source-format
    (qt-clipboard-format)
    (cond ((or (string=? source-format "verbatim") (string=? source-format "html"))
           (let* ((fm (format-determine (qt-clipboard-text) "verbatim")))
             (cond ((string=? fm "html") (clipboard-paste-import "html" "primary"))
                   ((string=? fm "latex") (clipboard-paste-import "latex" "primary"))
                   ((string=? fm "verbatim") (kbd-paste))
                   ((string=? fm "markdown") (paste-as-markdown))
             ) ;cond
           ) ;let*
          ) ;
          ((string=? source-format "texmacs-snippet") (paste-as-texmacs))
          (else (kbd-paste-verbatim))
    ) ;cond
  ) ;with
) ;tm-define

;; kbd-magic-paste
;; 魔法粘贴。通过`Ctrl+Shift+v`或者`编辑->魔法粘贴`触发，能够根据粘贴内容和当前模式，切换粘贴的方式。
;;
;; 语法
;; ----
;; (kbd-magic-paste)
;;
;; 逻辑
;; ----
;; 如果剪贴板中的内容是图像，那么调用`(ocr-paste)`，先OCR然后粘贴。否则：
;; 1. 代码模式：将剪贴板中的内容粘贴为代码块
;; 2. 数学模式：将剪贴板中的内容作为LaTeX格式粘贴
;; 3. 文本模式：将剪贴板中的内容作为纯文本粘贴
;;
;; TODO: 在文本模式中，可以自动识别剪贴板中的内容，并魔法粘贴。比如，内容格式经过识别，发现是LaTeX格式，
;; 那么应该粘贴为LaTeX格式
(tm-define (check-magic-paste)
  (when (not (defined? 'account-load-token))
    (use-modules (liii account))
  ) ;when
  (let* ((token (account-load-token))
         (base-url (current-stem-site))
         (check-url (string-append base-url "/api/v1/oauth2/magicPaste/check"))
         (headers (list (cons "Authorization" (string-append "Bearer " token))
                    (cons "Content-Type" "application/json")
                  ) ;list
         ) ;headers
        ) ;
    (if (string=? token "")
      "not-logged-in"
      (catch #t
        (lambda ()
          (let* ((r (http-post check-url '() "{}" headers)) (status (r 'status-code)))
            (cond ((= status 200) "allowed")
                  ((= status 401) "not-logged-in")
                  ((= status 403) "limit-exceeded")
                  (else "allowed")
            ) ;cond
          ) ;let*
        ) ;lambda
        (lambda (key . args) "allowed")
      ) ;catch
    ) ;if
  ) ;let*
) ;tm-define

(tm-widget (magic-paste-login-widget cmd)
  (padded (text "Please log in to use Magic Paste")
    ======
    (centered (explicit-buttons ("Login" (cmd "ok"))))
  ) ;padded
) ;tm-widget

(tm-widget (magic-paste-upgrade-widget cmd)
  (padded (text "Daily Magic Paste limit reached. Upgrade for unlimited access.")
    ======
    (centered (explicit-buttons ("Upgrade" (cmd "ok"))))
  ) ;padded
) ;tm-widget

(define (show-magic-paste-login-dialog)
  (dialogue-window magic-paste-login-widget
    (lambda (answ) (when (== answ "ok") (login)))
    "Magic Paste"
  ) ;dialogue-window
) ;define

(define (show-magic-paste-upgrade-dialog)
  (dialogue-window magic-paste-upgrade-widget
    (lambda (answ) (when (== answ "ok") (open-pricing-url)))
    "Magic Paste"
  ) ;dialogue-window
) ;define

(tm-define (with-magic-paste-check cont)
  (if (community-stem?)
    (cont)
    (let ((result (check-magic-paste)))
      (cond ((== result "allowed") (cont))
            ((== result "not-logged-in") (show-magic-paste-login-dialog))
            ((== result "limit-exceeded") (show-magic-paste-upgrade-dialog))
      ) ;cond
    ) ;let
  ) ;if
) ;tm-define

(tm-define (kbd-magic-paste)
  (if (string-starts? (qt-clipboard-format) "image")
    (begin
      (ocr-paste)
      (track-event "OCR_RECOGNIZE" '(("mode" . "paste")))
    ) ;begin
    (with-magic-paste-check (lambda ()
                              (with mode
                                (get-env "mode")
                                (cond ((== mode "prog")
                                       (clipboard-paste-import "code" "primary")
                                       (track-event "MAGIC_PASTE" '(("mode"
                                                                     . "prog")))
                                      ) ;
                                      ((== mode "math")
                                       (clipboard-paste-import "latex" "primary")
                                       (track-event "MAGIC_PASTE" '(("mode"
                                                                     . "math")))
                                      ) ;
                                      (else (smart-format-paste) (track-event "MAGIC_PASTE" '(("mode"
                                                                                               . "text"))))
                                ) ;cond
                              ) ;with
                            ) ;lambda
    ) ;with-magic-paste-check
  ) ;if
  (when (chat-input-buffer? (current-buffer-url))
    (qt-chat-notify-input-height)
  ) ;when
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "ocr-paste")
  ) ;when
) ;tm-define

(tm-define (any-image-context?)
  (tree-innermost (lambda (t) (tree-is? t 'image)) #t)
) ;tm-define

(tm-define (at-image-start?)
  (with image
    (any-image-context?)
    (and image
      (let* ((p (cursor-path)) (ip (tree->path image)))
        (or (== p ip) (and (== (cDr p) ip) (<= (cAr p) 1)))
      ) ;let*
    ) ;and
  ) ;with
) ;tm-define

(tm-define (notify-activated t) (noop))
(tm-define (notify-disactivated t) (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic gestures
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (swipe-horizontal t forward?)
  (and-with p (tree-outer t) (swipe-horizontal p forward?))
) ;tm-define

(tm-define (swipe-vertical t down?)
  (and-with p (tree-outer t) (swipe-vertical p down?))
) ;tm-define

(tm-define (swipe-left) (swipe-horizontal (focus-tree) #f))

(tm-define (swipe-right) (swipe-horizontal (focus-tree) #t))

(tm-define (swipe-up) (swipe-vertical (focus-tree) #f))

(tm-define (swipe-down) (swipe-vertical (focus-tree) #t))

(tm-define pinch-modified? #f)
(tm-define pinch-current-scale 1.0)
(tm-define pinch-current-angle 0.0)
(tm-define pinch-initial-zoom 1.0)

(tm-define (pinch-clear)
  (set! pinch-modified? #f)
  (set! pinch-current-scale 1.0)
  (set! pinch-current-angle 0.0)
  (set! pinch-initial-zoom 1.0)
) ;tm-define

(tm-define (structured-maximize t)
  (and-with p (tree-outer t) (structured-maximize p))
) ;tm-define

(tm-define (structured-minimize t)
  (and-with p (tree-outer t) (structured-minimize p))
) ;tm-define

(tm-define (pinch-start)
  (pinch-clear)
  (set! pinch-initial-zoom (get-window-zoom-factor))
) ;tm-define

(tm-define (pinch-end)
  (cond ((> pinch-current-scale 1.05) (structured-maximize (focus-tree)))
        ((< pinch-current-scale 0.95) (structured-minimize (focus-tree)))
  ) ;cond
  (pinch-clear)
) ;tm-define

(tm-define (pinch-scale scale)
  (set! pinch-current-scale scale)
  (let* ((lg (/ (log scale) (log 2.0)))
         (lg* (/ (round (* 24.0 lg)) 24.0))
         (normalized-scale (exp (* (log 2.0) lg*)))
         (new-zoom (* normalized-scale pinch-initial-zoom))
        ) ;
    (change-zoom-factor new-zoom)
  ) ;let*
) ;tm-define

(tm-define (pinch-rotate angle) (geometry-rotate (focus-tree) angle))

(tm-define (wheel-capture?) #f)
(tm-define (wheel-event x y) (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic predicates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (simple-tags) '(concat document tformat table row cell shown hidden))

(tm-define (complex-context? t)
  (and (nleaf? t) (nin? (tree-label t) (simple-tags)))
) ;tm-define

(tm-define (simple-context? t)
  (or (leaf? t) (and (tree-in? t (simple-tags)) (simple-context? (tree-down t))))
) ;tm-define

(tm-define (document-context? t) (tree-is? t 'document))

(tm-define (table-markup-context? t)
  (or (tree-in? t '(table tformat))
    (and (== (tree-arity t) 1)
      (or (tree-in? (tree-ref t 0) '(table tformat))
        (and (tm-func? (tree-ref t 0) 'document 1)
          (tree-in? (tree-ref t 0 0) '(table tformat))
        ) ;and
      ) ;or
    ) ;and
  ) ;or
) ;tm-define

(tm-define (structured-horizontal? t)
  (or (tree-is-dynamic? t) (table-markup-context? t))
) ;tm-define

(tm-define (structured-vertical? t)
  (or (tree-in? t '(tree))
    (table-markup-context? t)
    (list-structured-insert-context?)
  ) ;or
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus predicates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-has-variants? t) (> (length (focus-variants-of t)) 1))

(tm-define (focus-has-toggles? t)
  (or (numbered-context? t) (alternate-context? t))
) ;tm-define

(tm-define (focus-can-move? t) #t)

(tm-define (focus-can-insert-remove? t)
  (and (or (structured-horizontal? t) (structured-vertical? t))
    (or (cursor-inside? t) (in? (tree-label t) '(item item*)))
  ) ;and
) ;tm-define

(tm-define (focus-can-insert? t) (< (tree-arity t) (tree-maximal-arity t)))

(tm-define (focus-can-remove? t) (> (tree-arity t) (tree-minimal-arity t)))

(tm-define (focus-has-geometry? t) #f)

(tm-define (focus-has-preferences? t)
  (and (tree-compound? t) (tree-label-extension? (tree-label t)))
) ;tm-define

(tm-define (focus-has-preferences? t)
  (:require (tree-in? t '(reference pageref
                           eqref
                           smart-ref
                           hlink
                           locus
                           ornament))
  ) ;:require
  #t
) ;tm-define

(tm-define (focus-has-preferences? t)
  (:require (tree-in? t '(bibliography bibliography* thebibliography)))
  #t
) ;tm-define

(tm-define (focus-has-parameters? t) (focus-has-preferences? t))

(tm-define (focus-can-search? t) #f)
(tm-define (focus-has-search-menu? t) #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tree traversal
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (traverse-horizontal t forwards?)
  (if forwards? (go-to-next-word) (go-to-previous-word))
) ;tm-define

(tm-define (traverse-vertical t downwards?)
  (and-with p (tree-outer t) (traverse-vertical p downwards?))
) ;tm-define

(tm-define (traverse-vertical t downwards?)
  (:require (document-context? t))
  (with move (if downwards? go-to-next-tag go-to-previous-tag) (move 'document))
) ;tm-define

(define (find-similar-upwards t l)
  (cond ((in? (tree-label t) l) t)
        ((and (not (tree-is-buffer? t)) (tree-up t))
         (find-similar-upwards (tree-up t) l)
        ) ;
        (else #f)
  ) ;cond
) ;define

(define-macro (with-focus-in l . body)
  `(begin
     ,@body
     (selection-cancel)
     (and-with t (find-similar-upwards (focus-tree) ,l) (tree-focus t)))
) ;define-macro

(tm-define (traverse-incremental t forwards?)
  (let* ((l (similar-to (tree-label t)))
         (fun (if forwards? go-to-next-tag go-to-previous-tag))
        ) ;
    (with-focus-in l (fun l))
  ) ;let*
) ;tm-define

(tm-define (traverse-extremal t forwards?)
  (let* ((l (similar-to (tree-label t)))
         (fun (if forwards? go-to-next-tag go-to-previous-tag))
         (inc (lambda () (fun l)))
        ) ;
    (with-focus-in l (go-to-repeat inc) (structured-inner-extremal t forwards?))
  ) ;let*
) ;tm-define

(tm-define (traverse-previous) (traverse-incremental (focus-tree) #f))
(tm-define (traverse-next) (traverse-incremental (focus-tree) #t))
(tm-define (traverse-first) (traverse-extremal (focus-tree) #f))
(tm-define (traverse-last) (traverse-extremal (focus-tree) #t))
(tm-define (traverse-left) (traverse-horizontal (focus-tree) #f))
(tm-define (traverse-right) (traverse-horizontal (focus-tree) #t))
(tm-define (traverse-up) (traverse-vertical (focus-tree) #f))
(tm-define (traverse-down) (traverse-vertical (focus-tree) #t))
(tm-define (traverse-previous-section-title)
  (go-to-previous-tag (similar-to 'section))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured insert and remove
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-insert-horizontal t forwards?)
  (and-with p (tree-outer t) (structured-insert-horizontal p forwards?))
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (and-with p (tree-outer t) (structured-insert-vertical p downwards?))
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (and-with p (tree-outer t) (structured-remove-horizontal p forwards?))
) ;tm-define

(tm-define (structured-remove-vertical t downwards?)
  (and-with p (tree-outer t) (structured-remove-vertical p downwards?))
) ;tm-define

(tm-define (structured-insert-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (when (tree->path t :down)
    (insert-argument-at (tree->path t :down) forwards?)
  ) ;when
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (when (tree->path t :down)
    (remove-argument-at (tree->path t :down) forwards?)
  ) ;when
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (list-structured-insert-context?))
  (let ((item (current-list-item-marker)))
    (call-with-values (lambda () (find-item-wrapper-and-list item))
      (lambda (wrapper item-list)
        (when (and item item-list (tree-is? item-list 'document))
          (let* ((item-index (if wrapper (tree-index wrapper) (tree-index item)))
                 (list-type (list-family (or (get-current-list-label item) 'enumerate)))
                 (insert-pos (list-item-insert-index item-list item-index list-type downwards?))
                ) ;
            (let* ((new-list (tree-insert item-list insert-pos (list (blank-list-item-stree list-type)))
                   ) ;new-list
                   (new-item (tree-ref new-list insert-pos))
                  ) ;
              (if (and (tree-is? new-item 'concat) (> (tree-arity new-item) 1))
                (tree-go-to new-item 1 :end)
                (if (tree-is? new-item 'item*)
                  (tree-go-to new-item 0 :start)
                  (tree-go-to new-list insert-pos :end)
                ) ;if
              ) ;if
            ) ;let*
          ) ;let*
        ) ;when
      ) ;lambda
    ) ;call-with-values
  ) ;let
) ;tm-define

(tm-define (structured-remove-vertical t downwards?)
  (:require (list-structured-insert-context?))
  (let ((item (current-list-item-marker)))
    (call-with-values (lambda () (find-item-wrapper-and-list item))
      (lambda (wrapper item-list)
        (when (and item item-list (tree-is? item-list 'document))
          (let* ((item-index (if wrapper (tree-index wrapper) (tree-index item)))
                 (list-type (list-family (or (get-current-list-label item) 'enumerate)))
                 (range (list-item-remove-range item-list item-index list-type downwards?))
                ) ;
            (when range
              (let* ((new-list (remove-list-item-range item-list range))
                     (pos (min (car range) (- (tree-arity new-list) 1)))
                    ) ;
                (if (document-empty-for-list? new-list)
                  (let* ((list-parent (tree-up new-list))
                         (parent-doc (and list-parent (tree-up list-parent)))
                         (list-index (and list-parent (tree-index list-parent)))
                        ) ;
                    (when (and parent-doc list-index)
                      (tree-remove parent-doc list-index 1)
                      (if (== (tree-arity parent-doc) 0)
                        (begin
                          (tree-insert parent-doc 0 (list ""))
                          (tree-go-to parent-doc 0 :end)
                        ) ;begin
                        (tree-go-to parent-doc list-index :end)
                      ) ;if
                    ) ;when
                  ) ;let*
                  (if (>= pos 0) (tree-go-to new-list pos :end) (tree-go-to new-list :end))
                ) ;if
              ) ;let*
            ) ;when
          ) ;let*
        ) ;when
      ) ;lambda
    ) ;call-with-values
  ) ;let
) ;tm-define

(tm-define (structured-insert-extremal t forwards?)
  (structured-extremal t forwards?)
  (structured-insert-horizontal t forwards?)
) ;tm-define

(tm-define (structured-insert-incremental t downwards?)
  (structured-incremental t downwards?)
  (structured-insert-vertical t downwards?)
) ;tm-define

(tm-define (structured-insert-left)
  (structured-insert-horizontal (focus-tree) #f)
) ;tm-define
(tm-define (structured-insert-right)
  (structured-insert-horizontal (focus-tree) #t)
) ;tm-define
(tm-define (structured-remove-left)
  (structured-remove-horizontal (focus-tree) #f)
) ;tm-define
(tm-define (structured-remove-right)
  (structured-remove-horizontal (focus-tree) #t)
) ;tm-define
(tm-define (structured-insert-up) (structured-insert-vertical (focus-tree) #f))
(tm-define (structured-insert-down)
  (structured-insert-vertical (focus-tree) #t)
) ;tm-define
(tm-define (structured-remove-up) (structured-remove-vertical (focus-tree) #f))
(tm-define (structured-remove-down)
  (structured-remove-vertical (focus-tree) #t)
) ;tm-define
(tm-define (structured-insert-start)
  (structured-insert-extremal (focus-tree) #f)
) ;tm-define
(tm-define (structured-insert-end) (structured-insert-extremal (focus-tree) #t))
(tm-define (structured-insert-top)
  (structured-insert-incremental (focus-tree) #f)
) ;tm-define
(tm-define (structured-insert-bottom)
  (structured-insert-incremental (focus-tree) #t)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured movements
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-horizontal t forwards?)
  (and-with p (tree-outer t) (structured-horizontal p forwards?))
) ;tm-define

(tm-define (structured-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (with-focus-after t
    (when (tree-down t)
      (with move
        (if forwards? path-next-argument path-previous-argument)
        (with p (move (root-tree) (tree->path (tree-down t))) (if (nnull? p) (go-to p)))
      ) ;with
    ) ;when
  ) ;with-focus-after
) ;tm-define

(tm-define (structured-vertical t downwards?)
  (and-with p (tree-outer t) (structured-vertical p downwards?))
) ;tm-define

(tm-define (structured-inner-extremal t forwards?)
  (and-with p (tree-outer t) (structured-inner-extremal p forwards?))
) ;tm-define

(tm-define (structured-inner-extremal t forwards?)
  (:require (structured-horizontal? t))
  (with-focus-after t (tree-go-to t :down (if forwards? :end :start)))
) ;tm-define

(tm-define (structured-extremal t forwards?)
  (go-to-repeat (lambda () (structured-horizontal t forwards?)))
  (structured-inner-extremal t forwards?)
) ;tm-define

(tm-define (structured-incremental t downwards?)
  (go-to-repeat (lambda () (structured-vertical t downwards?)))
  (structured-inner-extremal t downwards?)
) ;tm-define

(tm-define (structured-exit t forwards?)
  (when (complex-context? t)
    (tree-go-to t (if forwards? :end :start))
  ) ;when
) ;tm-define

(tm-define (structured-left) (structured-horizontal (focus-tree) #f))
(tm-define (structured-right) (structured-horizontal (focus-tree) #t))
(tm-define (structured-up) (structured-vertical (focus-tree) #f))
(tm-define (structured-down) (structured-vertical (focus-tree) #t))
(tm-define (structured-start) (structured-extremal (focus-tree) #f))
(tm-define (structured-end) (structured-extremal (focus-tree) #t))
(tm-define (structured-top) (structured-incremental (focus-tree) #f))
(tm-define (structured-bottom) (structured-incremental (focus-tree) #t))
(tm-define (structured-exit-left) (structured-exit (focus-tree) #f))
(tm-define (structured-exit-right) (structured-exit (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Multi-purpose alignment
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (geometry-speed t down?)
  (and-with p (tree-outer t) (geometry-speed p down?))
) ;tm-define

(tm-define (geometry-variant t forwards?)
  (and-with p (tree-outer t) (geometry-variant p forwards?))
) ;tm-define

(tm-define (geometry-default t)
  (and-with p (tree-outer t) (geometry-default p))
) ;tm-define

(tm-define (geometry-horizontal t forwards?)
  (and-with p (tree-outer t) (geometry-horizontal p forwards?))
) ;tm-define

(tm-define (geometry-vertical t down?)
  (and-with p (tree-outer t) (geometry-vertical p down?))
) ;tm-define

(tm-define (geometry-extremal t forwards?)
  (and-with p (tree-outer t) (geometry-extremal p forwards?))
) ;tm-define

(tm-define (geometry-incremental t down?)
  (and-with p (tree-outer t) (geometry-incremental p down?))
) ;tm-define

(tm-define (geometry-scale t scale)
  (with p
    (tree-outer t)
    (if p (geometry-scale p scale) (set! pinch-current-scale scale))
  ) ;with
) ;tm-define

(tm-define (geometry-rotate t angle)
  (with p
    (tree-outer t)
    (if p (geometry-rotate p angle) (set! pinch-current-angle angle))
  ) ;with
) ;tm-define

(tm-define (geometry-slower) (geometry-speed (focus-tree) #f))
(tm-define (geometry-faster) (geometry-speed (focus-tree) #t))
(tm-define (geometry-circulate forwards?)
  (geometry-variant (focus-tree) forwards?)
) ;tm-define
(tm-define (geometry-reset) (geometry-default (focus-tree)))
(tm-define (geometry-left) (geometry-horizontal (focus-tree) #f))
(tm-define (geometry-right) (geometry-horizontal (focus-tree) #t))
(tm-define (geometry-up) (geometry-vertical (focus-tree) #f))
(tm-define (geometry-down) (geometry-vertical (focus-tree) #t))
(tm-define (geometry-start) (geometry-extremal (focus-tree) #f))
(tm-define (geometry-end) (geometry-extremal (focus-tree) #t))
(tm-define (geometry-top) (geometry-incremental (focus-tree) #f))
(tm-define (geometry-bottom) (geometry-incremental (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special structured editing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (special-navigate t direction)
  (and-with p (tree-outer t) (special-navigate p direction))
) ;tm-define

(tm-define (special-horizontal t forwards?)
  (and-with p (tree-outer t) (special-horizontal p forwards?))
) ;tm-define

(tm-define (special-vertical t down?)
  (and-with p (tree-outer t) (special-vertical p down?))
) ;tm-define

(tm-define (special-extremal t forwards?)
  (and-with p (tree-outer t) (special-extremal p forwards?))
) ;tm-define

(tm-define (special-incremental t down?)
  (and-with p (tree-outer t) (special-incremental p down?))
) ;tm-define

(tm-define (special-back) (special-navigate (focus-tree) :previous))
(tm-define (special-forward) (special-navigate (focus-tree) :next))
(tm-define (special-return) (special-navigate (focus-tree) :first))
(tm-define (special-shift-return) (special-navigate (focus-tree) :last))
(tm-define (special-left) (special-horizontal (focus-tree) #f))
(tm-define (special-right) (special-horizontal (focus-tree) #t))
(tm-define (special-up) (special-vertical (focus-tree) #f))
(tm-define (special-down) (special-vertical (focus-tree) #t))
(tm-define (special-first) (special-extremal (focus-tree) #f))
(tm-define (special-last) (special-extremal (focus-tree) #t))
(tm-define (special-previous) (special-incremental (focus-tree) #f))
(tm-define (special-next) (special-incremental (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tree editing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-insert-horizontal t forwards?)
  (:require (tree-is? t 'tree))
  (if (== (tree-down-index t) 0) (set! t (tree-up t)))
  (if (== (tm-car t) 'tree)
    (with pos
      (tree-down-index t)
      (if forwards? (set! pos (1+ pos)))
      (tree-insert! t pos '(""))
      (tree-go-to t pos 0)
    ) ;with
  ) ;if
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (:require (tree-is? t 'tree))
  (if (== (tree-down-index t) 0) (set! t (tree-up t)))
  (if (== (tm-car t) 'tree)
    (with pos
      (tree-down-index t)
      (cond (forwards? (tree-remove! t pos 1)
              (if (== pos (tree-arity t)) (tree-go-to t :end) (tree-go-to t pos :start))
            ) ;forwards?
            ((== pos 1) (tree-go-to t 0 :end))
            (else (tree-remove! t (- pos 1) 1))
      ) ;cond
    ) ;with
  ) ;if
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (tree-is? t 'tree))
  (if downwards?
    (if (== (tree-down-index t) 0)
      (with pos (tree-arity t) (tree-insert! t pos '("")) (tree-go-to t pos 0))
      (begin
        (set! t (tree-down t))
        (tree-set! t `(tree ,t ,""))
        (tree-go-to t 1 0)
      ) ;begin
    ) ;if
    (begin
      (if (!= (tree-down-index t) 0) (set! t (tree-down t)))
      (tree-set! t `(tree ,"" ,t))
      (tree-go-to t 0 0)
    ) ;begin
  ) ;if
) ;tm-define

(define (branch-active t)
  (with i
    (tree-down-index t)
    (if (and (= i 0) (tree-is? t :up 'tree)) (tree-up t) t)
  ) ;with
) ;define

(define (branch-go-to . l)
  (apply tree-go-to l)
  (if (tree-is? (cursor-tree) 'tree)
    (with last
      (cAr l)
      (if (nin? last '(:start :end)) (set! last :start))
      (tree-go-to (cursor-tree) 0 last)
    ) ;with
  ) ;if
) ;define

(tm-define (structured-horizontal t* forwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*)) (i (tree-down-index t)))
    (cond ((and (not forwards?) (> i 1)) (branch-go-to t (- i 1) :end))
          ((and forwards? (!= i 0) (< i (- (tree-arity t) 1)))
           (branch-go-to t (+ i 1) :start)
          ) ;
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (structured-vertical t* downwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*)) (i (tree-down-index t)))
    (cond ((and (not downwards?) (!= i 0)) (tree-go-to t 0 :end))
          ((and downwards? (== (tree-down-index t*) 0))
           (branch-go-to t* (quotient (tree-arity t*) 2) :start)
          ) ;
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (structured-extremal t* forwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*)) (i (tree-down-index t)))
    (cond ((not forwards?) (branch-go-to t 1 :start))
          (forwards? (branch-go-to t :last :end))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (structured-incremental t downwards?)
  (:require (tree-is? t 'tree))
  (go-to-repeat (if downwards? structured-down structured-up))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Extra editing functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kill-paragraph)
  (selection-set-start)
  (go-end-paragraph)
  (selection-set-end)
  (clipboard-cut "primary")
) ;tm-define

(tm-define (backward-kill-word) (kbd-select traverse-left) (kbd-delete))

(tm-define (kill-word) (kbd-select traverse-right) (kbd-delete))

(tm-define (yank-paragraph)
  (selection-set-start)
  (go-end-paragraph)
  (selection-set-end)
  (clipboard-copy "primary")
) ;tm-define

(tm-define (select-all)
  (let ((t (tree-ref (buffer-tree) 0)))
    ;; fix-me: 目前只针对一层 'hide-preamble 作了特殊处理
    ;;         最好改进为更加 general 的处理方式
    (if (and (tree-is? t 'hide-preamble)
          (= (tree-arity t) 1)
          (tree-is? (tree-ref t 0) 'document)
        ) ;and
      (select-all-correct 1)
      (tree-select (buffer-tree))
    ) ;if
  ) ;let
) ;tm-define

(tm-define (go-to-line n . opt-from)
  (if (nnull? opt-from) (cursor-history-add (car opt-from)))
  (with-innermost t 'document (tree-go-to t n 0))
) ;tm-define

(tm-define (go-to-column c . opt-from)
  (if (nnull? opt-from) (cursor-history-add (car opt-from)))
  (with-innermost t
    'document
    (with p (tree-cursor-path t) (tree-go-to t (cADr p) c))
  ) ;with-innermost
) ;tm-define

(tm-define (select-word w t col)
  (:synopsis "Selects word @w in tree @t, more or less around column @col")
  (let* ((st (tree->string t))
         (pos (- col (string-length w)))
         (beg (string-search-forwards w (max 0 pos) st))
        ) ;
    (if beg
      (with p
        (tree->path t)
        (go-to (rcons p beg))
        (selection-set-start)
        (go-to (rcons p (+ beg (string-length w))))
        (selection-set-end)
      ) ;with
    ) ;if
    beg
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard environment parameters for primitives
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (standard-parameters l)
  (:require (== l "action"))
  (list "locus-color")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "locus"))
  (list "locus-color" "visited-color")
) ;tm-define

(tm-define (standard-parameters l)
  (:require (== l "ornament"))
  (list "ornament-shape"
    "ornament-title-style"
    "ornament-border"
    "ornament-corner"
    "ornament-hpadding"
    "ornament-vpadding"
    "ornament-color"
    "ornament-extra-color"
    "ornament-sunny-color"
    "ornament-shadow-color"
  ) ;list
) ;tm-define

(tm-define (standard-parameters l)
  (:require (in? l '("reference" "pageref" "eqref" "smart-ref" "label" "tag")))
  (list)
) ;tm-define

(tm-define (standard-parameters l)
  (:require (in? l '("bibliography" "bibliography*" "thebibliography")))
  (list "bib-no-translate")
) ;tm-define

(tm-define (search-parameters l)
  (:require (in? (if (string? l) l (symbol->string l))
              '("reference" "pageref" "eqref" "smart-ref" "hlink")
            ) ;in?
  ) ;:require
  (standard-parameters "locus")
) ;tm-define

(tm-define (parameter-choice-list l)
  (:require (== l "ornament-shape"))
  (list "classic"
    "rounded"
    "angular"
    "cartoon"
    ;; "ring"
  ) ;list
) ;tm-define

(tm-define (parameter-choice-list l)
  (:require (== l "ornament-title-style"))
  (list "classic"
    "top left"
    "top center"
    "top right"
    "bottom left"
    "bottom center"
    "bottom right"
  ) ;list
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Inserting various kinds of content
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (label-insert t) (and-with p (tree-outer t) (label-insert p)))

(tm-define (label-insert t) (:require (tree-is-buffer? t)) (make 'label))

(tm-define (make-label) (label-insert (focus-tree)))

(tm-define (make-specific s)
  (if (or (== s "texmacs") (in-source?))
    (insert-go-to `(specific ,s ,"") '(1 0))
    (insert-go-to `(inactive (specific ,s ,"")) '(0 1 0))
  ) ;if
) ;tm-define

(tm-define (make-include u)
  (let ((delta-unix (url->delta-unix u)))
    (if delta-unix
      (insert `(include ,(utf8->cork delta-unix)))
      (set-message `(concat ,(translate "Unable to include file from another drive: ")
                      (verbatim ,(url->string u))
                      ,"")
        (translate "include file")
      ) ;set-message
    ) ;if
  ) ;let
) ;tm-define

(tm-define (make-inline-image l)
  (apply make-image (cons* (url->system (car l)) #f (cdr l)))
) ;tm-define

(tm-define (make-link-image l)
  (let ((delta-unix (url->delta-unix (car l))))
    (if delta-unix
      (apply make-image (cons* delta-unix #t (cdr l)))
      (set-message `(concat ,(translate "Unable to link images from another drive: ")
                      (verbatim ,(url->string (car l)))
                      ,"")
        (translate "link image")
      ) ;set-message
    ) ;if
  ) ;let
) ;tm-define

(tm-define (make-graphics-over-selection)
  (when (selection-active-any?)
    (with selection
      (selection-tree)
      (clipboard-cut "graphics background")
      (insert-go-to `(draw-over ,selection (graphics) ,"0cm") '(1 1))
    ) ;with
  ) ;when
) ;tm-define

(tm-define (make-graphics-over)
  (if (selection-active-any?)
    (with g
      '(with "gr-mode" (tuple "hand-edit" "penscript") (graphics))
      (with selection
        (selection-tree)
        (clipboard-cut "graphics background")
        (insert-go-to `(draw-over ,selection ,g ,"2cm") '(1 2 1))
      ) ;with
    ) ;with
    (with g
      '(with "gr-mode"
         (tuple "hand-edit" "penscript")
         "gr-grid"
         (tuple "cartesian" (point "0" "0") "2")
         "gr-edit-grid-aspect"
         (tuple (tuple "axes" "none") (tuple "1" "none") (tuple "10" "none"))
         "gr-edit-grid"
         (tuple "cartesian" (point "0" "0") "1")
         (graphics))
      (insert-go-to `(draw-over ,"" ,g ,"2cm") '(1 2 1))
    ) ;with
  ) ;if
) ;tm-define

(tm-define (make-anim l)
  (with duration
    "1s"
    (if (selection-active?)
      (let* ((selection (selection-tree)) (p (path-end selection (list))))
        (when (selection-active-large?)
          (set! selection `(par-block ,selection))
          (set! p (cons 0 p))
        ) ;when
        (clipboard-cut "graphics background")
        (insert-go-to `(,l ,selection ,duration) (cons 0 p))
      ) ;let*
      (insert-go-to `(,l ,"" ,duration) (list 0 0))
    ) ;if
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Detached notes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (propose-note-id ref?)
  (let* ((buf (buffer-tree))
         (is-ref? (cut tree-in? <> '(note-ref note-ref*)))
         (is-text? (cut tree-in?
                     <>
                     '(note-inline note-inline*
                        note-wide
                        note-wide*
                        note-footnote
                        note-footnote*)
                   ) ;cut
         ) ;is-text?
         (ref-l (tree-search buf is-ref?))
         (text-l (tree-search buf is-text?))
         (ref-id (lambda (t) (tree->stree (tm-ref t 0))))
         (text-id (lambda (t) (tree->stree (tm-ref t 1))))
         (refs (map ref-id ref-l))
         (texts (map text-id text-l))
         (diff (if ref? (list-difference texts refs) (list-difference refs texts)))
        ) ;
    (if (null? diff) (create-unique-id) (cAr diff))
  ) ;let*
) ;define

(tm-define (make-note-ref) (insert `(note-ref ,(propose-note-id #t))))

(tm-define (make-note-inline)
  (insert-go-to `(note-inline ,"" ,(propose-note-id #f)) '(0 0))
) ;tm-define

(tm-define (make-note-wide)
  (insert-go-to `(note-wide (document "") ,(propose-note-id #f)) '(0 0 0))
) ;tm-define

(tm-define (make-note-footnote)
  (insert-go-to `(note-footnote (document "") ,(propose-note-id #f)) '(0 0 0))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Thumbnails facility
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (thumbnail-suffixes)
  (list->url (map url-wildcard '("*.gif"
                                 "*.jpg"
                                 "*.jpeg"
                                 "*.JPG"
                                 "*.JPEG"
                                 "*.png"
                                 "*.PNG"))
  ) ;list->url
) ;define

(define (fill-row l nr)
  (cond ((= nr 0) '())
        ((nnull? l) (cons (car l) (fill-row (cdr l) (- nr 1))))
        (else (cons "" (fill-row l (- nr 1))))
  ) ;cond
) ;define

(define (make-rows l nr)
  (if (> (length l) nr)
    (cons (list-head l nr) (make-rows (list-tail l nr) nr))
    (list (fill-row l nr))
  ) ;if
) ;define

(define (make-thumbnails-sub l nr)
  (let* ((w (string-append (number->string (- (/ 1.0 nr) 0.02)) "par"))
         (mapper (lambda (x)
                   (and-let* ((delta-unix (url->delta-unix x)))
                     `(image ,delta-unix ,w ,"" ,"" ,"")
                   ) ;and-let*
                 ) ;lambda
         ) ;mapper
         (l1 (map mapper l))
         (l2 (make-rows l1 nr))
         (l3 (map (lambda (r) `(row ,@(map (lambda (c) `(cell ,c)) r))) l2))
        ) ;
    (if l1
      (insert `(tabular* (tformat (twith "table-width" "1par")
                           (twith "table-hyphen" "yes")
                           (table ,@l3)))
      ) ;insert
      (set-message `(concat ,(translate "Unable to make thumbnail from another drive: ")
                      (verbatim ,(url->string (car l)))
                      ,"")
        (translate "make thumbnail")
      ) ;set-message
    ) ;if
  ) ;let*
) ;define

(tm-define (make-thumbnails nr)
  (:argument nr "Number of pictures per row")
  (if (string? nr) (set! nr (min (string->number nr) 32)))
  (user-url "Picture directory"
    "directory"
    (lambda (dir)
      (let* ((find (url-append dir (thumbnail-suffixes)))
             (files (url->list (url-expand (url-complete find "r"))))
             (base (buffer-master))
             (rel-files (map (lambda (x) (url-delta base x)) files))
            ) ;
        (if (nnull? rel-files) (make-thumbnails-sub rel-files nr))
      ) ;let*
    ) ;lambda
  ) ;user-url
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Routines for floats
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (mini-flow-context? t) (tree-in? t (mini-flow-tag-list)))

(tm-define (in-main-flow?)
  (:synopsis "Are we inside the main document flow?")
  ;; FIXME: this routine can be improved quite a lot
  ;; we might make this property part of the DRD
  (not (tree-innermost mini-flow-context?))
) ;tm-define

(tm-define (make-marginal-note)
  (:synopsis "Insert a marginal note")
  (wrap-selection-small (insert-go-to '(marginal-note "normal" "c" "") '(2 0)))
) ;tm-define

(tm-define (test-marginal-note-hpos? hp)
  (and-with t (tree-innermost 'marginal-note #t) (tm-equal? (tree-ref t 0) hp))
) ;tm-define
(tm-define (set-marginal-note-hpos hp)
  (:synopsis "Set the horizontal position of the marginal note to @hp")
  (:check-mark "v" test-marginal-note-hpos?)
  (and-with t (tree-innermost 'marginal-note #t) (tree-set t 0 hp))
) ;tm-define

(tm-define (test-marginal-note-valign? va)
  (and-with t (tree-innermost 'marginal-note #t) (tm-equal? (tree-ref t 1) va))
) ;tm-define
(tm-define (set-marginal-note-valign va)
  (:synopsis "Set the vertical alignment of the marginal note to @va")
  (:check-mark "v" test-marginal-note-valign?)
  (and-with t (tree-innermost 'marginal-note #t) (tree-set t 1 va))
) ;tm-define

(tm-define (make-insertion s)
  (:synopsis "Make an insertion of type @s")
  (:applicable (in-main-flow?))
  (with pos
    (if (== s "float") "tbh" "")
    (insert-go-to (list 'float s pos (list 'document "")) (list 2 0 0))
  ) ;with
) ;tm-define

(define (any-float? t)
  (tree-in? t '(float wide-float phantom-float))
) ;define

(tm-define (insertion-positioning what flag)
  (:synopsis "Allow/disallow the position @what for innermost float")
  (and-with t
    (tree-innermost any-float? #t)
    (let ((op (if flag string-union string-minus)) (st (tree-ref t 1)))
      (tree-set! st (op (tree->string st) what))
    ) ;let
  ) ;and-with
) ;tm-define

(define (test-insertion-positioning? what)
  (and-with t
    (tree-innermost any-float? #t)
    (with c (string-ref what 0) (char-in-string? c (tree->string (tree-ref t 1))))
  ) ;and-with
) ;define

(define (not-test-insertion-positioning? s)
  (not (test-insertion-positioning? s))
) ;define

(tm-define (toggle-insertion-positioning what)
  (:check-mark "v" test-insertion-positioning?)
  (insertion-positioning what (not-test-insertion-positioning? what))
) ;tm-define

(tm-define (toggle-insertion-positioning-not s)
  (:check-mark "v" not-test-insertion-positioning?)
  (toggle-insertion-positioning s)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Balloons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (balloon-context? t) (tree-in? t (balloon-tag-list)))

(define (integer-floor x)
  (inexact->exact (floor x))
) ;define

(tm-define (display-balloon body balloon halign valign type)
  (:secure #t)
  (let* ((kind (or (tm->string type) "default"))
         (ha (or (tm->string halign) (if (== kind "mouse") "right" "left")))
         (va (or (tm->string valign) "Bottom"))
         (p (tree->path body))
         (st (tree->stree body))
         (id (or (list p st) st))
        ) ;
    (show-tooltip id body balloon ha va kind 0.833333)
  ) ;let*
) ;tm-define

(tm-define (make-balloon)
  (:synopsis "Insert a balloon")
  (wrap-selection-small (insert-go-to '(inactive (hover-balloon ""
                                                   ""
                                                   "left"
                                                   "Bottom")) '(0 0 0))
  ) ;wrap-selection-small
) ;tm-define

(tm-define (test-balloon-halign? ha)
  (and-with t (tree-innermost balloon-context? #t) (tm-equal? (tree-ref t 2) ha))
) ;tm-define
(tm-define (set-balloon-halign ha)
  (:synopsis "Set the horizontal alignment of the marginal note to @ha")
  (:check-mark "v" test-balloon-halign?)
  (and-with t (tree-innermost balloon-context? #t) (tree-set t 2 ha))
) ;tm-define

(tm-define (test-balloon-valign? va)
  (and-with t (tree-innermost balloon-context? #t) (tm-equal? (tree-ref t 3) va))
) ;tm-define
(tm-define (set-balloon-valign va)
  (:synopsis "Set the vertical alignment of the marginal note to @va")
  (:check-mark "v" test-balloon-valign?)
  (and-with t (tree-innermost balloon-context? #t) (tree-set t 3 va))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sound and video
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (make-sound u)
  (let ((delta-unix (url->delta-unix u)))
    (cond ((url-none? u)
           (set-message `(concat ,(translate "Unable to make sound which url is none: ")
                           (verbatim ,(url->string u))
                           ,"")
             (translate "make sound")
           ) ;set-message
          ) ;
          ((not delta-unix)
           (set-message `(concat ,(translate "Unable to make sound from another drive: ")
                           (verbatim ,(url->string u))
                           ,"")
             (translate "make sound")
           ) ;set-message
          ) ;
          (else (insert `(sound ,delta-unix)))
    ) ;cond
  ) ;let
) ;tm-define

(tm-define (make-animation u)
  (let ((delta-unix (url->delta-unix u)))
    (if delta-unix
      (interactive (lambda (w h len rep)
                     (if (== rep "no") (set! rep "false"))
                     (insert `(video ,delta-unix ,w ,h ,len ,rep))
                   ) ;lambda
        "Width"
        "Height"
        "Length"
        "Repeat?"
      ) ;interactive
      (set-message `(concat ,(translate "Unable to make animation from another drive: ")
                      (verbatim ,(url->string u))
                      ,"")
        (translate "make animation")
      ) ;set-message
    ) ;if
  ) ;let
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Labels attached to markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-label t) #f)

(tm-define (focus-get-label t)
  (and-with l (focus-label t) (tm->string (tm-ref l 0)))
) ;tm-define

(tm-define (focus-set-label t val)
  (and-with l (focus-label t) (tree-set l 0 val))
) ;tm-define

(tm-define (focus-list-search-label l)
  (and (nnull? l)
    (or (focus-search-label (car l)) (focus-list-search-label (cdr l)))
  ) ;and
) ;tm-define

(tm-define (focus-search-label t)
  (cond ((tm-func? t 'label 1) t)
        ((tm-in? t '(document concat table row cell))
         (focus-list-search-label (tm-children t))
        ) ;
        ((tm-in? t '(tformat with surround)) (focus-search-label (cAr (tm-children t))))
        (else #f)
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special keyboard behaviour when entering hybrid commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (hybrid-kbd-space) (activate-hybrid #f) (insert " "))

(tm-define (hybrid-kbd-curly-left)
  (with-innermost t
    'hybrid
    (with cmd
      (tm->string (tm-ref t 0))
      (cond ((or (not cmd) (== cmd "begin")) (insert "{"))
            ((in? cmd '("left\\" "right\\")) (insert "{") (activate-hybrid #f))
            (else (activate-hybrid #f))
      ) ;cond
    ) ;with
  ) ;with-innermost
) ;tm-define

(tm-define (hybrid-kbd-curly-right)
  (with-innermost t
    'hybrid
    (with cmd
      (tm->string (tm-ref t 0))
      (cond ((not cmd) (activate-hybrid #f))
            ((string-starts? (tm->string cmd) "begin{")
             (tree-remove (tm-ref t 0) 0 6)
             (activate-hybrid #f)
            ) ;
            ((in? cmd '("left\\" "right\\")) (insert "}") (activate-hybrid #f))
            (else (activate-hybrid #f))
      ) ;cond
    ) ;with
  ) ;with-innermost
) ;tm-define

(tm-define (hybrid-kbd-backslash)
  (with-innermost t
    'hybrid
    (with cmd
      (tm->string (tm-ref t 0))
      (cond ((in? cmd '("left" "right")) (insert "\\"))
            (else (activate-hybrid #f) (make-hybrid))
      ) ;cond
    ) ;with
  ) ;with-innermost
) ;tm-define

(tm-define (hybrid-kbd-sub) (activate-hybrid #f) (make-script #f #t))

(tm-define (hybrid-kbd-sup) (activate-hybrid #f) (make-script #t #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Search, replace, spell and tab-completion
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (key-press-command key)
  ;; FIXME: this routine should do exactly the same as key-press,
  ;; without modification of the internal state and without executing
  ;; the actual shortcut. It should rather return a command which
  ;; does all this, or #f
  (and-with p (kbd-find-key-binding key) (car p))
) ;tm-define

(tm-define (keyboard-press key time)
  (:mode search-mode?)
  (with cmd
    (key-press-command (string-append "search " key))
    (cond (cmd (cmd))
          ((key-press-search key) (noop))
          (else (key-press key))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (search-next) (key-press-search "next"))

(tm-define (search-previous) (key-press-search "previous"))

(tm-define (keyboard-press key time)
  (:mode replace-mode?)
  (with cmd
    (key-press-command (string-append "replace " key))
    (cond (cmd (cmd))
          ((key-press-replace key) (noop))
          (else (key-press key))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (keyboard-press key time)
  (:mode spell-mode?)
  (with cmd
    (key-press-command (string-append "spell " key))
    (cond (cmd (cmd))
          ((key-press-spell key) (noop))
          (else (key-press key))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (keyboard-press key time)
  (:mode complete-mode?)
  (with cmd
    (key-press-command (string-append "complete " key))
    (cond (cmd (cmd))
          ((key-press-complete key) (noop))
          (else (key-press key))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (keyboard-press key time)
  (:require (and (in-source?) (not (in-source-mode?))))
  (with cmd
    (key-press-command (string-append "complete " key))
    (cond (cmd (cmd))
          ((key-press-source-complete key) (noop))
          (else (key-press key))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (keyboard-press key time)
  (:mode remote-control-mode?)
  ;; (display* "Press " key "\n")
  (if (ahash-ref remote-control-remap key)
    (begin
      ;; (display* "Remap " (ahash-ref remote-control-remap key) "\n")
      (key-press (ahash-ref remote-control-remap key))
    ) ;begin
    (key-press key)
  ) ;if
) ;tm-define

(tm-define (focus-open-search-tool t) (:interactive #t) (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Marked text utilities
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (get-marked-color)
  (let* ((default-color (if (has-style-package? "dark") "#dc9f4f" "#ffe47f"))
         (color (get-preference "marked-color"))
        ) ;
    (if (or (== color "") (== color "default")) default-color color)
  ) ;let*
) ;tm-define

(tm-define (mark-text)
  (if (selection-active-any?)
    (begin
      (make 'marked)
      (when (not (== (get-marked-color) "#ffe47f"))
        (with-set (focus-tree) "marked-color" (get-marked-color))
      ) ;when
    ) ;begin
    (make-with "text-bg-color" (get-marked-color))
  ) ;if
) ;tm-define
