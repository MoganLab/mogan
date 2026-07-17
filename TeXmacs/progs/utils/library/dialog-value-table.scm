;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : dialog-value-table.scm
;; DESCRIPTION : QML 非阻塞模态对话框（live 写回文档）的「本地真相表」reader。
;;               供带 reset/Cancel 的对话框（FontSelector / ParagraphFormat）共用，
;;               避开「写树 → 跨 eval 读树」的延迟——reset/Cancel 把目标值写入本表，
;;               读取经 value-table-ref 表优先命中，即时生效，不碰文档树。
;; COPYRIGHT   : (C) 2026  Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY whatsoever. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils library dialog-value-table))

;; 表本体。entry-key 由调用方完全自定义——它须能唯一标识「某对话框实例的某字段」。
;; FontSelector 用 (specs var buffer)，ParagraphFormat 用 (int-key var)。本模块对 key
;; 内部形状不做任何假设，只把它当不透明比较键。

(define value-table (make-ahash-table))

;; 使用契约（典型对话框生命周期，调用方在对应阶段调下列 API）：
;;
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
;;
;; 关键不变量：凡写入文档树的路径（set/reset/cancel）都必须同步更新本表，否则 meta
;; 读到的表值会与文档真相背离。fallback 只在表缺项时兜底（正常路径表必命中）。

;; 读：表优先，缺项才调 fallback-proc（调用方注入的实时读取源，如 get-env/get-init
;; 或字体 initial-font-data）。fallback 无副作用，调用方负责。
(tm-define (value-table-ref entry-key fallback-proc)
  (or (ahash-ref value-table entry-key) (fallback-proc))
) ;tm-define

;; 写：记下运行期当前值（live 改动 / reset / Cancel 回放都经此）。写后下次 ref 命中表。
(tm-define (value-table-set! entry-key val)
  (ahash-set! value-table entry-key val)
) ;tm-define

;; 删单项：回退到 fallback 实时源（reset 单字段场景）。
(tm-define (value-table-remove! entry-key)
  (ahash-remove! value-table entry-key)
) ;tm-define

;; 清整组：注销 specs（对话框关闭）时回收该实例下所有字段，防泄漏。
;; entry-keys 为该实例全部字段的 entry-key 列表（调用方按自己的 keying 规则算好）。
(tm-define (value-table-clean entry-keys)
  (for (k entry-keys) (ahash-remove! value-table k))
) ;tm-define
