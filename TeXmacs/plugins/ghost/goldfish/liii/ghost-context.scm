;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ghost-context.scm
;; DESCRIPTION : Pure context filtering for Ghost Text FIM requests
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; 纯函数：按光标 path 把 TeXmacs stree 切成 (prefix . suffix)，供 FIM 请求。
;; 文本模式：table->"[TABLE]"，image->"[IMAGE]"，公式环境/inline-math->"[FORMULA]"，
;;           其余递归取文本。with 节点只取末尾 body（前面是 key/val 属性对）。
;; 数学模式：按公式内 path 切 stree（LaTeX 转换由 ghost-text.scm 用 convert 完成）。

(define-library (liii ghost-context)
  (export ghost-prefix-limit
          ghost-suffix-limit
          ghost-formula-labels
          ghost-split-text-context
          ghost-split-math-context)
  (import (scheme base)
          (scheme cxr))
  (begin

    (define ghost-prefix-limit 1500)
    (define ghost-suffix-limit 500)

    ;; 公式环境 label 列表，文本模式替换与数学模式定位共用
    (define ghost-formula-labels
      '(equation equation* eqnarray eqnarray* align align* math))

    ;; R7RS base 无 list-head，本地实现
    (define (list-take lst n)
      (if (or (<= n 0) (null? lst)) '() (cons (car lst) (list-take (cdr lst) (- n 1))))
    ) ;define

    ;; 取列表末元素
    (define (list-last lst)
      (if (null? (cdr lst)) (car lst) (list-last (cdr lst))))
     ;define

    ;; ===== 文本模式 =====

    (define (ghost-table-label? lab)
      (memq lab '(table table*)))
     ;define

    (define (ghost-image-label? lab)
      (memq lab '(image postscript graphics draw-over draw-under)))
     ;define

    (define (ghost-formula-label? lab)
      (memq lab ghost-formula-labels))
     ;define

    ;; inline 数学：(with "mode" "math" ...)（属性对可能为字符串或 symbol）
    (define (ghost-inline-math? st)
      (and (pair? st)
           (eq? (car st) 'with)
           (>= (length st) 3)
           (let ((a1 (list-ref st 1)))
             (and (pair? a1)
                  (or (and (string? (car a1)) (string=? (car a1) "mode"))
                      (eq? (car a1) 'mode))
                  (or (and (string? (cadr a1)) (string=? (cadr a1) "math"))
                      (eq? (cadr a1) 'math)))))
    ) ;define

    ;; 子树线性化为纯文本
    (define (ghost-flatten-text st)
      (cond
        ((string? st) st)
        ((null? st) "")
        ((not (pair? st)) "")
        ;; with：inline math 整体替换；否则只 flatten 末尾 body（跳过 key/val 属性对）
        ((eq? (car st) 'with)
         (if (ghost-inline-math? st)
           "[FORMULA]"
           (ghost-flatten-text (list-last st))))
        ((ghost-formula-label? (car st)) "[FORMULA]")
        ((ghost-table-label? (car st)) "[TABLE]")
        ((ghost-image-label? (car st)) "[IMAGE]")
        ;; document：子节点换行连接
        ((eq? (car st) 'document)
         (ghost-join-lines (map ghost-flatten-text (cdr st))))
        ;; 其他 compound（concat/para 等）：递归拼接子节点
        (else (ghost-flatten-list (cdr st)))
      ) ;cond
    ) ;define

    (define (ghost-flatten-list lst)
      (apply string-append (map ghost-flatten-text lst)))
     ;define

    (define (ghost-join-lines lst)
      (let loop ((xs lst) (acc '()))
        (if (null? xs)
          (apply string-append (reverse (intersperse-newlines acc)))
          (loop (cdr xs) (cons (car xs) acc))))
    ) ;define

    (define (intersperse-newlines lst)
      (if (or (null? lst) (null? (cdr lst)))
        lst
        (cons (car lst) (cons "\n" (intersperse-newlines (cdr lst)))))
    ) ;define

    ;; ===== 按光标切分 =====
    ;; path 是从根到叶子的索引序列，叶子层末位为字符偏移。

    (define (ghost-split-text-context st path)
      (let* ((pair (ghost-split-node st path))
             (before (car pair))
             (after (cdr pair)))
        (cons (ghost-truncate-suffix before ghost-prefix-limit)
              (ghost-truncate-prefix after ghost-suffix-limit)))
    ) ;define

    (define (ghost-split-node st path)
      (cond
        ((null? path) (cons "" (ghost-flatten-text st)))
        ((string? st)
         (let ((idx (car path)))
           (cons (safe-substring st 0 idx)
                 (safe-substring st idx (string-length st)))))
        ((not (pair? st)) (cons "" ""))
        (else (ghost-split-compound st path)))
    ) ;define

    ;; compound 切分：idx 为光标所在子节点（相对 children），rest 下钻。
    ;; with 的有效子节点只有末尾 body：属性对全归 prefix，对 body 下钻。
    ;; 数学/表格/图片整体替换保持标记完整。
    (define (ghost-split-compound st path)
      (let* ((idx (car path))
             (rest (cdr path))
             (lab (car st)))
        (cond
          ((or (ghost-inline-math? st)
               (ghost-formula-label? lab)
               (ghost-table-label? lab)
               (ghost-image-label? lab))
           (cons "" (ghost-flatten-text st)))
          ;; with：光标必在 body（末元素），属性对是前置文本，对 body 递归切分
          ((eq? lab 'with)
           (let ((body (list-last st)))
             (ghost-split-node body rest)))
          ((or (< idx 0) (>= (+ 1 idx) (length st)))
           (cons "" (ghost-flatten-text st)))
          (else
            (let* ((children (cdr st))
                   (before-children (list-take children idx))
                   (pivot (list-ref children idx))
                   (after-children (list-tail children (+ idx 1)))
                   (pivot-pair (ghost-split-node pivot rest))
                   (before-text
                     (string-append
                       (ghost-flatten-children lab before-children)
                       (car pivot-pair)))
                   (after-text
                     (string-append
                       (cdr pivot-pair)
                       (ghost-flatten-children lab after-children))))
              (cons before-text after-text)))))
    ) ;define

    (define (ghost-flatten-children lab children)
      (cond
        ((null? children) "")
        ((eq? lab 'document)
         (let ((txt (ghost-join-lines (map ghost-flatten-text children))))
           (if (string=? txt "") "" (string-append txt "\n"))))
        (else (ghost-flatten-list children)))
    ) ;define

    ;; ===== 截断 =====

    (define (ghost-truncate-suffix s limit)
      (let ((len (string-length s)))
        (if (<= len limit) s (substring s (- len limit) len))))
     ;define

    (define (ghost-truncate-prefix s limit)
      (let ((len (string-length s)))
        (if (<= len limit) s (substring s 0 limit))))
     ;define

    (define (safe-substring s from to)
      (let* ((len (string-length s))
             (f (max 0 (min from len)))
             (t (max f (min to len))))
        (substring s f t)))
     ;define

    ;; ===== 数学模式：按公式内 path 切 stree，返回 (before-stree . after-stree) =====

    (define (ghost-split-math-context formula-stree inner-path)
      (let ((pair (ghost-split-atom-only formula-stree inner-path)))
        (cons (car pair) (cdr pair))))
     ;define

    (define (ghost-split-atom-only st path)
      (cond
        ((null? path) (cons st ' ""))
        ((string? st)
         (let ((idx (car path)))
           (cons (safe-substring st 0 idx)
                 (safe-substring st idx (string-length st)))))
        ((not (pair? st)) (cons st ' ""))
        (else
          (let* ((idx (car path)) (rest (cdr path))
                 (children (cdr st)))
            (if (or (< idx 0) (>= idx (length children)))
              (cons st ' "")
              (let* ((before (list-take children idx))
                     (pivot (list-ref children idx))
                     (after (list-tail children (+ idx 1)))
                     (pp (ghost-split-atom-only pivot rest)))
                (cons (cons (car st) (append before (list (car pp))))
                      (cons (car st) (cons (cdr pp) after))))))))
    ) ;define
  ) ;begin
) ;define-library
