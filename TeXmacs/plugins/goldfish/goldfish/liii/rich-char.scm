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

(define-library (liii rich-char)
  (import (liii oop) (liii bitwise) (liii base) (liii string) (liii unicode))
  (export rich-char)
  (begin

    (define-case-class rich-char ((data any?))
  
      (define code-point
        (cond ((char? data)
               (char->integer data))
              ((integer? data)
               (if (and (>= data 0) (<= data #x10FFFF))
                   data
                   (value-error "rich-char: code point out of range" data)
               ) ;if
              ) ;
              (else
               (type-error "rich-char: only accept char and integer")
              ) ;else
        ) ;cond
      ) ;define

      (define (%equals that)
        (cond ((char? that)
               (= code-point (char->integer that)))
              ((rich-char :is-type-of that)
               (= code-point (that :to-integer))
              ) ;
              (else #f)
        ) ;cond
      ) ;define

      (define (%ascii?)
        (and (>= code-point 0) (<= code-point 127))
      ) ;define


      (define (%numeric?)
        (if (and (>= code-point 0) (<= code-point 255))
            (let ((ch (integer->char code-point)))
              (char-numeric? ch)
            ) ;let
            #f
        ) ;if
      ) ;define

      (define (%upper?)
        (and (>= code-point #x41) (<= code-point #x5A))
      ) ;define

      (define (%lower?)
        (and (>= code-point #x61) (<= code-point #x7A))
      ) ;define

      (define (%digit?)
        (or
         (and (>= code-point 48) (<= code-point 57))
         (and (>= code-point #xFF10) (<= code-point #xFF19))
         (and (>= code-point #x0660) (<= code-point #x0669))
         (and (>= code-point #x06F0) (<= code-point #x06F9))
         (and (>= code-point #x0966) (<= code-point #x096F))
         (and (>= code-point #x09E6) (<= code-point #x09EF))
         (and (>= code-point #x0A66) (<= code-point #x0A6F))
         (and (>= code-point #x0AE6) (<= code-point #x0AEF))
         (and (>= code-point #x0B66) (<= code-point #x0B6F))
         (and (>= code-point #x0BE6) (<= code-point #x0BEF))
         (and (>= code-point #x0C66) (<= code-point #x0C6F))
         (and (>= code-point #x0CE6) (<= code-point #x0CEF))
         (and (>= code-point #x0D66) (<= code-point #x0D6F))
         (and (>= code-point #x0E50) (<= code-point #x0E59))
         (and (>= code-point #x0ED0) (<= code-point #x0ED9))
         (and (>= code-point #x0F20) (<= code-point #x0F29))
         (and (>= code-point #x1040) (<= code-point #x1049))
         (and (>= code-point #x17E0) (<= code-point #x17E9))
         (and (>= code-point #x1810) (<= code-point #x1819))
        ) ;or
      ) ;define
  
      (define (%to-upper . args)
        (chain-apply args
          (rich-char
            (if (and (>= code-point #x61) (<= code-point #x7A))
                (bitwise-and code-point #b11011111)
                code-point
            ) ;if
          ) ;rich-char
        ) ;chain-apply
      ) ;define

      (define (%to-lower . args)
        (chain-apply args
          (rich-char
            (if (and (>= code-point #x41) (<= code-point #x5A))
                (bitwise-ior code-point #b00100000)
                code-point
            ) ;if
          ) ;rich-char
        ) ;chain-apply
      ) ;define

      (define (%to-bytevector)
        (codepoint->utf8 code-point)
      ) ;define

      (define (@from-bytevector x)
        (rich-char (utf8->codepoint x))
      ) ;define

      (define (%to-string)
        (if (%ascii?)
            (object->string (integer->char code-point))
            (string-append "#\\" (utf8->string (%to-bytevector)))
        ) ;if
      ) ;define

      (define (@from-string x)
        (when (not (string-starts? x "#\\"))
          (value-error "char@from-string: the input must start with #\\")
        ) ;when
        (if (= 1 ($ x :drop 2 :length))
            (rich-char :from-bytevector (string->utf8 ($ x :drop 2 :get)))
            (value-error "rich-char: must be u8 string which length equals 1")
        ) ;if
      ) ;define

      (define (%make-string)
        (utf8->string (%to-bytevector))
      ) ;define

      (define (@from-integer x . args)
        (chain-apply args
          (rich-char x)
        ) ;chain-apply
      ) ;define

      (define (%to-integer)
        code-point)

      ) ;define

    ) ;define-case-class
  ) ;begin