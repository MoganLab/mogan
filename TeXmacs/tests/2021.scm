;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2021.scm
;; DESCRIPTION : GUI 验证 confirm-close-dialog 的完整逻辑契约。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2021] 验证 QML 重写的「是否保存」确认框在以下路径的行为：
;;     - 普通文档 / scratch 文档的 Cancel、Don't save、Save 分支
;;     - Cancel / Don't save / Save 的回调触发契约
;;     - confirm-close-dialog 显式 opt-buffer 参数
;;
;;   通过环境变量绕过模态对话框：
;;     - MOGAN_TEST_CONFIRM_CLOSE 控制 cpp-confirm-close 的返回值
;;     - MOGAN_TEST_CHOOSE_FILE 控制 choose-file 的返回路径
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2021
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/texmacs/texmacs/tm-server.scm")

(check-set-mode! 'report-failed)

(define step-delay-ms 3000)

(define (tmp-url name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture name)
  (let ((src (string->url (string-append "$TEXMACS_PATH/tests/tmu/" name))))
    (when (url-exists? src)
      (system-copy src (tmp-url name))
    ) ;when
  ) ;let
) ;define

(define (buffer-present? buf)
  (in? buf (buffer-list))
) ;define

(define (make-dirty)
  (with buf (current-buffer) (when buf (buffer-pretend-modified buf)))
) ;define

;; 测试钩子预设

(define (preset-cancel!)
  (system-setenv "MOGAN_TEST_CONFIRM_CLOSE" "Cancel")
  (system-setenv "MOGAN_TEST_CHOOSE_FILE" "")
) ;define

(define (preset-dont-save!)
  (system-setenv "MOGAN_TEST_CONFIRM_CLOSE" "Don't save")
  (system-setenv "MOGAN_TEST_CHOOSE_FILE" "")
) ;define

(define (preset-save!)
  (system-setenv "MOGAN_TEST_CONFIRM_CLOSE" "Save")
  (system-setenv "MOGAN_TEST_CHOOSE_FILE" "")
) ;define

(define (preset-save-with-choose-file! url)
  (system-setenv "MOGAN_TEST_CONFIRM_CLOSE" "Save")
  (system-setenv "MOGAN_TEST_CHOOSE_FILE" (url->system url))
) ;define

(define (clear-hooks!)
  (system-setenv "MOGAN_TEST_CONFIRM_CLOSE" "")
  (system-setenv "MOGAN_TEST_CHOOSE_FILE" "")
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2021-step] ")
                           (display label)
                           (newline)
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_2021)
  (refresh-fixture "2021.tmu")
  (let ((doc (tmp-url "2021.tmu"))
        (scratch-save-url (tmp-url "2021_scratch_save.tmu"))
        (cb-save-url (tmp-url "2021_cb_save.tmu"))
        (opt-save-url (tmp-url "2021_opt_save.tmu"))
       ) ;

    ;; 清理环境变量与旧文件
    (clear-hooks!)
    (for-each (lambda (u) (when (url-exists? u) (system-remove u)))
      (list scratch-save-url cb-save-url opt-save-url)
    ) ;for-each

    (run-chain (append
                 ;; 0) 载入夹具
                 (list (cons "load fixture" (lambda () (load-buffer doc))))

                 ;; 1) 普通文档 Cancel
                 (list (cons "make dirty (1)" make-dirty))
                 (list (cons "preset=Cancel" preset-cancel!))
                 (list (cons "kill-tabpage (expect cancelled)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-true (buffer-present? buf))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 2) 普通文档 Don't save
                 (list (cons "preset=Don't save" preset-dont-save!))
                 (list (cons "kill-tabpage (expect closed without save)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-false (buffer-present? buf))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 3) 普通文档 Save
                 (list (cons "reload fixture" (lambda () (load-buffer doc))))
                 (list (cons "make dirty (3)" make-dirty))
                 (list (cons "preset=Save" preset-save!))
                 (list (cons "kill-tabpage (expect saved & closed)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-false (buffer-present? buf))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 4) scratch Cancel
                 (list (cons "new-document (scratch)" (lambda () (new-document))))
                 (list (cons "check scratch" (lambda () (check-true (url-scratch? (current-buffer)))))
                 ) ;list
                 (list (cons "make scratch dirty" make-dirty))
                 (list (cons "preset=Cancel (scratch)" preset-cancel!))
                 (list (cons "kill-tabpage (expect scratch kept)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-true (buffer-present? buf))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 5) scratch Don't save
                 (list (cons "make scratch dirty (2)" make-dirty))
                 (list (cons "preset=Don't save (scratch)" preset-dont-save!))
                 (list (cons "kill-tabpage (expect scratch closed)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-false (buffer-present? buf))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 6) scratch Save（通过 choose-file 另存为）
                 (list (cons "new-document (scratch save)" (lambda () (new-document))))
                 (list (cons "make scratch dirty (save)" make-dirty))
                 (list (cons "preset=Save (scratch) + choose-file hook"
                         (lambda () (preset-save-with-choose-file! scratch-save-url))
                       ) ;cons
                 ) ;list
                 (list (cons "kill-tabpage (expect scratch saved & closed)"
                         (lambda ()
                           (with buf
                             (current-buffer)
                             (safely-kill-tabpage)
                             (check-false (buffer-present? buf))
                             (check-true (url-exists? scratch-save-url))
                           ) ;with
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 7) 回调契约：Cancel / Don't save / Save
                 (list (cons "new-document (callback test)" (lambda () (new-document))))
                 (list (cons "make scratch dirty (callback)" make-dirty))
                 (list (cons "callback Cancel"
                         (lambda ()
                           (preset-cancel!)
                           (let ((saved #f) (dont-saved #f) (buf (current-buffer)))
                             (confirm-close-dialog "test"
                               (lambda () (set! saved #t))
                               (lambda () (set! dont-saved #t))
                               buf
                             ) ;confirm-close-dialog
                             (check-false saved)
                             (check-false dont-saved)
                             (check-true (buffer-present? buf))
                           ) ;let
                         ) ;lambda
                       ) ;cons
                 ) ;list
                 (list (cons "make scratch dirty (callback don't save)" make-dirty))
                 (list (cons "callback Don't save"
                         (lambda ()
                           (preset-dont-save!)
                           (let ((saved #f) (dont-saved #f) (buf (current-buffer)))
                             (confirm-close-dialog "test"
                               (lambda () (set! saved #t))
                               (lambda () (set! dont-saved #t) (buffer-close buf))
                               buf
                             ) ;confirm-close-dialog
                             (check-false saved)
                             (check-true dont-saved)
                             (check-false (buffer-present? buf))
                           ) ;let
                         ) ;lambda
                       ) ;cons
                 ) ;list
                 (list (cons "callback Save (scratch)"
                         (lambda () (new-document) (preset-save-with-choose-file! cb-save-url))
                       ) ;cons
                 ) ;list
                 (list (cons "make scratch dirty (callback save)" make-dirty))
                 (list (cons "check callback Save"
                         (lambda ()
                           (let ((saved #f) (dont-saved #f) (buf (current-buffer)))
                             (confirm-close-dialog "test"
                               (lambda () (set! saved #t))
                               (lambda () (set! dont-saved #t))
                               buf
                             ) ;confirm-close-dialog
                             (check-true saved)
                             (check-false dont-saved)
                             (check-false (buffer-present? buf))
                             (check-true (url-exists? cb-save-url))
                           ) ;let
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 8) opt-buffer：current-buffer 为 fixture，但关闭的是 scratch
                 (list (cons "load fixture (opt-buffer test)"
                         (lambda () (load-buffer doc) (clear-hooks!))
                       ) ;cons
                 ) ;list
                 (list (cons "make fixture dirty (opt)" make-dirty))
                 (list (cons "new scratch (opt)" (lambda () (new-document))))
                 (list (cons "make scratch dirty (opt)" make-dirty))
                 (list (cons "switch to fixture (opt)" (lambda () (switch-to-buffer doc))))
                 (list (cons "preset=Save opt-buffer=scratch"
                         (lambda () (preset-save-with-choose-file! opt-save-url))
                       ) ;cons
                 ) ;list
                 (list (cons "confirm-close-dialog with opt-buffer"
                         (lambda ()
                           (let ((scratch-buf (car (list-filter (buffer-list) url-scratch?)))
                                 (fixture-buf (current-buffer))
                                 (saved #f)
                                 (dont-saved #f)
                                ) ;
                             (check-true (url-scratch? scratch-buf))
                             (confirm-close-dialog "test"
                               (lambda () (set! saved #t))
                               (lambda () (set! dont-saved #t))
                               scratch-buf
                             ) ;confirm-close-dialog
                             (check-true saved)
                             (check-false dont-saved)
                             (check-true (url-exists? opt-save-url))
                             (check-false (buffer-present? scratch-buf))
                             (check-true (buffer-present? fixture-buf))
                           ) ;let
                         ) ;lambda
                       ) ;cons
                 ) ;list

                 ;; 收尾
                 (list (cons "check-report + quit"
                         (lambda () (clear-hooks!) (check-report) (quit-TeXmacs))
                       ) ;cons
                 ) ;list
               ) ;append
    ) ;run-chain
  ) ;let
) ;tm-define
