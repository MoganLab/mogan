
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : data-test.scm
;; DESCRIPTION : Test suite for lolly::data
;; COPYRIGHT   : (C) 2023  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (lolly data-test))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for integer->hexadecimal / padded variant
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-int-to-hex)
  (check (integer->hexadecimal 1) => "1")
  (check (integer->hexadecimal 10) => "A")
  (check (integer->hexadecimal 255) => "FF")
) ;define

(define (test-int-to-padded-hex)
  (define (f . args)
    (apply integer->padded-hexadecimal args)
  ) ;define
  (check (f 1 4) => "0001")
  (check (f 10 4) => "000A")
  (check (f 100 4) => "0064")
  (check (not (== (f 255 4) "AA")) => #t)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for hexadecimal->integer
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-hex-to-int)
  (check (hexadecimal->integer "1") => 1)
  (check (hexadecimal->integer "0x05") => 5)
  (check (hexadecimal->integer "00F6") => 246)
  (check (not (== (hexadecimal->integer "255") "597")) => #t)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for base64
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-encode-base64)
  (check (encode-base64 "abc") => "YWJj")
  (check (encode-base64 "123456") => "MTIzNDU2")
  (check (encode-base64 "what") => "d2hhdA==")
) ;define

(define (test-decode-base64)
  (check (decode-base64 "YWJj") => "abc")
  (check (decode-base64 "MTIzNDU2") => "123456")
  (check (decode-base64 "d2hhdA==") => "what")
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-data)
  (test-int-to-hex)
  (test-int-to-padded-hex)
  (test-hex-to-int)
  (test-encode-base64)
  (test-decode-base64)
  (check-report)
) ;tm-define
