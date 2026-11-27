;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6008.scm
;; DESCRIPTION : Integration test: tmfs buffer no-save and quit/close filtering
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/texmacs/texmacs/tm-server.scm")

(define (test_6008)
  (check-set-mode! 'report-failed)

  ;; 1. buffer-no-save? 谓词契约：tmfs 协议与 aux 缓冲区均为无需保存缓冲区
  (check (buffer-no-save? (unix->url "tmfs://startup-tab")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://chat-tab")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://chat/sid-1/input")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://chat/sid-1/message")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://aux/search")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://aux/replace")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://aux/page-header")) => #t)
  (check (buffer-no-save? (unix->url "tmfs://collab/test")) => #t)
  (check (buffer-no-save? (unix->url "/tmp/regular_doc.tmu")) => #f)
  (check (buffer-no-save? (unix->url "")) => #f)

  ;; 2. confirm-close-dialog 遇到 buffer-no-save? 时兜底直调 on-dont-save
  (let ((res1 #f))
    (confirm-close-dialog "Unsaved?"
      (lambda () (set! res1 'saved))
      (lambda () (set! res1 'dont-save))
      (unix->url "tmfs://chat-tab")
    ) ;confirm-close-dialog
    (check res1 => 'dont-save)
  ) ;let

  (let ((res2 #f))
    (confirm-close-dialog "Unsaved?"
      (lambda () (set! res2 'saved))
      (lambda () (set! res2 'dont-save))
      (unix->url "tmfs://startup-tab")
    ) ;confirm-close-dialog
    (check res2 => 'dont-save)
  ) ;let

  (let ((res3 #f))
    (confirm-close-dialog "Unsaved?"
      (lambda () (set! res3 'saved))
      (lambda () (set! res3 'dont-save))
      (unix->url "tmfs://chat/session-99/input")
    ) ;confirm-close-dialog
    (check res3 => 'dont-save)
  ) ;let

  ;; 3. safely-quit-TeXmacs 中待保存列表过滤：tmfs 缓冲区必须被彻底滤除
  (let* ((all-bufs (list (unix->url "tmfs://startup-tab")
                     (unix->url "tmfs://chat-tab")
                     (unix->url "tmfs://chat/s1/input")
                     (unix->url "tmfs://chat/s1/message")
                     (unix->url "/tmp/user_document.tmu")
                   ) ;list
         ) ;all-bufs
         (filtered (filter (non buffer-no-save?) all-bufs))
        ) ;
    (check filtered => (list (unix->url "/tmp/user_document.tmu")))
  ) ;let*

  ;; 4. buffer-modified? 对 tmfs 恒返回 #f
  (check (buffer-modified? (unix->url "tmfs://startup-tab")) => #f)
  (check (buffer-modified? (unix->url "tmfs://chat-tab")) => #f)
  (check (buffer-modified? (unix->url "tmfs://chat/test/input")) => #f)
  (check (buffer-modified? (unix->url "tmfs://chat/test/message")) => #f)

  (check-report)
) ;define
