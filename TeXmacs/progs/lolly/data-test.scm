
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

(define (regtest-int-to-hex)
  (regression-test-group "int"
    "hex"
    integer->hexadecimal
    :none
    (test "integer->hexadecimal 1" 1 "1")
    (test "integer->hexadecimal 10" 10 "A")
    (test "integer->hexadecimal 255" 255 "FF")
  ) ;regression-test-group
) ;define

(define (regtest-int-to-padded-hex)
  (regression-test-group "int"
    "padded-hex"
    (lambda (args) (apply integer->padded-hexadecimal args))
    :none
    (test "integer->padded-hexadecimal 1 4" '(1 4) "0001")
    (test "integer->padded-hexadecimal 10 4" '(10 4) "000A")
    (test "integer->padded-hexadecimal 100 4" '(100 4) "0064")
    (test-fails "integer->padded-hexadecimal 255 4" '(255 4) "AA")
  ) ;regression-test-group
) ;define

(define (regtest-hex-to-int)
  (regression-test-group "hex"
    "int"
    hexadecimal->integer
    :none
    (test "hexadecimal->integer 1" "1" 1)
    (test "hexadecimal->integer 0x05" "0x05" 5)
    (test "hexadecimal->integer 00F6" "00F6" 246)
    (test-fails "hexadecimal->integer 255" "255" "597")
  ) ;regression-test-group
) ;define

(define (regtest-encode-base64)
  (regression-test-group "encode-base64"
    "encode"
    encode-base64
    :none
    (test "encode-base64 abc" "abc" "YWJj")
    (test "encode-base64 123456" "123456" "MTIzNDU2")
    (test "encode-base64 what" "what" "d2hhdA==")
  ) ;regression-test-group
) ;define

(define (regtest-decode-base64)
  (regression-test-group "decode-base64"
    "decode"
    decode-base64
    :none
    (test "decode-base64 YWJj" "YWJj" "abc")
    (test "decode-base64 MTIzNDU2" "MTIzNDU2" "123456")
    (test "decode-base64 d2hhdA==" "d2hhdA==" "what")
  ) ;regression-test-group
) ;define

(tm-define (regtest-data)
  (let ((n (+ (regtest-int-to-hex)
             (regtest-int-to-padded-hex)
             (regtest-hex-to-int)
             (regtest-encode-base64)
             (regtest-decode-base64)
           ) ;+
        ) ;n
       ) ;
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of lolly::data: ok\n")
  ) ;let
) ;tm-define
