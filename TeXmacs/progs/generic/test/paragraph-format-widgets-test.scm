
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : paragraph-format-widgets-test.scm
;; DESCRIPTION : Test suite for paragraph-format 行间距预设数据契约。
;; COPYRIGHT   : (C) 2026  Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic test paragraph-format-widgets-test)
  (:use (generic paragraph-format-widgets) (kernel texmacs pref-keys))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;; 行间距预设数据契约：paragraph-format-ui-labels 的 sepPresets 必须是 4 项，
;; 每项含 label/sep/parSep 三键——这是 QML delegate（点 sep/parSep 两字段）
;; 与 C++ bridge（泛型透传 assoc pair）共同依赖的形状。改动预设取值会在此失败，
;; 提示同步 QML delegate 的字段读取。

(define (test-sep-presets-count)
  (let* ((labels (paragraph-format-ui-labels)) (presets (assoc-ref labels 'sepPresets)))
    (check-true (nnull? presets))
    ;; 固定 4 档行间距预设（QML presetBtnWidth 按 4 均分按钮宽度）。
    (check-true (= (length presets) 4))
  ) ;let*
) ;define

(define (test-sep-presets-shape)
  ;; 每项必须同时含 label/sep/parSep 三键——缺任一键 QML delegate 访问 undefined。
  ;; 分别断言每个键是否存在（assoc 返回的是 pair 而非 #t，不能塞进同一个 check-true）。
  (let* ((labels (paragraph-format-ui-labels)) (presets (assoc-ref labels 'sepPresets)))
    (for-each (lambda (p)
                (check-true (not (not (assoc 'label p))))
                (check-true (not (not (assoc 'sep p))))
                (check-true (not (not (assoc 'parSep p))))
              ) ;lambda
      presets
    ) ;for-each
  ) ;let*
) ;define

(define (test-sep-presets-parsep-all-zero)
  ;; par-par-sep 全档归零是本任务的核心语义——行距由 par-sep 承担，段间距不叠加。
  (let* ((labels (paragraph-format-ui-labels)) (presets (assoc-ref labels 'sepPresets)))
    (for-each (lambda (p) (check (cdr (assoc 'parSep p)) => "0fn")) presets)
  ) ;let*
) ;define

(define (test-ui-labels-keys)
  ;; ui-labels 必须含全部按钮文案键——缺任一键 QML 取 undefined。
  (let ((labels (paragraph-format-ui-labels)))
    (for-each (lambda (k) (check-true (not (not (assoc k labels)))))
      '(basic advanced reset ok cancel sepPresetLabel)
    ) ;for-each
    ;; 文案键必须是已翻译字符串。
    (for-each (lambda (k) (check-true (string? (cdr (assoc k labels)))))
      '(basic advanced reset ok cancel sepPresetLabel)
    ) ;for-each
  ) ;let
) ;define

;; pref-keys 接线契约：field 表第一列（var）必须取自 pref-keys.scm 的 (pref-par-*) proc
;; 返回值，不能是裸字符串。本组用预期字面值清单交叉核对——既验「fields 用了 proc」
;; （fields 取值 = proc 返回值），又验「声明完整、字面一致」（19 个 proc 全部被字段用到、
;; 且与 specs builder 读写处一致）。key 改名或漏声明会在此失败。

;; basic 字段顺序与字面（与 paragraph-basic-fields specs builder 对齐）。

(define paragraph-basic-pref-keys
  (list (pref-par-mode)
    (pref-par-left)
    (pref-par-right)
    (pref-par-first)
    (pref-par-sep)
    (pref-par-par-sep)
    (pref-par-columns)
    (pref-par-columns-sep)
  ) ;list
) ;define

(define (test-basic-fields-pref-keys)
  (check (paragraph-format-basic-var-names) => paragraph-basic-pref-keys)
  ;; 固定 8 个基础字段（Basic tab）。
  (check (length (paragraph-format-basic-var-names)) => 8)
) ;define

;; advanced 字段顺序与字面（与 paragraph-advanced-fields specs builder 对齐）。

(define paragraph-advanced-pref-keys
  (list (pref-par-hyphen)
    (pref-par-line-sep)
    (pref-par-ver-sep)
    (pref-par-hor-sep)
    (pref-par-flexibility)
    (pref-par-spacing)
    (pref-par-kerning-stretch)
    (pref-par-kerning-reduce)
    (pref-par-expansion)
    (pref-par-contraction)
    (pref-par-kerning-margin)
  ) ;list
) ;define

(define (test-advanced-fields-pref-keys)
  (check (paragraph-format-advanced-var-names) => paragraph-advanced-pref-keys)
  ;; 固定 11 个高级字段（Advanced tab）。
  (check (length (paragraph-format-advanced-var-names)) => 11)
) ;define

(tm-define (regtest-paragraph-format-widgets)
  (test-sep-presets-count)
  (test-sep-presets-shape)
  (test-sep-presets-parsep-all-zero)
  (test-ui-labels-keys)
  (test-basic-fields-pref-keys)
  (test-advanced-fields-pref-keys)
  (check-report)
) ;tm-define
