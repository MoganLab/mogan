
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-print.scm
;; DESCRIPTION : routines for printing documents
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-print)
  (:use (texmacs texmacs tm-files) (utils library cursor))
) ;texmacs-module

(import (only (liii uuid) uuid4))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Try to obtain the papersize in this order from
;; - the environment variable PAPERSIZE
;; - the contents of the file specified by the PAPERCONF environment variable
;; - the contents of the file "/etc/papersize"
;; or else default to "a4"
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define supported-sizes
  '("a0" "a1" "a2" "a3" "a4" "a5" "a6" "a7" "a8" "a9" "b0" "b1" "b2" "b3" "b4"
    "b5" "b6" "b7" "b8" "b9" "archA" "archB" "archC" "archD" "archE" "10x14"
    "11x17" "C5" "Comm10" "DL" "executive" "halfletter" "halfexecutive" "ledger"
    "legal" "letter" "Monarch" "csheet" "dsheet" "flsa" "flse" "folio"
    "lecture note" "note" "quarto" "statement" "tabloid" "16:9" "8:5" "3:2"
    "4:3" "5:4" "user")
) ;define

(define standard-sizes
  '("a0" "a1" "a2" "a3" "a4" "a5" "a6" "b3" "b4" "b5" "b6" "ledger" "legal"
    "letter" "folio")
) ;define

(tm-define (correct-paper-size s)
  (if (and (string? s) (in? s supported-sizes)) s "a4")
) ;tm-define

(tm-define (standard-paper-size s)
  (if (and (string? s) (in? s standard-sizes)) s "user")
) ;tm-define

(tm-define (get-default-paper-size) (correct-paper-size "a4"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Printing preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define preview-command "default")

(define (notify-preview-command var val)
  (set! preview-command val)
) ;define

(define (notify-printing-command var val)
  (set-printing-command val)
) ;define

(define (notify-paper-type var val)
  (set-printer-paper-type (locase-first val))
) ;define

(define (notify-printer-dpi var val)
  (set-printer-dpi val)
) ;define

(define-preferences ("texmacs->pdf:expand slides" "on" noop)
 ("preview command" "default" notify-preview-command)
 ("use external pdf viewer" "off" noop)
 ("printing command" (get-default-printing-command) notify-printing-command)
 ("paper type" (get-default-paper-size) notify-paper-type)
 ("printer dpi" "1200" notify-printer-dpi)
) ;define-preferences

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Printing wrapper for slides
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (user-confirm-open-pdf fname)
  (user-simple-confirm "Open PDF?"
    #t
    (lambda (open?) (when open? (preview-file fname)))
  ) ;user-simple-confirm
) ;define

(tm-define (wrapped-print-to-file fname)
  (system-wait "Exporting, " (translate "please wait"))
  (let* ((cur (current-buffer))
         (buf (buffer-new))
         (tmp-url (url-append (url-head cur) (string-append (uuid4) "." (url-suffix cur)))
         ) ;tmp-url
        ) ;
    (buffer-copy cur buf)
    (buffer-rename buf tmp-url)
    (when (screens-buffer?)
      (buffer-set-master tmp-url cur)
      (switch-to-buffer tmp-url)
      (set-drd cur)
      (dynamic-make-slides)
    ) ;when
    (switch-to-buffer tmp-url)
    (when (has-style-package? "dark")
      (remove-style-package "dark")
    ) ;when
    (print-to-file fname)
    (switch-to-buffer cur)
    (buffer-close tmp-url)
    (let ((export-kind (string-append (url-suffix fname) "_export")))
      (save-buffer-save cur (list) export-kind)
    ) ;let
  ) ;let*
  (system-wait "" "")
  (user-confirm-open-pdf fname)
) ;tm-define

(tm-define (wrapped-print-to-pdf-embeded-with-tm fname)
  (unless (string=? (url-suffix fname) "pdf")
    (texmacs-error "Wrapped-print-to-pdf-embeded-with-tm" "fname is not a pdf")
  ) ;unless
  (system-wait "Exporting, " (translate "please wait"))
  (let* ((cur (current-buffer))
         (buf (buffer-new))
         (tmp-url (url-append (url-head cur) (string-append (uuid4) "." (url-suffix cur)))
         ) ;tmp-url
        ) ;
    (buffer-copy cur buf)
    (buffer-rename buf tmp-url)
    (when (screens-buffer?)
      (buffer-set-master tmp-url cur)
      (switch-to-buffer tmp-url)
      (set-drd cur)
      (dynamic-make-slides)
    ) ;when
    (switch-to-buffer tmp-url)
    (when (has-style-package? "dark")
      (remove-style-package "dark")
    ) ;when
    (print-to-file fname)
    (unless (attach-doc-to-exported-pdf fname)
      (notify-now "Fail to attach tm to pdf")
    ) ;unless
    (switch-to-buffer cur)
    (buffer-close tmp-url)
    (save-buffer-save cur (list) "tm_pdf_export")
  ) ;let*
  (system-wait "" "")
  (user-confirm-open-pdf fname)
) ;tm-define

