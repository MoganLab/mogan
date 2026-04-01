;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : version-update.scm
;; DESCRIPTION : 版本更新检查（开发者配置）
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils misc version-update))

;; ============================================
;; 开发者配置区（修改此处调整行为）
;; ============================================
(define SNOOZE-DAYS 3)  ; 稍后提醒间隔，单位：天

;; Mock 远程版本号（用于测试，设为 #f 则使用真实网络请求）
;; 示例：(define MOCK-REMOTE-VERSION "2026.3.0")
(define MOCK-REMOTE-VERSION #f)

;; ============================================
;; 内部实现
;; ============================================

;; 获取 Mock 远程版本号（优先从持久化存储读取）
(tm-define (get-mock-remote-version)
  (:secure #t)
  (let ((stored (persistent-get (get-texmacs-home-path) MOCK-VERSION-KEY)))
    (if (and stored (!= stored ""))
        stored
        MOCK-REMOTE-VERSION)))

;; 设置 Mock 远程版本号（同时保存到持久化存储）
(tm-define (set-mock-remote-version! version)
  (:secure #t)
  (set! MOCK-REMOTE-VERSION version)
  (persistent-set (get-texmacs-home-path) MOCK-VERSION-KEY version))

;; 清除 Mock 远程版本号（恢复默认 #f）
(tm-define (clear-mock-remote-version)
  (:secure #t)
  (set! MOCK-REMOTE-VERSION #f)
  (persistent-remove (get-texmacs-home-path) MOCK-VERSION-KEY))

(define LAST-CHECK-KEY "version_last_check")
(define SNOOZE-UNTIL-KEY "version_snooze_until")
(define MOCK-VERSION-KEY "version_mock_remote")

(define (current-timestamp)
  (current-time))

;; 检查是否应该检查更新（考虑稍后提醒时间）
(tm-define (should-check-version-update?)
  (:secure #t)
  (let* ((now (current-timestamp))
         (snooze-until (or (persistent-get (get-texmacs-home-path) SNOOZE-UNTIL-KEY) "0"))
         (snooze-time (if (== snooze-until "") 0 (string->number snooze-until))))
    (>= now snooze-time)))

;; 强制清除所有记录（用于测试）
(tm-define (clear-version-update-history)
  (:secure #t)
  (persistent-remove (get-texmacs-home-path) SNOOZE-UNTIL-KEY)
  (clear-mock-remote-version))

;; 稍后提醒（使用默认间隔）
(tm-define (snooze-version-update)
  (:secure #t)
  (let* ((now (current-timestamp))
         (future (+ now (* SNOOZE-DAYS 24 3600))))
    (persistent-set (get-texmacs-home-path) SNOOZE-UNTIL-KEY
                    (number->string future))))

;; 获取下载页URL
;; 社区版跳转到 mogan.app，商业版跳转到 liiistem.cn/com
(tm-define (get-update-download-url)
  (:secure #t)
  (if (community-stem?)
      ;; 社区版官网
      (if (== (get-output-language) "chinese")
          "https://mogan.app/zh/"
          "https://mogan.app/en/")
      ;; 商业版官网
      (if (== (get-output-language) "chinese")
          "https://liiistem.cn/install.html"
          "https://liiistem.com/install.html")))
