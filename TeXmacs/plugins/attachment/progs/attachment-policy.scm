;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : attachment-policy.scm
;; DESCRIPTION : Host-side attachment pre-check policy (constants + pure
;;               function, shared by real/fake plugins through the same
;;               contract). No tm-define, no plugin-configure side effects.
;;
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; MIT License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 附件预检策略（宿主侧终态 — 真插件与假插件共同遵从同一规则与文案）。
;; 真插件未来可只删自己的重复定义而继续遵从本文件的契约；预检归属宿主
;; 是 6221 的架构迁移：本文件直接由 init-attachment.scm 顶部 load，不
;; 依赖用户目录的覆盖

(import (liii path) (liii string))

(define attachment-max-count 10)

(define attachment-max-total-bytes (* 80 1024 1024))

(define attachment-allowed-extensions
  '("pdf" "doc" "docx" "xls" "xlsx" "ppt" "pptx" "md" "txt" "csv" "epub" "stem"
    "tmu" "html" "json" "log" "c" "cpp" "h" "java" "py" "js" "ts" "go" "rs")
) ;define

;;; ---------- 预检纯函数 ----------

;; count/bytes 为本轮已选累计；size 由调用方在 (and (path-file? p)
;; (access file-path 'R_OK)) 守卫下求值后传入 — path-getsize 内的
;; file-exists? 对存在但不可读的文件抛 permission-error（boot.scm），
;; 不可访问时 size 取 0 与不可读规则前置重复时不冲突
;;
;; 通过返回 #f；违规返回原因字符串（mogan UI 原样 toast）。
;; 规则顺序：不存在 → 不可读 → 类型不支持 → 数量超限 → 总大小超限
(define (attachment-host-precheck file-path filename size count bytes)
  (let ((ext
          (let ((s (path->string (path-suffix (path filename)))))
            (if (string-starts? s ".") (string-downcase (string-drop s 1)) "")
          ) ;let
        ) ;ext
       ) ;
    (cond ((not (path-file? file-path))
           (string-append "附件文件不存在： " filename)
          ) ;
          ((not (access file-path 'R_OK))
           (string-append "附件文件不可读取： " filename)
          ) ;
          ((not (member ext attachment-allowed-extensions))
           (string-append "不支持的附件类型： " filename)
          ) ;
          ((>= count attachment-max-count)
           (string-append "附件数量超过上限（"
             (number->string attachment-max-count)
             " 个）"
           ) ;string-append
          ) ;
          ((> (+ bytes size) attachment-max-total-bytes)
           (string-append "附件总大小超过上限（"
             (number->string (/ attachment-max-total-bytes 1024 1024))
             " MiB）"
           ) ;string-append
          ) ;
          (else #f)
    ) ;cond
  ) ;let
) ;define
