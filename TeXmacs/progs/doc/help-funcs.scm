;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : help-funcs.scm
;; DESCRIPTION : loading help files
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (doc help-funcs)
  (:use (texmacs texmacs tm-files) (utils misc updater))
) ;texmacs-module

(import (liii json))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Loading help buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 帮助文件路径变量，指向TeXmacs文档根目录

(define help-file-path "$TEXMACS_DOC_PATH")
;; URL缓存哈希表，用于缓存URL的存在性检查结果

(define help-url-cache (make-ahash-table))
;; 帮助文档标题缓存，存储已解析的帮助文档标题

(define help-titles (make-ahash-table))
;; 文档解析时间记录，记录每个文档上次解析的时间

(define parse-times (make-ahash-table))

;; 解析文档标题函数
;; 参数 u: 文档URL
;; 返回值: 文档标题字符串

(define (parse-title u)
  ;; 导入文档树结构
  (with t
    (tree-import u "texmacs")
    ;; 从文档树中选择标题节点，支持多种标题标签格式
    (with tt
      (select t '(:* (:or title doc-title tmdoc-title tmdoc-title* tmweb-title)
                   :%1))
      ;; 如果没有找到标题则返回空列表，否则返回第一个标题
      (if (null? tt) '() (car tt))
    ) ;with
  ) ;with
) ;define

;; 获取帮助文件标题函数，带缓存机制
;; 参数 u: 文档URL
;; 返回值: 文档标题
(tm-define (help-file-title u)
  ;; 获取文件最后修改时间和缓存的解析时间
  (let ((mod-time (url-last-modified u)) (parse-time (or (ahash-ref parse-times u) 0)))
    ;; 如果文件已被修改或者从未解析过，则重新解析标题
    (if (> mod-time parse-time)
      (begin
        ;; 更新解析时间记录
        (ahash-set! parse-times u mod-time)
        ;; 解析并缓存文档标题
        (ahash-set! help-titles u (parse-title u))
      ) ;begin
    ) ;if
    ;; 返回缓存的标题
    (ahash-ref help-titles u)
  ) ;let
) ;tm-define

;; 检查帮助文档URL是否存在，带缓存机制
;; 参数 s: 相对路径的帮助文档名
;; 返回值: 布尔值，表示文档是否存在
(tm-define (url-exists-in-help? s)
  ;; 查找缓存中的结果
  (with entry
    (ahash-ref help-url-cache s)
    ;; 如果已有缓存结果则直接返回，否则进行实际检查并缓存结果
    (if (list? entry)
      (car entry)
      (car
        (ahash-set! help-url-cache s (list (url-exists? (url-unix help-file-path s))))
      ) ;car
    ) ;if
  ) ;with
) ;tm-define

;; 解析帮助文档的实际路径
;; 参数 s: 文档路径或URL
;; 返回值: 实际存在的文档URL，如果找不到则返回(url-none)

