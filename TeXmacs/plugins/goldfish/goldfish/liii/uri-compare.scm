;; ; (liii uri-compare) - URI 比较函数
;; ; 本模块包含 URI 的比较和哈希函数

(define-library (liii uri-compare)
  (import (scheme base) (liii uri-record))

  ;; ; ---------- 导出接口 ----------
  (export uri=?)
  (export uri<?)
  (export uri>?)
  (export uri-hash)

  (begin
    ;; ; ---------- 转换辅助函数 ----------
    ;; 将 URI 转换为字符串表示（用于比较）
    (define (uri->string-for-compare uri-obj)
      (if (not (uri? uri-obj))
        (error "uri->string-for-compare: expected uri"
        ) ;error
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

    ;; ; ---------- 比较函数 ----------
    ;; URI 相等比较
    (define (uri=? uri1 uri2)
      (and (uri? uri1)
        (uri? uri2)
        (string=? (uri->string-for-compare uri1)
          (uri->string-for-compare uri2)
        ) ;string=?
      ) ;and
    ) ;define

    ;; URI 小于比较（字典序）
    (define (uri<? uri1 uri2)
      (and (uri? uri1)
        (uri? uri2)
        (string<? (uri->string-for-compare uri1)
          (uri->string-for-compare uri2)
        ) ;string<?
      ) ;and
    ) ;define

    ;; URI 大于比较（字典序）
    (define (uri>? uri1 uri2)
      (and (uri? uri1)
        (uri? uri2)
        (string>? (uri->string-for-compare uri1)
          (uri->string-for-compare uri2)
        ) ;string>?
      ) ;and
    ) ;define

    ;; string>? 辅助函数
    (define (string>? s1 s2)
      (not (or (string<? s1 s2) (string=? s1 s2))
      ) ;not
    ) ;define

    ;; 计算 URI 的哈希值
    (define (uri-hash uri-obj)
      (if (not (uri? uri-obj))
        (error "uri-hash: expected uri")
        (let ((scheme (or (uri-scheme-raw uri-obj) "")
              ) ;scheme
              (netloc (uri-netloc-raw uri-obj))
              (path (or (uri-path-raw uri-obj) ""))
              (query (uri-query-raw uri-obj))
              (fragment (or (uri-fragment-raw uri-obj) "")
              ) ;fragment
             ) ;
          ;; 简单哈希：字符串长度之和（实际应使用更好的哈希算法）
          (+ (string-length scheme)
            (string-length netloc)
            (string-length path)
            (length query)
            (string-length fragment)
          ) ;+
        ) ;let
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
