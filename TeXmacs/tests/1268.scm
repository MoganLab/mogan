;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1268.scm
;; DESCRIPTION : PDF 附件（内嵌 tmu）中文文件名编码回归测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1268] 导出 PDF 并内嵌 tmu 后，okular（poppler）里附件文件名乱码。
;;   根因：写侧把 UTF-8 文件名原样写进 PDF text string，而 PDF 规范约定
;;   text string 不带 UTF-16BE BOM（FE FF）时按 PDFDocEncoding 解码，
;;   中文于是变乱码。本测试钉死两条契约：
;;     1. 写侧：附件名以 UTF-16BE + BOM 写入（字面量里是转义序列
;;        \376\377\000...，okular/poppler 据此按 UTF-16BE 解码）。
;;     2. 读侧：extract-attachments 回读时识别 BOM 并还原 UTF-8 文件名，
;;        解出的文件名与内容都和原 tmu 一致（mogan 自身的往返不回归）。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1268
;;
;; 本测试全部为同步断言（无 exec-delayed-at 异步链），headless 即可跑完。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(tm-define (test_1268)
  (define tmu-url "$TEXMACS_PATH/tests/tmu/1268_中文文件名.tmu")
  (define tmp-dir (url-temp-dir))
  ;; 基础 PDF 用仓库现成样例，避免依赖 headless 导出 PDF。
  (define base-pdf "$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf")
  (define out-pdf (url-append tmp-dir "1268_attach.pdf"))
  (define extracted (url-append tmp-dir "1268_中文文件名.tmu"))

  ;; 1 嵌入 tmu 附件。
  (check (pdf-make-attachments base-pdf (list tmu-url) out-pdf) => #t)
  (check (url-exists? out-pdf) => #t)

  ;; 2 写侧编码：附件名必须以 UTF-16BE BOM 开头。PDFHummus 会把
  ;;    0xFE/0xFF/0x00 转义成八进制 \376\377\000，故在原始字节流里查转义
  ;;    形式；修复前写的是 UTF-8 原文，不含 BOM，此断言红。
  (check (string-contains? (string-load out-pdf) "\\376\\377\\000") => #t)

  ;; 3 读侧往返：extract-attachments 解出 BOM'ed UTF-16BE 文件名后须还原
  ;;    成 UTF-8，落盘文件名与内容均与原 tmu 一致。
  (check (extract-attachments out-pdf) => #t)
  (check (url-exists? extracted) => #t)
  (check (string-load extracted) => (string-load tmu-url))

  (check-report)) ;tm-define
