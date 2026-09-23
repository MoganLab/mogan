;;
;; MODULE      : ocr-impl.scm
;; DESCRIPTION : ocr-impl mock stub for community edition
;;

(define-library (liii ocr-impl)
  (export ocr-to-latex-impl
    notify-user
    set-notify-user-handler!
  ) ;export
  (import (scheme base))
  (import (liii ocr-wait))
  (import (liii ocr-poller))
  (begin
    (define notify-user-handler (lambda (error-data) #f))

    (define (set-notify-user-handler! handler)
      (set! notify-user-handler handler)
    ) ;define

    (define (notify-user error-msg)
      (notify-user-handler error-msg)
    ) ;define

    (define (ocr-to-latex-impl base64-str mode inserter)
      (let ((cancelled? #f))
        (ocr-wait-open "Processing, please wait..." (lambda () (set! cancelled? #t)))
        (ocr-poll-delay
          (lambda ()
            (unless cancelled?
              (ocr-wait-close)
              (let* ((latex-code
                       (if (string=? mode "math")
                         "E=m*c^2"
                         (string-append
                           "\\begin{document}\n"
                           "Liii STEM 的 「OCR识别」 专属功能已免费开放体验！ \\\n"
                           "立即前往 \\url{https://liiistem.cn} ，注册即可获得7天体验会员！ \\\n"
                           "更有超值邀请福利：成功邀请1位好友注册使用，双方均可获得7天会员！\n"
                           "\\end{document}"
                         ) ;string-append
                       ) ;if
                     ) ;latex-code
                     (result (list (cons "format" "latex") (cons "text" latex-code)))
                    ) ;let*
                (inserter result)
              ) ;let*
            ) ;unless
          ) ;lambda
        ) ;ocr-poll-delay
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
