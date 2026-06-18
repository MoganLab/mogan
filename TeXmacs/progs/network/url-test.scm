
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

(define (regtest-zotero-url)
  (regression-test-group "url"
    "(url-or? u)"
    url-or?
    :none
    (test "case 1" "zotero://a/b/c" #f)
  ) ;regression-test-group
  (regression-test-group "url"
    "(url-complete u flags)"
    (lambda (x)
      (== (url-complete "zotero://a/b/c" x) (string->url "zotero://a/b/c"))
    ) ;lambda
    :none
    (test "r" "r" #t)
    (test "df" "df" #t)
    (test "rf" "rf" #t)
  ) ;regression-test-group
) ;define

(define (regtest-url-host)
  (regression-test-group "url"
    "host of the url"
    url-host
    :none
    (test "http 1" "http://mogan.app" "mogan.app")
    (test "http 2" "http://git.tmml.wiki/XmacsLabs/mogan" "git.tmml.wiki")
    (test "local file 1" "/tmp" "")
  ) ;regression-test-group
) ;define

(define (regtest-drive-letter)
  (regression-test-group "url"
    "drive letter of windows path"
    url-drive-letter
    :none
    (test "windows absolute path 1" "C:\\Users\\test" (if (os-win32?) "C" ""))
    (test "windows absolute path 2"
      "D:\\program files\\app"
      (if (os-win32?) "D" "")
    ) ;test
    (test "windows absolute path 3" "Z:\\" (if (os-win32?) "Z" ""))
    (test "network path" "\\\\server\\share" "")
    (test "linux path" "/home/user" "")
    (test "relative path" "docs\\file.txt" "")
    (test "single slash" "/" "")
  ) ;regression-test-group
) ;define

(tm-define (regtest-url)
  (let ((n (+ (regtest-url-host) (regtest-zotero-url) (regtest-drive-letter))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of url: ok\n")
  ) ;let
) ;tm-define
