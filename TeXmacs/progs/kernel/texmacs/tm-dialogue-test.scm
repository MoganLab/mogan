
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-dialogue-test.scm
;; DESCRIPTION : recent-files 路径归一的纯逻辑单元测试（不弹 GUI，headless 可跑）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-dialogue-test)
  (:use (kernel texmacs tm-dialogue))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for recent-files-canonical-path
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Windows 下 scheme 侧 url->system 记录 '\' 分隔，C++ 启动页
;; QDir::fromNativeSeparators 传入 '/' 分隔；归一必须消除该差异，
;; 否则 recent-files-remove-by-path 按路径移除失效（任务 0948 回归）。

(define (test-canonical-path-win-separators)
  (when (os-windows?)
    (check (recent-files-canonical-path "C:/Users/a b/x.tmu")
      =>
      "C:\\Users\\a b\\x.tmu"
    ) ;check
    (check (recent-files-canonical-path "C:\\Users\\a b\\x.tmu")
      =>
      "C:\\Users\\a b\\x.tmu"
    ) ;check
    ;; 含中文与空格的真实路径
    (check (recent-files-canonical-path "C:/Users/测试 文件/x.tmu")
      =>
      "C:\\Users\\测试 文件\\x.tmu"
    ) ;check
  ) ;when
) ;define

;; tmfs://（云端文档等虚拟路径）无盘符归一，往返后须原样保持。

(define (test-canonical-path-tmfs)
  (check (recent-files-canonical-path "tmfs://collab/8fc7bec4-f069-458f-8578-8fabcd4696a2"
         ) ;recent-files-canonical-path
    =>
    "tmfs://collab/8fc7bec4-f069-458f-8578-8fabcd4696a2"
  ) ;check
  (check (recent-files-canonical-path "tmfs://aux/test") => "tmfs://aux/test")
) ;define

;; 非 Windows 下 '/' 即系统分隔符，绝对路径归一幂等。

(define (test-canonical-path-unix-idempotent)
  (when (not (os-windows?))
    (check (recent-files-canonical-path "/home/u/a b.tm") => "/home/u/a b.tm")
  ) ;when
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tm-dialogue)
  (test-canonical-path-win-separators)
  (test-canonical-path-tmfs)
  (test-canonical-path-unix-idempotent)
  (check-report)
) ;tm-define
