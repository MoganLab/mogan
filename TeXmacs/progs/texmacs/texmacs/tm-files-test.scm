
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-files-test.scm
;; DESCRIPTION : test suite for file handling helpers
;; COPYRIGHT   : (C) 2026  LiiiSTEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-files-test)
  (:use (texmacs texmacs tm-files)))

(import (liii njson))

(define auto-backup-test-doc
  '(document
     (TeXmacs "2.1.4")
     (style (tuple "generic"))
     (body (document "Hello"))))

(define (auto-backup-test-info doc-id content-md5)
  (list (cons "doc_id" doc-id)
        (cons "md5" content-md5)
        (cons "display_name" "abc.tm")
        (cons "source_url" "file:///tmp/abc.tm")
        (cons "format" "texmacs")
        (cons "device_id" "device-1")
        (cons "liiistem_version" "test-version")
        (cons "doc" auto-backup-test-doc)))

(define (auto-backup-doc-has-field? doc field)
  (catch #t
    (lambda ()
      (njson-ref doc field)
      #t)
    (lambda args #f)))

(define (regtest-auto-backup-safe-base)
  (regression-test-group
   "auto-backup" "safe backup basename"
   (lambda (path)
     (auto-backup-safe-base (system->url path) "1234567890abcdef"))
   :none
   (test "strip extension" "/tmp/abc.tm" "abc")
   (test "sanitize spaces" "/tmp/a b.tm" "a_b")
   (test "sanitize punctuation" "/tmp/a@b$.tm" "a_b_")))

(define (regtest-auto-backup-doc-id)
  (let* ((doc-1 (auto-backup-doc-with-doc-id auto-backup-test-doc "doc-1"))
         (doc-2 (auto-backup-doc-with-doc-id auto-backup-test-doc "doc-2")))
    (regression-test-group
     "auto-backup" "doc id and canonical md5"
     (lambda (case)
       (cond ((== case "doc-id")
              (auto-backup-doc-id doc-1))
             ((== case "md5-ignores-doc-id")
              (== (auto-backup-canonical-md5 doc-1)
                  (auto-backup-canonical-md5 doc-2)))
             (else #f)))
     :none
     (test "doc id stored in initial collection" "doc-id" "doc-1")
     (test "md5 ignores doc id" "md5-ignores-doc-id" #t))))

(define (regtest-auto-backup-timestamp)
  (regression-test-group
   "auto-backup" "timestamp format"
   (lambda (case)
     (and (== case "length")
          (string-length (auto-backup-timestamp))))
   :none
   (test "yyyyMMddHHmmss" "length" 14)))

(define (regtest-auto-backup-official-url)
  (regression-test-group
   "auto-backup" "official url"
   (lambda (case)
     (and (== case "utm")
          (in? (auto-backup-official-url)
               '("https://liiistem.cn/?utm_source=auto_backup_button"
                 "https://liiistem.com/?utm_source=auto_backup_button"))))
   :none
   (test "utm source" "utm" #t)))

(define (regtest-auto-backup-manifest)
  (let ((manifest (auto-backup-empty-manifest)))
    (let ((legacy (string->njson "{\"doc_id\":\"doc-x\",\"upload\":{},\"versions\":[]}")))
      (njson-set! legacy "user_id" "legacy-user")
      (njson-set! manifest "documents" "doc-x" legacy))
    (for (i 0 8)
      (auto-backup-upsert-version!
       manifest
       (auto-backup-test-info "doc-x" (string-append "md5-" (number->string i)))
       (system->url (string-append "/tmp/auto-backup-test-"
                                   (number->string i) ".tmu"))
       "periodic"
       i))
    (let* ((doc (njson-ref manifest "documents" "doc-x"))
           (versions (njson-ref doc "versions")))
      (regression-test-group
       "auto-backup" "manifest retention and md5 shape"
       (lambda (case)
         (cond ((== case "retention")
                (njson-size versions))
               ((== case "no-doc-md5")
                (not (auto-backup-doc-has-field? doc "md5")))
               ((== case "no-user-id")
                (not (auto-backup-doc-has-field? doc "user_id")))
               ((== case "device-id")
                (== (njson-ref doc "device_id") "device-1"))
               ((== case "version-md5")
                (string? (njson-ref versions 0 "md5")))
               (else #f)))
       :none
       (test "rolling versions" "retention" 7)
       (test "document has no md5" "no-doc-md5" #t)
       (test "document has no user id" "no-user-id" #t)
       (test "document keeps device id" "device-id" #t)
       (test "version keeps md5" "version-md5" #t)))))

(tm-define (regtest-tm-files)
  (let ((n (+ (regtest-auto-backup-safe-base)
              (regtest-auto-backup-doc-id)
              (regtest-auto-backup-timestamp)
              (regtest-auto-backup-official-url)
              (regtest-auto-backup-manifest))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of tm-files: ok\n")))
