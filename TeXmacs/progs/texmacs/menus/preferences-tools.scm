
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : preferences-tools.scm
;; DESCRIPTION : 首选项的辅助函数：preference 读写 helper（LaTeX / BibTeX 双向
;;               偏好、image format、updater）+ QML facade 的纯函数族（field
;;               descriptor 构造、flag 解析、平台过滤、当前值/动态 options 解析、
;;               tab builder）。被 preferences-widgets.scm 的 facade 入口
;;               （preferences-qml-meta / -submit / -set-field）调用。
;;               依赖方向单向：widgets -> tools（tools 不反向依赖 widgets，
;;               避免循环）。
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;               (C) 2026  Yuki
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus preferences-tools)
  (:use (kernel texmacs pref-keys))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; preference 读写 helper（带副作用的偏好：buffer management / LaTeX / BibTeX
;; 双向偏好 / image format / updater）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (on-buffer-management-changed val)
  ;; val 由 preferences-qml-set-field 传入，是 internal key（"shared"/"separate"）。
  ;; 保留 pretty 显示串（"Multiple documents share window"）经 decode 表反查的兜底，
  ;; 防御未来接入 pretty 串的调用方。统一按 internal key 判断 shared。
  (let* ((internal (if (== val "shared")
                     val
                     (or (ahash-ref preference-decode-table (cons "buffer management" val)) val)
                   ) ;if
         ) ;internal
         (can-use-tabbar? (== internal "shared"))
        ) ;
    (set-boolean-preference "tab bar" can-use-tabbar?)
    (show-icon-bar 4 can-use-tabbar?)
    (set-preference "buffer management" internal)
  ) ;let*
) ;tm-define

;; LaTeX 双向偏好：import + export 各一 preference，统一展示键读用 OR / 写双向。
;; （source-tracking / conservative / transparent-source-tracking）

(define (get-latex-source-tracking)
  (or (get-boolean-preference "latex->texmacs:source-tracking")
    (get-boolean-preference "texmacs->latex:source-tracking")
  ) ;or
) ;define

(tm-define (set-latex-source-tracking on?)
  (set-boolean-preference "latex->texmacs:source-tracking" on?)
  (set-boolean-preference "texmacs->latex:source-tracking" on?)
  (refresh-now "source-tracking")
) ;tm-define

(define (get-latex-conservative)
  (and (get-boolean-preference "latex->texmacs:conservative")
    (get-boolean-preference "texmacs->latex:conservative")
  ) ;and
) ;define

(tm-define (set-latex-conservative on?)
  (set-boolean-preference "latex->texmacs:conservative" on?)
  (set-boolean-preference "texmacs->latex:conservative" on?)
  (refresh-now "source-tracking")
) ;tm-define

(define (get-latex-transparent-source-tracking)
  (or (get-boolean-preference "latex->texmacs:transparent-source-tracking")
    (get-boolean-preference "texmacs->latex:transparent-source-tracking")
  ) ;or
) ;define

(tm-define (set-latex-transparent-source-tracking on?)
  (set-boolean-preference "latex->texmacs:transparent-source-tracking" on?)
  (set-boolean-preference "texmacs->latex:transparent-source-tracking" on?)
) ;tm-define

;; BibTeX 双向保守转换偏好（import / export 各一）。

(define (get-bibtm-conservative)
  (get-boolean-preference "bibtex->texmacs:conservative")
) ;define

(define (set-bibtm-conservative on?)
  (set-boolean-preference "bibtex->texmacs:conservative" on?)
) ;define

(define (get-tmbib-conservative)
  (get-boolean-preference "texmacs->bibtex:conservative")
) ;define

(define (set-tmbib-conservative on?)
  (set-boolean-preference "texmacs->bibtex:conservative" on?)
) ;define

;; image format：返回 (internal-list pretty-list)，并顺带登记
;; texmacs->image:format 的 decode 表。internal/pretty 等长同序，供 QML facade
;; 的 options/optionsTr 对齐 combo 契约。