(tm-define (wrapped-print-to-pdf-embeded-with-tmu fname)
  (unless (string=? (url-suffix fname) "pdf")
    (texmacs-error "Wrapped-print-to-pdf-embeded-with-tmu" "fname is not a pdf")
  ) ;unless
  (system-wait "Exporting, " (translate "please wait"))
  (let* ((cur (current-buffer))
         (buf (buffer-new))
         (tmp-url (url-append (url-head cur) (string-append (uuid4) "." (url-suffix cur)))
         ) ;tmp-url
        ) ;
    (buffer-copy cur buf)
    (buffer-rename buf tmp-url)
    (when (screens-buffer?)
      (buffer-set-master tmp-url cur)
      (switch-to-buffer tmp-url)
      (set-drd cur)
      (dynamic-make-slides)
    ) ;when
    (switch-to-buffer tmp-url)
    (when (has-style-package? "dark")
      (remove-style-package "dark")
    ) ;when
    (print-to-file fname)
    (unless (attach-doc-to-exported-pdf fname)
      (notify-now "Fail to attach tmu to pdf")
    ) ;unless
    (switch-to-buffer cur)
    (buffer-close tmp-url)
    (save-buffer-save cur (list) "tmu_pdf_export")
  ) ;let*
  (system-wait "" "")
  (user-confirm-open-pdf fname)
) ;tm-define

(define (propose-export-pdf-name embedded?)
  ;; 导出 PDF 默认名：xxx.pdf；勾选嵌入源文档（tmu 附件）时为 xxx.tmu.pdf
  ;; （对齐旧「可编辑PDF」入口的 tmu.pdf 命名）。
  (with name
    (propose-name-buffer)
    (with t
      (url->system (url-tail (system->url name)))
      (string-append (cond ((== t "") "untitled")
                           ((string-ends? t ".tmu") (string-drop-right t 4))
                           ((string-ends? t ".tm") (string-drop-right t 3))
                           ((string-ends? t ".pdf") (string-drop-right t 4))
                           (else t)
                     ) ;cond
        (if embedded? ".tmu.pdf" ".pdf")
      ) ;string-append
    ) ;with
  ) ;with
) ;define

