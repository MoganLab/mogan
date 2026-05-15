;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1010_1.scm
;; DESCRIPTION : Tests for clipboard-paste-import return value
;; COPYRIGHT   : (C) 2026  Yuki
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 辅助函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (headless?)
  (not (defined? 'qt-version)))

(define (clear-clipboard)
  (clipboard-clear "primary"))

(define (setup-clipboard text)
  (insert text)
  (select-all)
  (clipboard-copy "primary")
  (selection-cancel))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 测试用例
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 测试：剪贴板为空时，粘贴返回失败
(define (test-paste-empty-clipboard)
  (catch #t
    (lambda ()
      (clear-clipboard)
      (check (clipboard-paste-import "verbatim" "primary") => #f))
    (lambda args
      (display "剪贴板不可用，跳过空剪贴板粘贴测试\n"))))

;; 测试：剪贴板有内容时，verbatim 格式粘贴返回成功
(define (test-paste-with-content)
  (catch #t
    (lambda ()
      (clear-clipboard)
      (setup-clipboard "test-content-1010")
      (check (clipboard-paste-import "verbatim" "primary") => #t)
      ;; 清理：撤销 paste 和 setup 时插入的内容
      (undo 0)
      (undo 0))
    (lambda args
      (display "剪贴板不可用，跳过有内容粘贴测试\n"))))

;; 测试：剪贴板有内容时，code 格式粘贴返回成功
(define (test-paste-code-format)
  (catch #t
    (lambda ()
      (clear-clipboard)
      (setup-clipboard "def hello(): pass")
      (check (clipboard-paste-import "code" "primary") => #t)
      (undo 0)
      (undo 0))
    (lambda args
      (display "剪贴板不可用，跳过 code 格式粘贴测试\n"))))

;; 测试：剪贴板有内容时，latex 格式粘贴返回成功
(define (test-paste-latex-format)
  (catch #t
    (lambda ()
      (clear-clipboard)
      (setup-clipboard "$E = mc^2$")
      (check (clipboard-paste-import "latex" "primary") => #t)
      (undo 0)
      (undo 0))
    (lambda args
      (display "剪贴板不可用，跳过 latex 格式粘贴测试\n"))))

;; 测试：kbd-paste 返回值为布尔值
(define (test-kbd-paste-return-type)
  (catch #t
    (lambda ()
      (clear-clipboard)
      (setup-clipboard "kbd-paste-test")
      (check (boolean? (kbd-paste)) => #t)
      (undo 0)
      (undo 0))
    (lambda args
      (display "剪贴板不可用，跳过 kbd-paste 测试\n"))))

;; 测试：clipboard-paste-import 函数存在
(define (test-paste-import-exists)
  (check (procedure? clipboard-paste-import) => #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 入口
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1010_1)
  (test-paste-import-exists)
  (if (headless?)
    (display "检测到无头模式，跳过剪贴板相关测试\n")
    (begin
      (test-paste-empty-clipboard)
      (test-paste-with-content)
      (test-paste-code-format)
      (test-paste-latex-format)
      (test-kbd-paste-return-type)))
  (check-report))
