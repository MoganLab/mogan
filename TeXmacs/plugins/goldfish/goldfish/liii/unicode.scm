(define-library (liii unicode)
  (export
    ;; UTF-8 函数
    utf8->string
    string->utf8
    utf8-string-length
    utf8-substring
    bytevector-advance-utf8
    codepoint->utf8
    utf8->codepoint
    utf8-string-trim-right
    utf8-string-trim-left
    utf8-string-trim-both

    ;; UTF-16BE 函数
    codepoint->utf16be
    utf16be->codepoint
    utf8->utf16be
    utf16be->utf8
    bytevector-utf16be-advance

    ;; UTF-16LE 函数
    codepoint->utf16le
    utf16le->codepoint
    utf8->utf16le
    utf16le->utf8
    bytevector-utf16le-advance

    ;; 十六进制字符串与码点转换函数
    hexstr->codepoint
    codepoint->hexstr

    ;; Unicode 常量
    unicode-max-codepoint
    unicode-replacement-char
  ) ;export

  (import (scheme base)
    (liii base)
    (liii bitwise)
    (liii error)
  ) ;import

  (begin

    (define* (utf8-substring str (start 0) (end #t))
      (utf8->string (string->utf8 str start end)
      ) ;utf8->string
    ) ;define*

    ;; ; 辅助函数：检查字符是否为空白字符
    (define (unicode-whitespace? c)
      (or (char=? c #\space)
        (char=? c #\tab)
        (char=? c #\newline)
        (char=? c #\return)
        (char=? c #\xa0)
        (char=? c #\x2000)
        (char=? c #\x2001)
        (char=? c #\x2002)
        (char=? c #\x2003)
        (char=? c #\x2004)
        (char=? c #\x2005)
        (char=? c #\x2006)
        (char=? c #\x2007)
        (char=? c #\x2008)
        (char=? c #\x2009)
        (char=? c #\x200A)
        (char=? c #\x202F)
        (char=? c #\x205F)
        (char=? c #\x3000)
      ) ;or
    ) ;define

    ;; ; utf8-string-trim-right: 从字符串右侧移除空白字符
    ;; ; 正确处理UTF-8编码的中文字符
    (define (utf8-string-trim-right str)
      (unless (string? str)
        (error 'type-error
          "utf8-string-trim-right: expected string"
        ) ;error
      ) ;unless

      (let* ((bv (string->utf8 str))
             (byte-len (bytevector-length bv))
            ) ;
        (if (= byte-len 0)
          ""
          (let loop
            ((byte-pos byte-len))
            (if (<= byte-pos 0)
              ""
              (let* ((prev-byte-pos (bytevector-rindex-utf8 bv 0 byte-pos)
                     ) ;prev-byte-pos
                     (char-bv (bytevector-copy bv
                                prev-byte-pos
                                byte-pos
                              ) ;bytevector-copy
                     ) ;char-bv
                     (ch (utf8->string char-bv))
                    ) ;
                (if (and (= (string-length ch) 1)
                      (unicode-whitespace? (string-ref ch 0))
                    ) ;and
                  (loop prev-byte-pos)
                  (if (= byte-pos byte-len)
                    str
                    (utf8->string (bytevector-copy bv 0 byte-pos)
                    ) ;utf8->string
                  ) ;if
                ) ;if
              ) ;let*
            ) ;if
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    ;; ; 辅助函数：从字节向量末尾向前查找UTF-8字符的起始位置
    (define (bytevector-rindex-utf8 bv start end)
      (let loop
        ((pos (- end 1)))
        (if (< pos start)
          start
          (let ((b (bytevector-u8-ref bv pos)))
            (if (or (<= 0 b 127) (<= 192 b 255))
              pos
              (loop (- pos 1))
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; ; utf8-string-trim-left: 从字符串左侧移除空白字符
    ;; ; 正确处理UTF-8编码的中文字符
    (define (utf8-string-trim-left str)
      (unless (string? str)
        (error 'type-error
          "utf8-string-trim-left: expected string"
        ) ;error
      ) ;unless

      (let* ((bv (string->utf8 str))
             (byte-len (bytevector-length bv))
            ) ;
        (if (= byte-len 0)
          ""
          (let loop
            ((byte-pos 0))
            (if (>= byte-pos byte-len)
              ""
              (let* ((next-byte-pos (bytevector-advance-utf8 bv
                                      byte-pos
                                      byte-len
                                    ) ;bytevector-advance-utf8
                     ) ;next-byte-pos
                     (char-bv (bytevector-copy bv
                                byte-pos
                                next-byte-pos
                              ) ;bytevector-copy
                     ) ;char-bv
                     (ch (utf8->string char-bv))
                    ) ;
                (if (and (= (string-length ch) 1)
                      (unicode-whitespace? (string-ref ch 0))
                    ) ;and
                  (loop next-byte-pos)
                  (if (= byte-pos 0)
                    str
                    (utf8->string (bytevector-copy bv byte-pos byte-len)
                    ) ;utf8->string
                  ) ;if
                ) ;if
              ) ;let*
            ) ;if
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    ;; ; utf8-string-trim-both: 从字符串两侧移除空白字符
    ;; ; 正确处理UTF-8编码的中文字符
    (define (utf8-string-trim-both str)
      (unless (string? str)
        (error 'type-error
          "utf8-string-trim-both: expected string"
        ) ;error
      ) ;unless

      (utf8-string-trim-right (utf8-string-trim-left str)
      ) ;utf8-string-trim-right
    ) ;define

    (define (codepoint->utf8 codepoint)
      (unless (integer? codepoint)
        (error 'type-error
          "codepoint->utf8: expected integer, got"
          codepoint
        ) ;error
      ) ;unless

      (when (or (< codepoint 0)
              (> codepoint 1114111)
            ) ;or
        (error 'value-error
          "codepoint->utf8: codepoint out of Unicode range"
          codepoint
        ) ;error
      ) ;when

      (cond ((<= codepoint 127)
             (bytevector codepoint)
            ) ;

            ((<= codepoint 2047)
             (let ((byte1 (bitwise-ior 192
                            (bitwise-and (ash codepoint -6) 31)
                          ) ;bitwise-ior
                   ) ;byte1
                   (byte2 (bitwise-ior 128
                            (bitwise-and codepoint 63)
                          ) ;bitwise-ior
                   ) ;byte2
                  ) ;
               (bytevector byte1 byte2)
             ) ;let
            ) ;

            ((<= codepoint 65535)
             (let ((byte1 (bitwise-ior 224
                            (bitwise-and (ash codepoint -12) 15)
                          ) ;bitwise-ior
                   ) ;byte1
                   (byte2 (bitwise-ior 128
                            (bitwise-and (ash codepoint -6) 63)
                          ) ;bitwise-ior
                   ) ;byte2
                   (byte3 (bitwise-ior 128
                            (bitwise-and codepoint 63)
                          ) ;bitwise-ior
                   ) ;byte3
                  ) ;
               (bytevector byte1 byte2 byte3)
             ) ;let
            ) ;

            (else (let ((byte1 (bitwise-ior 240
                                 (bitwise-and (ash codepoint -18) 7)
                               ) ;bitwise-ior
                        ) ;byte1
                        (byte2 (bitwise-ior 128
                                 (bitwise-and (ash codepoint -12) 63)
                               ) ;bitwise-ior
                        ) ;byte2
                        (byte3 (bitwise-ior 128
                                 (bitwise-and (ash codepoint -6) 63)
                               ) ;bitwise-ior
                        ) ;byte3
                        (byte4 (bitwise-ior 128
                                 (bitwise-and codepoint 63)
                               ) ;bitwise-ior
                        ) ;byte4
                       ) ;
                    (bytevector byte1 byte2 byte3 byte4)
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (utf8->codepoint bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf8->codepoint: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error
            "utf8->codepoint: empty bytevector"
          ) ;error
        ) ;when

        (let ((first-byte (bytevector-u8-ref bv 0)))
          (cond ((<= first-byte 127) first-byte)

                ((<= 194 first-byte 223)
                 (when (< len 2)
                   (error 'value-error
                     "utf8->codepoint: incomplete 2-byte sequence"
                   ) ;error
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1)))
                   (unless (<= 128 byte2 191)
                     (error 'value-error
                       "utf8->codepoint: invalid continuation byte"
                     ) ;error
                   ) ;unless
                   (bitwise-ior (ash (bitwise-and first-byte 31) 6)
                     (bitwise-and byte2 63)
                   ) ;bitwise-ior
                 ) ;let
                ) ;

                ((<= 224 first-byte 239)
                 (when (< len 3)
                   (error 'value-error
                     "utf8->codepoint: incomplete 3-byte sequence"
                   ) ;error
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1))
                       (byte3 (bytevector-u8-ref bv 2))
                      ) ;
                   (unless (and (<= 128 byte2 191)
                             (<= 128 byte3 191)
                           ) ;and
                     (error 'value-error
                       "utf8->codepoint: invalid continuation byte"
                     ) ;error
                   ) ;unless
                   (let ((codepoint (bitwise-ior (ash (bitwise-and first-byte 15) 12)
                                      (ash (bitwise-and byte2 63) 6)
                                      (bitwise-and byte3 63)
                                    ) ;bitwise-ior
                         ) ;codepoint
                        ) ;
                     (when (or (<= 55296 codepoint 57343)
                             (and (= first-byte 224)
                               (< codepoint 2048)
                             ) ;and
                             (and (= first-byte 237)
                               (>= codepoint 55296)
                             ) ;and
                           ) ;or
                       (error 'value-error
                         "utf8->codepoint: invalid codepoint"
                       ) ;error
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                ((<= 240 first-byte 244)
                 (when (< len 4)
                   (error 'value-error
                     "utf8->codepoint: incomplete 4-byte sequence"
                   ) ;error
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1))
                       (byte3 (bytevector-u8-ref bv 2))
                       (byte4 (bytevector-u8-ref bv 3))
                      ) ;
                   (unless (and (<= 128 byte2 191)
                             (<= 128 byte3 191)
                             (<= 128 byte4 191)
                           ) ;and
                     (error 'value-error
                       "utf8->codepoint: invalid continuation byte"
                     ) ;error
                   ) ;unless
                   (let ((codepoint (bitwise-ior (ash (bitwise-and first-byte 7) 18)
                                      (ash (bitwise-and byte2 63) 12)
                                      (ash (bitwise-and byte3 63) 6)
                                      (bitwise-and byte4 63)
                                    ) ;bitwise-ior
                         ) ;codepoint
                        ) ;
                     (when (or (< codepoint 65536)
                             (> codepoint 1114111)
                             (and (= first-byte 240)
                               (< codepoint 65536)
                             ) ;and
                             (and (= first-byte 244)
                               (> codepoint 1114111)
                             ) ;and
                           ) ;or
                       (error 'value-error
                         "utf8->codepoint: invalid codepoint"
                       ) ;error
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                (else (error 'value-error
                        "utf8->codepoint: invalid UTF-8 sequence"
                      ) ;error
                ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define unicode-max-codepoint 1114111)
    (define unicode-replacement-char 65533)

    (define (hexstr->codepoint hex-string)
      (unless (string? hex-string)
        (error 'type-error
          "hexstr->codepoint: expected string"
        ) ;error
      ) ;unless

      (when (string=? hex-string "")
        (error 'value-error
          "hexstr->codepoint: empty string"
        ) ;error
      ) ;when

      ;; 验证十六进制字符
      (let loop
        ((chars (string->list hex-string)))
        (unless (null? chars)
          (let ((c (car chars)))
            (unless (or (char-numeric? c)
                      (char<=? #\A c #\F)
                      (char<=? #\a c #\f)
                    ) ;or
              (error 'value-error
                "hexstr->codepoint: invalid hexadecimal string"
              ) ;error
            ) ;unless
            (loop (cdr chars))
          ) ;let
        ) ;unless
      ) ;let

      (let ((codepoint (string->number hex-string 16)
            ) ;codepoint
           ) ;
        (unless codepoint
          (error 'value-error
            "hexstr->codepoint: invalid hexadecimal format"
          ) ;error
        ) ;unless

        (when (or (< codepoint 0)
                (> codepoint unicode-max-codepoint)
              ) ;or
          (error 'value-error
            "hexstr->codepoint: codepoint out of Unicode range"
            codepoint
          ) ;error
        ) ;when

        codepoint
      ) ;let
    ) ;define

    (define (codepoint->hexstr codepoint)
      (unless (integer? codepoint)
        (error 'type-error
          "codepoint->hexstr: expected integer, got"
          codepoint
        ) ;error
      ) ;unless

      (when (or (< codepoint 0)
              (> codepoint unicode-max-codepoint)
            ) ;or
        (error 'value-error
          "codepoint->hexstr: codepoint out of Unicode range"
          codepoint
        ) ;error
      ) ;when

      (let ((hex-str (string-upcase (number->string codepoint 16)
                     ) ;string-upcase
            ) ;hex-str
           ) ;
        (if (and (> codepoint 0)
              (< codepoint 16)
              (= (string-length hex-str) 1)
            ) ;and
          (string-append "0" hex-str)
          hex-str
        ) ;if
      ) ;let
    ) ;define

    (define (codepoint->utf16be codepoint)
      (unless (integer? codepoint)
        (error 'type-error
          "codepoint->utf16be: expected integer, got"
          codepoint
        ) ;error
      ) ;unless

      (when (or (< codepoint 0)
              (> codepoint 1114111)
            ) ;or
        (error 'value-error
          "codepoint->utf16be: codepoint out of Unicode range"
          codepoint
        ) ;error
      ) ;when

      ;; 检查是否为代理对码点（无效）
      (when (<= 55296 codepoint 57343)
        (error 'value-error
          "codepoint->utf16be: codepoint in surrogate pair range"
          codepoint
        ) ;error
      ) ;when

      (cond ((<= codepoint 65535)
             ;; 基本多文种平面字符 - 单个码元
             (let ((high-byte (ash codepoint -8))
                   (low-byte (bitwise-and codepoint 255))
                  ) ;
               (bytevector high-byte low-byte)
             ) ;let
            ) ;

            (else
              ;; 辅助平面字符 - 代理对
              (let* ((codepoint-prime (- codepoint 65536))
                     (high-surrogate (+ 55296 (ash codepoint-prime -10))
                     ) ;high-surrogate
                     (low-surrogate (+ 56320
                                      (bitwise-and codepoint-prime 1023)
                                    ) ;+
                     ) ;low-surrogate
                     (high-surrogate-high (ash high-surrogate -8)
                     ) ;high-surrogate-high
                     (high-surrogate-low (bitwise-and high-surrogate 255)
                     ) ;high-surrogate-low
                     (low-surrogate-high (ash low-surrogate -8)
                     ) ;low-surrogate-high
                     (low-surrogate-low (bitwise-and low-surrogate 255)
                     ) ;low-surrogate-low
                    ) ;
                (bytevector high-surrogate-high
                  high-surrogate-low
                  low-surrogate-high
                  low-surrogate-low
                ) ;bytevector
              ) ;let*
            ) ;else
      ) ;cond
    ) ;define

    (define (utf16be->codepoint bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf16be->codepoint: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error
            "utf16be->codepoint: empty bytevector"
          ) ;error
        ) ;when

        (when (< len 2)
          (error 'value-error
            "utf16be->codepoint: incomplete UTF-16BE sequence"
          ) ;error
        ) ;when

        (let* ((first-high (bytevector-u8-ref bv 0))
               (first-low (bytevector-u8-ref bv 1))
               (first-codepoint (+ (ash first-high 8) first-low)
               ) ;first-codepoint
              ) ;

          (cond ((<= 55296 first-codepoint 56319)
                 ;; 高代理对 - 需要低代理对
                 (when (< len 4)
                   (error 'value-error
                     "utf16be->codepoint: incomplete surrogate pair"
                   ) ;error
                 ) ;when

                 (let* ((second-high (bytevector-u8-ref bv 2))
                        (second-low (bytevector-u8-ref bv 3))
                        (second-codepoint (+ (ash second-high 8) second-low)
                        ) ;second-codepoint
                       ) ;

                   (unless (<= 56320 second-codepoint 57343)
                     (error 'value-error
                       "utf16be->codepoint: invalid low surrogate"
                     ) ;error
                   ) ;unless

                   (let ((codepoint-prime (+ (ash (- first-codepoint 55296) 10)
                                            (- second-codepoint 56320)
                                          ) ;+
                         ) ;codepoint-prime
                        ) ;
                     (+ codepoint-prime 65536)
                   ) ;let
                 ) ;let*
                ) ;

                ((<= 56320 first-codepoint 57343)
                 ;; 低代理对作为第一个码元 - 无效
                 (error 'value-error
                   "utf16be->codepoint: invalid high surrogate"
                 ) ;error
                ) ;

                (else
                  ;; 基本多文种平面字符 - 单个码元
                  first-codepoint
                ) ;else
          ) ;cond
        ) ;let*
      ) ;let
    ) ;define

    (define (codepoint->utf16le codepoint)
      (unless (integer? codepoint)
        (error 'type-error
          "codepoint->utf16le: expected integer, got"
          codepoint
        ) ;error
      ) ;unless

      (when (or (< codepoint 0)
              (> codepoint 1114111)
            ) ;or
        (error 'value-error
          "codepoint->utf16le: codepoint out of Unicode range"
          codepoint
        ) ;error
      ) ;when

      ;; 检查是否为代理对码点（无效）
      (when (<= 55296 codepoint 57343)
        (error 'value-error
          "codepoint->utf16le: codepoint in surrogate pair range"
          codepoint
        ) ;error
      ) ;when

      (cond ((<= codepoint 65535)
             ;; 基本多文种平面字符 - 单个码元
             (let ((low-byte (bitwise-and codepoint 255))
                   (high-byte (ash codepoint -8))
                  ) ;
               (bytevector low-byte high-byte)
             ) ;let
            ) ;

            (else
              ;; 辅助平面字符 - 代理对
              (let* ((codepoint-prime (- codepoint 65536))
                     (high-surrogate (+ 55296 (ash codepoint-prime -10))
                     ) ;high-surrogate
                     (low-surrogate (+ 56320
                                      (bitwise-and codepoint-prime 1023)
                                    ) ;+
                     ) ;low-surrogate
                     (high-surrogate-low (bitwise-and high-surrogate 255)
                     ) ;high-surrogate-low
                     (high-surrogate-high (ash high-surrogate -8)
                     ) ;high-surrogate-high
                     (low-surrogate-low (bitwise-and low-surrogate 255)
                     ) ;low-surrogate-low
                     (low-surrogate-high (ash low-surrogate -8)
                     ) ;low-surrogate-high
                    ) ;
                (bytevector high-surrogate-low
                  high-surrogate-high
                  low-surrogate-low
                  low-surrogate-high
                ) ;bytevector
              ) ;let*
            ) ;else
      ) ;cond
    ) ;define

    (define (utf16le->codepoint bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf16le->codepoint: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error
            "utf16le->codepoint: empty bytevector"
          ) ;error
        ) ;when

        (when (< len 2)
          (error 'value-error
            "utf16le->codepoint: incomplete UTF-16LE sequence"
          ) ;error
        ) ;when

        (let* ((first-low (bytevector-u8-ref bv 0))
               (first-high (bytevector-u8-ref bv 1))
               (first-codepoint (+ (ash first-high 8) first-low)
               ) ;first-codepoint
              ) ;

          (cond ((<= 55296 first-codepoint 56319)
                 ;; 高代理对 - 需要低代理对
                 (when (< len 4)
                   (error 'value-error
                     "utf16le->codepoint: incomplete surrogate pair"
                   ) ;error
                 ) ;when

                 (let* ((second-low (bytevector-u8-ref bv 2))
                        (second-high (bytevector-u8-ref bv 3))
                        (second-codepoint (+ (ash second-high 8) second-low)
                        ) ;second-codepoint
                       ) ;

                   (unless (<= 56320 second-codepoint 57343)
                     (error 'value-error
                       "utf16le->codepoint: invalid low surrogate"
                     ) ;error
                   ) ;unless

                   (let ((codepoint-prime (+ (ash (- first-codepoint 55296) 10)
                                            (- second-codepoint 56320)
                                          ) ;+
                         ) ;codepoint-prime
                        ) ;
                     (+ codepoint-prime 65536)
                   ) ;let
                 ) ;let*
                ) ;

                ((<= 56320 first-codepoint 57343)
                 ;; 低代理对作为第一个码元 - 无效
                 (error 'value-error
                   "utf16le->codepoint: invalid high surrogate"
                 ) ;error
                ) ;

                (else
                  ;; 基本多文种平面字符 - 单个码元
                  first-codepoint
                ) ;else
          ) ;cond
        ) ;let*
      ) ;let
    ) ;define

    (define (utf8->utf16le bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf8->utf16le: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-advance-utf8 bv index len)
                    ) ;next-index
                   ) ;
                (if (= next-index index)
                  (error 'value-error
                    "utf8->utf16le: invalid UTF-8 sequence at index"
                    index
                  ) ;error
                  (let* ((utf8-bytes (bytevector-copy bv index next-index)
                         ) ;utf8-bytes
                         (codepoint (utf8->codepoint utf8-bytes))
                         (utf16le-bytes (codepoint->utf16le codepoint)
                         ) ;utf16le-bytes
                        ) ;
                    (loop next-index
                      (bytevector-append result utf16le-bytes)
                    ) ;loop
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (utf16le->utf8 bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf16le->utf8: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-utf16le-advance bv
                                  index
                                  len
                                ) ;bytevector-utf16le-advance
                    ) ;next-index
                   ) ;
                (if (= next-index index)
                  (error 'value-error
                    "utf16le->utf8: invalid UTF-16LE sequence at index"
                    index
                  ) ;error
                  (let* ((utf16le-bytes (bytevector-copy bv index next-index)
                         ) ;utf16le-bytes
                         (codepoint (utf16le->codepoint utf16le-bytes)
                         ) ;codepoint
                         (utf8-bytes (codepoint->utf8 codepoint))
                        ) ;
                    (loop next-index
                      (bytevector-append result utf8-bytes)
                    ) ;loop
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (bytevector-utf16le-advance bv
               index
               (end (bytevector-length bv))
             ) ;bytevector-utf16le-advance
      (unless (bytevector? bv)
        (error 'type-error
          "bytevector-utf16le-advance: expected bytevector"
        ) ;error
      ) ;unless

      (if (>= index end)
        index
        (let ((remaining (- end index)))
          (if (< remaining 2)
            index
            (let* ((low-byte (bytevector-u8-ref bv index))
                   (high-byte (bytevector-u8-ref bv (+ index 1))
                   ) ;high-byte
                   (codepoint (+ (ash high-byte 8) low-byte)
                   ) ;codepoint
                  ) ;

              (cond
                ;; 基本多文种平面字符 (非代理对)
                ((or (< codepoint 55296)
                   (> codepoint 57343)
                 ) ;or
                 (+ index 2)
                ) ;

                ;; 高代理对 (需要低代理对)
                ((<= 55296 codepoint 56319)
                 (if (< remaining 4)
                   index
                   (let* ((second-low (bytevector-u8-ref bv (+ index 2))
                          ) ;second-low
                          (second-high (bytevector-u8-ref bv (+ index 3))
                          ) ;second-high
                          (second-codepoint (+ (ash second-high 8) second-low)
                          ) ;second-codepoint
                         ) ;
                     (if (<= 56320 second-codepoint 57343)
                       (+ index 4)
                       index
                     ) ;if
                   ) ;let*
                 ) ;if
                ) ;

                ;; 低代理对作为第一个码元 - 无效
                ((<= 56320 codepoint 57343) index)

                (else index)
              ) ;cond
            ) ;let*
          ) ;if
        ) ;let
      ) ;if
    ) ;define*

    (define (utf8->utf16be bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf8->utf16be: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-advance-utf8 bv index len)
                    ) ;next-index
                   ) ;
                (if (= next-index index)
                  (error 'value-error
                    "utf8->utf16be: invalid UTF-8 sequence at index"
                    index
                  ) ;error
                  (let* ((utf8-bytes (bytevector-copy bv index next-index)
                         ) ;utf8-bytes
                         (codepoint (utf8->codepoint utf8-bytes))
                         (utf16be-bytes (codepoint->utf16be codepoint)
                         ) ;utf16be-bytes
                        ) ;
                    (loop next-index
                      (bytevector-append result utf16be-bytes)
                    ) ;loop
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (bytevector-utf16be-advance bv
               index
               (end (bytevector-length bv))
             ) ;bytevector-utf16be-advance
      (unless (bytevector? bv)
        (error 'type-error
          "bytevector-utf16be-advance: expected bytevector"
        ) ;error
      ) ;unless

      (if (>= index end)
        index
        (let ((remaining (- end index)))
          (if (< remaining 2)
            index
            (let* ((high-byte (bytevector-u8-ref bv index))
                   (low-byte (bytevector-u8-ref bv (+ index 1))
                   ) ;low-byte
                   (codepoint (+ (ash high-byte 8) low-byte)
                   ) ;codepoint
                  ) ;

              (cond
                ;; 基本多文种平面字符 (非代理对)
                ((or (< codepoint 55296)
                   (> codepoint 57343)
                 ) ;or
                 (+ index 2)
                ) ;

                ;; 高代理对 (需要低代理对)
                ((<= 55296 codepoint 56319)
                 (if (< remaining 4)
                   index
                   (let* ((second-high (bytevector-u8-ref bv (+ index 2))
                          ) ;second-high
                          (second-low (bytevector-u8-ref bv (+ index 3))
                          ) ;second-low
                          (second-codepoint (+ (ash second-high 8) second-low)
                          ) ;second-codepoint
                         ) ;
                     (if (<= 56320 second-codepoint 57343)
                       (+ index 4)
                       index
                     ) ;if
                   ) ;let*
                 ) ;if
                ) ;

                ;; 低代理对作为第一个码元 - 无效
                ((<= 56320 codepoint 57343) index)

                (else index)
              ) ;cond
            ) ;let*
          ) ;if
        ) ;let
      ) ;if
    ) ;define*

    (define (utf16be->utf8 bv)
      (unless (bytevector? bv)
        (error 'type-error
          "utf16be->utf8: expected bytevector"
        ) ;error
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-utf16be-advance bv
                                  index
                                  len
                                ) ;bytevector-utf16be-advance
                    ) ;next-index
                   ) ;
                (if (= next-index index)
                  (error 'value-error
                    "utf16be->utf8: invalid UTF-16BE sequence at index"
                    index
                  ) ;error
                  (let* ((utf16be-bytes (bytevector-copy bv index next-index)
                         ) ;utf16be-bytes
                         (codepoint (utf16be->codepoint utf16be-bytes)
                         ) ;codepoint
                         (utf8-bytes (codepoint->utf8 codepoint))
                        ) ;
                    (loop next-index
                      (bytevector-append result utf8-bytes)
                    ) ;loop
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
