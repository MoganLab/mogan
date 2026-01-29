
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : account.scm
;; DESCRIPTION : login
;; COPYRIGHT   : (C) 2025  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (liii account))
(import (liii os))

;; 获取当前时间戳（秒）
(define (get-current-time)
  (let* ((now (current-time))
         (seconds (car now))
         (microseconds (cdr now)))
    (+ seconds (/ microseconds 1000000.0))))

;; 获取账户数据目录
(define (get-account-data-dir)
  (url-append (get-texmacs-home-path) "system/account"))

;; 获取账户数据文件路径
(define (get-account-data-file filename)
  (url-append (get-account-data-dir) filename))

;; 常量定义
(define TOKEN_FILENAME "token.txt")
(define REFRESH_TOKEN_FILENAME "refresh_token.txt")
(define TOKEN_EXPIRY_FILENAME "token_expiry.txt")

;; 所有账户数据文件名列表
(define ACCOUNT_DATA_FILENAME_LIST
  (list TOKEN_FILENAME REFRESH_TOKEN_FILENAME TOKEN_EXPIRY_FILENAME))

;; 确保数据目录存在的辅助函数
(define (ensure-data-dir-exists)
  (let ((data-dir (get-account-data-dir)))
    (if (not (url-exists? data-dir))
        (system-mkdir data-dir))))

;; 通用：保存账户数据到文件
(define (save-account-data filename content)
  (ensure-data-dir-exists)
  (let ((data-file (get-account-data-file filename)))
    ;; 如果文件存在则删除后重新创建，确保覆盖内容
    (when (url-exists? data-file)
      (system-remove data-file))
    ;; 保存内容到文件
    (string-save content data-file)))

;; 通用：从文件读取账户数据
(define (load-account-data filename)
  (let ((data-file (get-account-data-file filename)))
    (if (url-exists? data-file)
        (string-load data-file)
        "")))

; (set-preference) 
(tm-define (current-stem-profile)
  (with sp (get-preference "stem-profile")
    (if (string=? sp "default")
        "production"
        sp)))

; 根据当前 profile 获取环境地址
(tm-define (current-stem-site)
  (cond
    ((string=? (current-stem-profile) "production") "https://liiistem.cn")
    ((string=? (current-stem-profile) "staging") "https://test-www.liiistem.cn")
    (else "local")))

;; OAuth2 配置
(tm-define (account-oauth2-config key)
  (let ((base-url (current-stem-site)))
    (cond
      ((== key "authorization-url") (string-append base-url "/oauth2-login.html"))
      ; 直接请求后台接口
      ((== key "access-token-url") (string-append base-url "/api/v1/oauth2/token"))
      ((== key "client-identifier") "public-client")
      ((== key "scope") "user+llm")
      ((== key "port-list") "6029,8087,9256,7438,5173,6391,8642,9901,44118,55055,1895")
      ((== key "user-info-url") (string-append base-url "/api/v1/oauth2/membershipInfo"))
      ((== key "pricing-url") (string-append base-url "/pricing.html"))
      ((== key "click-return-liii-url") "https://liiistem.cn/?from=login_button")
      (else ""))))

;; 本地
; (tm-define (account-oauth2-config key)
;   (cond
;     ((== key "authorization-url") "http://127.0.0.1:3000/oauth2-login")
;     ((== key "access-token-url") "http://127.0.0.1:8081/api/v1/oauth2/token") ;init:oauth2/token -> api/v1/oauth2/token
;     ((== key "client-identifier") "public-client")
;     ((== key "client-secret") "secret")
;     ((== key "scope") "user+llm")
;     ((== key "port-list") "6029,8087,9256,7438,5173,6391,8642,9901,44118,55055,1895")
;     ((== key "user-info-url") "http://127.0.0.1:8081/api/v1/oauth2/membershipInfo") ;init:api/oauthUser/membershipInfo -> api/v1/oauth2/membershipInfo
;     ((== key "pricing-url") "http://127.0.0.1:3000/pricing.html")
;     ((== key "click-return-liii-url") "http://127.0.0.1:3000/?from=login_button")
;     (else "")))

;; 1.1 token 保存到数据文件
(tm-define (account-save-token token)
  (save-account-data TOKEN_FILENAME token))

;; 1.2 读取token数据文件
(tm-define (account-load-token)
  (load-account-data TOKEN_FILENAME))

;; 2.1 refresh_token 保存到数据文件
(tm-define (account-save-refresh-token refresh-token)
  (save-account-data REFRESH_TOKEN_FILENAME refresh-token))

;; 2.2读取refresh_token数据文件
(tm-define (account-load-refresh-token)
  (load-account-data REFRESH_TOKEN_FILENAME))

;; 3.1 token过期时间保存到数据文件
(tm-define (account-save-token-expiry expiry-time)
  (save-account-data TOKEN_EXPIRY_FILENAME expiry-time))

;; 3.2 读取token过期时间数据文件
(tm-define (account-load-token-expiry)
  (load-account-data TOKEN_EXPIRY_FILENAME))

;; 4 清除所有token数据
(tm-define (account-clear-tokens)
  (ensure-data-dir-exists)
  (for-each
   (lambda (filename)
     (let ((data-file (get-account-data-file filename)))
       (when (url-exists? data-file)
         (system-remove data-file))))
   ACCOUNT_DATA_FILENAME_LIST))