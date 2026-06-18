
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : telemetry-utils.scm
;; DESCRIPTION : Telemetry utilities for paths, config, and device info
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (telemetry telemetry-utils))

(import (scheme base)
  (liii base)
  (liii njson)
  (liii os)
  (liii path)
  (liii string)
  (liii uuid)
) ;import

(import (only (srfi srfi-19) current-date date->string date-zone-offset))

(define telemetry-buffer-size 300)

(define telemetry-flush-interval-ms 300000)
(define-public telemetry-max-queue-size 1000)

(define telemetry-meta-max-entries 200)

(define *telemetry-file-seq* 0)

(define-public (telemetry-get-buffer-size) telemetry-buffer-size)

(define-public (telemetry-set-buffer-size! size)
  (set! telemetry-buffer-size size)
) ;define-public

(define-public (telemetry-get-flush-interval) telemetry-flush-interval-ms)

(define-public (telemetry-set-flush-interval! interval)
  (set! telemetry-flush-interval-ms interval)
) ;define-public

(define-public (telemetry-enabled?)
  (if (community-stem?)
    #f
    (let ((pref (get-preference "telemetry")))
      (not (or (== pref "off") (== pref "0")))
    ) ;let
  ) ;if
) ;define-public

(define (telemetry-home-path)
  (url->system (get-texmacs-home-path))
) ;define

(define (telemetry-ensure-dir dir)
  (if (not (path-exists? dir))
    (begin
      (telemetry-ensure-dir (path-parent dir))
      (mkdir dir)
    ) ;begin
  ) ;if
) ;define

(define-public (telemetry-dir)
  (let ((dir (string-append (telemetry-home-path) "/system/telemetry")))
    (telemetry-ensure-dir dir)
    dir
  ) ;let
) ;define-public

(define-public (telemetry-main-dir)
  (let ((dir (string-append (telemetry-dir) "/main")))
    (telemetry-ensure-dir dir)
    dir
  ) ;let
) ;define-public

(define-public (telemetry-meta-path)
  (string-append (telemetry-main-dir) "/main-telemetry.json")
) ;define-public

(define-public (telemetry-generate-filename)
  (let* ((date-str (date->string (current-date) "~Y~m~d-~H~M~S"))
         (seq (begin
                (set! *telemetry-file-seq* (+ *telemetry-file-seq* 1))
                (number->string *telemetry-file-seq*)
              ) ;begin
         ) ;seq
        ) ;
    (string-append "detail-telemetry-" date-str "-" seq ".jsonl")
  ) ;let*
) ;define-public

(define-public (telemetry-device-id)
  (let ((id (stem-device-id)))
    (if (string? id) id "unknown")
  ) ;let
) ;define-public

(define *telemetry-session-id* (uuid4))

(define-public (telemetry-session-id) *telemetry-session-id*)

(define-public (telemetry-app-version) (xmacs-version))

(define-public (telemetry-platform)
  (let* ((pretty (get-pretty-os-name))
         (no-underscore (string-replace pretty "_" ""))
         (no-spaces (string-replace no-underscore " " ""))
         (normalized (string-downcase no-spaces))
        ) ;
    normalized
  ) ;let*
) ;define-public

(define-public (telemetry-language)
  (let ((lang (get-locale-language)))
    (language-to-locale lang)
  ) ;let
) ;define-public

(define-public (telemetry-timezone)
  (catch #t
    (lambda ()
      (let ((offset (date-zone-offset (current-date))))
        (if (zero? offset)
          "UTC"
          (let* ((sign (if (>= offset 0) "+" "-"))
                 (abs-offset (abs offset))
                 (hours (quotient abs-offset 3600))
                 (minutes (quotient (remainder abs-offset 3600) 60))
                ) ;
            (string-append sign
              (if (< hours 10) "0" "")
              (number->string hours)
              ":"
              (if (< minutes 10) "0" "")
              (number->string minutes)
            ) ;string-append
          ) ;let*
        ) ;if
      ) ;let
    ) ;lambda
    (lambda args "UTC")
  ) ;catch
) ;define-public

