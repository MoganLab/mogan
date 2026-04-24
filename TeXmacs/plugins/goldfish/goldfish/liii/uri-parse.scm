;; ; (liii uri-parse) - URI 解析辅助函数
;; ; 本模块包含 URI 解析和构造的辅助函数

(define-library (liii uri-parse)
  (import (scheme base)
    (scheme char)
    (liii string)
    (liii error)
  ) ;import

  ;; ; ---------- 导出接口 ----------
  (export parse-netloc)
  (export build-netloc)

  (begin
    ;; ; ---------- 解析辅助函数 ----------
    ;; 解析 netloc 部分为 (user password host port) 列表
    (define (parse-netloc netloc-str)
      (if (string=? netloc-str "")
        '(#f #f #f #f)
        (let* ((user+host (if (string-index netloc-str #\@)
                            (let ((at-pos (string-index netloc-str #\@)))
                              (cons (substring netloc-str 0 at-pos)
                                (substring netloc-str
                                  (+ at-pos 1)
                                  (string-length netloc-str)
                                ) ;substring
                              ) ;cons
                            ) ;let
                            (cons #f netloc-str)
                          ) ;if
               ) ;user+host
               (user-part (car user+host))
               (host-part (cdr user+host))
               (user (if user-part
                       (let ((colon-pos (string-index user-part #\:))
                            ) ;
                         (if colon-pos
                           (substring user-part 0 colon-pos)
                           user-part
                         ) ;if
                       ) ;let
                       #f
                     ) ;if
               ) ;user
               (password (if (and user-part
                               (string-index user-part #\:)
                             ) ;and
                           (let ((colon-pos (string-index user-part #\:))
                                ) ;
                             (substring user-part
                               (+ colon-pos 1)
                               (string-length user-part)
                             ) ;substring
                           ) ;let
                           #f
                         ) ;if
               ) ;password
               (host+port (if (and host-part
                                (string-index host-part #\:)
                              ) ;and
                            (let ((colon-pos (string-index host-part #\:))
                                 ) ;
                              (cons (substring host-part 0 colon-pos)
                                (string->number (substring host-part
                                                  (+ colon-pos 1)
                                                  (string-length host-part)
                                                ) ;substring
                                ) ;string->number
                              ) ;cons
                            ) ;let
                            (cons host-part #f)
                          ) ;if
               ) ;host+port
              ) ;
          (list user
            password
            (car host+port)
            (cdr host+port)
          ) ;list
        ) ;let*
      ) ;if
    ) ;define

    ;; 从组件构造 netloc 字符串
    (define (build-netloc user password host port)
      (if (not host)
        ""
        (string-append (if user
                         (string-append user
                           (if password
                             (string-append ":" password)
                             ""
                           ) ;if
                           "@"
                         ) ;string-append
                         ""
                       ) ;if
          host
          (if port
            (string-append ":"
              (number->string port)
            ) ;string-append
            ""
          ) ;if
        ) ;string-append
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
