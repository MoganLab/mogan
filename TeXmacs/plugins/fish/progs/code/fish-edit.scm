;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-edit.scm
;; DESCRIPTION : editing fish programs
;; COPYRIGHT   : (C) 2025   vesita
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; DESCRIPTION:
;;;   This module provides editing functionalities for fish language within
;;;   TeXmacs. It defines language-specific behaviors such as indentation,
;;;   commenting, and paste operations for fish code snippets.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义模块，使用 prog-edit 和 fish-mode 模块
(texmacs-module (code fish-edit)
  (:use (prog prog-edit)
    (code fish-mode)))

;;------------------------------------------------------------------------------
;; 缩进设置
;;

;; 定义fish代码的制表符停止位为4个空格（符合fish标准）
(tm-define (get-tabstop)
  (:mode in-prog-fish?)
  4)

;; 定义fish特定的关键字，这些关键字后面需要增加缩进
(define fish-increase-indent-keys
  '("define" "if" "loop" "case" "caseof" "section" "command"))

;; 定义fish特定的配对关键字，这些关键字会减少缩进
(define fish-decrease-indent-keys
  '("else" "case"))

;; 定义fish特定的结束关键字，这些关键字会减少缩进
(define fish-end-keys
  '("end" "endif" "endloop" "endsection" "endcase" "endcommand"))

;; 去除字符串右侧的空白字符
(define (string-strip-right s)
  (with char-set:not-whitespace (char-set-complement char-set:whitespace)
    (with n (string-length s)
      (with r (or (string-rindex s char-set:not-whitespace) n)
        (string-take s (min n (+ 1 r)))))))

;; 检查字符串是否以特定关键字结尾
(define (ends-with-keyword? s keys)
  (and (nnull? keys)
    (or (string-ends? s (car keys))
      (ends-with-keyword? s (cdr keys)))))

;; 检查字符串是否以特定关键字开头
(define (starts-with-keyword? s keys)
  (and (nnull? keys)
    (or (string-starts? s (string-append (car keys) " "))
      (starts-with-keyword? s (cdr keys)))))

;; 定义fish代码的缩进计算函数
(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-fish?)
  (if (<= row 0) 0
    (let* ((prev-row (- row 1))
           (prev-line (program-row prev-row))
           (stripped-prev (string-strip-right (if prev-line prev-line "")))
           (prev-indent (string-get-indent stripped-prev))
           (tab-width (get-tabstop)))
      (cond
        ;; 如果前行以增加缩进的关键字结尾，则当前行应增加缩进
        ((ends-with-keyword? stripped-prev fish-increase-indent-keys)
          (+ prev-indent tab-width))
        ;; 如果当前行以减少缩进的关键字开头，则减少缩进
        ((starts-with-keyword? (program-row row) fish-decrease-indent-keys)
          (max 0 (- prev-indent tab-width)))
        ;; 如果当前行以结束关键字开头，则减少缩进
        ((starts-with-keyword? (program-row row) fish-end-keys)
          (max 0 (- prev-indent tab-width)))
        ;; 否则保持前行的缩进
        (else prev-indent)))))

;;------------------------------------------------------------------------------
;; 自动插入、高亮和选择括号和引号
;;

(tm-define (fish-bracket-open lbr rbr)
  ;; 插入一对括号或引号，并将光标定位在中间
  (bracket-open lbr rbr "\\"))

(tm-define (fish-bracket-close lbr rbr)
  ;; 处理闭合括号或引号，并正确放置光标位置
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-fish?)
  ;; 当光标移动时高亮匹配的括号
  (select-brackets-after-movement "([{" ")]}" "\\"))

;;------------------------------------------------------------------------------
;; 粘贴操作
;;

;; 定义fish代码环境中的粘贴操作，使用fish格式导入剪贴板内容
(tm-define (kbd-paste)
  (:mode in-prog-fish?)
  (clipboard-paste-import "fish" "primary"))

(kbd-map
  (:mode in-prog-fish?)
  ;; fish编程模式下的键盘快捷键
  ("A-tab" (insert-tabstop))                 ;; Alt+Tab：插入制表符
  ("cmd S-tab" (remove-tabstop))             ;; Cmd+Shift+Tab：移除制表符
  ("{" (fish-bracket-open "{" "}" ))       ;; 自动插入匹配的大括号
  ("}" (fish-bracket-close "{" "}" ))      ;; 处理闭合大括号
  ("(" (fish-bracket-open "(" ")" ))       ;; 自动插入匹配的小括号
  (")" (fish-bracket-close "(" ")" ))      ;; 处理闭合小括号
  ("[" (fish-bracket-open "[" "]" ))       ;; 自动插入匹配的方括号
  ("]" (fish-bracket-close "[" "]" ))      ;; 处理闭合方括号
  ("\"" (fish-bracket-open "\"" "\"" ))    ;; 自动插入匹配的双引号
  ("'" (fish-bracket-open "'" "'" )))      ;; 自动插入匹配的单引号
