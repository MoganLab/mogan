
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE     : startup-tab-file.scm
;; DESCRIPTION: Scheme bindings for startup tab file operations
;; COPYRIGHT  : (C) 2026 Yuki Lu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (startup-tab startup-tab-file)
  (:use (texmacs texmacs tm-files))
  (:use (texmacs menus file-menu))
  (:use (kernel texmacs tm-dialogue))
  (:use (utils library cursor)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 按样式创建文档
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (new-document-with-style style-id)
  ;; 创建指定样式的新文档
  ;; style-id: "generic", "beamer", "book", "exam", "letter", "article"
  ;; 使用 with-buffer 保证后续初始化在目标缓冲区中执行
  (with-default-view
    (let ((buf (if (window-per-buffer?) (open-window) (new-buffer))))
      ;; 缓冲区就绪后再延迟初始化样式
      (delayed
        (:idle 100)
        (with-buffer buf
          (init-style style-id))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 文件操作封装
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-file-open)
  ;; 打开文件对话框封装
  (open-document))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 最近文档管理
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-get-recent-docs)
  ;; 获取最近文档路径，过滤和排序与 File -> Recent 保持一致
  (let* ((raw (string->number (get-preference "startup-tab:max-recent")))
         (nr (if (number? raw) raw 10))
         (nr (max 1 nr)))
    (map url->system (recent-file-list nr))))

(tm-define (startup-tab-add-recent-doc path)
  ;; 在全局 recent-file 状态中新增或刷新文档
  (learn-interactive 'recent-buffer (list (cons "0" path))))

(tm-define (startup-tab-clear-recent-doc path)
  ;; 从全局 recent-file 状态中移除指定文档
  (recent-files-remove-by-path path))

(tm-define (startup-tab-clear-all-recent)
  ;; 清空最近文档
  (noop))
