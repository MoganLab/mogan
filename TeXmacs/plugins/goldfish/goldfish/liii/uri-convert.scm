;; ; (liii uri-convert) - URI 转换函数
;; ; 本模块包含 URI 的转换函数（uri->string, uri->human-string）

(define-library (liii uri-convert)
  (import (scheme base)
    (liii error)
    (liii uri-record)
    (liii uri-parse)
  ) ;import

  ;; ; ---------- 导出接口 ----------
  (export uri->string)
  (export uri->human-string)

  (begin
    ;; ; ---------- 转换函数 ----------
    (define (uri->string uri-obj)
      (if (not (uri? uri-obj))
        (error "uri->string: expected uri")
        (let* ((scheme (uri-scheme-raw uri-obj))
               (netloc (uri-netloc-raw uri-obj))
               (path (uri-path-raw uri-obj))
               (query (uri-query-raw uri-obj))
               (fragment (uri-fragment-raw uri-obj))
              ) ;
          (string-append (if scheme
                           (string-append scheme ":")
                           ""
                         ) ;if
            (if (and scheme (not (string=? netloc "")))
              "//"
              ""
            ) ;if
            (if (not (string=? netloc ""))
              netloc
              ""
            ) ;if
            (or path "")
            (if (null? query)
              ""
              (string-append "?"
                (alist->query-string query)
              ) ;string-append
            ) ;if
            (if fragment
              (string-append "#" fragment)
              ""
            ) ;if
          ) ;string-append
        ) ;let*
      ) ;if
    ) ;define

    (define (uri->human-string uri-obj)
      ;; 生成人类可读的 URI 字符串（去除敏感信息如密码）
      (if (not (uri? uri-obj))
        (error "uri->human-string: expected uri"
        ) ;error
        (let* ((scheme (uri-scheme-raw uri-obj))
               (netloc-parts (parse-netloc (uri-netloc-raw uri-obj))
               ) ;netloc-parts
               (host (list-ref netloc-parts 2))
               (port (list-ref netloc-parts 3))
               (path (uri-path-raw uri-obj))
              ) ;
          (string-append (if scheme
                           (string-append scheme "://")
                           ""
                         ) ;if
            (or host "")
            (if port
              (string-append ":"
                (number->string port)
              ) ;string-append
              ""
            ) ;if
            (or path "/")
          ) ;string-append
        ) ;let*
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
