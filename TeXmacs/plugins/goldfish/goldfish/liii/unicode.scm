;
; Copyright (C) 2025 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii unicode)
  (export
   ;; UTF-8 函数
   utf8->string string->utf8 utf8-string-length utf8-substring bytevector-advance-utf8
   codepoint->utf8 utf8->codepoint

   ;; UTF-16BE 函数
   codepoint->utf16be utf16be->codepoint utf8->utf16be utf16be->utf8 bytevector-utf16be-advance

   ;; UTF-16LE 函数
   codepoint->utf16le utf16le->codepoint utf8->utf16le utf16le->utf8 bytevector-utf16le-advance

   ;; 十六进制字符串与码点转换函数
   hexstr->codepoint codepoint->hexstr

   ;; Unicode 常量
   unicode-max-codepoint unicode-replacement-char
  ) ;export

  (import (scheme base)
          (liii base)
          (liii bitwise)
          (liii error)
  ) ;import

  (begin

    (define* (utf8-substring str (start 0) (end #t))
      (utf8->string (string->utf8 str start end))
    ) ;define*

    (define (codepoint->utf8 codepoint)
      (unless (integer? codepoint)
        (error 'type-error "codepoint->utf8: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint #x10FFFF))
        (error 'value-error "codepoint->utf8: codepoint out of Unicode range" codepoint)
      ) ;when

      (cond
        ((<= codepoint #x7F)
         (bytevector codepoint)
        ) ;

        ((<= codepoint #x7FF)
         (let ((byte1 (bitwise-ior #b11000000 (bitwise-and (ash codepoint -6) #b00011111)))
               (byte2 (bitwise-ior #b10000000 (bitwise-and codepoint #b00111111))))
           (bytevector byte1 byte2)
         ) ;let
        ) ;

        ((<= codepoint #xFFFF)
         (let ((byte1 (bitwise-ior #b11100000 (bitwise-and (ash codepoint -12) #b00001111)))
               (byte2 (bitwise-ior #b10000000 (bitwise-and (ash codepoint -6) #b00111111)))
               (byte3 (bitwise-ior #b10000000 (bitwise-and codepoint #b00111111))))
           (bytevector byte1 byte2 byte3)
         ) ;let
        ) ;

        (else
         (let ((byte1 (bitwise-ior #b11110000 (bitwise-and (ash codepoint -18) #b00000111)))
               (byte2 (bitwise-ior #b10000000 (bitwise-and (ash codepoint -12) #b00111111)))
               (byte3 (bitwise-ior #b10000000 (bitwise-and (ash codepoint -6) #b00111111)))
               (byte4 (bitwise-ior #b10000000 (bitwise-and codepoint #b00111111))))
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
          (cond
            ((<= first-byte #x7F)
             first-byte
            ) ;

            ((<= #xC2 first-byte #xDF)
             (when (< len 2)
               (error 'value-error "utf8->codepoint: incomplete 2-byte sequence")
             ) ;when
             (let ((byte2 (bytevector-u8-ref bv 1)))
               (unless (<= #x80 byte2 #xBF)
                 (error 'value-error "utf8->codepoint: invalid continuation byte")
               ) ;unless
               (bitwise-ior
                 (ash (bitwise-and first-byte #b00011111) 6)
                 (bitwise-and byte2 #b00111111)
               ) ;bitwise-ior
             ) ;let
            ) ;

            ((<= #xE0 first-byte #xEF)
             (when (< len 3)
               (error 'value-error "utf8->codepoint: incomplete 3-byte sequence")
             ) ;when
             (let ((byte2 (bytevector-u8-ref bv 1))
                   (byte3 (bytevector-u8-ref bv 2)))
               (unless (and (<= #x80 byte2 #xBF) (<= #x80 byte3 #xBF))
                 (error 'value-error "utf8->codepoint: invalid continuation byte")
               ) ;unless
               (let ((codepoint (bitwise-ior
                                 (ash (bitwise-and first-byte #b00001111) 12)
                                 (ash (bitwise-and byte2 #b00111111) 6)
                                 (bitwise-and byte3 #b00111111))))
                 (when (or (<= #xD800 codepoint #xDFFF)
                           (and (= first-byte #xE0) (< codepoint #x0800))
                           (and (= first-byte #xED) (>= codepoint #xD800)))
                   (error 'value-error "utf8->codepoint: invalid codepoint")
                 ) ;when
                 codepoint
               ) ;let
             ) ;let
            ) ;

            ((<= #xF0 first-byte #xF4)
             (when (< len 4)
               (error 'value-error "utf8->codepoint: incomplete 4-byte sequence")
             ) ;when
             (let ((byte2 (bytevector-u8-ref bv 1))
                   (byte3 (bytevector-u8-ref bv 2))
                   (byte4 (bytevector-u8-ref bv 3)))
               (unless (and (<= #x80 byte2 #xBF) (<= #x80 byte3 #xBF) (<= #x80 byte4 #xBF))
                 (error 'value-error "utf8->codepoint: invalid continuation byte")
               ) ;unless
               (let ((codepoint (bitwise-ior
                                 (ash (bitwise-and first-byte #b00000111) 18)
                                 (ash (bitwise-and byte2 #b00111111) 12)
                                 (ash (bitwise-and byte3 #b00111111) 6)
                                 (bitwise-and byte4 #b00111111))))
                 (when (or (< codepoint #x10000)
                           (> codepoint #x10FFFF)
                           (and (= first-byte #xF0) (< codepoint #x10000))
                           (and (= first-byte #xF4) (> codepoint #x10FFFF)))
                   (error 'value-error "utf8->codepoint: invalid codepoint")
                 ) ;when
                 codepoint
               ) ;let
             ) ;let
            ) ;

            (else
             (error 'value-error "utf8->codepoint: invalid UTF-8 sequence")
            ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define unicode-max-codepoint #x10FFFF)
    (define unicode-replacement-char #xFFFD)

    (define (hexstr->codepoint hex-string)
      (unless (string? hex-string)
        (error 'type-error "hexstr->codepoint: expected string")
      ) ;unless

      (when (string=? hex-string "")
        (error 'value-error "hexstr->codepoint: empty string")
      ) ;when

      ;; 验证十六进制字符
      (let loop ((chars (string->list hex-string)))
        (unless (null? chars)
          (let ((c (car chars)))
            (unless (or (char-numeric? c)
                        (char<=? #\A c #\F)
                        (char<=? #\a c #\f))
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
          (error 'value-error "hexstr->codepoint: codepoint out of Unicode range" codepoint)
        ) ;when

        codepoint
      ) ;let
    ) ;define

    (define (codepoint->hexstr codepoint)
      (unless (integer? codepoint)
        (error 'type-error "codepoint->hexstr: expected integer, got" codepoint)
      ) ;unless

      (when (or (< codepoint 0) (> codepoint unicode-max-codepoint))
        (error 'value-error "codepoint->hexstr: codepoint out of Unicode range" codepoint)
      ) ;when

      (let ((hex-str (string-upcase (number->string codepoint 16))))
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

      (when (or (< codepoint 0) (> codepoint #x10FFFF))
        (error 'value-error "codepoint->utf16be: codepoint out of Unicode range" codepoint)
      ) ;when

      ;; 检查是否为代理对码点（无效）
      (when (<= #xD800 codepoint #xDFFF)
        (error 'value-error "codepoint->utf16be: codepoint in surrogate pair range" codepoint)
      ) ;when

      (cond
        ((<= codepoint #xFFFF)
         ;; 基本多文种平面字符 - 单个码元
         (let ((high-byte (ash codepoint -8))
               (low-byte (bitwise-and codepoint #xFF)))
           (bytevector high-byte low-byte)
         ) ;let
        ) ;

        (else
         ;; 辅助平面字符 - 代理对
         (let* ((codepoint-prime (- codepoint #x10000))
                (high-surrogate (+ #xD800 (ash codepoint-prime -10)))
                (low-surrogate (+ #xDC00 (bitwise-and codepoint-prime #x3FF)))
                (high-surrogate-high (ash high-surrogate -8))
                (high-surrogate-low (bitwise-and high-surrogate #xFF))
                (low-surrogate-high (ash low-surrogate -8))
                (low-surrogate-low (bitwise-and low-surrogate #xFF)))
           (bytevector high-surrogate-high high-surrogate-low
                       low-surrogate-high low-surrogate-low
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
               (first-codepoint (+ (ash first-high 8) first-low)))

          (cond
            ((<= #xD800 first-codepoint #xDBFF)
             ;; 高代理对 - 需要低代理对
             (when (< len 4)
               (error 'value-error "utf16be->codepoint: incomplete surrogate pair")
             ) ;when

             (let* ((second-high (bytevector-u8-ref bv 2))
                    (second-low (bytevector-u8-ref bv 3))
                    (second-codepoint (+ (ash second-high 8) second-low)))

               (unless (<= #xDC00 second-codepoint #xDFFF)
                 (error 'value-error "utf16be->codepoint: invalid low surrogate")
               ) ;unless

               (let ((codepoint-prime (+ (ash (- first-codepoint #xD800) 10)
                                         (- second-codepoint #xDC00))))
                 (+ codepoint-prime #x10000)
               ) ;let
             ) ;let*
            ) ;

            ((<= #xDC00 first-codepoint #xDFFF)
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

      (when (or (< codepoint 0) (> codepoint #x10FFFF))
        (error 'value-error "codepoint->utf16le: codepoint out of Unicode range" codepoint)
      ) ;when

      ;; 检查是否为代理对码点（无效）
      (when (<= #xD800 codepoint #xDFFF)
        (error 'value-error "codepoint->utf16le: codepoint in surrogate pair range" codepoint)
      ) ;when

      (cond
        ((<= codepoint #xFFFF)
         ;; 基本多文种平面字符 - 单个码元
         (let ((low-byte (bitwise-and codepoint #xFF))
               (high-byte (ash codepoint -8)))
           (bytevector low-byte high-byte)
         ) ;let
        ) ;

        (else
         ;; 辅助平面字符 - 代理对
         (let* ((codepoint-prime (- codepoint #x10000))
                (high-surrogate (+ #xD800 (ash codepoint-prime -10)))
                (low-surrogate (+ #xDC00 (bitwise-and codepoint-prime #x3FF)))
                (high-surrogate-low (bitwise-and high-surrogate #xFF))
                (high-surrogate-high (ash high-surrogate -8))
                (low-surrogate-low (bitwise-and low-surrogate #xFF))
                (low-surrogate-high (ash low-surrogate -8)))
           (bytevector high-surrogate-low high-surrogate-high
                       low-surrogate-low low-surrogate-high
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
               (first-codepoint (+ (ash first-high 8) first-low)))

          (cond
            ((<= #xD800 first-codepoint #xDBFF)
             ;; 高代理对 - 需要低代理对
             (when (< len 4)
               (error 'value-error "utf16le->codepoint: incomplete surrogate pair")
             ) ;when

             (let* ((second-low (bytevector-u8-ref bv 2))
                    (second-high (bytevector-u8-ref bv 3))
                    (second-codepoint (+ (ash second-high 8) second-low)))

               (unless (<= #xDC00 second-codepoint #xDFFF)
                 (error 'value-error "utf16le->codepoint: invalid low surrogate")
               ) ;unless

               (let ((codepoint-prime (+ (ash (- first-codepoint #xD800) 10)
                                         (- second-codepoint #xDC00))))
                 (+ codepoint-prime #x10000)
               ) ;let
             ) ;let*
            ) ;

            ((<= #xDC00 first-codepoint #xDFFF)
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
            (let loop ((index 0)
                       (result (bytevector)))
              (if (>= index len)
                  result
                  (let ((next-index (bytevector-advance-utf8 bv index len)))
                    (if (= next-index index)
                        (error 'value-error "utf8->utf16le: invalid UTF-8 sequence at index" index)
                        (let* ((utf8-bytes (bytevector-copy bv index next-index))
                               (codepoint (utf8->codepoint utf8-bytes))
                               (utf16le-bytes (codepoint->utf16le codepoint)))
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
        (error 'type-error "utf16le->utf8: expected bytevector")
      ) ;unless

      (let ((len (bytevector-length bv)))
        (if (= len 0)
            (bytevector)
            (let loop ((index 0)
                       (result (bytevector)))
              (if (>= index len)
                  result
                  (let ((next-index (bytevector-utf16le-advance bv index len)))
                    (if (= next-index index)
                        (error 'value-error "utf16le->utf8: invalid UTF-16LE sequence at index" index)
                        (let* ((utf16le-bytes (bytevector-copy bv index next-index))
                               (codepoint (utf16le->codepoint utf16le-bytes))
                               (utf8-bytes (codepoint->utf8 codepoint)))
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

    (define* (bytevector-utf16le-advance bv index (end (bytevector-length bv)))
      (unless (bytevector? bv)
        (error 'type-error "bytevector-utf16le-advance: expected bytevector")
      ) ;unless

      (if (>= index end)
          index  ; 已经到达结束位置
          (let ((remaining (- end index)))
            (if (< remaining 2)
                index  ; 字节不足，无法构成完整的 UTF-16LE 字符
                (let* ((low-byte (bytevector-u8-ref bv index))
                       (high-byte (bytevector-u8-ref bv (+ index 1)))
                       (codepoint (+ (ash high-byte 8) low-byte)))

                  (cond
                   ;; 基本多文种平面字符 (非代理对)
                   ((or (< codepoint #xD800) (> codepoint #xDFFF))
                    (+ index 2)
                   ) ;

                   ;; 高代理对 (需要低代理对)
                   ((<= #xD800 codepoint #xDBFF)
                    (if (< remaining 4)
                        index  ; 字节不足，无法构成完整的代理对
                        (let* ((second-low (bytevector-u8-ref bv (+ index 2)))
                               (second-high (bytevector-u8-ref bv (+ index 3)))
                               (second-codepoint (+ (ash second-high 8) second-low)))
                          (if (<= #xDC00 second-codepoint #xDFFF)
                              (+ index 4)  ; 有效的代理对
                              index    ; 无效的低代理对
                          ) ;if
                        ) ;let*
                    ) ;if
                   ) ;

                   ;; 低代理对作为第一个码元 - 无效
                   ((<= #xDC00 codepoint #xDFFF)
                    index
                   ) ;

                   (else
                    index
                   ) ;else
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
            (let loop ((index 0)
                       (result (bytevector)))
              (if (>= index len)
                  result
                  (let ((next-index (bytevector-advance-utf8 bv index len)))
                    (if (= next-index index)
                        (error 'value-error "utf8->utf16be: invalid UTF-8 sequence at index" index)
                        (let* ((utf8-bytes (bytevector-copy bv index next-index))
                               (codepoint (utf8->codepoint utf8-bytes))
                               (utf16be-bytes (codepoint->utf16be codepoint)))
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

    (define* (bytevector-utf16be-advance bv index (end (bytevector-length bv)))
      (unless (bytevector? bv)
        (error 'type-error "bytevector-utf16be-advance: expected bytevector")
      ) ;unless

      (if (>= index end)
          index  ; 已经到达结束位置
          (let ((remaining (- end index)))
            (if (< remaining 2)
                index  ; 字节不足，无法构成完整的 UTF-16BE 字符
                (let* ((high-byte (bytevector-u8-ref bv index))
                       (low-byte (bytevector-u8-ref bv (+ index 1)))
                       (codepoint (+ (ash high-byte 8) low-byte)))

                  (cond
                   ;; 基本多文种平面字符 (非代理对)
                   ((or (< codepoint #xD800) (> codepoint #xDFFF))
                    (+ index 2)
                   ) ;

                   ;; 高代理对 (需要低代理对)
                   ((<= #xD800 codepoint #xDBFF)
                    (if (< remaining 4)
                        index  ; 字节不足，无法构成完整的代理对
                        (let* ((second-high (bytevector-u8-ref bv (+ index 2)))
                               (second-low (bytevector-u8-ref bv (+ index 3)))
                               (second-codepoint (+ (ash second-high 8) second-low)))
                          (if (<= #xDC00 second-codepoint #xDFFF)
                              (+ index 4)  ; 有效的代理对
                              index    ; 无效的低代理对
                          ) ;if
                        ) ;let*
                    ) ;if
                   ) ;

                   ;; 低代理对作为第一个码元 - 无效
                   ((<= #xDC00 codepoint #xDFFF)
                    index
                   ) ;

                   (else
                    index
                   ) ;else
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
            (let loop ((index 0)
                       (result (bytevector)))
              (if (>= index len)
                  result
                  (let ((next-index (bytevector-utf16be-advance bv index len)))
                    (if (= next-index index)
                        (error 'value-error "utf16be->utf8: invalid UTF-16BE sequence at index" index)
                        (let* ((utf16be-bytes (bytevector-copy bv index next-index))
                               (codepoint (utf16be->codepoint utf16be-bytes))
                               (utf8-bytes (codepoint->utf8 codepoint)))
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