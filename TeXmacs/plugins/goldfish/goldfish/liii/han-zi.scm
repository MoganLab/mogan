;;
;; Copyright (C) 2024 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

(define-library (liii han-zi)
  (export han-zi->number number->han-zi han-zi-number?)
  (begin

    (define (han-zi->number ch)
      (unless (char? ch)
        (error 'type-error "han-zi->number: parameter must be character")
      ) ;unless
      (case ch
       ((#\零 #\〇) 0)
       ((#\一 #\壹) 1)
       ((#\二 #\贰) 2)
       ((#\三 #\叁) 3)
       ((#\四 #\肆) 4)
       ((#\五 #\伍) 5)
       ((#\六 #\陆) 6)
       ((#\七 #\柒) 7)
       ((#\八 #\捌) 8)
       ((#\九 #\玖) 9)
       ((#\十 #\拾) 10)
       ((#\百 #\佰) 100)
       ((#\千 #\仟) 1000)
       ((#\万) 10000)
       ((#\亿) 100000000)
       (else #f)
      ) ;case
    ) ;define

    (define (number->han-zi n style)
      (unless (integer? n)
        (error 'type-error "number->han-zi: parameter must be integer")
      ) ;unless
      (cond ((< n 0) #f)
            ((= n 0) (case style ((common financial) #\零) ((year) #\〇) (else #f)))
            ((= n 1) (case style ((common year) #\一) ((financial) #\壹) (else #f)))
            ((= n 2) (case style ((common year) #\二) ((financial) #\贰) (else #f)))
            ((= n 3) (case style ((common year) #\三) ((financial) #\叁) (else #f)))
            ((= n 4) (case style ((common year) #\四) ((financial) #\肆) (else #f)))
            ((= n 5) (case style ((common year) #\五) ((financial) #\伍) (else #f)))
            ((= n 6) (case style ((common year) #\六) ((financial) #\陆) (else #f)))
            ((= n 7) (case style ((common year) #\七) ((financial) #\柒) (else #f)))
            ((= n 8) (case style ((common year) #\八) ((financial) #\捌) (else #f)))
            ((= n 9) (case style ((common year) #\九) ((financial) #\玖) (else #f)))
            ((= n 10) (case style ((common) #\十) ((financial) #\拾) (else #f)))
            ((= n 100) (case style ((common) #\百) ((financial) #\佰) (else #f)))
            ((= n 1000) (case style ((common) #\千) ((financial) #\仟) (else #f)))
            ((= n 10000) (case style ((common financial) #\万) (else #f)))
            ((= n 100000000) (case style ((common financial) #\亿) (else #f)))
            (else #f)
      ) ;cond
    ) ;define

    (define (han-zi-number? ch)
      (if (char? ch) (not (not (han-zi->number ch))) #f)
    ) ;define

  ) ;begin
) ;define-library
