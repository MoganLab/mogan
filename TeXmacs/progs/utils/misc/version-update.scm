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
(define MOCK-REMOTE-VERSION "2026.3.0")

;; ============================================
;; 内部实现
;; ============================================

;; 获取/设置 Mock 远程版本号（用于测试）
(tm-define (get-mock-remote-version)
  (:secure #t)
  MOCK-REMOTE-VERSION)

(tm-define (set-mock-remote-version! version)
  (:secure #t)
  (set! MOCK-REMOTE-VERSION version))

(define LAST-CHECK-KEY "version_last_check")
(define IGNORED-VERSION-KEY "version_ignored")
(define SNOOZE-UNTIL-KEY "version_snooze_until")

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
  (persistent-remove (get-texmacs-home-path) IGNORED-VERSION-KEY)
  (display "Version update history cleared\n"))

;; 稍后提醒（使用默认间隔）
(tm-define (snooze-version-update)
  (:secure #t)
  (let* ((now (current-timestamp))
         (future (+ now (* SNOOZE-DAYS 24 3600))))
    (persistent-set (get-texmacs-home-path) SNOOZE-UNTIL-KEY
                    (number->string future))))

;; 跳过此版本
(tm-define (ignore-version version)
  (:secure #t)
  (persistent-set (get-texmacs-home-path) IGNORED-VERSION-KEY version)
  (persistent-remove (get-texmacs-home-path) SNOOZE-UNTIL-KEY))

;; 检查版本是否被忽略
(tm-define (is-version-ignored? version)
  (:secure #t)
  (== (persistent-get (get-texmacs-home-path) IGNORED-VERSION-KEY) version))

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
