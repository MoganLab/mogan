;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : paragraph-format-widgets.scm
;; DESCRIPTION : QML 段落格式对话框的 scheme facade。int 句柄注册表 + live 写回
;;               （make-multi-line-with）+ 打开时快照（Cancel/重置撤销）。显示真相源
;;               在 QML values（参考 FormDialog），scheme 不维护运行期当前值——避开
;;               make-multi-line-with 相对 get-env 的延迟导致的显示滞后。
;; COPYRIGHT   : (C) 2026  Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic paragraph-format-widgets) (:use (generic format-edit)))

;; 段落参数清单与取值（与 format-widgets.scm 的 enum 取值一一对应）。
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

;; int 句柄注册表：C++ bridge 持 int key，scheme 侧用它找回 specs（仅为句柄映射，
;; getter/setter 未使用——段落直接用 get-env/make-multi-line-with）。

(define paragraph-specs-registry (make-ahash-table))

(define paragraph-specs-next 1)

;; key -> ahash(var -> val)：对话框打开瞬间的段落参数快照，供 Cancel/重置回放。
;; 必须在 live 改动前快照——live 写回会改文档树，事后 get-env 读到的是改后值。
;; 运行期当前值不在此维护——显示真相源在 QML values（参考 FormDialog）。

(define paragraph-snapshot (make-ahash-table))

(tm-define (paragraph-format-lookup-specs key)
  (ahash-ref paragraph-specs-registry key)
) ;tm-define

(tm-define (paragraph-format-register-specs specs)
  (with key
    paragraph-specs-next
    (ahash-set! paragraph-specs-registry key specs)
    ;; 快照打开瞬间的段落参数（live 改动前）。
    (with snap
      (make-ahash-table)
      (for (f (append paragraph-basic-fields paragraph-advanced-fields))
        (ahash-set! snap (car f) (get-env (car f)))
      ) ;for
      (ahash-set! paragraph-snapshot key snap)
    ) ;with
    (set! paragraph-specs-next (+ paragraph-specs-next 1))
    key
  ) ;with
) ;tm-define

(tm-define (paragraph-format-cleanup key)
  (ahash-remove! paragraph-specs-registry key)
  (ahash-remove! paragraph-snapshot key)
) ;tm-define

;; 返回某分组字段的 meta 列表（供 QML 打开时读一次初始化 values）。value 用 get-env
;; （仅打开时读，运行期不重读——显示真相源是 QML values）。label 走 translate（与
;; FormDialog 一致，scheme 侧翻译好再传 QML）。
(tm-define (paragraph-format-meta key which)
  (with fields
    (if (== which "basic") paragraph-basic-fields paragraph-advanced-fields)
    (map (lambda (f)
           (with (var label options editable)
             f
             (list (cons 'label (translate label))
               (cons 'options options)
               (cons 'var var)
               (cons 'value (get-env var))
               (cons 'editable editable)
             ) ;list
           ) ;with
         ) ;lambda
      fields
    ) ;map
  ) ;with
) ;tm-define

;; live 写回单参数：make-multi-line-with 写文档。make-multi-with 期望扁平
;; (var val var val ...)（与原 widget differences-list = assoc->list 一致）。显示真相源
;; 在 QML values，不重读 get-env（延迟会显示滞后）。
(tm-define (paragraph-format-set key var val)
  (make-multi-line-with (list var val))
  val
) ;tm-define

;; 快照撤销（Cancel/重置共用）：一次 make-multi-line-with 写所有差异，避免多次嵌套
;; with 吞选区。revert 不关窗（重置用）；cancel 在 bridge 层再关窗。
(tm-define (paragraph-format-revert key)
  (with snap
    (ahash-ref paragraph-snapshot key)
    (when snap
      (with changes
        (list)
        (for (f (append paragraph-basic-fields paragraph-advanced-fields))
          (with var
            (car f)
            (with old
              (ahash-ref snap var)
              (when (and old (!= old (get-env var)))
                ;; make-multi-line-with 期望扁平 (var val var val ...)。
                (set! changes (cons* var old changes))
              ) ;when
            ) ;with
          ) ;with
        ) ;for
        (when (nnull? changes)
          (make-multi-line-with changes)
        ) ;when
      ) ;with
    ) ;when
  ) ;with
) ;tm-define

;; 落定：改动已随每次 set live 写回，commit 只需注销 specs。
(tm-define (paragraph-format-commit key) (paragraph-format-cleanup key))

;; 取消：快照撤销 + 注销。
(tm-define (paragraph-format-cancel key)
  (paragraph-format-revert key)
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
