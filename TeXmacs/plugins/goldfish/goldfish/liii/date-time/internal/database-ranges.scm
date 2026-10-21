;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

;; Copyright (C) 2026 Peter McGoron
;;
;; Permission is hereby granted, free of charge, to any person obtaining
;; a copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
;; LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
;; OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
;; WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;;

(define-library (liii date-time internal database-ranges)
  (import (scheme base)
    (liii date-time time-delta)
    (liii date-time internal util)
    (liii date-time internal tz)
    (liii date-time internal tzif)
    (liii date-time internal dst)
    (liii date-time internal database-types)
  ) ;import
  (export build-ranges find-offset/utc find-offset/wall)
  (begin

    (define (seconds->time-delta s)
      (let-values (((m sec) (floor/ s 60)))
        (let-values (((h min) (floor/ m 60)))
          (time-delta+ (hours-delta h) (minutes-delta min) (seconds-delta sec))
        ) ;let-values
      ) ;let-values
    ) ;define

    (define (build-empty-ranges first-trans)
      (let* ((off-sec (if first-trans (tzif-transition-offset first-trans) 0))
             (off (seconds->time-delta off-sec))
             (dst (if first-trans (tzif-transition-dst? first-trans) #f))
             (des (if first-trans (tzif-transition-designation first-trans) "UTC"))
             (r (vector (time-range #f #f off dst des)))
            ) ;
        (values r r)
      ) ;let*
    ) ;define

    (define (fill-intermediate-ranges utc-v loc-v transitions len)
      (do ((i 0 (+ i 1)))
        ((= i (- len 1)))
        (let* ((curr (vector-ref transitions i))
               (nxt (vector-ref transitions (+ i 1)))
               (t1 (tzif-transition-time curr))
               (t2 (tzif-transition-time nxt))
               (off-sec (tzif-transition-offset curr))
               (off (seconds->time-delta off-sec))
               (dst (tzif-transition-dst? curr))
               (des (tzif-transition-designation curr))
              ) ;
          (vector-set! utc-v (+ i 1) (time-range t1 t2 off dst des))
          (vector-set! loc-v
            (+ i 1)
            (time-range (+ t1 off-sec) (+ t2 off-sec) off dst des)
          ) ;vector-set!
        ) ;let*
      ) ;do
    ) ;define

    (define (build-ranges first-trans transitions)
      (let ((len (vector-length transitions)))
        (if (zero? len)
          (build-empty-ranges first-trans)
          (let* ((utc-v (make-vector (+ len 1)))
                 (loc-v (make-vector (+ len 1)))
                 (first-off-sec (if first-trans (tzif-transition-offset first-trans) 0))
                 (first-off (seconds->time-delta first-off-sec))
                 (first-dst (if first-trans (tzif-transition-dst? first-trans) #f))
                 (first-des (if first-trans (tzif-transition-designation first-trans) "UTC"))
                 (first-t (tzif-transition-time (vector-ref transitions 0)))
                ) ;
            (vector-set! utc-v 0 (time-range #f first-t first-off first-dst first-des))
            (vector-set! loc-v
              0
              (time-range #f (+ first-t first-off-sec) first-off first-dst first-des)
            ) ;vector-set!
            (fill-intermediate-ranges utc-v loc-v transitions len)
            (let* ((last-idx (- len 1))
                   (curr (vector-ref transitions last-idx))
                   (t1 (tzif-transition-time curr))
                   (off-sec (tzif-transition-offset curr))
                   (off (seconds->time-delta off-sec))
                   (dst (tzif-transition-dst? curr))
                   (des (tzif-transition-designation curr))
                  ) ;
              (vector-set! utc-v (+ last-idx 1) (time-range t1 #f off dst des))
              (vector-set! loc-v (+ last-idx 1) (time-range (+ t1 off-sec) #f off dst des))
            ) ;let*
            (values utc-v loc-v)
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (search-ranges ranges time)
      (let loop
        ((l 0) (r (- (vector-length ranges) 1)))
        (if (< r l)
          #f
          (let* ((mid (+ l (floor-quotient (- r l) 2)))
                 (entry (vector-ref ranges mid))
                 (st (time-range-start entry))
                 (en (time-range-end entry))
                ) ;
            (cond
             ((and (or (not st) (<= st time)) (or (not en) (< time en))) mid)
             ((and st (< time st)) (loop l (- mid 1)))
             (else (loop (+ mid 1) r))
            ) ;cond
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (calc-rule-offset-utc rule unix-time)
      (let* ((std-offset (tzrule-std-offset rule)) (std-desig (tzrule-std-name rule)))
        (if (not (tzrule-dst-transition rule))
          (time-range #f #f std-offset #f std-desig)
          (let* ((dst-offset (tzrule-dst-offset rule))
                 (std-trans (tzrule-std-transition rule))
                 (dst-trans (tzrule-dst-transition rule))
                 (dst-desig (tzrule-dst-name rule))
                 (dst->std (lambda (y . opt) (transition-unix dst-offset y std-trans)))
                 (std->dst (lambda (y . opt) (transition-unix std-offset y dst-trans)))
                ) ;
            (let-values (((year subindex)
                          (find-an-offset-rule unix-time dst->std std->dst dst-offset std-offset)
                         ) ;
                        ) ;
              (let ((v (offset-rule time-range unix-time year subindex dst->std
                         std->dst dst-offset std-offset dst-desig std-desig
                       ) ;offset-rule
                    ) ;v
                   ) ;
                (if (> (vector-length v) 0)
                  (vector-ref v 0)
                  (time-range #f #f std-offset #f std-desig)
                ) ;if
              ) ;let
            ) ;let-values
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

    (define (find-offset/utc tz unix-time)
      (let* ((ranges (timezone-utc-ranges tz)) (i (search-ranges ranges unix-time)))
        (cond ((not i) (vector-ref ranges 0))
              ((and (= i (- (vector-length ranges) 1)) (timezone-rule tz))
               (calc-rule-offset-utc (timezone-rule tz) unix-time)
              ) ;
              (else (vector-ref ranges i))
        ) ;cond
      ) ;let*
    ) ;define

    (define (calc-rule-offset-wall rule local-time)
      (let* ((std-offset (tzrule-std-offset rule)) (std-desig (tzrule-std-name rule)))
        (if (not (tzrule-dst-transition rule))
          (vector (time-range #f #f std-offset #f std-desig))
          (let* ((dst-offset (tzrule-dst-offset rule))
                 (std-trans (tzrule-std-transition rule))
                 (dst-trans (tzrule-dst-transition rule))
                 (dst-desig (tzrule-dst-name rule))
                 (dst->std (lambda (y off) (add-offset (transition-unix dst-offset y std-trans) off))
                 ) ;dst->std
                 (std->dst (lambda (y off) (add-offset (transition-unix std-offset y dst-trans) off))
                 ) ;std->dst
                ) ;
            (let-values (((year subindex)
                          (find-an-offset-rule local-time dst->std std->dst dst-offset std-offset)
                         ) ;
                        ) ;
              (let ((is-in (lambda (subidx)
                             (offset-rule time-range local-time year subidx
                               dst->std std->dst dst-offset std-offset dst-desig
                               std-desig
                             ) ;offset-rule
                           ) ;lambda
                    ) ;is-in
                   ) ;
                (vector-append (is-in (- subindex 1)) (is-in subindex) (is-in (+ subindex 1)))
              ) ;let
            ) ;let-values
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

    (define (range-matches-wall? entry local-time)
      (let ((st (time-range-start entry)) (en (time-range-end entry)))
        (and (or (not st) (<= st local-time)) (or (not en) (< local-time en)))
      ) ;let
    ) ;define

    (define (find-offset/wall tz local-time)
      (let* ((ranges (timezone-local-ranges tz)) (i (search-ranges ranges local-time)))
        (cond
         ((and i (= i (- (vector-length ranges) 1)) (timezone-rule tz))
          (calc-rule-offset-wall (timezone-rule tz) local-time)
         ) ;
         ((not i) #())
         (else
           (let* ((r-len (vector-length ranges))
                  (check
                    (lambda (idx)
                      (if (and (>= idx 0) (< idx r-len))
                        (let ((e (vector-ref ranges idx)))
                          (if (range-matches-wall? e local-time) (vector e) #())
                        ) ;let
                        #()
                      ) ;if
                    ) ;lambda
                  ) ;check
                 ) ;
             (vector-append (check (- i 1)) (check i) (check (+ i 1)))
           ) ;let*
         ) ;else
        ) ;cond
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
