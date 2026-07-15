;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : paragraph-format-widgets.scm
;; DESCRIPTION : QML 段落格式对话框的 scheme facade（「格式→段落」与「文档→段落」共用）。
;;               int 句柄注册表 + 按 scope 分流的双读写通路（段落 with / 文档 initial）
;;               + 打开时快照（Cancel 撤销）。显示真相源在 QML values（参考 FormDialog），
;;               scheme 不维护运行期当前值——避开写回相对读取的延迟导致的显示滞后。
;; COPYRIGHT   : (C) 2026  Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic paragraph-format-widgets) (:use (generic format-edit)))

;; 段落参数清单与取值（迁移自原 format-widgets.scm 的 tm-widget enum 取值）。
;; editable=#t 的字段允许 QML 端双击键入预设外的自定义值（对应原 enum 末尾空串槽位）。

(define paragraph-basic-fields
  (list (list "par-mode" "Alignment" '("left" "center" "right" "justify") #f)
    (list "par-left" "Left margin" '("0tab" "1tab" "2tab") #t)
    (list "par-right" "Right margin" '("0tab" "1tab" "2tab") #t)
    (list "par-first" "First indentation" '("0fn" "2fn") #t)
    (list "par-sep" "Interline space" '("0fn" "0.2fn" "0.25fn" "0.5fn" "1fn") #t)
    (list "par-par-sep"
      "Interparagraph space"
      '("0fn" "0.3333fn" "0.5fn" "0.6666fn" "1fn" "0.5fns")
      #t
    ) ;list
    (list "par-columns" "Number of columns" '("1" "2" "3" "4" "5" "6") #f)
    (list "par-columns-sep" "Column separation" '("1fn" "2fn" "3fn") #t)
  ) ;list
) ;define

(define paragraph-advanced-fields
  (list (list "par-hyphen" "Line breaking" '("normal" "professional") #f)
    (list "par-line-sep"
      "Extra interline space"
      '("0fn" "0.025fns" "0.05fns" "0.1fns" "0.2fns" "0.5fns" "1fns")
      #t
    ) ;list
    (list "par-ver-sep"
      "Minimal line separation"
      '("0fn" "0.1fn" "0.2fn" "0.5fn" "1fn")
      #t
    ) ;list
    (list "par-hor-sep"
      "Horizontal collapse distance"
      '("0.1fn" "0.2fn" "0.5fn" "1fn" "2fn" "5fn" "10fn" "100fn")
      #t
    ) ;list
    (list "par-flexibility" "Space stretchability" '("1" "2" "4" "1000") #t)
    (list "par-spacing"
      "CJK spacing"
      '("plain" "quanjiao" "banjiao" "hangmobanjiao" "kaiming")
      #f
    ) ;list
    (list "par-kerning-stretch"
      "Intercharacter stretching"
      '("auto" "tolerant" "0" "0.02" "0.05" "0.1" "0.2" "0.5" "1")
      #t
    ) ;list
    (list "par-kerning-reduce"
      "Intercharacter compression"
      '("auto" "0" "0.01" "0.02" "0.03" "0.05" "0.1" "0.2")
      #t
    ) ;list
    (list "par-expansion"
      "Character expansion"
      '("auto" "tolerant" "0" "0.01" "0.02" "0.05" "0.1" "0.2")
      #t
    ) ;list
    (list "par-contraction"
      "Character contraction"
      '("auto" "tolerant" "0" "0.01" "0.02" "0.05" "0.1" "0.2")
      #t
    ) ;list
    (list "par-kerning-margin" "Use margin kerning" '("false" "true") #f)
  ) ;list
) ;define

;; 行间距预设按钮（label, val），val 为 par-sep 值，从小到大排列。

(define paragraph-sep-presets
  (list (list "1.0x" "0fn")
    (list "1.25x" "0.25fn")
    (list "1.5x" "0.5fn")
    (list "2.0x" "1fn")
  ) ;list
) ;define

;; int 句柄注册表：C++ bridge 持 int key，scheme 侧用它找回 specs。specs 是三元组
;; (scope getter setter)——scope 决定读写通路：
;;   'paragraph：get-env 读段落环境，make-multi-line-with 写段落 with。
;;   'document：get-init 读文档初始环境，init-multi 写文档 initial。
;; 两条通路对同一批 par-* 参数名，写回位置不同（段落 with vs 文档 initial），故按
;; scope 分流，不可混用。

(define paragraph-specs-registry (make-ahash-table))

;; specsKey 分配：优先复用 cleanup 回收的 key（自由链表），不够再自增。
;; 避免单调递增导致句柄无限增长（长期反复开关对话框 / 测试反复 register）。

(define paragraph-specs-next 1)

(define paragraph-specs-free (list))

;; key -> ahash(var -> val)：对话框打开瞬间的参数快照，供 Cancel 回放。
;; 必须在 live 改动前快照——live 写回会改文档树/init，事后读到的是改后值。
;; 运行期当前值不在此维护——显示真相源在 QML values（参考 FormDialog）。
;; 注：文档级「重置」走 init-default（恢复默认），不用快照；快照仅 Cancel 用。

(define paragraph-snapshot (make-ahash-table))

;; key -> ahash(var -> bool)：文档级打开瞬间各字段是否有显式 init（init-has?）。
;; 仅 'document scope 填充。Cancel 写回时用——打开时无显式 init 的字段走
;; init-default（移除 init，回到继承全局默认），而非把快照里的默认值字符串
;; 固化成显式 init（否则文档树多出冗余 init，且后续改全局默认不再跟随）。
;; 'paragraph scope 不填（段落 with 无「有无」之分，快照值即真相）。

(define paragraph-snapshot-has (make-ahash-table))

;; key -> scope：标记该 specs 是段落级还是文档级，读写/revert 按 scope 分流。

(define paragraph-scope (make-ahash-table))

(tm-define (paragraph-format-lookup-specs key)
  (ahash-ref paragraph-specs-registry key)
) ;tm-define

;; 分配一个 specsKey：优先复用 paragraph-specs-free 里回收的，否则自增
;; paragraph-specs-next。用 helper 返回值承载分配结果（更新自由链表/计数器的
;; 副作用），避免在 with 的初始化表达式里 set! 绑定变量——初始化时该变量绑定
;; 尚未建立，会 unbound。

(define (paragraph-format-alloc-key)
  (if (nnull? paragraph-specs-free)
    (with key
      (car paragraph-specs-free)
      (set! paragraph-specs-free (cdr paragraph-specs-free))
      key
    ) ;with
    (with key
      paragraph-specs-next
      (set! paragraph-specs-next (+ paragraph-specs-next 1))
      key
    ) ;with
  ) ;if
) ;define

;; specs 形如 (scope getter setter)：scope ∈ {'paragraph,'document}，getter/setter
;; 分别为 get-env/make-multi-line-with 或 get-init/init-multi。段落级默认。
(tm-define (paragraph-format-register-specs specs)
  (with (scope getter setter)
    specs
    (with key
      (paragraph-format-alloc-key)
      (ahash-set! paragraph-specs-registry key specs)
      (ahash-set! paragraph-scope key scope)
      ;; 快照打开瞬间的参数（live 改动前）。getter 按 scope 读 env 或 init。
      (with snap
        (make-ahash-table)
        (for (f (paragraph-all-fields-for scope))
          (ahash-set! snap (car f) (getter (car f)))
        ) ;for
        (ahash-set! paragraph-snapshot key snap)
      ) ;with
      ;; 文档级另记打开瞬间各字段是否有显式 init——Cancel 写回按此决定
      ;; init-default（移除）还是写回快照值，避免把默认值固化成显式 init。
      (when (== scope 'document)
        (with has
          (make-ahash-table)
          (for (f (paragraph-all-fields-for scope))
            (ahash-set! has (car f) (init-has? (car f)))
          ) ;for
          (ahash-set! paragraph-snapshot-has key has)
        ) ;with
      ) ;when
      key
    ) ;with
  ) ;with
) ;tm-define

(tm-define (paragraph-format-cleanup key)
  (ahash-remove! paragraph-specs-registry key)
  (ahash-remove! paragraph-snapshot key)
  (ahash-remove! paragraph-snapshot-has key)
  (ahash-remove! paragraph-scope key)
  ;; 回收 key 供下次 register 复用。
  (set! paragraph-specs-free (cons key paragraph-specs-free))
) ;tm-define

;; 按 scope 返回某分组字段表。文档级基础 tab 去掉 par-left/par-right（边距对
;; 文档初始环境无意义，与原 tm-widget flag?=#t 的 assuming (not flag?) 一致）；
;; 高级 tab 两级相同。

(define (paragraph-fields-for scope which)
  (with fields
    (if (== which "basic") paragraph-basic-fields paragraph-advanced-fields)
    (if (and (== scope 'document) (== which "basic"))
      (list-filter fields
        (lambda (f) (not (in? (car f) (list "par-left" "par-right"))))
      ) ;list-filter
      fields
    ) ;if
  ) ;with
) ;define

;; 某 scope 的 basic+advanced 全字段（register 快照 / restore 遍历共用）。

(define (paragraph-all-fields-for scope)
  (append (paragraph-fields-for scope "basic")
    (paragraph-fields-for scope "advanced")
  ) ;append
) ;define

;; 全部字段名（basic+advanced 的 var），供文档级 init-default 恢复默认。

(define (paragraph-all-var-names)
  (map car (append paragraph-basic-fields paragraph-advanced-fields))
) ;define

;; 返回某分组字段的 meta 列表（供 QML 打开时读一次初始化 values）。value 用 getter
;; 按 scope 读（仅打开时读，运行期不重读——显示真相源是 QML values）。label 走
;; translate（与 FormDialog 一致，scheme 侧翻译好再传 QML）。
(tm-define (paragraph-format-meta key which)
  (with specs
    (ahash-ref paragraph-specs-registry key)
    (with (scope getter setter)
      specs
      (with fields
        (paragraph-fields-for scope which)
        (map (lambda (f)
               (with (var label options editable)
                 f
                 (list (cons 'label (translate label))
                   (cons 'options options)
                   (cons 'var var)
                   (cons 'value (getter var))
                   (cons 'editable editable)
                 ) ;list
               ) ;with
             ) ;lambda
          fields
        ) ;map
      ) ;with
    ) ;with
  ) ;with
) ;tm-define

;; live 写回单参数：setter 按 scope 写段落 with 或文档 initial。段落级
;; make-multi-with 期望扁平 (var val ...)（与原 widget differences-list 一致）；
;; 文档级 init-multi 同样期望扁平 (var val ...)。显示真相源在 QML values，不重读。
(tm-define (paragraph-format-set key var val)
  (with specs
    (ahash-ref paragraph-specs-registry key)
    (with (_ getter setter) specs (setter (list var val)))
  ) ;with
  val
) ;tm-define

;; 快照写回（Cancel 用）：把所有与当前不同的参数写回打开时的值。setter 按 scope
;; 选 make-multi-line-with（段落 with）或 init-multi（文档 initial），扁平
;; (var val ...) 一次写入，避免段落级多次嵌套 with 吞选区。
;; 文档级修正：打开时无显式 init 的字段写回 :default——init-multi 命中
;; (== (cadr l) :default) 走 init-default 移除 init，回到继承全局默认；否则会把
;; 快照里的默认值字符串固化成冗余显式 init。段落级无此问题（with 无「有无」之分）。

;; 判断某字段写回时是否该走 :default（仅文档级 + 打开时无显式 init 时为真）。

(define (paragraph-format-restore-default? scope has var)
  (and (== scope 'document) has (not (ahash-ref has var)))
) ;define

(define (paragraph-format-restore-snapshot key)
  (with specs
    (ahash-ref paragraph-specs-registry key)
    (when specs
      (with (scope getter setter)
        specs
        (with snap
          (ahash-ref paragraph-snapshot key)
          (when snap
            (with has
              (ahash-ref paragraph-snapshot-has key)
              (with changes
                (list)
                ;; 按 scope 取字段（与 register-specs 快照同源），文档级不含 par-left/par-right。
                (for (f (paragraph-all-fields-for scope))
                  (with var
                    (car f)
                    (with old
                      (ahash-ref snap var)
                      (when (and old (!= old (getter var)))
                        ;; :default 让 init-multi 移除 init（文档级打开时无显式 init），
                        ;; 其余写快照值。段落级 restore-default? 恒为 #f，写 old。
                        (with restore-val
                          (if (paragraph-format-restore-default? scope has var) :default old)
                          (set! changes (cons* var restore-val changes))
                        ) ;with
                      ) ;when
                    ) ;with
                  ) ;with
                ) ;for
                (when (nnull? changes)
                  (setter changes)
                ) ;when
              ) ;with
            ) ;with
          ) ;when
        ) ;with
      ) ;with
    ) ;when
  ) ;with
) ;define

;; 重置（重置按钮）：文档级 init-default 恢复默认（与原 tm-widget flag?=#t 的 Reset
;; 一致——文档级 Reset 是「恢复默认」）；段落级快照写回（回到打开时）。不关窗。
;; 文档级用本 facade 已知的全部字段名做 init-default，不依赖 format-widgets 的
;; paragraph-parameters（跨模块变量在此 unbound）。
(tm-define (paragraph-format-revert key)
  (with scope
    (ahash-ref paragraph-scope key)
    (if (== scope 'document)
      (apply init-default (paragraph-all-var-names))
      (paragraph-format-restore-snapshot key)
    ) ;if
  ) ;with
) ;tm-define

;; 落定：改动已随每次 set live 写回，commit 只需注销 specs。
(tm-define (paragraph-format-commit key) (paragraph-format-cleanup key))

;; 取消：快照写回（回到打开时，两级共用）+ 注销。
(tm-define (paragraph-format-cancel key)
  (paragraph-format-restore-snapshot key)
  (paragraph-format-cleanup key)
) ;tm-define

;; UI 标签（translate 走项目翻译）。
(tm-define (paragraph-format-ui-labels)
  (list (cons 'basic (translate "Basic"))
    (cons 'advanced (translate "Advanced"))
    (cons 'reset (translate "Reset"))
    (cons 'ok (translate "Ok"))
    (cons 'cancel (translate "Cancel"))
    (cons 'sepPresetLabel (translate "Line spacing presets"))
    (cons 'sepPresets
      (map (lambda (p) (list (cons 'label (car p)) (cons 'val (cadr p))))
        paragraph-sep-presets
      ) ;map
    ) ;cons
  ) ;list
) ;tm-define
