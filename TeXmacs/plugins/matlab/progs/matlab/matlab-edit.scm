;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : matlab-edit.scm
;; DESCRIPTION : editing Matlab programs
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
;;;   This module provides editing functionalities for MATLAB language within
;;;   TeXmacs. It defines language-specific behaviors such as indentation,
;;;   commenting, and paste operations for MATLAB code snippets.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义模块，使用 prog-edit 和 matlab-mode 模块
(texmacs-module (code matlab-edit)
  (:use (prog prog-edit)
    (code matlab-mode)))

;;------------------------------------------------------------------------------
;; 缩进设置
;;

;; 定义MATLAB代码的制表符停止位为4个空格（符合MATLAB标准）
(tm-define (get-tabstop)
  (:mode in-prog-matlab?)
  4)

;; 定义MATLAB特定的关键字，这些关键字后面需要增加缩进
(define matlab-increase-indent-keys 
  '("for" "if" "while" "switch" "try" "classdef" "methods" "properties" "events"))

;; 定义MATLAB特定的配对关键字，这些关键字会减少缩进
(define matlab-decrease-indent-keys 
  '("elseif" "else" "catch" "case" "otherwise"))

;; 定义MATLAB特定的结束关键字，这些关键字会减少缩进
(define matlab-end-keys
  '("end"))

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

;; 定义MATLAB代码的缩进计算函数
(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-matlab?)
  (if (<= row 0) 0
    (let* ((prev-row (- row 1))
           (prev-line (program-row prev-row))
           (stripped-prev (string-strip-right (if prev-line prev-line "")))
           (prev-indent (string-get-indent stripped-prev))
           (tab-width (get-tabstop)))
      (cond 
        ;; 如果前行以增加缩进的关键字结尾，则当前行应增加缩进
        ((ends-with-keyword? stripped-prev matlab-increase-indent-keys)
          (+ prev-indent tab-width))
        ;; 如果当前行以减少缩进的关键字开头，则减少缩进
        ((starts-with-keyword? (program-row row) matlab-decrease-indent-keys)
          (max 0 (- prev-indent tab-width)))
        ;; 如果当前行以结束关键字开头，则减少缩进
        ((starts-with-keyword? (program-row row) matlab-end-keys)
          (max 0 (- prev-indent tab-width)))
        ;; 否则保持前行的缩进
        (else prev-indent)))))

;;------------------------------------------------------------------------------
;; 自动插入、高亮和选择括号和引号
;;

(tm-define (matlab-bracket-open lbr rbr)
  ;; 插入一对括号或引号，并将光标定位在中间
  (bracket-open lbr rbr "\\"))

(tm-define (matlab-bracket-close lbr rbr)
  ;; 处理闭合括号或引号，并正确放置光标位置
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-matlab?)
  ;; 当光标移动时高亮匹配的括号
  (select-brackets-after-movement "([{" ")]}" "\\"))

;;------------------------------------------------------------------------------
;; 粘贴操作
;;

;; 定义MATLAB代码环境中的粘贴操作，使用MATLAB格式导入剪贴板内容
(tm-define (kbd-paste)
  (:mode in-prog-matlab?)
  (clipboard-paste-import "matlab" "primary"))

(kbd-map
  (:mode in-prog-matlab?)
  ;; MATLAB编程模式下的键盘快捷键
  ("A-tab" (insert-tabstop))                 ;; Alt+Tab：插入制表符
  ("cmd S-tab" (remove-tabstop))             ;; Cmd+Shift+Tab：移除制表符
  ("{" (matlab-bracket-open "{" "}" ))       ;; 自动插入匹配的大括号
  ("}" (matlab-bracket-close "{" "}" ))      ;; 处理闭合大括号
  ("(" (matlab-bracket-open "(" ")" ))       ;; 自动插入匹配的小括号
  (")" (matlab-bracket-close "(" ")" ))      ;; 处理闭合小括号
  ("[" (matlab-bracket-open "[" "]" ))       ;; 自动插入匹配的方括号
  ("]" (matlab-bracket-close "[" "]" ))      ;; 处理闭合方括号
  ("\"" (matlab-bracket-open "\"" "\"" ))    ;; 自动插入匹配的双引号
  ("'" (matlab-bracket-open "'" "'" )))      ;; 自动插入匹配的单引号