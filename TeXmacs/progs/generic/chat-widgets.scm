
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-widgets.scm
;; DESCRIPTION : commands for a static chat sidebar skeleton
;; COPYRIGHT   : (C) 2025  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-widgets))

(tm-define (open-chat-sidebar)
  (:interactive #t)
  (show-chat-sidebar #t))

(tm-define (close-chat-sidebar)
  (:interactive #t)
  (show-chat-sidebar #f))

(tm-define (toggle-chat-sidebar)
  (:interactive #t)
  (if (chat-sidebar-visible?)
      (close-chat-sidebar)
      (open-chat-sidebar)))
