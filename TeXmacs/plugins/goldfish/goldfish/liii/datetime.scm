;
; Copyright (C) 2024-2025 The Goldfish Scheme Authors
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

(define-library (liii datetime)
  (import (liii oop) (liii error) (liii string))
  (export datetime date years days)
  (begin

    (define-object years
      (define (@leap? year)
        (when (not (integer? year))
          (type-error "years@leap? must accept integer")
        ) ;when
        (or (and (zero? (modulo year 4)) 
                 (not (zero? (modulo year 100))))
            (zero? (modulo year 400)))
        ) ;or

 ;define
    ) ;define-object

    (define (weekday-for-date year month day)
      (let* ((m (if (> month 2) month (+ month 12)))
             (y (if (> month 2) year (- year 1)))
             (century (quotient y 100))
             (year-of-century (remainder y 100)))
        (modulo
         (- (+ day
               (quotient (+ (* 13 (- m 2)) 13) 5)
               year-of-century
               (quotient year-of-century 4)
               (quotient century 4)
               (* 5 century) 6)
            3
         ) ;-
         7
        ) ;modulo
      ) ;let*
    ) ;define

    (define-case-class datetime
      ((year integer?)
       (month integer?)
       (day integer?)
       (hour integer? 0)
       (minute integer? 0)
       (second integer? 0)
       (micro-second integer? 0)
      ) ;

      (chained-define (@now)
        (let ((time-vec (g_datetime-now)))
          (datetime
            :year (vector-ref time-vec 0)
            :month (vector-ref time-vec 1)
            :day (vector-ref time-vec 2)
            :hour (vector-ref time-vec 3)
            :minute (vector-ref time-vec 4)
            :second (vector-ref time-vec 5)
            :micro-second (vector-ref time-vec 6)
          ) ;datetime
        ) ;let
      ) ;chained-define

      (define (%to-string)
        (define (pad6 n)  ; 补零到 6 位（微秒）
          (let ((s (number->string n)))
            (string-append (make-string (- 6 (string-length s)) #\0) s)
          ) ;let
        ) ;define
        (if (zero? micro-second)
            (%format "yyyy-MM-dd HH:mm:ss")
            (string-append (%format "yyyy-MM-dd HH:mm:ss") "." (pad6 micro-second))
        ) ;if
      ) ;define

      (define (%plus-days days-to-add)
  
        (define (days-in-month m y)
          (cond ((member m '(4 6 9 11)) 30)
                ((= m 2) (if (years :leap? y) 29 28))
                (else 31)
          ) ;cond
        ) ;define
  
        (let loop ((y year)
                   (m month)
                   (d day)
                   (remaining-days days-to-add))
          (cond
            ;; No more days to add
            ((zero? remaining-days)
             (datetime :year y
                       :month m
                       :day d
                       :hour hour
                       :minute minute
                       :second second
                       :micro-second micro-second
             ) ;datetime
            ) ;
      
            ;; Adding days (positive)
            ((> remaining-days 0)
             (let ((days-in-current-month (days-in-month m y)))
               (if (<= (+ d remaining-days) days-in-current-month)
                   ;; Simple case: result is within the same month
                   (loop y m (+ d remaining-days) 0)
                   ;; Complex case: need to move to next month
                   (let ((next-month (if (= m 12) 1 (+ m 1)))
                         (next-year (if (= m 12) (+ y 1) y)))
                     (loop next-year 
                           next-month 
                           1 
                           (- (+ remaining-days d) days-in-current-month 1)
                     ) ;loop
                   ) ;let
               ) ;if
             ) ;let
            ) ;
      
            ;; Subtracting days (negative)
            (else
             (if (> d (abs remaining-days))
                 ;; Simple case: result is within the same month
                 (loop y m (+ d remaining-days) 0)
                 ;; Complex case: need to move to previous month
                 (let* ((prev-month (if (= m 1) 12 (- m 1)))
                        (prev-year (if (= m 1) (- y 1) y))
                        (days-in-prev-month (days-in-month prev-month prev-year)))
                   (loop prev-year 
                         prev-month 
                         days-in-prev-month 
                         (+ remaining-days d)
                   ) ;loop
                 ) ;let*
             ) ;if
            ) ;else
          ) ;cond
        ) ;let
      ) ;define

      (define (%plus-months months-to-add)
        (define (days-in-month m y)
          (cond ((member m '(4 6 9 11)) 30)
                ((= m 2) (if (years :leap? y) 29 28))
                (else 31)
          ) ;cond
        ) ;define
  
        ;; Calculate new year and month
        (let* ((total-months (+ (+ (* year 12) month -1) months-to-add))
               (new-year (quotient total-months 12))
               (new-month (+ (remainder total-months 12) 1))
               ;; Adjust day if necessary
               (days-in-new-month (days-in-month new-month new-year))
               (new-day (min day days-in-new-month)))
    
          (datetime :year new-year
                    :month new-month
                    :day new-day
                    :hour hour
                    :minute minute
                    :second second
                    :micro-second micro-second
          ) ;datetime
        ) ;let*
      ) ;define

      (define (%plus-years years-to-add)
        (define (days-in-month m y)
          (cond ((member m '(4 6 9 11)) 30)
                ((= m 2) (if (years :leap? y) 29 28))
                (else 31)
          ) ;cond
        ) ;define
  
        ;; Calculate new year
        (let* ((new-year (+ year years-to-add))
               ;; Adjust day if necessary (for Feb 29 in leap years)
               (days-in-new-month (days-in-month month new-year))
               (new-day (min day days-in-new-month)))
    
          (datetime :year new-year
                    :month month
                    :day new-day
                    :hour hour
                    :minute minute
                    :second second
                    :micro-second micro-second
          ) ;datetime
        ) ;let*
      ) ;define

      (define (%weekday)
        (weekday-for-date year month day)
      ) ;define

      (define (%format format-str)
        (define (pad2 n)
          (if (< n 10)
              (string-append "0" (number->string n))
              (number->string n)
          ) ;if
        ) ;define
        
        (define (pad3 n)
          (let ((s (number->string (quotient n 1000)))) ; Convert micro-second to milli-second
            (let ((len (string-length s)))
              (cond ((>= len 3) (substring s 0 3))
                    ((= len 2) (string-append "0" s))
                    ((= len 1) (string-append "00" s))
                    (else "000")
              ) ;cond
            ) ;let
          ) ;let
        ) ;define
        
        (define (format-is-valid? format-str)
          (or (string-contains format-str "yyyy")
              (string-contains format-str "MM")
              (string-contains format-str "dd")
              (string-contains format-str "HH")
              (string-contains format-str "mm")
              (string-contains format-str "ss")
              (string-contains format-str "SSS")
          ) ;or
        ) ;define
        
        (unless (format-is-valid? format-str)
          (value-error "datetime%format: invalid format string")
        ) ;unless
        
        (let loop ((result "")
                   (remaining format-str))
          (if (string-null? remaining)
              result
              (cond
                ((string-starts? remaining "yyyy")
                 (loop (string-append result (number->string year))
                       (substring remaining 4 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "MM")
                 (loop (string-append result (pad2 month))
                       (substring remaining 2 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "dd")
                 (loop (string-append result (pad2 day))
                       (substring remaining 2 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "HH")
                 (loop (string-append result (pad2 hour))
                       (substring remaining 2 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "mm")
                 (loop (string-append result (pad2 minute))
                       (substring remaining 2 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "ss")
                 (loop (string-append result (pad2 second))
                       (substring remaining 2 (string-length remaining))
                 ) ;loop
                ) ;
                ((string-starts? remaining "SSS")
                 (loop (string-append result (pad3 micro-second))
                       (substring remaining 3 (string-length remaining))
                 ) ;loop
                ) ;
                (else
                 (loop (string-append result (substring remaining 0 1))
                       (substring remaining 1 (string-length remaining))
                 ) ;loop
                ) ;else
              ) ;cond
          ) ;if
        ) ;let
      ) ;define

      (define (%to-date)
        (date :year year :month month :day day)
      ) ;define

 ;define
    ) ;define-case-class

    (define-case-class date
      ((year integer?)
       (month integer?)
       (day integer?)
      ) ;

      (chained-define (@now)
        (let ((time-vec (g_date-now)))
          (date
            :year (vector-ref time-vec 0)
            :month (vector-ref time-vec 1)
            :day (vector-ref time-vec 2)
          ) ;date
        ) ;let
      ) ;chained-define

      (define (%to-string)
        (%format "yyyy-MM-dd")
      ) ;define

      (define (%format format-str)
        (let ((pad2 (lambda (n)
                      (if (< n 10)
                          (string-append "0" (number->string n))
                          (number->string n))))
                      ) ;if
          
          (unless (or (string-contains format-str "yyyy")
                      (string-contains format-str "MM")
                      (string-contains format-str "dd"))
            (value-error "date%format: invalid format string")
          ) ;unless
          
          (let loop ((result "")
                     (remaining format-str))
            (if (string-null? remaining)
                result
                (cond
                  ((string-starts? remaining "yyyy")
                   (loop (string-append result (number->string year))
                         (substring remaining 4 (string-length remaining))
                   ) ;loop
                  ) ;
                  ((string-starts? remaining "MM")
                   (loop (string-append result (pad2 month))
                         (substring remaining 2 (string-length remaining))
                   ) ;loop
                  ) ;
                  ((string-starts? remaining "dd")
                   (loop (string-append result (pad2 day))
                         (substring remaining 2 (string-length remaining))
                   ) ;loop
                  ) ;
                  (else
                   (loop (string-append result (substring remaining 0 1))
                         (substring remaining 1 (string-length remaining))
                   ) ;loop
                  ) ;else
                ) ;cond
            ) ;if
          ) ;let
        ) ;let
      ) ;define

      (define (%to-datetime)
        (datetime :year year :month month :day day 
                  :hour 0 :minute 0 :second 0 :micro-second 0
        ) ;datetime
      ) ;define

      (define (%weekday)
        (weekday-for-date year month day)
      ) ;define

 ;define
    ) ;define-case-class


    (define-object days
      (define (julian-day-number year month day)
        (let* ((a (quotient (- 14 month) 12))
               (y (+ year 4800 (- a)))
               (m (+ month (* 12 a) (- 3)))
               (jdn (+ day
                       (quotient (+ (* 153 m) 2) 5)
                       (* 365 y)
                       (quotient y 4)
                       (- (quotient y 100))
                       (quotient y 400)
                       (- 32045)))
               ) ;jdn
          jdn
        ) ;let*
      ) ;define

      (define (@between start end)
        (define (extract-date-fields obj)
          (cond 
            ((datetime :is-type-of obj)
             (list (obj 'year) (obj 'month) (obj 'day))
            ) ;
            ((date :is-type-of obj)
             (list (obj 'year) (obj 'month) (obj 'day))
            ) ;
            (else
             (type-error "days@between expects datetime or date objects")
            ) ;else
          ) ;cond
        ) ;define
        
        (let* ((start-fields (extract-date-fields start))
               (end-fields (extract-date-fields end))
               (start-jdn (apply julian-day-number start-fields))
               (end-jdn (apply julian-day-number end-fields))
               (days-difference (- end-jdn start-jdn)))
          days-difference
        ) ;let*
      ) ;define

    ) ;define-object

  ) ;begin
) ;define-library

