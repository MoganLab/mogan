;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : notificationbar.scm
;; DESCRIPTION : SCM notification bar business logic
;; COPYRIGHT   : (C) 2026 Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus notificationbar)
  (:use
    (utils library cursor)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; SCM notification bar state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define notification-bar-rotation-interval 5000)
(tm-define notification-bar-rotation-started? #f)

(define MEMBERSHIP-NOTICE-SNOOZE-DAYS 7)
(define MEMBERSHIP-NOTICE-SNOOZE-UNTIL-KEY "membership_notice_snooze_until")
(define MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-DAYS 1)
(define MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-UNTIL-KEY
  "membership_renew_soon_notice_snooze_until")

(define notification-bar-last-rendered-item #f)

(define notification-bar-guest-visible? #f)

(define notification-bar-membership-has-data? #f)
(define notification-bar-membership-member-type "")
(define notification-bar-membership-period-label "")
(define notification-bar-membership-period-label-color "")
(define notification-bar-membership-product-type "")
(define notification-bar-membership-session-dismissed? #f)
(define notification-bar-membership-renew-soon-session-dismissed? #f)
(define MEMBERSHIP-RENEW-SOON-THRESHOLD-DAYS 7)

(define (notification-bar-non-empty-string? s)
  (and (string? s) (!= s "") (!= s "undefined")))

(tm-define (notification-bar-set-update-state active? remote-version)
  #f)

(tm-define (notification-bar-set-guest-visible visible?)
  (let ((was-visible? notification-bar-guest-visible?))
    (set! notification-bar-guest-visible? visible?)
    (when (or (not visible?) (not was-visible?))
      (set! notification-bar-membership-session-dismissed? #f))
    visible?))

(tm-define (notification-bar-set-membership-state has-data? member-type
                                                  period-label
                                                  period-label-color
                                                  product-type)
  (set! notification-bar-membership-has-data? has-data?)
  (set! notification-bar-membership-member-type
        (if (notification-bar-non-empty-string? member-type) member-type ""))
  (set! notification-bar-membership-period-label
        (if (notification-bar-non-empty-string? period-label) period-label ""))
  (set! notification-bar-membership-period-label-color
        (if (notification-bar-non-empty-string? period-label-color)
            period-label-color
            ""))
  (set! notification-bar-membership-product-type
        (if (notification-bar-non-empty-string? product-type) product-type ""))
  has-data?)

(tm-define (notification-bar-clear-membership-state)
  (notification-bar-set-membership-state #f "" "" "" ""))

(define (notification-bar-read-snooze-until key)
  (or (persistent-get (get-texmacs-home-path) key)
      "0"))

(define (notification-bar-notice-snoozed? key)
  (let* ((now (current-time))
         (snooze-until (notification-bar-read-snooze-until key))
         (snooze-time
           (if (== snooze-until "")
               0
               (or (string->number snooze-until) 0))))
    (> snooze-time now)))

(define (notification-bar-snooze-notice! key snooze-days)
  (let* ((now (current-time))
         (future (+ now (* snooze-days 24 3600))))
    (persistent-set (get-texmacs-home-path)
                    key
                    (number->string future))))

(define (notification-bar-clear-notice-history! key)
  (persistent-remove (get-texmacs-home-path) key))

(tm-define (notification-bar-membership-notice-snoozed?)
  (notification-bar-notice-snoozed? MEMBERSHIP-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-membership-notice-snooze-until)
  (:secure #t)
  (notification-bar-read-snooze-until MEMBERSHIP-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-snooze-membership-notice)
  (:secure #t)
  (set! notification-bar-membership-session-dismissed? #t)
  (notification-bar-snooze-notice! MEMBERSHIP-NOTICE-SNOOZE-UNTIL-KEY
                                   MEMBERSHIP-NOTICE-SNOOZE-DAYS))

(tm-define (notification-bar-dismiss-membership-notice)
  (:secure #t)
  (set! notification-bar-membership-session-dismissed? #t))

(tm-define (notification-bar-clear-membership-notice-history)
  (:secure #t)
  (set! notification-bar-membership-session-dismissed? #f)
  (notification-bar-clear-notice-history!
    MEMBERSHIP-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-renew-soon-notice-snoozed?)
  (notification-bar-notice-snoozed?
    MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-renew-soon-notice-snooze-until)
  (:secure #t)
  (notification-bar-read-snooze-until
    MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-snooze-renew-soon-notice)
  (:secure #t)
  (set! notification-bar-membership-renew-soon-session-dismissed? #t)
  (notification-bar-snooze-notice!
    MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-UNTIL-KEY
    MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-DAYS))

(tm-define (notification-bar-dismiss-renew-soon-notice)
  (:secure #t)
  (set! notification-bar-membership-renew-soon-session-dismissed? #t))

(tm-define (notification-bar-clear-renew-soon-notice-history)
  (:secure #t)
  (set! notification-bar-membership-renew-soon-session-dismissed? #f)
  (notification-bar-clear-notice-history!
    MEMBERSHIP-RENEW-SOON-NOTICE-SNOOZE-UNTIL-KEY))

(tm-define (notification-bar-membership-free-user?)
  (and (== notification-bar-membership-member-type "Regular User")
       (== notification-bar-membership-period-label "Non-member")))

(tm-define (notification-bar-guest-active?)
  (and notification-bar-guest-visible?
       (not notification-bar-membership-session-dismissed?)
       (not (notification-bar-membership-notice-snoozed?))))

(define (notification-bar-digit-char? ch)
  (and (char>=? ch #\0) (char<=? ch #\9)))

(define (notification-bar-extract-number-tokens s)
  (let loop ((chars (string->list s)) (current '()) (tokens '()))
    (cond
      ((null? chars)
       (reverse
         (if (null? current)
             tokens
             (cons (reverse-list->string current) tokens))))
      ((notification-bar-digit-char? (car chars))
       (loop (cdr chars) (cons (car chars) current) tokens))
      ((null? current)
       (loop (cdr chars) '() tokens))
      (else
       (loop (cdr chars) '()
             (cons (reverse-list->string current) tokens))))))

(define (notification-bar-floor-div a b)
  (if (>= a 0)
      (quotient a b)
      (- (quotient (- (- a) 1) b) 1)))

(define (notification-bar-days-from-civil year month day)
  (let* ((y (if (<= month 2) (- year 1) year))
         (era (notification-bar-floor-div y 400))
         (yoe (- y (* era 400)))
         (m (+ month (if (> month 2) -3 9)))
         (doy (+ (quotient (+ (* 153 m) 2) 5) (- day 1)))
         (doe (+ (* yoe 365)
                 (quotient yoe 4)
                 (- (quotient yoe 100))
                 doy)))
    (+ (- (* era 146097) 719468) doe)))

(define (notification-bar-civil-from-days z)
  (let* ((z* (+ z 719468))
         (era (notification-bar-floor-div z* 146097))
         (doe (- z* (* era 146097)))
         (yoe (quotient (- doe
                           (quotient doe 1460)
                           (- (quotient doe 36524))
                           (quotient doe 146096))
                        365))
         (year (+ yoe (* era 400)))
         (doy (- doe (+ (* yoe 365)
                        (quotient yoe 4)
                        (- (quotient yoe 100)))))
         (mp (quotient (+ (* 5 doy) 2) 153))
         (day (+ (- doy (quotient (+ (* 153 mp) 2) 5)) 1))
         (month (+ mp (if (< mp 10) 3 -9)))
         (year* (+ year (if (<= month 2) 1 0))))
    (list year* month day)))

(define (notification-bar-current-day-number)
  (quotient (current-time) 86400))

(define (notification-bar-current-date-parts)
  (notification-bar-civil-from-days (notification-bar-current-day-number)))

(define (notification-bar-valid-date? year month day)
  (and (integer? year)
       (integer? month)
       (integer? day)
       (>= month 1)
       (<= month 12)
       (>= day 1)
       (<= day 31)))

(define (notification-bar-infer-year-for-month-day month day)
  (let* ((current-parts (notification-bar-current-date-parts))
         (current-year (list-ref current-parts 0))
         (current-day (notification-bar-current-day-number))
         (candidate-day (notification-bar-days-from-civil current-year
                                                          month
                                                          day))
         (delta (- candidate-day current-day)))
    (cond
      ((< delta -180) (+ current-year 1))
      ((> delta 180) (- current-year 1))
      (else current-year))))

(define (notification-bar-membership-expiry-day-number)
  (let* ((tokens
           (notification-bar-extract-number-tokens
             notification-bar-membership-period-label))
         (count (length tokens)))
    (cond
      ((== count 3)
       (let* ((first (or (string->number (list-ref tokens 0)) 0))
              (second (or (string->number (list-ref tokens 1)) 0))
              (third (or (string->number (list-ref tokens 2)) 0))
              (year (if (> first 31) first third))
              (month second)
              (day (if (> first 31) third first)))
         (and (notification-bar-valid-date? year month day)
              (notification-bar-days-from-civil year month day))))
      ((== count 2)
       (let* ((month (or (string->number (list-ref tokens 0)) 0))
              (day (or (string->number (list-ref tokens 1)) 0))
              (year (notification-bar-infer-year-for-month-day month day)))
         (and (notification-bar-valid-date? year month day)
              (notification-bar-days-from-civil year month day))))
      (else #f))))

(define (notification-bar-membership-days-left)
  (and-with expiry-day (notification-bar-membership-expiry-day-number)
    (- expiry-day (notification-bar-current-day-number))))

(tm-define (notification-bar-membership-renew-soon-by-date?)
  (and-with days-left (notification-bar-membership-days-left)
    (and (>= days-left 0)
         (<= days-left MEMBERSHIP-RENEW-SOON-THRESHOLD-DAYS))))

(tm-define (notification-bar-membership-expired-by-date?)
  (and-with days-left (notification-bar-membership-days-left)
    (< days-left 0)))

(tm-define (notification-bar-membership-renew-soon?)
  (if (notification-bar-membership-days-left)
      (notification-bar-membership-renew-soon-by-date?)
      (== notification-bar-membership-product-type "Renew Early")))

(tm-define (notification-bar-membership-renew-soon-active?)
  (and (not notification-bar-guest-visible?)
       notification-bar-membership-has-data?
       (notification-bar-membership-renew-soon?)
       (not notification-bar-membership-renew-soon-session-dismissed?)
       (not (notification-bar-renew-soon-notice-snoozed?))
       (not (notification-bar-membership-free-user?))))

(tm-define (notification-bar-membership-expired-active?)
  (and (not notification-bar-guest-visible?)
       notification-bar-membership-has-data?
       (if (notification-bar-membership-days-left)
           (notification-bar-membership-expired-by-date?)
           (not (notification-bar-membership-renew-soon?)))
       (not notification-bar-membership-session-dismissed?)
       (not (notification-bar-membership-notice-snoozed?))
       (not (notification-bar-membership-free-user?))))

(tm-define (notification-bar-active-items)
  (with items '()
    (if (notification-bar-membership-renew-soon-active?)
        (set! items (append items (list "membership-renew-soon"))))
    (if (or (notification-bar-membership-expired-active?)
            (notification-bar-guest-active?))
        (set! items (append items (list "membership"))))
    items))

(tm-define (notification-bar-count)
  (length (notification-bar-active-items)))

(tm-define (notification-bar-index)
  (with count (notification-bar-count)
    (if (<= count 1)
        0
        (with step (max 1 (quotient notification-bar-rotation-interval 1000))
          (modulo (quotient (current-time) step) count)))))

(tm-define (notification-bar-current-item)
  (with items (notification-bar-active-items)
    (if (null? items)
        (begin
          (set! notification-bar-last-rendered-item #f)
          #f)
        (with item (list-ref items (notification-bar-index))
          (set! notification-bar-last-rendered-item item)
          item))))

(tm-define (notification-bar-rendered-item)
  (or notification-bar-last-rendered-item ""))

(tm-define (notification-bar-membership-message)
  (translate "Upgrade to unlock AI writing, MathOCR, and more advanced features."))

(tm-define (notification-bar-membership-renew-soon-message)
  (translate
    "Your membership will expire within 7 days. Renew early for more savings"))

(tm-define (notification-bar-membership-button-label)
  (translate "Try now"))

(tm-define (notification-bar-membership-renew-soon-button-label)
  (translate "Renew Early"))

(tm-define (notification-bar-snooze-action-label)
  (with item notification-bar-last-rendered-item
    (cond
      ((== item "membership-renew-soon")
       (translate "Do not show me in 1 day"))
      ((== item "membership")
       (translate "Do not show me in 7 days"))
      (else ""))))

(tm-define (notification-bar-open-membership-renew-soon-plans)
  (notification-bar-snooze-renew-soon-notice)
  (open-pricing-url)
  (when (current-view) (update-menus)))

(tm-define (notification-bar-open-membership-plans)
  (notification-bar-snooze-membership-notice)
  (open-pricing-url)
  (when (current-view) (update-menus)))

(tm-define (notification-bar-snooze-membership-renew-soon)
  (notification-bar-snooze-renew-soon-notice)
  (when (current-view) (update-menus)))

(tm-define (notification-bar-snooze-membership-expired)
  (notification-bar-snooze-membership-notice)
  (when (current-view) (update-menus)))

(tm-define (notification-bar-handle-close)
  (:secure #t)
  (with item notification-bar-last-rendered-item
    (cond
      ((== item "membership-renew-soon")
       (notification-bar-dismiss-renew-soon-notice)
       (when (current-view) (update-menus))
       #t)
      ((== item "membership")
       (notification-bar-dismiss-membership-notice)
       (when (current-view) (update-menus))
       #t)
      (else #f))))

(tm-define (notification-bar-rotate)
  (when (and (current-view) (> (notification-bar-count) 1))
    (update-menus))
  (delayed
    (:idle notification-bar-rotation-interval)
    (notification-bar-rotate)))

(tm-define (notification-bar-ensure-running)
  (when (not notification-bar-rotation-started?)
    (set! notification-bar-rotation-started? #t)
    (delayed
      (:idle notification-bar-rotation-interval)
      (notification-bar-rotate))))

(notification-bar-ensure-running)

(menu-bind texmacs-notification-bar-three
  (text (notification-bar-membership-renew-soon-message))
  >>>
  ((eval (notification-bar-membership-renew-soon-button-label))
   (notification-bar-open-membership-renew-soon-plans)))

(menu-bind texmacs-notification-bar-four
  (text (notification-bar-membership-message))
  >>>
  ((eval (notification-bar-membership-button-label))
   (notification-bar-open-membership-plans)))

(menu-bind texmacs-notification-bar
  (if (== (notification-bar-current-item) "membership-renew-soon")
      (link texmacs-notification-bar-three))
  (if (== (notification-bar-current-item) "membership")
      (link texmacs-notification-bar-four)))