(define (image-format-list-pair)
  (let* ((desired-image-format-list '(("svg" "Svg")
                                      ("eps" "Eps")
                                      ("png" "Png")
                                      ("tif" "Tiff")
                                      ("jpg" "Jpeg")
                                      ("pdf" "Pdf"))
         ) ;desired-image-format-list
         (valid-image-format-list (filter (lambda (x) (file-converter-exists? "x.pdf" (string-append "x." (car x))))
                                    desired-image-format-list
                                  ) ;filter
         ) ;valid-image-format-list
        ) ;
    (eval `(define-preference-names ,"texmacs->image:format"
             ,@valid-image-format-list)
    ) ;eval
    (apply map list valid-image-format-list)
  ) ;let*
) ;define

;; updater last check：info 行显示上次检查更新时间（updater 插件注入）。

(define (updater-last-check-formatted)
  "Time since last update check formatted for use in the preferences dialog"
  (with c
    (updater-last-check)
    (if (<= c 0)
      "Never"
      (with h
        (ceiling (/ (- (current-time) c) 3600))
        (cond ((< h 24) (replace "Less than %1 hour(s) ago" h))
              ((< h 720) (replace "%1 days ago" (ceiling (/ h 24))))
              (else (translate "More than 1 month ago"))
        ) ;cond
      ) ;with
    ) ;if
  ) ;with
) ;define

(define (last-check-string)
  (if (use-plugin-updater?) (updater-last-check-formatted) "Never (unsupported)")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; QML facade 纯函数族：紧凑字段格式 (key label options options-pretty
;; editable? . flags) -> assoc-list field-descriptor（供 C++ bridge 消费）。
;;
;; kind 分流（由 options 是否非空决定）：有 options -> combo；无 options 且 key
;; 非空 -> toggle；key 空 -> info。
;;
;; flag plist 约定：restart? / radio-group + enabled-when-key/-val / group /
;; group-span / hint / column / layout / action-button / platform-filter
;; （后者仅 scheme-side 过滤、bridge 无需感知）。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 取字段的 flag 尾巴：紧凑格式前 5 项固定、flags 是可变尾巴（plist）。
;; list-tail 跳过前 5 项；无 flags（只有 5 项）返回 '()（安全 no-op）。

(define (field-flags field)
  (if (>= (length field) 5) (list-tail field 5) '())
) ;define

;; plist -> alist：交替的 keyword / value 对转成 (keyword . value) 对。
;; 用 car/cdr 直接取、递归 cddr——不用 with（mogan 的 with 对 dotted-pair 解构不稳）。

(define (preferences-qml-plist->alist plist)
  (cond ((or (null? plist) (null? (cdr plist))) '())
        (else (cons (cons (car plist) (cadr plist))
                (preferences-qml-plist->alist (cddr plist))
              ) ;cons
        ) ;else
  ) ;cond
) ;define

;; action 按钮的显示文案：按 action 名路由（目前仅 open-auto-backup-location）。
;; action 函数由插件注入（(plugin autosave) 模块），先 use-modules 兜底加载、
;; 再调其 label 函数；未注入则空串。bridge callAction(name) 透传到
;; preferences-qml-call-action。

(define (preferences-qml-action-button-label action)
  (cond ((== action 'open-auto-backup-location)
         (when (not (defined? 'auto-backup-button-label))
           (use-modules (plugin autosave))
         ) ;when
         (if (defined? 'auto-backup-button-label) (auto-backup-button-label) "")
        ) ;
        (else "")
  ) ;cond
) ;define

;; flag plist -> assoc pairs：遍历 alist、按 symbol 名映射成 bridge 可消费的
;; assoc pairs（platform-filter 是 scheme-side 过滤用的、此处跳过）。

(define (preferences-qml-flags->assoc flags)
  (apply append
    (map (lambda (pair)
           (let ((kw (car pair)) (val (cdr pair)))
             (cond ((== kw 'restart?) (list (cons 'restart? val)))
                   ((== kw 'radio-group) (list (cons 'radioGroup val)))
                   ;; enabled-when：字段始终显示，但仅当某 key 等于 val 时可勾（否则锁定灰显）。
                   ;; 用于 latex transparent / Store tracking、Math semantic：依赖键开才解锁。
                   ((== kw 'enabled-when-key) (list (cons 'enabledWhenKey val)))
                   ((== kw 'enabled-when-val) (list (cons 'enabledWhenVal val)))
                   ;; group / hint 文案：.scm 源码字面量是 UTF-8 字节（reader 不转 Cork），
                   ;; 先 utf8->cork 归一化，再 translate 查翻译表。否则含非 ASCII 的文案
                   ;; （如 "TeXmacs → Html" 的 → 箭头）被当 Cork 字节二次解码 → 乱码。
                   ;; bridge cork_to_utf8 再把 Cork 还原成 UTF-8 给 QML。
                   ((== kw 'group) (list (cons 'group (translate (utf8->cork val)))))
                   ;; group-span：该 group 标题横跨整行（统领下方左右两列），如 IR 的
                   ;; "Remote controllers with keyboard simulation"。未标的 group 在列内各自渲染。
                   ((== kw 'group-span) (list (cons 'groupSpan val)))
                   ((== kw 'hint) (list (cons 'hint (translate (utf8->cork val)))))
                   ((== kw 'column) (list (cons 'column val)))
                   ((== kw 'layout) (list (cons 'layout val)))
                   ;; action-button：combo 旁的行内按钮。val = action-name（symbol）。
                   ;; buttonAction 透传 action 名，QML 点击经 bridge callAction -> facade 路由。
                   ((== kw 'action-button)
                    (list (cons 'buttonAction val)
                      (cons 'buttonLabel
                        (translate (utf8->cork (preferences-qml-action-button-label val)))
                      ) ;cons
                    ) ;list
                   ) ;
                   (else '())
             ) ;cond
           ) ;let
         ) ;lambda
      (preferences-qml-plist->alist flags)
    ) ;map
  ) ;apply
) ;define

;; 平台过滤 predicate：按 flag 里的 'platform-filter 值过滤字段，返回 #f 表示该
;; 字段在当前平台不显示。

(define (preferences-qml-platform-shows? field)
  (let* ((flags (preferences-qml-plist->alist (field-flags field)))
         (pf (assoc 'platform-filter flags))
        ) ;
    (cond ((not pf) #t)
          ((== (cdr pf) 'look-and-feel)
           ;; look and feel 的 options 在 field->descriptor 时按平台裁剪，字段本身始终显示。
           #t
          ) ;
          ((== (cdr pf) 'macos-only) (os-macos?))
          ((== (cdr pf) 'native-pdf-only) (supports-native-pdf?))
          ((== (cdr pf) 'qt-only) (qt-gui?))
          ((== (cdr pf) 'updater-only) (use-plugin-updater?))
          (else #t)
    ) ;cond
  ) ;let*
) ;define

;; 取字段的当前值（内部键 / on/off / 翻译显示串）。

(define (preferences-qml-current-value key kind options options-pretty)
  (cond ((== kind "combo")
         (let* ((pretty (get-pretty-preference key))
                (idx (list-find-index options-pretty (lambda (p) (== p pretty))))
               ) ;
           (if idx (list-ref options idx) pretty)
         ) ;let*
        ) ;
        ;; latex 双向偏好用统一展示键（latex:source-tracking / conservative /
        ;; transparent-source-tracking），非真实 preference key，读用 OR/AND helper。
        ((== key "latex:source-tracking") (if (get-latex-source-tracking) "on" "off"))
        ((== key "latex:conservative") (if (get-latex-conservative) "on" "off"))
        ((== key "latex:transparent-source-tracking")
         (if (get-latex-transparent-source-tracking) "on" "off")
        ) ;
        ;; Last check info：非真实 preference，显示上次检查更新时间（updater 插件注入）。
        ((== key "updater:last-check") (last-check-string))
        ((== kind "toggle") (if (get-boolean-preference key) "on" "off"))
        (else "")
  ) ;cond
) ;define

;; look and feel 平台允许的内部键列表（用于过滤 options）。

(define (preferences-qml-general-look-and-feel-allowed)
  (cond ((os-win32?) '("default" "emacs" "windows"))
        ((os-macos?) '("default" "emacs" "macos"))
        (else '("default" "emacs" "gnome" "kde"))
  ) ;cond
) ;define

;; look and feel 内部键 -> pretty 显示名（直接编码 define-preference-names 的 decode 表；
;; 硬编码是为了 meta 构建时按平台过滤后仍能给出等长同序的 options/options-pretty）。

(define (preferences-qml-general-look-and-feel-pretty ik)
  (cond ((== ik "default") (translate "Default"))
        ((== ik "emacs") (translate "Emacs"))
        ((== ik "gnome") (translate "Gnome"))
        ((== ik "kde") (translate "KDE"))
        ((== ik "macos") (translate "macOS"))
        ((== ik "windows") (translate "Windows"))
        (else ik)
  ) ;cond
) ;define

;; 动态解析某字段 key 的最终 options / options-pretty：
;; 大多字段透传静态 options；少数动态选项（language / scripting language /
;; image format）在 meta 构建时拉取一次、覆盖空 options。look and feel 的
;; options 按平台裁剪（保持 options / options-pretty 等长同序）。
;; 返回 (final-options final-options-pretty) 二元组。

(define (preferences-qml-resolve-options key options options-pretty)
  (cond
    ;; look and feel：按平台裁剪 options（options 静态含全部平台、options-pretty 同步裁剪）。
    ((== key (pref-general-look-and-feel))
     (let* ((laf-allowed (preferences-qml-general-look-and-feel-allowed))
            (laf-pairs (list-filter (map (lambda (ik) (cons ik (preferences-qml-general-look-and-feel-pretty ik)))
                                      options
                                    ) ;map
                         (lambda (pair) (member (car pair) laf-allowed))
                       ) ;list-filter
            ) ;laf-pairs
           ) ;
       (list (map car laf-pairs) (map cdr laf-pairs))
     ) ;let*
    ) ;
    ;; language：动态按 supported-languages 拉取 options（顶层 define 里用空 '() 避开
    ;; module 加载时批量求值崩溃）。options = 小写内部键；optionsTr = pretty 显示名
    ;; （首字母大写形，由 preferences-widgets.scm 的 set-preference-name 登记进 encode 表——
    ;; get-pretty-preference "language" 返回此形，须与 optionsTr 同序等长才能在
    ;; current_value 里反查到 index）。supported-languages 是变量、不带括号引用。
    ((== key (pref-general-language))
     (list supported-languages (map upcase-first supported-languages))
    ) ;
    ;; scripting language：动态按 scripts-list（lazy-plugin-force 副作用）拉取 options。
    ((== key (pref-scripting-language))
     (let* ((sl (cons "none" (map scripts-name (scripts-list)))))
       (list sl (cons (translate "None") (map scripts-name (scripts-list))))
     ) ;let*
    ) ;
    ;; image format：动态按 image-format-list-pair（file-converter-exists? 副作用）
    ;; 拉取 internal/pretty 双列——options 用 internal 键、optionsTr 用 pretty 显示名，
    ;; 与其它 combo 字段对齐（get-pretty-preference 返回 pretty、经 current_value 反查 index）。
    ((== key (pref-convert-image-format))
     (let* ((pair (image-format-list-pair)))
       (list (car pair) (cadr pair))
     ) ;let*
    ) ;
    ;; 其余字段：直接透传字段定义里的静态 options / options-pretty。
    (else (list options options-pretty))
  ) ;cond
) ;define

;; field -> assoc-list field-descriptor：紧凑格式 (key label options
;; options-pretty editable? . flags) 转成 bridge 可消费的 field-descriptor
;; （参考 ParagraphFormat 的 meta 输出）。用 let* + list-ref 显式取各位置——
;; mogan 的 with 对 5 元 + rest 解构行为不稳。
;;
;; kind 分流：options 非空 -> combo；options 空 + key 非空 -> toggle；key 空 -> info。

(define (preferences-qml-field->descriptor field)
  (let* ((key (list-ref field 0))
         (label (list-ref field 1))
         (options (list-ref field 2))
         (options-pretty (list-ref field 3))
         (editable? (list-ref field 4))
         (flags (field-flags field))
         (opt-pairs (preferences-qml-resolve-options key options options-pretty))
         (final-options (car opt-pairs))
         (final-options-pretty (cadr opt-pairs))
         (kind (cond ((list-find flags (lambda (x) (== x 'info))) "info")
                     ((or (nlist? final-options) (null? final-options))
                      (if (== key "") "info" "toggle")
                     ) ;
                     (else "combo")
               ) ;cond
         ) ;kind
         (base (list (cons 'kind kind) (cons 'key key) (cons 'label (translate label))))
         (value-pairs (if (== kind "combo")
                        (list (cons 'options final-options)
                          (cons 'optionsTr (map translate final-options-pretty))
                          (cons 'editable editable?)
                          (cons 'value
                            (preferences-qml-current-value key kind final-options final-options-pretty)
                          ) ;cons
                        ) ;list
                        (list (cons 'value (preferences-qml-current-value key kind '() '())))
                      ) ;if
         ) ;value-pairs
         (flag-pairs (preferences-qml-flags->assoc flags))
        ) ;
    (append base value-pairs flag-pairs)
  ) ;let*
) ;define

;; tab builder：遍历字段定义列表（先平台过滤）、map 成 assoc-list field-descriptor。

(tm-define (preferences-qml-build-tab fields)
  (map preferences-qml-field->descriptor
    (list-filter fields preferences-qml-platform-shows?)
  ) ;map
) ;tm-define