(define (export-pdf-default-dir)
  ;; 与 choose-file 缺省目录逻辑同源（Issue #327）：scratch 优先用上次文件
  ;; 对话框目录，其次文档目录下的 LiiiSTEM；tmfs（云/帮助文档）落系统下载
  ;; 目录；本地文档用其所在目录。
  (let* ((master (buffer-get-master (current-buffer)))
         (last-dir (and (url-scratch? master)
                     (defined? 'get-last-file-dialog-directory)
                     (get-last-file-dialog-directory)
                   ) ;and
         ) ;last-dir
        ) ;
    (cond ((and last-dir (string? last-dir) (not (string-null? last-dir)))
           (system->url last-dir)
          ) ;
          ((url-scratch? master) (url-append (get-documents-path) "LiiiSTEM"))
          ((url-rooted-tmfs? master) (get-downloads-path))
          (else (url-head master))
    ) ;cond
  ) ;let*
) ;define

(tm-define (export-as-pdf)
  (:synopsis "Export as PDF, optionally embedding the source document")
  ;; QML 对话框只收集一个选项：是否把源文档作为附件嵌入 PDF。确认时
  ;; cpp-export-pdf-dialog 返回 (tuple (tuple "embed" "true"/"false"))，Cancel
  ;; / 关闭返回空树。选项初值与 label 在此侧取好（label 已翻译）。
  (with result
    (cpp-export-pdf-dialog (stree->tree `(export-pdf-form (toggle ,(translate "Embed source document")
                                                            ,"embed"
                                                            ,"false"))
                           ) ;stree->tree
    ) ;cpp-export-pdf-dialog
    ;; tree->stree 后每个 kv 为 (tuple key value)：cadr=key、caddr=value。
    (with r
      (cdr (tree->stree result))
      (if (null? r)
        (noop)
        (with embed
          #f
          (for-each (lambda (kv) (when (== (cadr kv) "embed") (set! embed (== (caddr kv) "true"))))
            r
          ) ;for-each
          ;; 目的地走通用文件选择，默认文件名随「嵌入源文档」联动：
          ;; xxx.tmu.pdf / xxx.pdf。
          (choose-file (if embed wrapped-print-to-pdf-embeded-with-tmu wrapped-print-to-file)
            "Save pdf file"
            "pdf"
            "Save as:"
            (url-append (export-pdf-default-dir)
              (system->url (propose-export-pdf-name embed))
            ) ;url-append
          ) ;choose-file
        ) ;with
      ) ;if
    ) ;with
  ) ;with
) ;tm-define

(tm-define (attach-doc-to-exported-pdf fname)
  (let* ((tem-url (buffer-new))
         (new-url (url-relative tem-url (url-basename fname)))
         (cur-url (current-buffer-url))
         (cur-tree (buffer-get cur-url))
         (linked-file (pdf-get-linked-file-paths cur-tree cur-url))
         (linked-file-with-main (array-url-append new-url linked-file))
         (new-tree (pdf-replace-linked-path cur-tree cur-url))
        ) ;
    (buffer-rename tem-url new-url)
    (buffer-copy cur-url new-url)
    ;; copy also attachments and auxiliary data
    (with-buffer cur-url
      (let* ((attl (list-attachments))
             (atts (map get-attachment attl))
             (auxl (list-auxiliaries))
             (auxs (map get-auxiliary auxl))
            ) ;
        (with-buffer new-url
          (for-each set-attachment attl atts)
          (for-each set-auxiliary auxl auxs)
        ) ;with-buffer
      ) ;let*
    ) ;with-buffer
    (buffer-save new-url)
    (pdf-make-attachments fname linked-file-with-main fname)
    (buffer-close new-url)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Printing commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (propose-postscript-name)
  (with name
    (propose-name-buffer)
    (cond ((string-ends? name ".tm") (string-append (string-drop-right name 3) ".ps"))
          ((string-ends? name ".tmu") (string-append (string-drop-right name 4) ".ps"))
          (else name)
    ) ;cond
  ) ;with
) ;define

(tm-property (print-to-file name)
  (:synopsis "Print to file")
  (:argument name print-file "File name")
  (:default name (propose-postscript-name))
) ;tm-property

(tm-property (print-pages first last)
  (:synopsis "Print page selection")
  (:argument first "First page")
  (:proposals first (list "1" ""))
  (:argument last "Last page")
  (:proposals last (list (number->string (get-page-count)) ""))
) ;tm-property

(tm-property (print-pages-to-file name first last)
  (:synopsis "Print page selection to file")
  (:argument name print-file "File name")
  (:default name (propose-postscript-name))
  (:argument first "First page")
  (:proposals first (list "1" ""))
  (:argument last "Last page")
  (:proposals last (list (number->string (get-page-count)) ""))
) ;tm-property

(tm-define (preview-file u)
  (if (get-boolean-preference "use external pdf viewer")
    (load-external u)
    (load-pdf-buffer u)
  ) ;if
) ;tm-define

(tm-define (preview-buffer)
  (let ((export-kind (string-append (if (supports-native-pdf?) "pdf" "ps") "_export")))
    (save-buffer-save (current-buffer) (list) export-kind)
  ) ;let
  (with-default-view (with file
                       (url-glue (url-temp) (if (supports-native-pdf?) ".pdf" ".ps"))
                       (print-to-file file)
                       (preview-file file)
                     ) ;with
  ) ;with-default-view
) ;tm-define

(tm-define (print-page-selection-to-file)
  (:synopsis "Print page selection to file")
  ;; QML 对话框收集路径 + 页码范围，一次提交；用户 OK 时 cpp-print-to-file-dialog
  ;; 返回 (tuple (tuple "name" <n>) (tuple "first" <f>) (tuple "last" <l>))，Cancel
  ;; / 关闭返回空树。字段初值在此侧取好（propose-postscript-name 建议文件名、
  ;; 页码范围默认 1..总页数），label 已翻译。
  (with result
    (cpp-print-to-file-dialog (stree->tree `(print-to-file-form (path ,(translate "File name:")
                                                                  ,"name"
                                                                  ,(propose-postscript-name))
                                              (number ,(translate "First page:")
                                                ,"first"
                                                ,"1")
                                              (number ,(translate "Last page:")
                                                ,"last"
                                                ,(number->string (get-page-count))))
                              ) ;stree->tree
    ) ;cpp-print-to-file-dialog
    ;; tree->stree 后每个 kv 为 (tuple key value)：cadr=key、caddr=value。
    (with r
      (cdr (tree->stree result))
      (if (null? r)
        (noop)
        (with name
          ""
          first
          ""
          last
          ""
          (for-each (lambda (kv)
                      (let ((k (cadr kv)) (v (caddr kv)))
                        (cond ((== k "name") (set! name v))
                              ((== k "first") (set! first v))
                              ((== k "last") (set! last v))
                        ) ;cond
                      ) ;let
                    ) ;lambda
            r
          ) ;for-each
          (print-pages-to-file name first last)
        ) ;with
      ) ;if
    ) ;with
  ) ;with
) ;tm-define
