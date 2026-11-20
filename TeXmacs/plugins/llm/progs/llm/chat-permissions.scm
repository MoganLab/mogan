;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-permissions.scm
;; DESCRIPTION : Read-only declarations for Chat virtual buffers
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-permissions))

;; 权限声明必须轻量加载，保证首次创建 Chat buffer 前已经生效。
(tmfs-permission-handler (chat name type)
  (if (and (string? name) (string-ends? name "/message")) (== type "read") #t)
) ;tmfs-permission-handler
