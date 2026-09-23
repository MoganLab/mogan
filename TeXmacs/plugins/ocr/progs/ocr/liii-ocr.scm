;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : liii-ocr.scm
;; DESCRIPTION : ocr
;; COPYRIGHT   : (C) 2025  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ocr liii-ocr)
  (:use (account liii))
  (:use (kernel texmacs tm-dialogue))
  (:use (utils misc wait-dialog))
  (:use (ocr ocr-auth))
  (:use (ocr ocr-clipboard))
  (:use (ocr ocr-insert))
) ;texmacs-module
(import (liii os))

;; 商业版完整 OCR 链路。协议隔离架构：
;;   * goldfish 库（私有协议，plugins/ocr/goldfish）：纯计算与编排——
;;     (liii ocr-impl) 上传/识别/缓存/通知、(liii ocr-silent) 静默识别、
;;     (liii ocr-render) 布局渲染、(liii ocr-network) 网络请求体；
;;     只用 (liii ...)/(scheme ...)/s7 内建，GPL 符号与 mogan glue 一律不调。
;;   * texmacs 模块（GPL，本目录）：编辑器交互与 GPL 插件转换——
;;     (ocr ocr-auth) 认证头/站点、(ocr ocr-clipboard) 剪贴板与图片提取、
;;     (ocr ocr-insert) 插入遍历；本模块作为总入口接线并转发 tm-define，
;;     供 UI 层（ocr-paste / 图片右键 / PDF 阅读器）调用。
;;   * 依赖方向：progs -> goldfish 单向；goldfish 需要的外部能力
;;     （对话框/认证/调度/转换器）经 setter 注入闭包。
;;
;; 注意：stem --prepare 会用本文件覆盖 mogan 自带的社区占位版
;; (ocr liii-ocr)（仅存临时文件+插静态模板，无真实识别），保证商业版使用完整链路。

(set! *load-path*
  (let ((sys-path (string-append (getenv "TEXMACS_PATH") "/plugins/ocr/goldfish")))
    (if (member sys-path *load-path*) *load-path* (cons sys-path *load-path*))
  ) ;let
) ;set!

(import (liii ocr-impl))
(import (liii ocr-wait))
(import (liii ocr-silent))
(import (liii ocr-network))
(import (liii ocr-poller))
(import (liii ocr-render))

;; 对话框函数（user-ask/open-url/account-oauth2-config）只在本模块的
;; texmacs 模块环境中可见，注入给 goldfish 库 (liii ocr-impl) 使用。

(define (open-login-message-widget)
  (user-ask (list (cork->utf8 (translate "Sign in to use OCR"))
              "question"
              (translate "Sign In")
            ) ;list
    (lambda (anw)
      (if (string=? anw (translate "Sign In"))
        (login)
        (noop)
      ) ;if
    ) ;lambda
  ) ;user-ask
) ;define

(define (open-no-network-message-widget)
  (user-ask (list (string-append (cork->utf8 (translate "The OCR feature requires an internet connection to use...")
                                 ) ;cork->utf8
                    "\n"
                    (cork->utf8 (translate "Connect to the network to enjoy convenient and fast OCR features!")
                    ) ;cork->utf8
                  ) ;string-append
              "question"
              (translate "ok")
            ) ;list
    (lambda (anw) (noop))
  ) ;user-ask
) ;define

(define (open-exhausted-message-widget)
  (user-ask (list (string-append (cork->utf8 (translate "Daily OCR limit reached"))
                    "\n"
                    (cork->utf8 (translate "Upgrade to continue using OCR."))
                  ) ;string-append
              "question"
              (translate "Upgrade")
            ) ;list
    (lambda (anw)
      (if (string=? anw (translate "Upgrade"))
        (open-url (account-oauth2-config "pricing-url"))
        (noop)
      ) ;if
    ) ;lambda
  ) ;user-ask
) ;define

(define (open-message-widget msg)
  (user-ask (list msg "question" (translate "ok")) (lambda (anw) (noop)))
) ;define

(define (ocr-notify-user error-data)
  (cond ((eq? error-data 'no-network) (open-no-network-message-widget))
        ((eq? error-data 'require-login) (open-login-message-widget))
        ((eq? error-data 'ocr-limited) (open-exhausted-message-widget))
        (else (open-message-widget error-data))
  ) ;cond
) ;define

(set-notify-user-handler! ocr-notify-user)

;; 等待弹窗注入：通用门面 (utils misc wait-dialog)（GPL 层）包装 Qt 侧
;; glue（cpp-wait-dialog-open/close），goldfish 编排层 (liii ocr-impl) 不可
;; 直接调用（协议隔离），经 set-ocr-wait-provider! 注入；open 双参数（英文
;; 文案 key + on-cancel 回调），translate 与取消路由（wait-dialog-cancelled）
;; 由门面完成。未命中缓存发起上传前打开，识别完成（插入前）/失败（通知
;; 前）关闭。
(set-ocr-wait-provider! wait-dialog-open wait-dialog-close)

;; 认证头/站点 provider 注入：account/stem 符号属 GPL 层，goldfish 不可直接调用
(set-ocr-auth-provider! ocr-auth-headers ocr-auth-site)

;; 轮询调度器注入：delayed 宏属 GPL 层（tm-dialogue），goldfish 不可直接调用
(set-ocr-poll-delayer! (lambda (thunk) (delayed (:pause 100) (thunk))))

;; 转换器注入：latex/html/markdown -> texmacs 属 GPL 插件实现，goldfish 不可
;; 直接调用；闭包内晚绑定，保持原有的运行时符号解析语义
(set-latex-conv! (lambda (s) (latex->texmacs (parse-latex s))))
(set-html-conv! (lambda (s) (html->texmacs (parse-html-snippet s))))
(set-markdown-conv! (lambda (s) (markdown-snippet->stree s)))

(tm-define (ocr-to-latex-by-image t)
  (let ((base64-str (ocr-get-image-base64 t)))
    (when base64-str
      (ocr-to-latex-impl base64-str "text" (lambda (j) (ocr-insert-by-image t j)))
    ) ;when
  ) ;let
) ;tm-define

(tm-define (ocr-to-latex-by-cursor t)
  (let ((base64-str (ocr-get-image-base64 t)))
    (when base64-str
      (ocr-to-latex-impl base64-str (ocr-cursor-mode) ocr-insert-by-cursor)
    ) ;when
  ) ;let
) ;tm-define

(tm-define (ocr-recognize-silent)
  (let ((raw (ocr-image-tree->raw-bytes (ocr-clipboard->image-tree))))
    (when raw
      (ocr-recognize-silent raw)
    ) ;when
  ) ;let
) ;tm-define
