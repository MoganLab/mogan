;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : lua-edit.scm
;; DESCRIPTION : editing lua programs
;; COPYRIGHT   : (C) 2025  Fanjie Meng
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code lua-edit)
  (:use (prog prog-edit)
        (code lua-mode)))          ; 确保已有 lua-mode.scm 或 lua-lang.scm

(tm-define (get-tabstop)
  (:mode in-prog-lua?)
  2)

;; 定义Lua特定的配对关键字，这些关键字会减少缩进
(define lua-decrease-indent-keys 
  '("end" "until"))

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

;; 定义Lua代码的缩进计算函数
(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-lua?)
  (if (<= row 0) 0
      (let* ((prev-row (- row 1))
             (prev-line (program-row prev-row))
             (stripped-prev (string-strip-right (if prev-line prev-line "")))
             (prev-indent (string-get-indent stripped-prev))
             (tab-width (get-tabstop)))
        (cond 
          ;; 如果当前行以减少缩进的关键字开头，则减少缩进
          ((starts-with-keyword? (program-row row) lua-decrease-indent-keys)
           (max 0 (- prev-indent tab-width)))
          ;; 否则保持前行的缩进
          (else prev-indent)))))

(tm-define (lua-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\"))

(tm-define (lua-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-lua?)
  (select-brackets-after-movement "([{" ")]}" "\\"))

(tm-define (kbd-paste)
  (:mode in-prog-lua?)
  (clipboard-paste-import "lua" "primary"))

(kbd-map
  (:mode in-prog-lua?)
  ("{"   (lua-bracket-open "{" "}"))
  ("}"   (lua-bracket-close "{" "}"))
  ("("   (lua-bracket-open "(" ")"))
  (")"   (lua-bracket-close "(" ")"))
  ("["   (lua-bracket-open "[" "]"))
  ("]"   (lua-bracket-close "[" "]"))
  ("\"" (lua-bracket-open "\"" "\""))
  ("'"  (lua-bracket-open "'" "'"))
  ("A-tab" (insert-tabstop))
  ("S-tab" (remove-tabstop)))
