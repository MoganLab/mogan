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

(define-library (liii rich-string)
  (import (liii string)
          (liii oop)
          (liii base)
          (liii rich-char)
          (liii rich-vector)
          (liii vector)
          (rename (liii rich-option) (rich-option option))
  ) ;import
  (export rich-string)
  (begin

    (define-case-class rich-string
      ((data string?))
  
      (define N (utf8-string-length data))

      (define (@empty . args)
        (chain-apply args (rich-string ""))
      ) ;define

      (define (@value-of v . args)
        (chain-apply args
          (cond ((char? v) (rich-string (string v)))
                ((number? v) (rich-string (number->string v)))
                ((symbol? v) (rich-string (symbol->string v)))
                ((string? v) (rich-string v))
                ((rich-char :is-type-of v)
                 (rich-string (v :make-string))
                ) ;
                (else (type-error "Expected types are char, rich-char, number, symbol or string"))
          ) ;cond
        ) ;chain-apply
      ) ;define

      (define (%get) data)

      (define (%length)
        N
      ) ;define

      (define (%char-at index)
        (when (not (integer? index))
          (type-error "rich-string%char-at: index must be integer" index)
        ) ;when

        (let* ((start index)
               (end (+ index 1))
               (byte-seq (string->utf8 data start end)))
          (rich-char :from-bytevector byte-seq)
        ) ;let*
      ) ;define

      (define (%apply i)
        (%char-at i)
      ) ;define

      (define (%find pred) ((%to-rich-vector) :find pred))

      (define (%find-last pred) ((%to-rich-vector) :find-last pred))

      (define (%head)
        (if (string-null? data)
            (index-error "rich-string%head: string is empty")
            (%char-at 0)
        ) ;if
      ) ;define

      (define (%head-option)
        (if (string-null? data)
            (none)
            (option (%char-at 0))
        ) ;if
      ) ;define

      (define (%last)
        (if (string-null? data)
            (index-error "rich-string%last: string is empty")
            (%char-at (- N 1))
        ) ;if
      ) ;define

      (define (%last-option)
        (if (string-null? data)
            (none)
            (option (%char-at (- N 1)))
        ) ;if
      ) ;define

      (define (%slice from until . args)
        (chain-apply args
          (let* ((start (max 0 from))
                 (end (min N until)))
            (cond ((and (zero? start) (= end N))
                   (%this))
                  ((>= start end)
                   (rich-string :empty)
                  ) ;
                  (else
                   (rich-string (u8-substring data start end))
                  ) ;else
            ) ;cond
          ) ;let*
        ) ;chain-apply
      ) ;define

      (define (%take n . args)
        (chain-apply args
          (%slice 0 n)
        ) ;chain-apply
      ) ;define

      (define (%take-right n . args)
        (chain-apply args
          (%slice (- N n) N)
        ) ;chain-apply
      ) ;define

      (define (%drop n . args)
        (chain-apply args
          (%slice n N)
        ) ;chain-apply
      ) ;define

      (define (%drop-right n . args)
        (chain-apply args
          (%slice 0 (- N n))
        ) ;chain-apply
      ) ;define

      (define (%empty?)
        (string-null? data)
      ) ;define

      (define (%starts-with prefix)
        (string-starts? data prefix)
      ) ;define

      (define (%ends-with suffix)
        (string-ends? data suffix)
      ) ;define

      (define (%forall pred)
        ((%to-rich-vector) :forall pred)
      ) ;define

      (define (%exists pred)
        ((%to-rich-vector) :exists pred)
      ) ;define

      (define (%contains elem)
        (cond ((rich-string :is-type-of elem)
               (string-contains data (elem :get)))
    
              ((string? elem)
               (string-contains data elem)
              ) ;
        
              ((rich-char :is-type-of elem)
               (string-contains data (elem :make-string))
              ) ;
        
              ((char? elem)
               (string-contains data (string elem))
              ) ;
        
              (else (type-error "elem must be char or string"))
        ) ;cond
      ) ;define

      (define* (%index-of str/char (start-index 0))
        (define (bytes-match? data-bv data-pos str-bv str-size data-size)
          (let loop ((i 0))
            (if (>= i str-size)
                #t
                (and (< (+ data-pos i) data-size)
                     (= (bytevector-u8-ref data-bv (+ data-pos i))
                        (bytevector-u8-ref str-bv i)
                     ) ;=
                     (loop (+ i 1))
                ) ;and
            ) ;if
          ) ;let
        ) ;define

        (define (char-index->byte-pos bv size char-index)
          (let loop ((i 0) (pos 0))
            (if (>= i char-index)
                pos
                (loop (+ i 1) (bytevector-advance-utf8 bv pos size))
            ) ;if
          ) ;let
        ) ;define
  
        (define* (inner-index-of str start-index)
          (if (or (string-null? data) (string-null? str))
              -1
              (let* ((data-bv (string->utf8 data))
                     (str-bv (string->utf8 str))
                     (data-size (bytevector-length data-bv))
                     (str-size (bytevector-length str-bv)))
                (if (or (negative? start-index)
                        (>= start-index N))
                    -1
                    (let ((start-byte-pos (char-index->byte-pos data-bv data-size start-index)))
                      (let search ((byte-pos start-byte-pos) (current-char-index start-index))
                        (cond
                          ((> (+ byte-pos str-size) data-size) -1)
                          ((bytes-match? data-bv byte-pos str-bv str-size data-size)
                           current-char-index
                          ) ;
                          (else
                           (search (bytevector-advance-utf8 data-bv byte-pos data-size)
                                   (+ current-char-index 1)
                           ) ;search
                          ) ;else
                        ) ;cond
                      ) ;let
                    ) ;let
                ) ;if
              ) ;let*
          ) ;if
        ) ;define*

        (unless (integer? start-index)
          (type-error "rich-string%index-of: the second parameter must be integer")
        ) ;unless
  
        (let ((positive-start-index (max 0 start-index)))
          (cond ((string? str/char)
                 (inner-index-of str/char positive-start-index))
                ((rich-string :is-type-of str/char)
                 (inner-index-of (str/char :get) positive-start-index)
                ) ;
                ((char? str/char)
                 (inner-index-of (string str/char) positive-start-index)
                ) ;
                ((rich-char :is-type-of str/char)
                 (inner-index-of (str/char :make-string) positive-start-index)
                ) ;
                (else (type-error "rich-string%index-of: first parameter must be string/rich-string/char/rich-char"))
          ) ;cond
        ) ;let
      ) ;define*

      (define (%map f . args)
        (chain-apply args
          (rich-string ((%to-rich-vector)
                        :map f
                        :map (lambda (x) (x :make-string))
                        :make-string)
          ) ;rich-string
        ) ;chain-apply
      ) ;define

      (define (%filter pred . args)
        (chain-apply args
          (rich-string ((%to-rich-vector)
                        :filter pred
                        :map (lambda (x) (x :make-string))
                        :make-string)
          ) ;rich-string
        ) ;chain-apply
      ) ;define

      (define (%reverse . args)
        (chain-apply args
          (rich-string ((%to-rich-vector)
                        :reverse
                        :map (lambda (x) (x :make-string))
                        :make-string)
          ) ;rich-string
        ) ;chain-apply
      ) ;define

      (define (%for-each f)
        ((%to-rich-vector) :for-each f)
      ) ;define

      (define (%count pred?)
        ((%to-rich-vector) :count pred?)
      ) ;define

      (define (%index-where pred)
        (let ((bytes (string->utf8 data))
              (len (bytevector-length (string->utf8 data))))
          (let loop ((byte-pos 0) (char-index 0))
            (cond
              ((>= byte-pos len) -1)
              (else
               (let* ((next-pos (bytevector-advance-utf8 bytes byte-pos len))
                      (char-bytes (bytevector-copy bytes byte-pos next-pos))
                      (char (rich-char :from-bytevector char-bytes)))
                 (if (pred char)
                     char-index
                     (loop next-pos (+ char-index 1))
                 ) ;if
               ) ;let*
              ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;define

      (define (%take-while pred . args)
        (chain-apply args
          (let ((stop-index (%index-where (lambda (c) (not (pred c))))))
            (if (= stop-index -1)
                (%this)
                (%slice 0 stop-index)
            ) ;if
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%drop-while pred . args)
        (chain-apply args
          (let ((index (%index-where (lambda (c) (not (pred c))))))
            (if (= index -1)
                (rich-string "")
                (%slice index N)
            ) ;if
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%to-string)
        data
      ) ;define

      (define (%to-vector)
        (if (string-null? data)
            (vector)
            (let* ((bv (string->utf8 data))
                   (bv-size (length bv))
                   (result (make-vector N)))
              (let loop ((i 0) (j 0))
                (if (>= i N)
                    result
                    (let* ((next-j (bytevector-advance-utf8 bv j bv-size))
                           (ch (rich-char :from-bytevector (bytevector-copy bv j next-j))))
                      (vector-set! result i ch)
                      (loop (+ i 1) next-j)
                    ) ;let*
                ) ;if
              ) ;let
            ) ;let*
        ) ;if
      ) ;define

      (define (%to-rich-vector)
        (rich-vector (%to-vector))
      ) ;define

      (define (%+ s . args)
        (chain-apply args
          (cond
            ((string? s)
             (rich-string (string-append data s))
            ) ;
            ((rich-string :is-type-of s)
             (rich-string (string-append data (s :get)))
            ) ;
            ((number? s)
             (rich-string (string-append data (number->string s)))
            ) ;
            (else
              (type-error (string-append (object->string s) "is not string or rich-string or number"))
            ) ;else
          ) ;cond
        ) ;chain-apply
      ) ;define

      (define (%strip-left . args)
        (chain-apply args
          (rich-string (string-trim data))
        ) ;chain-apply
      ) ;define

      (define (%strip-right . args)
        (chain-apply args
          (rich-string (string-trim-right data))
        ) ;chain-apply
      ) ;define

      (define (%strip-both . args)
        (chain-apply args
          (rich-string (string-trim-both data))
        ) ;chain-apply
      ) ;define

      (define (%strip-prefix prefix . args)
        (chain-apply args
          (rich-string (string-remove-prefix data prefix))
        ) ;chain-apply
      ) ;define

      (define (%strip-suffix suffix . args)
        (chain-apply args
          (rich-string (string-remove-suffix data suffix))
        ) ;chain-apply
      ) ;define

      (define (%replace-first old new . args)
        (chain-apply args
          (let ((next-pos (%index-of old)))
            (if (= next-pos -1)
                (%this)
                ((%slice 0 next-pos)
                 :+ new
                 :+ (%drop (+ next-pos ($ old :length)))
                ) ;
            ) ;if
          ) ;let
        ) ;chain-apply
      ) ;define

      (define (%replace old new . args)
        (define (replace-helper str old new start)
          (let ((next-pos ((rich-string str) :index-of old start)))
            (if (= next-pos -1)
                str
                (replace-helper ((rich-string str) :replace-first old new :get)
                                old new next-pos
                ) ;replace-helper
            ) ;if
          ) ;let
        ) ;define
        (chain-apply args
          (rich-string (replace-helper data old new 0))
        ) ;chain-apply
      ) ;define

      (define* (%pad-left len ch . args)
        (let ((result (if (<= len N)
                          (%this)
                          (let ((padding (make-string (- len N) ch)))
                            (rich-string (string-append padding data)))))
                          ) ;let
          (if (null? args)
              result
              (apply result args)
          ) ;if
        ) ;let
      ) ;define*

      (define* (%pad-right len ch . args)
        (let ((result (if (<= len N)
                          (%this)
                          (let ((padding (make-string (- len N) ch)))
                            (rich-string (string-append data padding)))))
                          ) ;let
          (if (null? args)
              result
              (apply result args)
          ) ;if
        ) ;let
      ) ;define*

      (define (%split sep)
        (let ((str-len N)
              (sep-len (utf8-string-length sep)))
    
          (define (split-helper start acc)
            (let ((next-pos (%index-of sep start)))
              (if (= next-pos -1)
                  (cons (%drop start :get) acc)
                  (split-helper (+ next-pos sep-len) (cons (%slice start next-pos :get) acc))
              ) ;if
            ) ;let
          ) ;define
    
          (if (zero? sep-len)
              ((%to-rich-vector) :map (lambda (c) (c :make-string)))
              (rich-vector (reverse-list->vector (split-helper 0 '()))))
          ) ;if
        ) ;let

      ) ;define

    ) ;define-case-class
  ) ;begin
