;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : kbd-define-test.scm
;; DESCRIPTION : Test suite for delayed-kbd-map
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui kbd-define-test) (:use (kernel gui kbd-define)))

(import (liii check))

(check-set-mode! 'report-failed)

;; kbd-find-key-binding 对 shorthand 返回 (插入串 help) 列表

(define (test-enqueue-defers)
  ;; 入队后未 flush:绑定留在队列里,尚未写表
  (delayed-kbd-map ("T17a" "AAA"))
  (check (kbd-pending-empty?) => #f)
) ;define

(define (test-flush-registers)
  (kbd-flush-pending)
  (check (kbd-pending-empty?) => #t)
  (check (kbd-find-key-binding "T17a") => (list "AAA" ""))
) ;define

(define (test-getter-drains)
  ;; 不经显式 flush,按键查询路径(kbd-find-key-binding)自动 drain
  (delayed-kbd-map ("T17b" "BBB"))
  (check (kbd-find-key-binding "T17b") => (list "BBB" ""))
  (check (kbd-pending-empty?) => #t)
) ;define

(define (test-override-after-delayed)
  ;; 同步写不触发 drain:delayed 批次的 old 在查询 drain 时才执行,
  ;; 按执行先后后执行的 old 覆盖先写入的 new
  (delayed-kbd-map ("T17c" "old"))
  (kbd-map ("T17c" "new"))
  (check (kbd-find-key-binding "T17c") => (list "old" ""))
) ;define

(define (test-override-before-delayed)
  ;; 同步先写 old,delayed 后入队 new:flush 后后定义胜出
  (kbd-map ("T17d" "old"))
  (delayed-kbd-map ("T17d" "new"))
  (kbd-flush-pending)
  (check (kbd-find-key-binding "T17d") => (list "new" ""))
) ;define

(define (test-multi-key-sequence)
  ;; 多键序列:完整序列命中 shorthand
  (delayed-kbd-map ("T17e x y" "EEE"))
  (kbd-flush-pending)
  (check (kbd-find-key-binding "T17e x y") => (list "EEE" ""))
) ;define

(define (cleanup)
  ;; kbd-unmap 的条目是裸按键字符串;"T17e"/"T17e x" 是多键序列的中间回显绑定
  (kbd-unmap "T17a" "T17b" "T17c" "T17d" "T17e" "T17e x" "T17e x y")
) ;define

(tm-define (regtest-kbd-define)
  (test-enqueue-defers)
  (test-flush-registers)
  (test-getter-drains)
  (test-override-after-delayed)
  (test-override-before-delayed)
  (test-multi-key-sequence)
  (cleanup)
  (check-report)
) ;tm-define
