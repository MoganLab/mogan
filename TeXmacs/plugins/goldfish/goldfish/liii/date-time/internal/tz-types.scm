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

(define-library (liii date-time internal tz-types)
  (import (scheme base))
  (export tz-offset tz-offset? tz-offset-sign-negative? tz-offset-hour
    tz-offset-minute tz-offset-second tz-month-week-day tz-month-week-day?
    tz-month-week-day:month tz-month-week-day:week tz-month-week-day:day
    tz-julian-without-leap-day tz-julian-without-leap-day?
    tz-julian-without-leap-day:date tz-julian-with-leap-day
    tz-julian-with-leap-day? tz-julian-with-leap-day:date tz-transition
    tz-transition? tz-transition-date tz-transition-time tz-string tz-string?
    tz-string-stdname tz-string-stdoffset tz-string-dstname tz-string-dstoffset
    tz-string-change-to-dst tz-string-change-to-std
  ) ;export
  (begin

    (define-record-type <tz-offset>
      (tz-offset sign? hour minute second)
      tz-offset?
      (sign? tz-offset-sign-negative?)
      (hour tz-offset-hour)
      (minute tz-offset-minute)
      (second tz-offset-second)
    ) ;define-record-type

    (define-record-type <tz-month-week-day>
      (tz-month-week-day month week day)
      tz-month-week-day?
      (month tz-month-week-day:month)
      (week tz-month-week-day:week)
      (day tz-month-week-day:day)
    ) ;define-record-type

    (define-record-type <tz-julian-without-leap-day>
      (tz-julian-without-leap-day date)
      tz-julian-without-leap-day?
      (date tz-julian-without-leap-day:date)
    ) ;define-record-type

    (define-record-type <tz-julian-with-leap-day>
      (tz-julian-with-leap-day date)
      tz-julian-with-leap-day?
      (date tz-julian-with-leap-day:date)
    ) ;define-record-type

    (define-record-type <tz-transition>
      (tz-transition date time)
      tz-transition?
      (date tz-transition-date)
      (time tz-transition-time)
    ) ;define-record-type

    (define-record-type <tz-string>
      (tz-string stdname stdoffset dstname dstoffset change-to-dst change-to-std)
      tz-string?
      (stdname tz-string-stdname)
      (stdoffset tz-string-stdoffset)
      (dstname tz-string-dstname)
      (dstoffset tz-string-dstoffset)
      (change-to-dst tz-string-change-to-dst)
      (change-to-std tz-string-change-to-std)
    ) ;define-record-type

  ) ;begin
) ;define-library
