;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : dialog-value-table.scm
;; DESCRIPTION : live 写回文档的 QML 对话框（FontSelector / ParagraphFormat）共用的
;;               「本地真相表」：reset/Cancel 把目标值写入本表，读取经 value-table-ref
;;               表优先命中、即时生效，避开「写树 → 跨 eval 读树」的延迟。
;; COPYRIGHT   : (C) 2026  Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY whatsoever. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils library dialog-value-table))

;; entry-key 由调用方自定义，唯一标识「某对话框实例的某字段」：FontSelector 用
;; (specs var buffer)，ParagraphFormat 用 (int-key var)。模块对 key 形状不做假设。
;;
;; 不变量：凡写文档树的路径（set/reset/cancel）都必须同步更新本表，否则 meta 读到的
;; 表值会与文档背离。适用前提：对话框打开期间文档不被其它入口改动（read-through 缓存
;; 无外部失效钩子；当前对话框为非阻塞模态、QML 仅在打开与 reset 后重读 meta，成立）。
;;
;; falsy 值：底层 s7 hash-table 把「存 #f」当删除（ahash-remove! 即 set #f），故 #f 不能
;; 作缓存值。ref 用 (or ...) 正合此意——缺项（含被 set #f 的）走 fallback。调用方只缓存
;; 非空 string，此前提天然满足。

(define value-table (make-ahash-table))

;; 使用契约（典型对话框生命周期，调用方在对应阶段调下列 API）：
;;   register（打开时，live 改动前）
;;     └─ value-table-set! 逐字段写入打开时值——之后 meta 读命中表、不读树。
;;   meta（读取显示值）
;;     └─ value-table-ref key (lambda () (实时源))——表优先，缺项才 fallback 实时源。
;;   set（live 写回）
;;     └─ setter 写树 + value-table-set! 同步写表——下次 meta 读命中表、不读树。
;;   reset / cancel（撤销）
;;     └─ 撤销写树 + value-table-set! 把目标值写表（快照值或 init-default 后的默认值）
;;        ——随后 QML 重读 meta 走表，即时生效，避跨 eval 读树滞后。
;;   cleanup（关窗注销）
;;     └─ value-table-clean 该实例全字段 entry-keys，防泄漏。

;; 读：表优先，缺项才调 fallback-proc（调用方注入的实时读取源）。
(tm-define (value-table-ref entry-key fallback-proc)
  (or (ahash-ref value-table entry-key) (fallback-proc))
) ;tm-define

;; 写：记当前值，下次 ref 命中表。
(tm-define (value-table-set! entry-key val)
  (ahash-set! value-table entry-key val)
) ;tm-define

;; 删单项：回退到 fallback 实时源。
(tm-define (value-table-remove! entry-key)
  (ahash-remove! value-table entry-key)
) ;tm-define

;; 清整组：关窗注销时回收该实例全部字段，防泄漏。
(tm-define (value-table-clean entry-keys)
  (for (k entry-keys) (ahash-remove! value-table k))
) ;tm-define