(define (url-resolve-help s)
  ;; 如果是完整的带有后缀的URL且文件存在，直接返回
  (if (and (in? (url-suffix s) '("tex" "tm" "tmu")) (url-exists? s))
    s
    ;; 否则根据当前语言设置查找相应语言版本的帮助文档
    (let* ((lan (string-take (language-to-locale (get-output-language)) 2))
           (suf (string-append "." lan ".tm"))
           (dir help-file-path)
          ) ;
      (cond
        ;; 查找指定语言的文档
        ((url-exists? (url-unix dir (string-append s suf)))
         (url-resolve (url-unix dir (string-append s suf)) "r")
        ) ;
        ;; 如果不是英语且存在英语文档，则使用英语文档
        ((and (!= suf ".en.tm")
           (url-exists? (url-unix dir (string-append s ".en.tm")))
           (url-resolve (url-unix dir (string-append s ".en.tm")) "r")
           ;; 如果都不存在，返回空URL
           (else (url-none))
         ) ;and
        ) ;
      ) ;cond
    ) ;let*
  ) ;if
) ;define

;; 加载帮助文档的核心函数
;; 参数 s: 文档路径
;; 参数 type: 文档类型 ("normal", "article", "book")

(define (load-help-buffer-sub s type)
  ;; 解析文档的实际路径
  (let ((name (url-resolve-help s)))
    (cond
      ;; 如果找不到文档，显示错误消息
      ((url-none? name)
       (set-message
         `(concat ,"Error: help file " (verbatim ,s) ," not found")
         "load help file"
       ) ;set-message
      ) ;
      ;; 如果是书籍类型，使用手册展开方式加载
      ((== type "book") (tmdoc-expand-help-manual name))
      ;; 其他类型使用普通帮助文档展开方式加载
      (else (tmdoc-expand-help name type))
    ) ;cond
  ) ;let
) ;define

;; 加载普通帮助文档
(tm-define (load-help-buffer s) (load-help-buffer-sub s "normal"))
;; 加载文章类型帮助文档
(tm-define (load-help-article s) (load-help-buffer-sub s "article"))
;; 加载书籍类型帮助文档
(tm-define (load-help-book s) (load-help-buffer-sub s "book"))

;; 加载在线帮助文档
(tm-define (load-help-online s)
  (load-help-buffer (url-append "https://www.texmacs.org/tmbrowse" s))
) ;tm-define


;; 获取远程欢迎页面URL

(define (get-remote-welcome-url)
  ;; 根据语言设置返回相应的欢迎页面URL
  (if (== (get-output-language) "chinese")
    "http://git.tmml.wiki/XmacsLabs/planet/raw/main/doc/welcome.zh.tm"
    "http://git.tmml.wiki/XmacsLabs/planet/raw/main/doc/welcome.en.tm"
  ) ;if
) ;define

;; 加载远程欢迎页面

(define (load-remote-welcome)
  (load-buffer (get-remote-welcome-url))
) ;define

;; 获取远程星球页面URL

(define (get-remote-planet-url)
  "http://git.tmml.wiki/XmacsLabs/planet/raw/main/index.tm"
) ;define

;; 加载远程星球页面

(define (load-remote-planet)
  (load-buffer (get-remote-planet-url))
) ;define

;; 加载Mogan欢迎页面
(tm-define (mogan-welcome)
  ;; 根据当前语言设置加载相应语言的Mogan欢迎文档
  (let* ((lan (string-take (language-to-locale (get-output-language)) 2))
         (path (string-append "$TEXMACS_PATH/doc/about/mogan/stem." lan ".tmu"))
         (en_doc (string-append "$TEXMACS_PATH/doc/about/mogan/stem.en.tmu"))
        ) ;
    ;; 优先加载本地化语言文档，如果不存在则加载英语文档
    (if (url-exists? path)
      (load-buffer (system->url path))
      (load-buffer (system->url en_doc))
    ) ;if
  ) ;let*
) ;tm-define

;; 显示带回调的消息对话框，点击 OK 后执行指定操作
(tm-define (show-message-with-callback msg title callback)
  (:interactive #t)
  (dialogue-window (message-widget msg) callback title)
) ;tm-define

;; 检查更新基础 URL

(define (get-update-base-url)
  (if (== (get-preference "stem-profile") "staging")
    "https://test.liiistem.cn"
    "https://liiistem.cn"
  ) ;if
) ;define

(define (get-latest-version-url community?)
  (string-append (get-update-base-url)
    (if community?
      "/api/v1/public/update/win-x64/latest"
      "/api/v1/public/commercial/update/win-x64/latest"
    ) ;if
  ) ;string-append
) ;define

;; 获取远程最新版本号（返回字符串，失败返回空字符串）

(define (fetch-latest-version url)
  (with content
    (string-load (string->url url))
    (if (and (string? content) (!= content ""))
      (catch #t
        (lambda ()
          (let* ((json-obj (string->json content)) (data-obj (json-ref json-obj "data")))
            (if (json-object? data-obj)
              (let ((ver (json-ref data-obj "version")))
                (if (string? ver) ver "")
              ) ;let
              ""
            ) ;if
          ) ;let*
        ) ;lambda
        (lambda (key . args) "")
      ) ;catch
      ""
    ) ;if
  ) ;with
) ;define

;; 检查更新并显示弹窗
(tm-define (check-for-updates)
  (let* ((community? (community-stem?))
         (cur-ver (xmacs-version))
         (latest-ver (fetch-latest-version (get-latest-version-url community?)))
         (url (if community?
                "https://liiistem.cn?utm_source=mogan-community&utm_medium=referral&utm_campaign=version-check"
                "https://liiistem.cn?utm_source=mogan-commercial&utm_medium=referral&utm_campaign=version-check"
              ) ;if
         ) ;url
         (failed? (or (== latest-ver "") (not (string? latest-ver))))
         (is-latest? (and (not failed?) (== cur-ver latest-ver)))
         (primary-enabled? (and (not failed?) (not is-latest?)))
         (primary-btn-label (if (use-plugin-updater?) "Update Now" "Go to Download"))
         (cancel-btn-label "Close")
         (title (translate "Check for updates"))
         (status-line (cond (failed? (translate "Failed to check for updates."))
                            (is-latest? (translate "Current version is up to date."))
                            (else (translate "A new version is available."))
                      ) ;cond
         ) ;status-line
         (ver-line (if community?
                     (replace (translate "Mogan STEM latest stable version: v%1") latest-ver)
                     (replace (translate "Liii STEM latest stable version: v%1") latest-ver)
                   ) ;if
         ) ;ver-line
         (msg (if failed?
                (string-append (replace (translate "Current version: v%1") cur-ver)
                  "\n"
                  status-line
                ) ;string-append
                (string-append (replace (translate "Current version: v%1") cur-ver)
                  "\n"
                  ver-line
                  "\n"
                  status-line
                ) ;string-append
              ) ;if
         ) ;msg
        ) ;
    (when (cpp-version-dialog title
            msg
            (translate primary-btn-label)
            (translate cancel-btn-label)
            primary-enabled?
          ) ;cpp-version-dialog
      (if (use-plugin-updater?) (updater-trigger-manual-update) (open-url url))
    ) ;when
  ) ;let*
) ;tm-define

;; 显示Mogan版本信息（兼容旧调用）
(tm-define (mogan-version) (check-for-updates))

;; 加载Xmacs星球页面
(tm-define (xmacs-planet)
  ;; 如果可以访问远程星球页面则加载，否则加载本地欢迎文档
  (if (url-exists? (get-remote-planet-url))
    (load-remote-planet)
    (load-help-article "about/welcome/new-welcome")
  ) ;if
) ;tm-define

;; 加载远程文档
(tm-define (load-remote-doc path)
  ;; 根据语言设置加载远程文档的本地化版本
  (let* ((lan (string-take (language-to-locale (get-output-language)) 2))
         (lan_doc (string-append "http://git.tmml.wiki/texmacs/doc/raw/master/"
                    path "." lan ".tm"
                  ) ;string-append
         ) ;lan_doc
         (en_doc (string-append "http://git.tmml.wiki/texmacs/doc/raw/master/" path ".en.tm")
         ) ;en_doc
        ) ;
    ;; 检查本地化文档是否存在（HTTP状态码为200），存在则加载，否则加载英语版本
    (if (== (http-status-code lan_doc) 200)
      (load-buffer lan_doc)
      (load-buffer en_doc)
    ) ;if
  ) ;let*
) ;tm-define

;; 加载本地文档
(tm-define (load-local-doc path)
  ;; 根据语言设置加载本地文档的本地化版本
  (let* ((lan (string-take (language-to-locale (get-output-language)) 2))
         (lan_doc (string-append (url->system (get-texmacs-path)) "/doc/" path "." lan ".tmu")
         ) ;lan_doc
         (en_doc (string-append (url->system (get-texmacs-path)) "/doc/" path ".en.tmu"))
        ) ;
    ;; 如果本地化文档存在则加载，否则加载英语版本
    (if (url-exists? lan_doc) (load-buffer lan_doc) (load-buffer en_doc))
  ) ;let*
) ;tm-define

;; 加载本地插件文档
(tm-define (load-local-plugin-doc name)
  ;; 按照优先级顺序加载插件文档:
  ;; 1. 用户目录中的本地化语言 .tmu 文档
  ;; 2. 系统目录中的本地化语言 .tmu 文档
  ;; 3. 用户目录中的英语 .tmu 文档
  ;; 4. 系统目录中的英语 .tmu 文档
  ;; 5. 用户目录中的本地化语言 .tm 文档
  ;; 6. 系统目录中的本地化语言 .tm 文档
  ;; 7. 用户目录中的英语 .tm 文档
  ;; 8. 系统目录中的英语 .tm 文档
  (let* ((local_plugin_path (system->url "$TEXMACS_HOME_PATH"))
         (plugin_path (system->url "$TEXMACS_PATH"))
         (lan (string-take (language-to-locale (get-output-language)) 2))
         (path (string-append "plugins/" name "/doc/" name))
         (lan_tmu_doc (url-append plugin_path (string->url (string-append path "." lan ".tmu")))
         ) ;lan_tmu_doc
         (local_lan_tmu_doc (url-append local_plugin_path (string->url (string-append path "." lan ".tmu")))
         ) ;local_lan_tmu_doc
         (en_tmu_doc (url-append plugin_path (string->url (string-append path ".en.tmu")))
         ) ;en_tmu_doc
         (local_en_tmu_doc (url-append local_plugin_path (string->url (string-append path ".en.tmu")))
         ) ;local_en_tmu_doc
         (lan_doc (url-append plugin_path (string->url (string-append path "." lan ".tm")))
         ) ;lan_doc
         (local_lan_doc (url-append local_plugin_path (string->url (string-append path "." lan ".tm")))
         ) ;local_lan_doc
         (en_doc (url-append plugin_path (string->url (string-append path ".en.tm"))))
         (local_en_doc (url-append local_plugin_path (string->url (string-append path ".en.tm")))
         ) ;local_en_doc
        ) ;
    (cond ((url-exists? local_lan_tmu_doc) (load-buffer local_lan_tmu_doc))
          ((url-exists? lan_tmu_doc) (load-buffer lan_tmu_doc))
          ((url-exists? local_en_tmu_doc) (load-buffer local_en_tmu_doc))
          ((url-exists? en_tmu_doc) (load-buffer en_tmu_doc))
          ((url-exists? local_lan_doc) (load-buffer local_lan_doc))
          ((url-exists? lan_doc) (load-buffer lan_doc))
          ((url-exists? local_en_doc) (load-buffer local_en_doc))
          (else (load-buffer en_doc))
    ) ;cond
  ) ;let*
) ;tm-define
