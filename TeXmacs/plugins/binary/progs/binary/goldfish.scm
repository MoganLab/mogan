
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldfish.scm
;; DESCRIPTION : goldfish Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary goldfish) (:use (binary common)))

(define (goldfish-binary-candidates)
  (cond
   ((os-windows?)
    (list (string-append (url->system (get-texmacs-path))
            "/plugins/goldfish/bin/goldfish.exe"
          ) ;string-append
    ) ;list
   ) ;
   (else
     (list (string-append (url->system (get-texmacs-path))
             "/plugins/goldfish/bin/goldfish"
           ) ;string-append
     ) ;list
   ) ;else
  ) ;cond
) ;define

(tm-define (find-binary-goldfish)
  (:synopsis "Find the url to the goldfish binary, return (url-none) if not found"
  ) ;:synopsis
  (find-binary (goldfish-binary-candidates) "goldfish")
) ;tm-define

(tm-define (has-binary-goldfish?) (not (url-none? (find-binary-goldfish))))

(tm-define (version-binary-goldfish) (version-binary (find-binary-goldfish)))

(tm-define (goldfish-plugin-launcher plugin-name)
  (:synopsis "Launch command for the goldfish entry of a plugin")
  (:argument plugin-name "Plugin name, e.g. llm or attachment")
  ;; 返回命令串 <goldfish> load <入口脚本>。入口脚本沿用户目录覆盖系统
  ;; 目录的搜索路径解析（同 plugin-initialize 的 init 文件解析方式）
  (let* ((entry-rel (string-append "plugins/" plugin-name "/goldfish/tm-" plugin-name ".scm")
         ) ;entry-rel
         (entry (url-materialize (url-unix "$TEXMACS_HOME_PATH:$TEXMACS_PATH" entry-rel) "r")
         ) ;entry
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " load "
      (string-quote entry)
    ) ;string-append
  ) ;let*
) ;tm-define

(tm-define (goldfish-plugin-serializer lan t)
  (:synopsis "Serializer for plugins speaking %-prefixed protocol lines")
  (:argument lan "Plugin language")
  (:argument t "Stree to serialize")
  ;; connection-write 调用前已做 tree_herk_to_utf8，传入的 stree 字符串是
  ;; UTF-8。% 开头的单字符串文档（%xxx 协议行）须原样透传：object->string
  ;; 会给字符串加引号/转义并包 (document ...) 外壳，子进程将无法识别协议行
  (if (and (pair? t)
        (>= (length t) 2)
        (eq? (car t) 'document)
        (string? (cadr t))
        (string-starts? (cadr t) "%")
      ) ;and
    (string-append (cadr t) "\n<EOF>\n")
    (string-append (object->string t) "\n<EOF>\n")
  ) ;if
) ;tm-define