(define-public (telemetry-now) (inexact->exact (truncate (current-time))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; JSON serialization helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (telemetry-alist? v)
  (if (null? v)
    #t
    (and (pair? v) (pair? (car v)) (string? (caar v)) (telemetry-alist? (cdr v)))
  ) ;if
) ;define

(define (telemetry->json-escape s)
  (let loop
    ((chars (string->list s)) (result '()))
    (if (null? chars)
      (list->string (reverse result))
      (let ((c (car chars)))
        (cond ((char=? c #\\) (loop (cdr chars) (cons #\\ (cons #\\ result))))
              ((char=? c #\") (loop (cdr chars) (cons #\" (cons #\\ result))))
              ((char=? c #\newline) (loop (cdr chars) (cons #\n (cons #\\ result))))
              ((char=? c #\return) (loop (cdr chars) (cons #\r (cons #\\ result))))
              ((char=? c #\tab) (loop (cdr chars) (cons #\t (cons #\\ result))))
              (else (loop (cdr chars) (cons c result)))
        ) ;cond
      ) ;let
    ) ;if
  ) ;let
) ;define
(define-public (telemetry->json v)
  (cond ((string? v) (string-append "\"" (telemetry->json-escape v) "\""))
        ((number? v) (number->string v))
        ((boolean? v) (if v "true" "false"))
        ((eq? v 'null) "null")
        ((equal? v '(())) "{}")
        ((null? v) "[]")
        ((telemetry-alist? v)
         ;; alist -> JSON object
         (string-append "{"
           (string-join (map (lambda (p) (string-append "\"" (car p) "\":" (telemetry->json (cdr p))))
                          v
                        ) ;map
             ","
           ) ;string-join
           "}"
         ) ;string-append
        ) ;
        ((pair? v)
         ;; list -> JSON array
         (string-append "[" (string-join (map telemetry->json v) ",") "]")
        ) ;
        (else (string-append "\"" (telemetry->json-escape (object->string v)) "\""))
  ) ;cond
) ;define-public

;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Meta file helpers (using njson for reliability)
;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (telemetry-read-meta)
  (catch #t
    (lambda ()
      (let ((path (telemetry-meta-path)))
        (if (path-exists? path)
          (let ((njson-data (file->njson path)))
            (vector->list (njson->json njson-data))
          ) ;let
          '()
        ) ;if
      ) ;let
    ) ;lambda
    (lambda args '())
  ) ;catch
) ;define-public

(define-public (telemetry-write-meta entries)
  (let* ((path (telemetry-meta-path)) (tmp-path (string-append path ".tmp")))
    (define (try-write)
      (njson->file tmp-path (json->njson (list->vector entries)))
      (when (path-exists? path)
        (path-unlink path)
      ) ;when
      (path-rename tmp-path path)
      #t
    ) ;define
    (catch #t
      (lambda () (try-write))
      (lambda args
        ;; Retry once (e.g. file locked on Windows by goldfish reader)
        (when (path-exists? tmp-path)
          (path-unlink tmp-path)
        ) ;when
        (catch #t
          (lambda () (try-write))
          (lambda args2
            (display (string-append "[telemetry] error: meta write failed after retry: "
                       (object->string args2)
                       "\n"
                     ) ;string-append
            ) ;display
            (when (path-exists? tmp-path)
              (path-unlink tmp-path)
            ) ;when
            #f
          ) ;lambda
        ) ;catch
      ) ;lambda
    ) ;catch
  ) ;let*
) ;define-public

(define-public (telemetry-meta-add-entry filename)
  (let* ((entries (telemetry-read-meta))
         (new-entry `((,"filename" unquote filename)
                      (,"timestamp" unquote (telemetry-now)))
         ) ;new-entry
         (updated (cons new-entry entries))
        ) ;
    (if (> (length updated) telemetry-meta-max-entries)
      (let ((dropped (list-tail updated telemetry-meta-max-entries)))
        ;; 删除被滚出的旧 jsonl（失败静默跳过，如被 goldfish 占用）
        (for-each (lambda (entry)
                    (let ((f (assoc-ref entry "filename")))
                      (when f
                        (let ((p (telemetry-full-path f)))
                          (when (path-exists? p)
                            (catch #t (lambda () (path-unlink p)) (lambda args #f))
                          ) ;when
                        ) ;let
                      ) ;when
                    ) ;let
                  ) ;lambda
          dropped
        ) ;for-each
        (set! updated (list-head updated telemetry-meta-max-entries))
      ) ;let
    ) ;if
    (if (telemetry-write-meta updated) updated #f)
  ) ;let*
) ;define-public

(define-public (telemetry-full-path filename)
  (string-append (telemetry-main-dir) "/" filename)
) ;define-public

(define-public (telemetry-make-event event-type properties)
  `((,"eventType" unquote event-type)
    (,"timestamp" unquote (telemetry-now))
    (,"distinctId" unquote (telemetry-device-id))
    (,"sessionId" unquote (telemetry-session-id))
    (,"eventId" unquote (uuid4))
    (,"appVersion" unquote (telemetry-app-version))
    (,"deviceId" unquote (telemetry-device-id))
    (,"platform" unquote (telemetry-platform))
    (,"language" unquote (telemetry-language))
    (,"timezone" unquote (telemetry-timezone))
    (,"properties" unquote (if (null? properties) '(()) properties)))
) ;define-public
