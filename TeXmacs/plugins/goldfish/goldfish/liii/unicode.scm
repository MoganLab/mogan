(define-library (liii unicode)
  (export
    ;; UTF-8 函数
    utf8-string
    utf8-make-string
    utf8->string
    string->utf8
    utf8-string-length
    utf8-substring
    bytevector-advance-utf8
    codepoint->utf8
    utf8->codepoint
    utf8->codepoint-at
    utf8-string-trim-right
    utf8-string-trim-left
    utf8-string-trim-both
    utf8-string-set!

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

  (import (scheme base) (liii base) (liii ascii) (liii bitwise) (liii error))

  (begin

    (define (utf8-string . chars)
      (let loop
        ((rest chars) (chunks '()))
        (cond ((null? rest)
               (if (null? chunks) "" (utf8->string (apply bytevector-append (reverse chunks))))
              ) ;
              ((char? (car rest))
               (loop (cdr rest) (cons (codepoint->utf8 (char->integer (car rest))) chunks))
              ) ;
              (else (error 'type-error "utf8-string: expected char" (car rest)))
        ) ;cond
      ) ;let
    ) ;define

    (define (utf8-make-string k . char-opt)
      (unless (and (integer? k) (>= k 0))
        (error 'out-of-range "utf8-make-string: length must be non-negative integer" k)
      ) ;unless
      (if (zero? k)
        ""
        (let ((c (if (null? char-opt) #\space (car char-opt))))
          (unless (char? c)
            (error 'type-error "utf8-make-string: expected char" c)
          ) ;unless
          (let* ((utf8-bv (codepoint->utf8 (char->integer c)))
                 (unit-len (bytevector-length utf8-bv))
                 (total-len (* k unit-len))
                 (result (make-bytevector total-len))
                ) ;
            (let outer
              ((i 0))
              (if (= i k)
                (utf8->string result)
                (let inner
                  ((j 0) (offset (* i unit-len)))
                  (if (= j unit-len)
                    (outer (+ i 1))
                    (begin
                      (bytevector-u8-set! result offset (bytevector-u8-ref utf8-bv j))
                      (inner (+ j 1) (+ offset 1))
                    ) ;begin
                  ) ;if
                ) ;let
              ) ;if
            ) ;let
          ) ;let*
        ) ;let
      ) ;if
    ) ;define

    (define* (utf8-substring str (start 0) (end #t))
      (utf8->string (string->utf8 str start end))
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
        (char=? c #\x200a)
        (char=? c #\x202f)
        (char=? c #\x205f)
        (char=? c #\x3000)
      ) ;or
    ) ;define

    ;; ; utf8-string-trim-right: 从字符串右侧移除空白字符
    ;; ; 正确处理UTF-8编码的中文字符
    (define (utf8-string-trim-right str)
      (unless (string? str)
        (error 'type-error "utf8-string-trim-right: expected string")
      ) ;unless

      (let* ((bv (string->utf8 str)) (byte-len (bytevector-length bv)))
        (if (= byte-len 0)
          ""
          (let loop
            ((byte-pos byte-len))
            (if (<= byte-pos 0)
              ""
              (let* ((prev-byte-pos (bytevector-rindex-utf8 bv 0 byte-pos))
                     (char-bv (bytevector-copy bv prev-byte-pos byte-pos))
                     (ch (utf8->string char-bv))
                    ) ;
                (if (and (= (string-length ch) 1) (unicode-whitespace? (string-ref ch 0)))
                  (loop prev-byte-pos)
                  (if (= byte-pos byte-len) str (utf8->string (bytevector-copy bv 0 byte-pos)))
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
            (if (or (<= 0 b 127) (<= 192 b 255)) pos (loop (- pos 1)))
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    ;; ; utf8-string-trim-left: 从字符串左侧移除空白字符
    ;; ; 正确处理UTF-8编码的中文字符
    (define (utf8-string-trim-left str)
      (unless (string? str)
        (error 'type-error "utf8-string-trim-left: expected string")
      ) ;unless

      (let* ((bv (string->utf8 str)) (byte-len (bytevector-length bv)))
        (if (= byte-len 0)
          ""
          (let loop
            ((byte-pos 0))
            (if (>= byte-pos byte-len)
              ""
              (let* ((next-byte-pos (bytevector-advance-utf8 bv byte-pos byte-len))
                     (char-bv (bytevector-copy bv byte-pos next-byte-pos))
                     (ch (utf8->string char-bv))
                    ) ;
                (if (and (= (string-length ch) 1) (unicode-whitespace? (string-ref ch 0)))
                  (loop next-byte-pos)
                  (if (= byte-pos 0) str (utf8->string (bytevector-copy bv byte-pos byte-len)))
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
        (error 'type-error "utf8-string-trim-both: expected string")
      ) ;unless

      (utf8-string-trim-right (utf8-string-trim-left str))
    ) ;define

    (define (utf8-string-set! str index char)
      (unless (string? str)
        (error 'type-error "utf8-string-set!: expected string" str)
      ) ;unless
      (unless (and (integer? index) (>= index 0))
        (error 'out-of-range
          "utf8-string-set!: index must be non-negative integer"
          index
        ) ;error
      ) ;unless
      (unless (char? char)
        (error 'type-error "utf8-string-set!: expected char" char)
      ) ;unless
      (let* ((bv (string->byte-vector str))
             (byte-len (bytevector-length bv))
             (char-bv (codepoint->utf8 (char->integer char)))
            ) ;
        (let loop
          ((byte-pos 0) (char-index 0))
          (if (>= byte-pos byte-len)
            (error 'out-of-range "utf8-string-set!: index out of range" index)
            (let ((next-byte-pos (bytevector-advance-utf8 bv byte-pos byte-len)))
              (if (= char-index index)
                (let ((old-char-len (- next-byte-pos byte-pos))
                      (char-bv-len (bytevector-length char-bv))
                     ) ;
                  (if (= old-char-len char-bv-len)
                    ;; 等长替换：直接修改原 bytevector，无需复制
                    (let replace-char
                      ((j 0))
                      (if (= j char-bv-len)
                        (byte-vector->string bv)
                        (begin
                          (bytevector-u8-set! bv (+ byte-pos j) (bytevector-u8-ref char-bv j))
                          (replace-char (+ j 1))
                        ) ;begin
                      ) ;if
                    ) ;let
                    ;; 不等长替换：make-bytevector + 手动循环填充，减少临时对象分配
                    (let* ((new-len (+ (- byte-len old-char-len) char-bv-len))
                           (result (make-bytevector new-len))
                          ) ;
                      (let copy-front
                        ((i 0))
                        (if (= i byte-pos)
                          (let copy-char
                            ((j 0))
                            (if (= j char-bv-len)
                              (let copy-back
                                ((src next-byte-pos) (dst (+ byte-pos char-bv-len)))
                                (if (= src byte-len)
                                  (byte-vector->string result)
                                  (begin
                                    (bytevector-u8-set! result dst (bytevector-u8-ref bv src))
                                    (copy-back (+ src 1) (+ dst 1))
                                  ) ;begin
                                ) ;if
                              ) ;let
                              (begin
                                (bytevector-u8-set! result (+ byte-pos j) (bytevector-u8-ref char-bv j))
                                (copy-char (+ j 1))
                              ) ;begin
                            ) ;if
                          ) ;let
                          (begin
                            (bytevector-u8-set! result i (bytevector-u8-ref bv i))
                            (copy-front (+ i 1))
                          ) ;begin
                        ) ;if
                      ) ;let
                    ) ;let*
                  ) ;if
                ) ;let
                (loop next-byte-pos (+ char-index 1))
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (codepoint->utf8 codepoint)
      (unless (integer? codepoint)
        (error 'type-error "codepoint->utf8: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint 1114111))
        (error 'value-error "codepoint->utf8: codepoint out of Unicode range" codepoint)
      ) ;when

      (cond ((<= codepoint 127) (bytevector codepoint))

            ((<= codepoint 2047)
             (let ((byte1 (bitwise-ior 192 (bitwise-and (ash codepoint -6) 31)))
                   (byte2 (bitwise-ior 128 (bitwise-and codepoint 63)))
                  ) ;
               (bytevector byte1 byte2)
             ) ;let
            ) ;

            ((<= codepoint 65535)
             (let ((byte1 (bitwise-ior 224 (bitwise-and (ash codepoint -12) 15)))
                   (byte2 (bitwise-ior 128 (bitwise-and (ash codepoint -6) 63)))
                   (byte3 (bitwise-ior 128 (bitwise-and codepoint 63)))
                  ) ;
               (bytevector byte1 byte2 byte3)
             ) ;let
            ) ;

            (else (let ((byte1 (bitwise-ior 240 (bitwise-and (ash codepoint -18) 7)))
                        (byte2 (bitwise-ior 128 (bitwise-and (ash codepoint -12) 63)))
                        (byte3 (bitwise-ior 128 (bitwise-and (ash codepoint -6) 63)))
                        (byte4 (bitwise-ior 128 (bitwise-and codepoint 63)))
                       ) ;
                    (bytevector byte1 byte2 byte3 byte4)
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (utf8->codepoint bv)
      (unless (bytevector? bv)
        (error 'type-error "utf8->codepoint: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error "utf8->codepoint: empty bytevector")
        ) ;when

        (let ((first-byte (bytevector-u8-ref bv 0)))
          (cond ((<= first-byte 127) first-byte)

                ((<= 194 first-byte 223)
                 (when (< len 2)
                   (error 'value-error "utf8->codepoint: incomplete 2-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1)))
                   (unless (<= 128 byte2 191)
                     (error 'value-error "utf8->codepoint: invalid continuation byte")
                   ) ;unless
                   (bitwise-ior (ash (bitwise-and first-byte 31) 6) (bitwise-and byte2 63))
                 ) ;let
                ) ;

                ((<= 224 first-byte 239)
                 (when (< len 3)
                   (error 'value-error "utf8->codepoint: incomplete 3-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1)) (byte3 (bytevector-u8-ref bv 2)))
                   (unless (and (<= 128 byte2 191) (<= 128 byte3 191))
                     (error 'value-error "utf8->codepoint: invalid continuation byte")
                   ) ;unless
                   (let ((codepoint (bitwise-ior (ash (bitwise-and first-byte 15) 12)
                                      (ash (bitwise-and byte2 63) 6)
                                      (bitwise-and byte3 63)
                                    ) ;bitwise-ior
                         ) ;codepoint
                        ) ;
                     (when (or (<= 55296 codepoint 57343)
                             (and (= first-byte 224) (< codepoint 2048))
                             (and (= first-byte 237) (>= codepoint 55296))
                           ) ;or
                       (error 'value-error "utf8->codepoint: invalid codepoint")
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                ((<= 240 first-byte 244)
                 (when (< len 4)
                   (error 'value-error "utf8->codepoint: incomplete 4-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv 1))
                       (byte3 (bytevector-u8-ref bv 2))
                       (byte4 (bytevector-u8-ref bv 3))
                      ) ;
                   (unless (and (<= 128 byte2 191) (<= 128 byte3 191) (<= 128 byte4 191))
                     (error 'value-error "utf8->codepoint: invalid continuation byte")
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
                             (and (= first-byte 240) (< codepoint 65536))
                             (and (= first-byte 244) (> codepoint 1114111))
                           ) ;or
                       (error 'value-error "utf8->codepoint: invalid codepoint")
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                (else (error 'value-error "utf8->codepoint: invalid UTF-8 sequence"))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (utf8->codepoint-at bv start)
      (unless (bytevector? bv)
        (error 'type-error "utf8->codepoint-at: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (>= start len)
          (error 'value-error "utf8->codepoint-at: start index past end of bytevector")
        ) ;when

        (let ((first-byte (bytevector-u8-ref bv start)))
          (cond ((<= first-byte 127) first-byte)

                ((<= 194 first-byte 223)
                 (when (> (+ start 2) len)
                   (error 'value-error "utf8->codepoint-at: incomplete 2-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv (+ start 1))))
                   (unless (<= 128 byte2 191)
                     (error 'value-error "utf8->codepoint-at: invalid continuation byte")
                   ) ;unless
                   (bitwise-ior (ash (bitwise-and first-byte 31) 6) (bitwise-and byte2 63))
                 ) ;let
                ) ;

                ((<= 224 first-byte 239)
                 (when (> (+ start 3) len)
                   (error 'value-error "utf8->codepoint-at: incomplete 3-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv (+ start 1)))
                       (byte3 (bytevector-u8-ref bv (+ start 2)))
                      ) ;
                   (unless (and (<= 128 byte2 191) (<= 128 byte3 191))
                     (error 'value-error "utf8->codepoint-at: invalid continuation byte")
                   ) ;unless
                   (let ((codepoint (bitwise-ior (ash (bitwise-and first-byte 15) 12)
                                      (ash (bitwise-and byte2 63) 6)
                                      (bitwise-and byte3 63)
                                    ) ;bitwise-ior
                         ) ;codepoint
                        ) ;
                     (when (or (<= 55296 codepoint 57343)
                             (and (= first-byte 224) (< codepoint 2048))
                             (and (= first-byte 237) (>= codepoint 55296))
                           ) ;or
                       (error 'value-error "utf8->codepoint-at: invalid codepoint")
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                ((<= 240 first-byte 244)
                 (when (> (+ start 4) len)
                   (error 'value-error "utf8->codepoint-at: incomplete 4-byte sequence")
                 ) ;when
                 (let ((byte2 (bytevector-u8-ref bv (+ start 1)))
                       (byte3 (bytevector-u8-ref bv (+ start 2)))
                       (byte4 (bytevector-u8-ref bv (+ start 3)))
                      ) ;
                   (unless (and (<= 128 byte2 191) (<= 128 byte3 191) (<= 128 byte4 191))
                     (error 'value-error "utf8->codepoint-at: invalid continuation byte")
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
                             (and (= first-byte 240) (< codepoint 65536))
                             (and (= first-byte 244) (> codepoint 1114111))
                           ) ;or
                       (error 'value-error "utf8->codepoint-at: invalid codepoint")
                     ) ;when
                     codepoint
                   ) ;let
                 ) ;let
                ) ;

                (else (error 'value-error "utf8->codepoint-at: invalid UTF-8 sequence"))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define unicode-max-codepoint 1114111)
    (define unicode-replacement-char 65533)

    (define (hexstr->codepoint hex-string)
      (unless (string? hex-string)
        (error 'type-error "hexstr->codepoint: expected string")
      ) ;unless

      (when (string=? hex-string "")
        (error 'value-error "hexstr->codepoint: empty string")
      ) ;when

      ;; 验证十六进制字符
      (let loop
        ((chars (string->list hex-string)))
        (unless (null? chars)
          (let ((c (car chars)))
            (unless (or (ascii-numeric? c) (char<=? #\A c #\F) (char<=? #\a c #\f))
              (error 'value-error "hexstr->codepoint: invalid hexadecimal string")
            ) ;unless
            (loop (cdr chars))
          ) ;let
        ) ;unless
      ) ;let

      (let ((codepoint (string->number hex-string 16)))
        (unless codepoint
          (error 'value-error "hexstr->codepoint: invalid hexadecimal format")
        ) ;unless

        (when (or (< codepoint 0) (> codepoint unicode-max-codepoint))
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
        (error 'type-error "codepoint->hexstr: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint unicode-max-codepoint))
        (error 'value-error
          "codepoint->hexstr: codepoint out of Unicode range"
          codepoint
        ) ;error
      ) ;when

      (let ((hex-str (ascii-upcase (number->string codepoint 16))))
        (if (and (> codepoint 0) (< codepoint 16) (= (string-length hex-str) 1))
          (string-append "0" hex-str)
          hex-str
        ) ;if
      ) ;let
    ) ;define

    (define (codepoint->utf16be codepoint)
      (unless (integer? codepoint)
        (error 'type-error "codepoint->utf16be: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint 1114111))
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
             (let ((high-byte (ash codepoint -8)) (low-byte (bitwise-and codepoint 255)))
               (bytevector high-byte low-byte)
             ) ;let
            ) ;

            (else
              ;; 辅助平面字符 - 代理对
              (let* ((codepoint-prime (- codepoint 65536))
                     (high-surrogate (+ 55296 (ash codepoint-prime -10)))
                     (low-surrogate (+ 56320 (bitwise-and codepoint-prime 1023)))
                     (high-surrogate-high (ash high-surrogate -8))
                     (high-surrogate-low (bitwise-and high-surrogate 255))
                     (low-surrogate-high (ash low-surrogate -8))
                     (low-surrogate-low (bitwise-and low-surrogate 255))
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
        (error 'type-error "utf16be->codepoint: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error "utf16be->codepoint: empty bytevector")
        ) ;when

        (when (< len 2)
          (error 'value-error "utf16be->codepoint: incomplete UTF-16BE sequence")
        ) ;when

        (let* ((first-high (bytevector-u8-ref bv 0))
               (first-low (bytevector-u8-ref bv 1))
               (first-codepoint (+ (ash first-high 8) first-low))
              ) ;

          (cond ((<= 55296 first-codepoint 56319)
                 ;; 高代理对 - 需要低代理对
                 (when (< len 4)
                   (error 'value-error "utf16be->codepoint: incomplete surrogate pair")
                 ) ;when

                 (let* ((second-high (bytevector-u8-ref bv 2))
                        (second-low (bytevector-u8-ref bv 3))
                        (second-codepoint (+ (ash second-high 8) second-low))
                       ) ;

                   (unless (<= 56320 second-codepoint 57343)
                     (error 'value-error "utf16be->codepoint: invalid low surrogate")
                   ) ;unless

                   (let ((codepoint-prime (+ (ash (- first-codepoint 55296) 10) (- second-codepoint 56320))
                         ) ;codepoint-prime
                        ) ;
                     (+ codepoint-prime 65536)
                   ) ;let
                 ) ;let*
                ) ;

                ((<= 56320 first-codepoint 57343)
                 ;; 低代理对作为第一个码元 - 无效
                 (error 'value-error "utf16be->codepoint: invalid high surrogate")
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
        (error 'type-error "codepoint->utf16le: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint 1114111))
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
             (let ((low-byte (bitwise-and codepoint 255)) (high-byte (ash codepoint -8)))
               (bytevector low-byte high-byte)
             ) ;let
            ) ;

            (else
              ;; 辅助平面字符 - 代理对
              (let* ((codepoint-prime (- codepoint 65536))
                     (high-surrogate (+ 55296 (ash codepoint-prime -10)))
                     (low-surrogate (+ 56320 (bitwise-and codepoint-prime 1023)))
                     (high-surrogate-low (bitwise-and high-surrogate 255))
                     (high-surrogate-high (ash high-surrogate -8))
                     (low-surrogate-low (bitwise-and low-surrogate 255))
                     (low-surrogate-high (ash low-surrogate -8))
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
        (error 'type-error "utf16le->codepoint: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (when (= len 0)
          (error 'value-error "utf16le->codepoint: empty bytevector")
        ) ;when

        (when (< len 2)
          (error 'value-error "utf16le->codepoint: incomplete UTF-16LE sequence")
        ) ;when

        (let* ((first-low (bytevector-u8-ref bv 0))
               (first-high (bytevector-u8-ref bv 1))
               (first-codepoint (+ (ash first-high 8) first-low))
              ) ;

          (cond ((<= 55296 first-codepoint 56319)
                 ;; 高代理对 - 需要低代理对
                 (when (< len 4)
                   (error 'value-error "utf16le->codepoint: incomplete surrogate pair")
                 ) ;when

                 (let* ((second-low (bytevector-u8-ref bv 2))
                        (second-high (bytevector-u8-ref bv 3))
                        (second-codepoint (+ (ash second-high 8) second-low))
                       ) ;

                   (unless (<= 56320 second-codepoint 57343)
                     (error 'value-error "utf16le->codepoint: invalid low surrogate")
                   ) ;unless

                   (let ((codepoint-prime (+ (ash (- first-codepoint 55296) 10) (- second-codepoint 56320))
                         ) ;codepoint-prime
                        ) ;
                     (+ codepoint-prime 65536)
                   ) ;let
                 ) ;let*
                ) ;

                ((<= 56320 first-codepoint 57343)
                 ;; 低代理对作为第一个码元 - 无效
                 (error 'value-error "utf16le->codepoint: invalid high surrogate")
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
        (error 'type-error "utf8->utf16le: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-advance-utf8 bv index len)))
                (if (= next-index index)
                  (error 'value-error "utf8->utf16le: invalid UTF-8 sequence at index" index)
                  (let* ((utf8-bytes (bytevector-copy bv index next-index))
                         (codepoint (utf8->codepoint utf8-bytes))
                         (utf16le-bytes (codepoint->utf16le codepoint))
                        ) ;
                    (loop next-index (bytevector-append result utf16le-bytes))
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
        (error 'type-error "utf16le->utf8: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-utf16le-advance bv index len)))
                (if (= next-index index)
                  (error 'value-error "utf16le->utf8: invalid UTF-16LE sequence at index" index)
                  (let* ((utf16le-bytes (bytevector-copy bv index next-index))
                         (codepoint (utf16le->codepoint utf16le-bytes))
                         (utf8-bytes (codepoint->utf8 codepoint))
                        ) ;
                    (loop next-index (bytevector-append result utf8-bytes))
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (bytevector-utf16le-advance bv index (end (bytevector-length bv)))
      (unless (bytevector? bv)
        (error 'type-error "bytevector-utf16le-advance: expected bytevector")
      ) ;unless

      (if (>= index end)
        index
        (let ((remaining (- end index)))
          (if (< remaining 2)
            index
            (let* ((low-byte (bytevector-u8-ref bv index))
                   (high-byte (bytevector-u8-ref bv (+ index 1)))
                   (codepoint (+ (ash high-byte 8) low-byte))
                  ) ;

              (cond
                ;; 基本多文种平面字符 (非代理对)
                ((or (< codepoint 55296) (> codepoint 57343)) (+ index 2))

                ;; 高代理对 (需要低代理对)
                ((<= 55296 codepoint 56319)
                 (if (< remaining 4)
                   index
                   (let* ((second-low (bytevector-u8-ref bv (+ index 2)))
                          (second-high (bytevector-u8-ref bv (+ index 3)))
                          (second-codepoint (+ (ash second-high 8) second-low))
                         ) ;
                     (if (<= 56320 second-codepoint 57343) (+ index 4) index)
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
        (error 'type-error "utf8->utf16be: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-advance-utf8 bv index len)))
                (if (= next-index index)
                  (error 'value-error "utf8->utf16be: invalid UTF-8 sequence at index" index)
                  (let* ((utf8-bytes (bytevector-copy bv index next-index))
                         (codepoint (utf8->codepoint utf8-bytes))
                         (utf16be-bytes (codepoint->utf16be codepoint))
                        ) ;
                    (loop next-index (bytevector-append result utf16be-bytes))
                  ) ;let*
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define* (bytevector-utf16be-advance bv index (end (bytevector-length bv)))
      (unless (bytevector? bv)
        (error 'type-error "bytevector-utf16be-advance: expected bytevector")
      ) ;unless

      (if (>= index end)
        index
        (let ((remaining (- end index)))
          (if (< remaining 2)
            index
            (let* ((high-byte (bytevector-u8-ref bv index))
                   (low-byte (bytevector-u8-ref bv (+ index 1)))
                   (codepoint (+ (ash high-byte 8) low-byte))
                  ) ;

              (cond
                ;; 基本多文种平面字符 (非代理对)
                ((or (< codepoint 55296) (> codepoint 57343)) (+ index 2))

                ;; 高代理对 (需要低代理对)
                ((<= 55296 codepoint 56319)
                 (if (< remaining 4)
                   index
                   (let* ((second-high (bytevector-u8-ref bv (+ index 2)))
                          (second-low (bytevector-u8-ref bv (+ index 3)))
                          (second-codepoint (+ (ash second-high 8) second-low))
                         ) ;
                     (if (<= 56320 second-codepoint 57343) (+ index 4) index)
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
        (error 'type-error "utf16be->utf8: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
          (bytevector)
          (let loop
            ((index 0) (result (bytevector)))
            (if (>= index len)
              result
              (let ((next-index (bytevector-utf16be-advance bv index len)))
                (if (= next-index index)
                  (error 'value-error "utf16be->utf8: invalid UTF-16BE sequence at index" index)
                  (let* ((utf16be-bytes (bytevector-copy bv index next-index))
                         (codepoint (utf16be->codepoint utf16be-bytes))
                         (utf8-bytes (codepoint->utf8 codepoint))
                        ) ;
                    (loop next-index (bytevector-append result utf8-bytes))
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
