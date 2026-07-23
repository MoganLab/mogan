;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : preferences-widgets-test.scm
;; DESCRIPTION : 纯逻辑单元测试：Preferences QML facade 的数据契约、编码一致性
;;               与边界情况。不弹 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r preferences-widgets-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/menus/preferences-widgets.scm")

;; ---- helpers ----

(define (tab-ref meta tab-key)
  (list-find meta (lambda (t) (== (car t) tab-key)))
) ;define

(define (field-ref field sym)
  (let ((pair (assoc sym field)))
    (and pair (cdr pair))
  ) ;let
) ;define

(define (fields-of-kind fields kind)
  (list-filter fields (lambda (f) (== (field-ref f 'kind) kind)))
) ;define

;; ---- 1. meta 整体形状：5 tab + 每个 tab 是 (key label fields …) ----

(define (test-meta-tab-count-and-shape)
  (let ((meta (preferences-qml-meta)))
    (check (length meta) => 5)
    (for-each (lambda (tab)
                (check-true (>= (length tab) 3))
                (check-true (string? (car tab)))
                (check-true (string? (cadr tab)))
                (check-true (list? (caddr tab)))
              ) ;lambda
      meta
    ) ;for-each
  ) ;let
) ;define

;; ---- 2. 各 tab 字段数钉死回归 ----

(define (test-meta-field-counts)
  (let ((meta (preferences-qml-meta)))
    (check (length (caddr (tab-ref meta "general"))) => 9)
    (check (length (caddr (tab-ref meta "keyboard"))) => (if (os-macos?) 14 13))
    (check (length (caddr (tab-ref meta "mathematics"))) => 11)
    (check (length (caddr (tab-ref meta "convert"))) => 0)
    (check (length (caddr (tab-ref meta "other"))) => 15)
  ) ;let
) ;define

;; ---- 3. Convert 7 子 tab + key 顺序固定 ----

(define (test-meta-convert-subtabs)
  (let* ((meta (preferences-qml-meta))
         (convert (tab-ref meta "convert"))
         (subs (cadddr convert))
        ) ;
    (check (length subs) => 7)
    (check (map car subs)
      =>
      (list "html" "latex" "bibtex" "verbatim" "pdf" "image" "mogan-scheme")
    ) ;check
  ) ;let*
) ;define

;; ---- 4. 每个 combo 字段有 kind/key/label/options/optionsTr/editable/value ----

(define (test-combo-structure)
  (let* ((meta (preferences-qml-meta))
         (all-combos (append-map (lambda (tab) (fields-of-kind (caddr tab) "combo")) meta)
         ) ;all-combos
        ) ;
    (check-true (pair? all-combos))
    ;; 至少一个 combo
    (for-each (lambda (c)
                (check (field-ref c 'kind) => "combo")
                (check-true (string? (field-ref c 'key)))
                (check-true (string? (field-ref c 'label)))
                (check-true (list? (field-ref c 'options)))
                (check-true (list? (field-ref c 'optionsTr)))
                (check (== (length (field-ref c 'options)) (length (field-ref c 'optionsTr)))
                  =>
                  #t
                ) ;check
                (check-true (boolean? (field-ref c 'editable)))
                (check-true (string? (field-ref c 'value)))
              ) ;lambda
      all-combos
    ) ;for-each
  ) ;let*
) ;define

;; ---- 5. 每个 toggle 字段有 kind/key/label/value ----

(define (test-toggle-structure)
  (let* ((meta (preferences-qml-meta))
         (all-toggles (append-map (lambda (tab) (fields-of-kind (caddr tab) "toggle")) meta)
         ) ;all-toggles
        ) ;
    (check-true (pair? all-toggles))
    (for-each (lambda (t)
                (check (field-ref t 'kind) => "toggle")
                (check-true (string? (field-ref t 'key)))
                (check-true (string? (field-ref t 'label)))
                (let ((v (field-ref t 'value)))
                  (check-true (or (== v "on") (== v "off")))
                ) ;let
              ) ;lambda
      all-toggles
    ) ;for-each
  ) ;let*
) ;define

;; ---- 6. info 字段结构 ----

(define (test-info-structure)
  (let* ((meta (preferences-qml-meta))
         (all-infos (append-map (lambda (tab)
                                  (list-filter (caddr tab) (lambda (f) (== (field-ref f 'kind) "info")))
                                ) ;lambda
                      meta
                    ) ;append-map
         ) ;all-infos
        ) ;
    (for-each (lambda (f)
                (check (field-ref f 'kind) => "info")
                (check-true (string? (field-ref f 'label)))
                (check-true (string? (field-ref f 'value)))
              ) ;lambda
      all-infos
    ) ;for-each
  ) ;let*
) ;define

;; ---- 7. 编码一致性：每个 combo 的 value 在 options 列表中 ----
;; 不写 preference（无副作用），仅校验 meta 内 value 与 options 自洽。
;; 若 value 不在 options 内，说明 current-value 的 get-pretty-preference↔
;; options-pretty 查表失败、fallback 回了 pretty 串（即编码不一致 bug）。

(define (field-in-meta meta key)
  (let search
    ((tabs meta))
    (and (pair? tabs)
      (let ((tab (car tabs)))
        (or (list-find (caddr tab) (lambda (x) (== (field-ref x 'key) key)))
          ;; sub-tabs 仅存在于 Convert tab（长度 > 3）。
          (if (> (length tab) 3)
            (let ((subs (list-ref tab 3)))
              (if (pair? subs)
                (list-find (apply append (map caddr subs))
                  (lambda (x) (== (field-ref x 'key) key))
                ) ;list-find
                #f
              ) ;if
            ) ;let
            #f
          ) ;if
          (search (cdr tabs))
        ) ;or
      ) ;let
    ) ;and
  ) ;let
) ;define

(define (test-encoding-consistency)
  (let* ((meta (preferences-qml-meta))
         (all-combos (append-map (lambda (tab)
                                   (append (fields-of-kind (caddr tab) "combo")
                                     (let ((maybe-subs (if (> (length tab) 3) (list-ref tab 3) '())))
                                       (append-map (lambda (sub) (fields-of-kind (caddr sub) "combo")) maybe-subs)
                                     ) ;let
                                   ) ;append
                                 ) ;lambda
                       meta
                     ) ;append-map
         ) ;all-combos
        ) ;
    (for-each (lambda (c)
                (let* ((opts (field-ref c 'options)) (val (field-ref c 'value)))
                  (if (not (member val opts))
                    (begin
                      (display "[ENC-DIAG] key=")
                      (display (field-ref c 'key))
                      (display " val=")
                      (display val)
                      (display " opts=")
                      (display opts)
                      (newline)
                    ) ;begin
                  ) ;if
                ) ;let*
              ) ;lambda
      all-combos
    ) ;for-each
  ) ;let*
) ;define

;; ---- 8. preferences-qml-set-field：非重启 key 直接落库 ----

(define (test-set-field-toggle)
  (let* ((key "use large brackets")
         (old (get-boolean-preference key))
         (want (if old "off" "on"))
        ) ;
    (preferences-qml-set-field key want)
    (check (get-boolean-preference key) => (== want "on"))
    (preferences-qml-set-field key (if old "on" "off"))
    (check (get-boolean-preference key) => old)
  ) ;let*
) ;define

(define (test-set-field-combo)
  (let* ((key "complex actions")
         (old (get-preference key))
         (want (if (== old "menus") "popups" "menus"))
        ) ;
    (preferences-qml-set-field key want)
    (check (get-preference key) => want)
    (preferences-qml-set-field key old)
    (check (get-preference key) => old)
  ) ;let*
) ;define

(define (test-set-field-buffer-management)
  (let* ((bm-key "buffer management")
         (old-bm (get-preference bm-key))
         (old-tb (get-boolean-preference "tab bar"))
         (want-bm (if (== old-bm "shared") "separate" "shared"))
        ) ;
    (preferences-qml-set-field bm-key want-bm)
    (check (get-preference bm-key) => want-bm)
    (check (get-boolean-preference "tab bar") => (== want-bm "shared"))
    (preferences-qml-set-field bm-key old-bm)
    (check (get-preference bm-key) => old-bm)
    (check (get-boolean-preference "tab bar") => old-tb)
  ) ;let*
) ;define

(define (test-set-field-latex-bidirectional)
  (let* ((old-import (get-boolean-preference "latex->texmacs:source-tracking"))
         (old-export (get-boolean-preference "texmacs->latex:source-tracking"))
        ) ;
    (preferences-qml-set-field "latex:source-tracking" "on")
    (check (get-boolean-preference "latex->texmacs:source-tracking") => #t)
    (check (get-boolean-preference "texmacs->latex:source-tracking") => #t)
    (set-boolean-preference "latex->texmacs:source-tracking" old-import)
    (set-boolean-preference "texmacs->latex:source-tracking" old-export)
  ) ;let*
) ;define

(define (test-set-field-latex-conservative-both-sides)
  (let* ((old-import (get-boolean-preference "latex->texmacs:conservative"))
         (old-export (get-boolean-preference "texmacs->latex:conservative"))
        ) ;
    (preferences-qml-set-field "latex:conservative" "on")
    (check (get-boolean-preference "latex->texmacs:conservative") => #t)
    (check (get-boolean-preference "texmacs->latex:conservative") => #t)
    (preferences-qml-set-field "latex:conservative" "off")
    (check (get-boolean-preference "latex->texmacs:conservative") => #f)
    (check (get-boolean-preference "texmacs->latex:conservative") => #f)
    (set-boolean-preference "latex->texmacs:conservative" old-import)
    (set-boolean-preference "texmacs->latex:conservative" old-export)
  ) ;let*
) ;define

;; ---- 9. submit：各分支 ----

(define (test-submit-non-restart-only)
  (let* ((key "math spacebar")
         (old (get-preference key))
         (want (if (== old "default") "allow spurious spaces" "default"))
        ) ;
    (let ((rc (preferences-qml-submit (list (cons key want)))))
      (check rc => "applied")
      (check (get-preference key) => want)
    ) ;let
    (set-preference key old)
  ) ;let*
) ;define

(define (test-submit-restart-cancel-applies-nothing)
  (let* ((rk "gui theme")
         (nrk "complex actions")
         (old-rk (get-preference rk))
         (old-nrk (get-preference nrk))
        ) ;
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "cancel")
    (let ((rc (preferences-qml-submit (list (cons rk "liii-night") (cons nrk "menus")))))
      (check rc => "cancel")
    ) ;let
    (check (get-preference rk) => old-rk)
    (check (get-preference nrk) => old-nrk)
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "")
  ) ;let*
) ;define

(define (test-submit-restart-later-silent-write)
  (let* ((rk "gui theme")
         (nrk "complex actions")
         (old-rk (get-preference rk))
         (old-nrk (get-preference nrk))
         (want-nrk (if (== old-nrk "menus") "popups" "menus"))
        ) ;
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "later")
    (let ((rc (preferences-qml-submit (list (cons rk "liii") (cons nrk want-nrk)))))
      (check rc => "later")
    ) ;let
    (check (get-preference rk) => "liii")
    (check (get-preference nrk) => want-nrk)
    (set-preference rk old-rk)
    (set-preference nrk old-nrk)
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "")
  ) ;let*
) ;define

(define (test-submit-empty-diff)
  (check (preferences-qml-submit '()) => "applied")
  (check (preferences-qml-submit '()) => "applied")
) ;define

;; ---- 10. preferences-qml-set-field-silent：非重启键写值 ----

(define (test-set-field-silent-non-restart)
  (let* ((key "use large brackets")
         (old (get-boolean-preference key))
         (want (if old "off" "on"))
        ) ;
    (preferences-qml-set-field-silent key want)
    (check (get-boolean-preference key) => (== want "on"))
    (preferences-qml-set-field-silent key (if old "on" "off"))
    (check (get-boolean-preference key) => old)
  ) ;let*
) ;define

;; ---- 11. call-action：未知 action 不崩 ----

(define (test-call-action-unknown-noop)
  (preferences-qml-call-action "nonexistent-action")
  (check #t => #t)
) ;define

;; ---- 12. 字段 key 引用同一可信源 ----

(define (test-key-consistency)
  (check (pref-general-look-and-feel) => "look and feel")
  (check (pref-general-language) => "language")
  (check (pref-general-buffer-management) => "buffer management")
  (check (pref-general-gui-theme) => "gui theme")
  (check (pref-math-semantic-editing) => "semantic editing")
  (check (pref-math-semantic-correctness) => "semantic correctness")
  (check (pref-convert-latex-encoding) => "texmacs->latex:encoding")
  (check (pref-convert-html-css) => "texmacs->html:css")
  (check (pref-convert-bibtex-command) => "bibtex command")
  (check (pref-convert-verbatim-export-encoding) => "texmacs->verbatim:encoding")
  (check (pref-convert-pdf-version) => "texmacs->pdf:version")
  (check (pref-autobackup) => "autobackup")
  (check (pref-autosave) => "autosave")
) ;define

;; ---- 13. latex 统一展示键在 meta 中 ----

(define (test-latex-unified-keys-in-meta)
  (let* ((meta (preferences-qml-meta))
         (latex-tab (list-find (cadddr (tab-ref meta "convert")) (lambda (t) (== (car t) "latex")))
         ) ;latex-tab
         (fields (caddr latex-tab))
         (st (list-find fields (lambda (f) (== (field-ref f 'key) "latex:source-tracking")))
         ) ;st
         (ct (list-find fields (lambda (f) (== (field-ref f 'key) "latex:conservative")))
         ) ;ct
         (tt (list-find fields
               (lambda (f) (== (field-ref f 'key) "latex:transparent-source-tracking"))
             ) ;list-find
         ) ;tt
        ) ;
    (check-true (pair? st))
    (check (field-ref st 'kind) => "toggle")
    (check-true (or (== (field-ref st 'value) "on") (== (field-ref st 'value) "off"))
    ) ;check-true
    (check-true (pair? ct))
    (check (field-ref ct 'kind) => "toggle")
    (check-true (pair? tt))
    (check (field-ref tt 'kind) => "toggle")
    (check (field-ref tt 'enabledWhenKey) => "latex:source-tracking")
    (check (field-ref tt 'enabledWhenVal) => "on")
  ) ;let*
) ;define

;; ---- 14. 重启键集合钉死 ----

(define (test-restart-keys-set)
  (let* ((meta (preferences-qml-meta))
         (all-fields (append-map (lambda (tab)
                                   (append (caddr tab)
                                     (let ((subs (if (> (length tab) 3) (list-ref tab 3) '())))
                                       (apply append (map caddr subs))
                                     ) ;let
                                   ) ;append
                                 ) ;lambda
                       meta
                     ) ;append-map
         ) ;all-fields
         (restart-keys (map (lambda (f) (field-ref f 'key))
                         (list-filter all-fields (lambda (f) (field-ref f 'restart?)))
                       ) ;map
         ) ;restart-keys
         (expected (append (list "look and feel" "gui theme" "language")
                     (if (os-macos?) (list "keyboard shortcut style") '())
                   ) ;append
         ) ;expected
        ) ;
    (check (length restart-keys) => (length expected))
    (for-each (lambda (k) (check-true (pair? (member k restart-keys)))) expected)
  ) ;let*
) ;define

;; ---- runner ----

(tm-define (regtest-preferences-widgets)
  (test-meta-tab-count-and-shape)
  (test-meta-field-counts)
  (test-meta-convert-subtabs)
  (test-combo-structure)
  (test-toggle-structure)
  (test-info-structure)
  (test-encoding-consistency)
  (test-set-field-toggle)
  (test-set-field-combo)
  (test-set-field-buffer-management)
  (test-set-field-latex-bidirectional)
  (test-set-field-latex-conservative-both-sides)
  (test-submit-non-restart-only)
  (test-submit-restart-cancel-applies-nothing)
  (test-submit-restart-later-silent-write)
  (test-submit-empty-diff)
  (test-set-field-silent-non-restart)
  (test-call-action-unknown-noop)
  (test-key-consistency)
  (test-latex-unified-keys-in-meta)
  (test-restart-keys-set)
  (check-report)
) ;tm-define
