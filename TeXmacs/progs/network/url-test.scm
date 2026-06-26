
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : url-test.scm
;; DESCRIPTION : Test suite for url
;; COPYRIGHT   : (C) 2023  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (network url-test) (:use (network url)))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for url-or?, url-complete, url-host
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-zotero-url)
  (check (url-or? "zotero://a/b/c") => #f)
  (check (== (url-complete "zotero://a/b/c" "r") (string->url "zotero://a/b/c"))
    =>
    #t
  ) ;check
  (check (== (url-complete "zotero://a/b/c" "df") (string->url "zotero://a/b/c"))
    =>
    #t
  ) ;check
  (check (== (url-complete "zotero://a/b/c" "rf") (string->url "zotero://a/b/c"))
    =>
    #t
  ) ;check
) ;define

(define (test-url-host)
  (check (url-host "http://mogan.app") => "mogan.app")
  (check (url-host "http://git.tmml.wiki/XmacsLabs/mogan") => "git.tmml.wiki")
  (check (url-host "/tmp") => "")
) ;define

(define (test-drive-letter)
  (check (url-drive-letter "C:\\Users\\test") => (if (os-win32?) "C" ""))
  (check (url-drive-letter "D:\\program files\\app") => (if (os-win32?) "D" ""))
  (check (url-drive-letter "Z:\\") => (if (os-win32?) "Z" ""))
  (check (url-drive-letter "\\\\server\\share") => "")
  (check (url-drive-letter "/home/user") => "")
  (check (url-drive-letter "docs\\file.txt") => "")
  (check (url-drive-letter "/") => "")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-url)
  (test-zotero-url)
  (test-url-host)
  (test-drive-letter)
  (check-report)
) ;tm-define
