;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : python-edit.scm
;; DESCRIPTION : Editing python programs
;; COPYRIGHT   : (C) 2014
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code python-edit) (:use (prog prog-edit)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatic indentation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (string-strip-right s)
  ;; 移除字符串末尾的空白字符
  (with char-set:not-whitespace
    (char-set-complement char-set:whitespace)
    (with n
      (string-length s)
      (with r
        (or (string-rindex s char-set:not-whitespace) n)
        (string-take s (min n (+ 1 r)))
      ) ;with
    ) ;with
  ) ;with
) ;define

(define (strip-comment-buggy s)
  "Removes comment from python line."
  (with i (string-index s #\#) (if i (string-take s i) s))
) ;define

(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-python?)
  ;; 根据前一行计算当前行的缩进量
  ;; 如果前一行以冒号(:)结尾，则增加一个制表位的缩进
  (if (<= row 0)
    0
    (let* ((r (program-row (- row 1)))
           (s (string-strip-right (strip-comment-buggy (if r r ""))))
           (i (string-get-indent s))
           (c (if (== s "") "" (string-take-right s 1)))
          ) ;
      (if (== c ":") (+ i (get-tabstop)) i)
    ) ;let*
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatic insertion, highlighting and selection of brackets and quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (python-bracket-open lbr rbr)
  ;; 插入一对括号或引号，并将光标定位在中间
  (bracket-open lbr rbr "\\")
) ;tm-define

(tm-define (python-bracket-close lbr rbr)
  ;; 处理闭合括号或引号，并正确放置光标位置
  (bracket-close lbr rbr "\\")
) ;tm-define


(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-python?)
  ;; 当光标移动时高亮匹配的括号
  (select-brackets-after-movement "([{" ")]}" "\\")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Copy and Paste
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-paste)
  (:mode in-prog-python?)
  ;; 使用Python特定格式粘贴剪贴板内容
  (clipboard-paste-import "python" "primary")
) ;tm-define

(kbd-map (:mode in-prog-python?)
  ;; Python编程模式下的键盘快捷键
  ("A-tab" (insert-tabstop))
  ;; Alt+Tab：插入制表符
  ("cmd S-tab" (remove-tabstop))
  ;; Cmd+Shift+Tab：移除制表符
  ("{" (python-bracket-open "{" "}"))
  ;; 自动插入匹配的大括号
  ("}" (python-bracket-close "{" "}"))
  ;; 处理闭合大括号
  ("(" (python-bracket-open "(" ")"))
  ;; 自动插入匹配的小括号
  (")" (python-bracket-close "(" ")"))
  ;; 处理闭合小括号
  ("[" (python-bracket-open "[" "]"))
  ;; 自动插入匹配的方括号
  ("]" (python-bracket-close "[" "]"))
  ;; 处理闭合方括号
  ("\"" (python-bracket-open "\"" "\""))
  ;; 自动插入匹配的双引号
  ("'" (python-bracket-open "'" "'"))
) ;kbd-map
;; 自动插入匹配的单引号
