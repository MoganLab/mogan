;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0780.scm
;; DESCRIPTION : 回归测试：render-doc-to-png（无窗口 PNG 渲染）与 current-window-url
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [0780] Stage 1：edit_main_rep::render_to_images + buffer_render_to_images
;;   + Scheme (render-doc-to-png)，以及 get_current_window_safe /
;;   (current-window-url)。本测试钉死：
;;     1. 给定可排版 buffer，(render-doc-to-png) 返回 #t 且产出 PNG 文件。
;;     2. (current-window-url) 在无窗口时不崩，返回一个 url（headless 下为
;;        url-none）——旧的 (current-window) 此时 ASSERT/crash，是上次无窗口
;;        渲染失败的根因。
;;
;; USAGE
;;   xmake b stem && xmake r 0780          # headless，同步断言
;;
;; 注意：断言同步执行（不用 exec-delayed-at），headless 即可跑（render-doc-to-png
;; 内部自建 typesetter，不经 apply_changes / 窗口）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)

;; render-doc-to-png：加载一个 .tmu，渲染到临时 PNG；断言返回 #t 且文件已生成。
;; （render-doc-to-png 经 make_raster_image→mupdf 写出，成功即产出有效 PNG。）
(define (test-render-doc-to-png)
  (let* ((tmu "$TEXMACS_PATH/tests/tmu/0101.tmu")
         (png (url-glue (url-temp) ".png")))
    (load-buffer tmu)
    (check (render-doc-to-png tmu png 1.0) => #t)
    (check (url-exists? png) => #t)))

;; current-window-url：get_current_window_safe 的 Scheme 入口。headless 下无窗口，
;; 返回 url-none 而非断言崩溃（旧 (current-window) 此处 ASSERT）。
(define (test-current-window-url)
  (let ((w (current-window-url)))
    (check (url? w) => #t)))

(tm-define (test_0780)
  (test-render-doc-to-png)
  (test-current-window-url)
  (check-report)
) ;tm-define
